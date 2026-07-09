// =============================================================================
// pi_piano_lightbar — 88-key Piano LED MIDI Controller
// =============================================================================
// RP2040 USB MIDI Host → 88-key WS2812 LED strip controller
//
// 硬件: Raspberry Pi Pico, WS2812 GRB 3-byte 灯条, 88键
// 引脚: LED data = GPIO28
//
// 架构:
//   main.cpp          — 初始化硬件, 启动主循环
//   led/ws2812.cpp    — WS2812 PIO 底层驱动
//   led/led_controller — LED 颜色控制器 (颜色预设 + 按键映射)
//   (后续) usb/        — TinyUSB Host MIDI 接收
//   (后续) midi/       — MIDI 协议解析
//   (后续) piano/      — 音符→LED 映射
//
// MIDI 映射: A0=note21=LED[0], C8=note108=LED[87]
// =============================================================================

#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "led/ws2812.h"
#include "led/led_controller.h"

// 硬件引脚定义
static constexpr uint LED_PIN = 28;           // WS2812 数据引脚
static constexpr uint NUM_LEDS = 88;          // 88 键钢琴
static constexpr uint PICO_ONBOARD_LED = 25;  // Pico 板载 LED

int main() {
    stdio_init_all();

    // 板载 LED 常亮，指示系统运行
    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);

    printf("=== LedController Color Test ===\n");

    // 初始化 WS2812 灯条: PIO0, SM0, GPIO28, 88颗, GRB 颜色顺序
    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS, led::ColorOrder::GRB);

    // 创建 LED 颜色控制器 (上层统一调用接口)
    led::LedController leds(ws2812);

    // 依次循环显示所有颜色预设，每 1.5 秒切换
    while (true) {
        printf("RED\n");
        leds.setAllKeys(led::LedColor::RED);
        leds.update();
        sleep_ms(1500);

        printf("GREEN\n");
        leds.setAllKeys(led::LedColor::GREEN);
        leds.update();
        sleep_ms(1500);

        printf("BLUE\n");
        leds.setAllKeys(led::LedColor::BLUE);
        leds.update();
        sleep_ms(1500);

        printf("WHITE\n");
        leds.setAllKeys(led::LedColor::WHITE);
        leds.update();
        sleep_ms(1500);

        printf("LAKE_BLUE\n");
        leds.setAllKeys(led::LedColor::LAKE_BLUE);
        leds.update();
        sleep_ms(1500);

        printf("GRASS_GREEN\n");
        leds.setAllKeys(led::LedColor::GRASS_GREEN);
        leds.update();
        sleep_ms(1500);

        printf("PINK\n");
        leds.setAllKeys(led::LedColor::PINK);
        leds.update();
        sleep_ms(1500);

        printf("PURPLE\n");
        leds.setAllKeys(led::LedColor::PURPLE);
        leds.update();
        sleep_ms(1500);

        printf("OFF\n");
        leds.setAllKeys(led::LedColor::OFF);
        leds.update();
        sleep_ms(1500);
    }

    return 0;
}