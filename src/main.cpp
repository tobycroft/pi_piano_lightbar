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
#include "usb/usb_midi_host.h"
#include "system/bootsel_button.h"

static constexpr uint LED_PIN = 0;
static constexpr uint NUM_LEDS = piano::NUM_LEDS;
static constexpr uint VBUS_PIN = 24;

enum class UsbRole {
    Device,
    Host
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
    usb::UsbMidiHost midi_host;

    usb::UsbMidiIn* midi_in = nullptr;
    UsbRole role = detect_role();

    if (role == UsbRole::Device) {
        printf("Role: Device (VBUS detected)\n");
        midi_in = &midi_device;
    } else {
        printf("Role: Host (no VBUS)\n");
        if (midi_host.init()) {
            midi_in = &midi_host;
        } else {
            printf("Host init failed, trying device mode...\n");
            midi_in = &midi_device;
        }
    }

    bool midi_active = false;

    uint32_t bootsel_press_start = 0;
    bool bootsel_was_pressed = false;
    uint32_t last_bootsel_check = 0;

    printf("LED test running, waiting for USB MIDI...\n");

    while (true) {
        if (role == UsbRole::Device) {
            tud_task();
        } else {
            tuh_task();
        }

        uint32_t now = to_ms_since_boot(get_absolute_time());

        if (now - last_bootsel_check >= 100) {
            last_bootsel_check = now;
            bool pressed = bootsel_button_is_pressed();

            if (pressed && !bootsel_was_pressed) {
                bootsel_press_start = now;
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

            for (auto& e : events) {
                int idx = piano::note_to_index(e.note);
                if (idx < 0) continue;

                if (e.type == midi::EventType::NoteOn) {
                    led_ctrl.set_led(static_cast<uint>(idx), 255, 255, 255);
                } else {
                    led_ctrl.clear_led(static_cast<uint>(idx));
                }
            }

            if (!events.empty()) {
                led_ctrl.update();
            }

            if (midi_in && !midi_in->is_connected()) {
                printf("USB MIDI disconnected, returning to test mode...\n");
                led_ctrl.clear_all();
                midi_in->reset();
                midi_active = false;
            }
        }

        sleep_ms(1);
    }

    return 0;
}