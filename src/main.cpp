#include <cstdio>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "tusb.h"
#include "hardware/gpio.h"

#include "piano/piano_config.h"
#include "led/ws2812.h"
#include "led/led_controller.h"
#include "led/led_animator.h"
#include "usb/usb_midi_host.h"
#include "usb/usb_midi.h"
#include "system/bootsel_button.h"

// WS2812 data pin: use GP28 (wired to the strip)
static constexpr uint LED_PIN = 28;
static constexpr uint NUM_LEDS = piano::NUM_LEDS;
static constexpr uint VBUS_PIN = 24;
static constexpr uint PICO_ONBOARD_LED = 25;

// Low brightness level for "always-on" background (0-255, very dim)
static constexpr uint8_t BG_BRIGHTNESS = 5;

enum class MidiChannelMode {
    All,
    Ch1,
    Ch4
};

int main() {
    stdio_init_all();

    // Initialize onboard LED (GP25) - power/status indicator
    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);  // Initially ON (no device)

    // Initialize WS2812 data pin (GP28) explicitly
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Initialize VBUS sense pin (GP24) for monitoring
    // Note: The Pico's VBUS pin already has an external voltage divider
    // (100k/100k to GND), so do NOT add a pull-down here.
    gpio_init(VBUS_PIN);
    gpio_set_dir(VBUS_PIN, GPIO_IN);
    // No pull-up or pull-down - VBUS has external divider to GND

    printf("=== Piano LED MIDI Guidance (C++) ===\n");

    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS);
    led::LedController led_ctrl(ws2812);
    led::LedAnimator animator(led_ctrl);

    // Startup: set all LEDs to low brightness (always-on background)
    // This verifies the strip connection and provides a constant dim glow
    printf("LED strip: low brightness all-on (background)\n");
    for (uint i = 0; i < NUM_LEDS; i++) {
        led_ctrl.set_led(i, BG_BRIGHTNESS, BG_BRIGHTNESS, BG_BRIGHTNESS);
    }
    led_ctrl.update();

    usb::UsbMidiHost midi_host;
    usb::UsbMidiIn* midi_in = &midi_host;
    bool midi_active = false;

    // ------------------------------------------------------------------
    // USB initialization: host mode only (USB MIDI controller)
    // ------------------------------------------------------------------
    tusb_init();

    if (midi_host.init()) {
        printf("Mode: USB Host (waiting for MIDI device)\n");
    } else {
        printf("ERROR: USB Host init failed\n");
    }

    bool active_leds[NUM_LEDS] = {false};
    uint32_t led_on_time[NUM_LEDS] = {0};
    uint32_t onboard_blink_until = 0;

    MidiChannelMode channel_mode = MidiChannelMode::All;

    uint32_t bootsel_press_start = 0;
    bool bootsel_was_pressed = false;
    uint32_t last_bootsel_check = 0;

    printf("Waiting for USB MIDI...\n");

    while (true) {
        // Process TinyUSB host events
        tuh_task();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        // BOOTSEL button handling (every 100ms)
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

        // ------------------------------------------------------------------
        // State machine: idle (no MIDI device) vs active (MIDI connected)
        // ------------------------------------------------------------------
        if (!midi_active) {
            // Host mode: wait for a USB MIDI device to be connected
            // Onboard LED: solid ON (no device, idle)
            gpio_put(PICO_ONBOARD_LED, 1);
            if (midi_in && midi_in->is_connected()) {
                printf("USB MIDI detected, switching to MIDI mode...\n");
                gpio_put(PICO_ONBOARD_LED, 0);
                // Keep low brightness background - MIDI events will highlight individual notes
                midi_active = true;
                continue;
            }
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
                    // Onboard LED: flash on MIDI activity
                    gpio_put(PICO_ONBOARD_LED, 1);
                    onboard_blink_until = now + 50;
                } else {
                    active_leds[idx] = false;
                    // Return to low brightness background instead of turning off
                    led_ctrl.set_led(static_cast<uint>(idx), BG_BRIGHTNESS, BG_BRIGHTNESS, BG_BRIGHTNESS);
                    updated = true;
                }
            }

            // LED timeout: return to low brightness after 50ms of no MIDI activity
            for (int i = 0; i < static_cast<int>(NUM_LEDS); i++) {
                if (active_leds[i] && (now - led_on_time[i] >= 50)) {
                    active_leds[i] = false;
                    led_ctrl.set_led(static_cast<uint>(i), BG_BRIGHTNESS, BG_BRIGHTNESS, BG_BRIGHTNESS);
                    updated = true;
                }
            }

            if (updated) {
                led_ctrl.update();
            }

            // Onboard LED: reset after blink timeout
            if (now >= onboard_blink_until) {
                gpio_put(PICO_ONBOARD_LED, 0);
            }

            // Check for device disconnection
            if (midi_in && !midi_in->is_connected()) {
                printf("USB MIDI disconnected, returning to idle...\n");
                // Return to low brightness background
                for (uint i = 0; i < NUM_LEDS; i++) {
                    led_ctrl.set_led(i, BG_BRIGHTNESS, BG_BRIGHTNESS, BG_BRIGHTNESS);
                }
                led_ctrl.update();
                for (int i = 0; i < static_cast<int>(NUM_LEDS); i++) active_leds[i] = false;
                midi_in->reset();
                midi_active = false;
            }
        }

        sleep_ms(1);
    }
}