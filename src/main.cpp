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

    printf("=== LED Strip Diagnostic Test ===\n");
    printf("Pin: %d, LEDs: %d\n", LED_PIN, NUM_LEDS);

    // Initialize the LED strip
    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS);

    // ============================================================
    // PART 1: Individual color channels
    // Driver now uses RBG wire format (detected from your feedback)
    // ============================================================
    printf("\n--- Part 1: Individual channels ---\n");

    printf("RED\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 0, 0);
    ws2812.write();
    sleep_ms(3000);

    printf("GREEN\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 255, 0);
    ws2812.write();
    sleep_ms(3000);

    printf("BLUE\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 0, 255);
    ws2812.write();
    sleep_ms(3000);

    // ============================================================
    // PART 2: Mixed colors
    // ============================================================
    printf("\n--- Part 2: Mixed colors ---\n");

    printf("YELLOW (R+G)\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 255, 0);
    ws2812.write();
    sleep_ms(3000);

    printf("CYAN (G+B)\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 255, 255);
    ws2812.write();
    sleep_ms(3000);

    printf("PURPLE (R+B)\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 0, 255);
    ws2812.write();
    sleep_ms(3000);

    printf("WHITE (R+G+B)\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 255, 255);
    ws2812.write();
    sleep_ms(3000);

    // ============================================================
    // PART 3: Green channel brightness ramp
    // Goes from 0 to 255 in steps, lets you see if green works
    // at any brightness level
    // ============================================================
    printf("\n--- Part 3: Green brightness ramp ---\n");
    for (int step = 0; step <= 255; step += 16) {
        printf("Green: %d\n", step);
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, static_cast<uint8_t>(step), 0);
        ws2812.write();
        sleep_ms(500);
    }

    // ============================================================
    // PART 4: Single LED scan - verify each LED works
    // ============================================================
    printf("\n--- Part 4: Single LED scan ---\n");
    for (int cycle = 0; cycle < 2; cycle++) {
        for (uint i = 0; i < NUM_LEDS; i++) {
            for (uint j = 0; j < NUM_LEDS; j++) ws2812.set(j, 0, 0, 0);
            ws2812.set(i, 255, 255, 255);
            ws2812.write();
            sleep_ms(15);
        }
    }

    // ============================================================
    // FINAL: All LEDs white, stay on forever
    // ============================================================
    printf("\nDone. All LEDs white.\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 255, 255);
    ws2812.write();

    while (true) {
        tight_loop_contents();
    }

    return 0;
}