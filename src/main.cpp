// =============================================================================
// pi_piano_lightbar — 88-key Piano LED MIDI Controller
// =============================================================================
// RP2040 USB MIDI Host → 88-key WS2812 LED strip controller
//
// 硬件: Raspberry Pi Pico, WS2812 GRB 3-byte 灯条, 88键
// 引脚: LED data = GPIO28, 板载 LED = GPIO25
//
// 架构:
//   main.cpp               — 初始化硬件, 启动主循环
//   led/ws2812.cpp          — WS2812 PIO 底层驱动
//   led/led_controller.cpp  — LED 颜色控制器
//   led/led_animator.cpp    — 跑马灯动画
//   usb/usb_midi_host.cpp   — TinyUSB Host MIDI 接收
//   midi/midi_parser.cpp    — MIDI 协议解析
//   piano/piano_config.h    — 音符→LED 映射
//
// MIDI 映射: A0=note21=LED[0], C8=note108=LED[87]
// =============================================================================

#include <cstdio>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "tusb.h"

#include "piano/piano_config.h"
#include "piano/color_scheme.h"
#include "led/ws2812.h"
#include "led/led_controller.h"
#include "led/led_animator.h"
#include "usb/usb_midi_host.h"
#include "system/bootsel_button.h"

// 硬件引脚定义
static constexpr uint LED_PIN = 28;           // WS2812 数据引脚
static constexpr uint PICO_ONBOARD_LED = 25;  // Pico 板载 LED

// 板载 LED 闪烁时长 (ms)
static constexpr uint32_t ONBOARD_LED_BLINK_MS = 80;

// 配色预览时长 (ms)
static constexpr uint32_t PREVIEW_DURATION_MS = 1000;

// BOOTSEL 长按进入 bootloader 的时长 (ms)
static constexpr uint32_t BOOTSEL_LONG_PRESS_MS = 3000;

// BOOTSEL 检测间隔 (ms)
static constexpr uint32_t BOOTSEL_CHECK_INTERVAL_MS = 100;

int main() {
    stdio_init_all();

    // 初始化板载 LED — 上电常亮
    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);

    printf("=== Piano LED MIDI Controller ===\n");
    printf("LED strip: GPIO%d, %d LEDs\n", LED_PIN, piano::NUM_LEDS);

    // 初始化 WS2812 灯条
    led::Ws2812 ws2812(pio0, 0, LED_PIN, piano::NUM_LEDS, led::ColorOrder::GRB);
    led::LedController led_ctrl(ws2812);
    led::LedAnimator animator(led_ctrl);

    // 清空灯条，启动跑马灯
    led_ctrl.setAllKeys(led::LedColor::OFF);
    led_ctrl.setBrightness(255);
    led_ctrl.update();

    // 初始化 USB MIDI Host
    usb::UsbMidiHost midi_host;
    if (!midi_host.init()) {
        printf("FATAL: USB MIDI Host init failed!\n");
        // 初始化失败，仍然跑跑马灯
        while (true) {
            animator.tick();
            sleep_ms(1);
        }
    }

    printf("Chase animation running, waiting for USB MIDI device...\n");

    // 状态跟踪
    bool midi_connected = false;
    bool was_connected = false;

    // 板载 LED 闪烁状态
    bool onboard_led_on = true;  // 初始状态: 亮
    uint32_t onboard_led_off_at = 0;

    // MIDI 按键状态
    bool active_notes[piano::NUM_LEDS] = {false};

    // 配色方案状态
    int current_scheme = 0;          // 当前配色方案索引
    bool preview_active = false;     // 是否正在预览配色
    uint32_t preview_start = 0;      // 预览开始时间

    // BOOTSEL 按键状态
    bool bootsel_was_pressed = false;
    uint32_t bootsel_press_start = 0;
    uint32_t last_bootsel_check = 0;

    while (true) {
        // 处理 USB Host 事件
        tuh_task();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        midi_connected = midi_host.is_connected();

        // --- BOOTSEL 按键检测 ---
        if (now - last_bootsel_check >= BOOTSEL_CHECK_INTERVAL_MS) {
            last_bootsel_check = now;
            bool pressed = bootsel_button_is_pressed();

            if (pressed && !bootsel_was_pressed) {
                bootsel_press_start = now;
                printf("BOOTSEL pressed\n");
            }

            if (!pressed && bootsel_was_pressed) {
                // 短按: 切换配色方案
                if (now - bootsel_press_start < BOOTSEL_LONG_PRESS_MS) {
                    current_scheme = (current_scheme + 1) % piano::kNumColorSchemes;
                    printf("Scheme switched to: %d\n", current_scheme);

                    // 仅在非 MIDI 模式下预览配色
                    if (!midi_connected) {
                        // 点亮全部按键展示当前配色
                        for (int i = 0; i < static_cast<int>(piano::NUM_LEDS); i++) {
                            led::LedColor color = piano::key_color(i, current_scheme);
                            led_ctrl.setKey(static_cast<uint>(i), color);
                        }
                        led_ctrl.update();
                        preview_active = true;
                        preview_start = now;
                    }
                }
            }

            if (pressed && bootsel_was_pressed) {
                // 长按 3 秒 → 进入 bootloader
                if (now - bootsel_press_start >= BOOTSEL_LONG_PRESS_MS) {
                    printf("BOOTSEL held 3s, entering bootloader...\n");
                    led_ctrl.clear();
                    sleep_ms(100);
                    rom_reset_usb_boot(0, 0);
                }
            }

            bootsel_was_pressed = pressed;
        }

        // --- 配色预览超时处理 (非 MIDI 模式) ---
        if (preview_active) {
            if (now - preview_start >= PREVIEW_DURATION_MS) {
                led_ctrl.setAllKeys(led::LedColor::OFF);
                led_ctrl.update();
                preview_active = false;
                printf("Preview ended\n");
            }
            // 预览期间不跑跑马灯
            if (!midi_connected) {
                sleep_ms(1);
                continue;
            }
        }

        // --- 连接状态变化处理 ---
        if (midi_connected && !was_connected) {
            // MIDI 设备刚接入 → 熄灭板载 LED
            printf("MIDI device connected! Switching to MIDI mode...\n");
            gpio_put(PICO_ONBOARD_LED, 0);
            onboard_led_on = false;

            // 取消预览，清空灯条
            preview_active = false;
            led_ctrl.setAllKeys(led::LedColor::OFF);
            led_ctrl.update();
            for (auto& n : active_notes) n = false;
        }

        if (!midi_connected && was_connected) {
            // MIDI 设备断开 → 恢复板载 LED 常亮, 恢复最高亮度
            printf("MIDI device disconnected, returning to chase mode...\n");
            gpio_put(PICO_ONBOARD_LED, 1);
            onboard_led_on = true;
            midi_host.reset();

            // 恢复最高亮度，清空灯条，启动跑马灯
            led_ctrl.setBrightness(255);
            animator.reset();
        }

        was_connected = midi_connected;

        // --- 主逻辑 ---
        if (midi_connected) {
            // MIDI 模式: 处理 MIDI 事件
            auto events = midi_host.poll();

            if (!events.empty()) {
                // 收到 MIDI 数据 → 板载 LED 闪烁 80ms
                gpio_put(PICO_ONBOARD_LED, 1);
                onboard_led_on = true;
                onboard_led_off_at = now + ONBOARD_LED_BLINK_MS;

                bool updated = false;
                for (auto& e : events) {
                    int idx = piano::note_to_index(e.note);
                    if (idx < 0) continue;  // 超出 88 键范围，忽略

                    if (e.type == midi::EventType::NoteOn) {
                        active_notes[idx] = true;
                        // 将 MIDI velocity (0~127) 映射为亮度 (0~255)
                        uint8_t brightness = static_cast<uint8_t>(
                            (static_cast<uint16_t>(e.velocity) * 255) / 127);
                        // 根据当前配色方案选择白键/黑键颜色
                        led::LedColor color = piano::key_color(idx, current_scheme);
                        led_ctrl.setKey(static_cast<uint>(idx), color, brightness);
                        updated = true;
                    } else {
                        active_notes[idx] = false;
                        led_ctrl.setKey(static_cast<uint>(idx),
                                        led::LedColor::OFF);
                        updated = true;
                    }
                }

                if (updated) {
                    led_ctrl.update();
                }
            }

            // 板载 LED 闪烁时间到 → 熄灭
            if (onboard_led_on && onboard_led_off_at > 0 && now >= onboard_led_off_at) {
                gpio_put(PICO_ONBOARD_LED, 0);
                onboard_led_on = false;
                onboard_led_off_at = 0;
            }
        } else {
            // 跑马灯模式: 运行 chase 动画 (预览期间跳过)
            if (!preview_active) {
                animator.tick();
            }
        }

        sleep_ms(1);
    }

    return 0;
}