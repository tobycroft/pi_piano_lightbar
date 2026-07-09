#include <cstdio>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "tusb.h"
#include "hardware/gpio.h"

#include "piano/piano_config.h"
#include "led/ws2812.h"
#include "led/led_controller.h"
#include "led/led_animator.h"
#include "usb/usb_midi_device.h"
#include "usb/usb_midi.h"
#if CFG_TUH_ENABLED
#include "usb/usb_midi_host.h"
#endif
#include "system/bootsel_button.h"

static constexpr uint LED_PIN = 0;
static constexpr uint NUM_LEDS = piano::NUM_LEDS;
static constexpr uint VBUS_PIN = 24;

enum class UsbRole {
    Device,
    Host
};

enum class MidiChannelMode {
    All,
    Ch1,
    Ch4
};

static UsbRole detect_role() {
    bool vbus = gpio_get(VBUS_PIN);
    return vbus ? UsbRole::Device : UsbRole::Host;
}

int main() {
    stdio_init_all();

    gpio_init(VBUS_PIN);
    gpio_set_dir(VBUS_PIN, GPIO_IN);
    gpio_pull_down(VBUS_PIN);

    printf("=== Piano LED MIDI Guidance (C++) ===\n");

    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS);
    led::LedController led_ctrl(ws2812);
    led::LedAnimator animator(led_ctrl);

    led_ctrl.clear_all();

    usb::UsbMidiDevice midi_device;
#if CFG_TUH_ENABLED
    usb::UsbMidiHost midi_host;
#endif

    usb::UsbMidiIn* midi_in = nullptr;
    UsbRole role = detect_role();

#if CFG_TUH_ENABLED
    if (role == UsbRole::Device) {
        printf("Role: Device (VBUS detected)\n");
        tud_init(TUD_OPT_RHPORT);
        midi_in = &midi_device;
    } else {
        printf("Role: Host (no VBUS)\n");
        if (midi_host.init()) {
            midi_in = &midi_host;
        } else {
            printf("Host init failed, trying device mode...\n");
            tud_init(TUD_OPT_RHPORT);
            role = UsbRole::Device;
            midi_in = &midi_device;
        }
    }
#else
    printf("Role: Device (VBUS detected)\n");
    tud_init(TUD_OPT_RHPORT);
    midi_in = &midi_device;
    role = UsbRole::Device;
#endif

    bool midi_active = false;
    bool active_leds[NUM_LEDS] = {false};
    uint32_t led_on_time[NUM_LEDS] = {0};

    MidiChannelMode channel_mode = MidiChannelMode::All;

    uint32_t bootsel_press_start = 0;
    bool bootsel_was_pressed = false;
    uint32_t last_bootsel_check = 0;

    printf("LED test running, waiting for USB MIDI...\n");

    while (true) {
#if CFG_TUH_ENABLED
        if (role == UsbRole::Device) {
            tud_task();
        } else {
            tuh_task();
        }
#else
        tud_task();
#endif

        uint32_t now = to_ms_since_boot(get_absolute_time());

        if (now - last_bootsel_check >= 100) {
            last_bootsel_check = now;
            bool pressed = bootsel_button_is_pressed();

            if (pressed && !bootsel_was_pressed) {
                bootsel_press_start = now;
                printf("BOOTSEL pressed\n");
            } else if (!pressed && bootsel_was_pressed) {
                uint32_t duration = now - bootsel_press_start;
                if (duration < 3000) {
                    switch (channel_mode) {
                        case MidiChannelMode::All:
                            channel_mode = MidiChannelMode::Ch1;
                            printf("Channel mode: Ch1\n");
                            break;
                        case MidiChannelMode::Ch1:
                            channel_mode = MidiChannelMode::Ch4;
                            printf("Channel mode: Ch4\n");
                            break;
                        case MidiChannelMode::Ch4:
                            channel_mode = MidiChannelMode::All;
                            printf("Channel mode: All\n");
                            break;
                    }
                }
            } else if (pressed && bootsel_was_pressed) {
                if (now - bootsel_press_start >= 3000) {
                    printf("BOOTSEL held 3s, entering bootloader...\n");
                    led_ctrl.clear_all();
                    sleep_ms(100);
                    rom_reset_usb_boot(0, 0);
                }
            }
            bootsel_was_pressed = pressed;
        }

        if (!midi_active) {
            if (midi_in && midi_in->is_connected()) {
                printf("USB MIDI detected, switching to MIDI mode...\n");
                led_ctrl.clear_all();
                midi_active = true;
                continue;
            }

            animator.tick();
        } else {
            auto events = midi_in ? midi_in->poll() : std::vector<midi::MidiEvent>{};
            bool updated = false;

            for (auto& e : events) {
                if (channel_mode == MidiChannelMode::Ch1 && e.channel != 0) continue;
                if (channel_mode == MidiChannelMode::Ch4 && e.channel != 3) continue;

                int idx = piano::note_to_index(e.note);
                if (idx < 0) continue;

                if (e.type == midi::EventType::NoteOn) {
                    active_leds[idx] = true;
                    led_on_time[idx] = now;
                    led_ctrl.set_led(static_cast<uint>(idx), 255, 255, 255);
                    updated = true;
                } else {
                    active_leds[idx] = false;
                    led_ctrl.clear_led(static_cast<uint>(idx));
                    updated = true;
                }
            }

            for (int i = 0; i < static_cast<int>(NUM_LEDS); i++) {
                if (active_leds[i] && (now - led_on_time[i] >= 50)) {
                    active_leds[i] = false;
                    led_ctrl.clear_led(static_cast<uint>(i));
                    updated = true;
                }
            }

            if (updated) {
                led_ctrl.update();
            }

            if (midi_in && !midi_in->is_connected()) {
                printf("USB MIDI disconnected, returning to test mode...\n");
                led_ctrl.clear_all();
                for (int i = 0; i < static_cast<int>(NUM_LEDS); i++) active_leds[i] = false;
                midi_in->reset();
                midi_active = false;
            }
        }

        sleep_ms(1);
    }
}