#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "led/ws2812.h"

// LED strip data pin
static constexpr uint LED_PIN = 28;
static constexpr uint NUM_LEDS = 88;
static constexpr uint PICO_ONBOARD_LED = 25;

int main() {
    stdio_init_all();

    // Onboard LED ON = program is running
    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);

    printf("=== LED Strip Test ===\n");
    printf("Pin: %d, LEDs: %d\n", LED_PIN, NUM_LEDS);

    // Initialize the LED strip
    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS);

    printf("Lighting all LEDs...\n");

    // ============================================================
    // TEST 1: All LEDs white at full brightness
    // ============================================================
    printf("Test 1: All white\n");
    for (uint i = 0; i < NUM_LEDS; i++) {
        ws2812.set(i, 255, 255, 255);
    }
    ws2812.write();
    sleep_ms(2000);

    // ============================================================
    // TEST 2: All LEDs red
    // ============================================================
    printf("Test 2: All red\n");
    for (uint i = 0; i < NUM_LEDS; i++) {
        ws2812.set(i, 255, 0, 0);
    }
    ws2812.write();
    sleep_ms(2000);

    // ============================================================
    // TEST 3: All LEDs green
    // ============================================================
    printf("Test 3: All green\n");
    for (uint i = 0; i < NUM_LEDS; i++) {
        ws2812.set(i, 0, 255, 0);
    }
    ws2812.write();
    sleep_ms(2000);

    // ============================================================
    // TEST 4: All LEDs blue
    // ============================================================
    printf("Test 4: All blue\n");
    for (uint i = 0; i < NUM_LEDS; i++) {
        ws2812.set(i, 0, 0, 255);
    }
    ws2812.write();
    sleep_ms(2000);

    // ============================================================
    // TEST 5: Chase pattern (first 10 LEDs scroll)
    // ============================================================
    printf("Test 5: Chase\n");
    for (int cycle = 0; cycle < 5; cycle++) {
        for (uint i = 0; i < NUM_LEDS; i++) {
            ws2812.clear();
            // Light current LED white
            ws2812.set(i, 255, 255, 255);
            // Light a few neighbors dim
            if (i > 0) ws2812.set(i - 1, 64, 64, 64);
            if (i > 1) ws2812.set(i - 2, 16, 16, 16);
            ws2812.write();
            sleep_ms(30);
        }
    }

    // ============================================================
    // FINAL: All LEDs white, stay on forever
    // ============================================================
    printf("Done. All LEDs on (white) forever.\n");
    for (uint i = 0; i < NUM_LEDS; i++) {
        ws2812.set(i, 255, 255, 255);
    }
    ws2812.write();

    while (true) {
        tight_loop_contents();
    }

    return 0;
}