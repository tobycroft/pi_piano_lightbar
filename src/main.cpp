#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "led/ws2812.h"

// LED strip data pin
static constexpr uint LED_PIN = 28;
static constexpr uint NUM_LEDS = 88;
static constexpr uint PICO_ONBOARD_LED = 25;

// Test different color orders to detect strip type
// Most WS2812 strips use GRB, but some use RGB, BGR, etc.
enum class ColorOrder {
    RGB,  // Red byte first, then Green, then Blue
    GRB,  // Green byte first, then Red, then Blue (WS2812 standard)
    BGR,  // Blue byte first, then Green, then Red
    BRG,  // Blue byte first, then Red, then Green
};

static void fill_all(led::Ws2812& ws, ColorOrder order, uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color;
    switch (order) {
        case ColorOrder::RGB:
            color = (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
            break;
        case ColorOrder::GRB:
            color = (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(r) << 8) | b;
            break;
        case ColorOrder::BGR:
            color = (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(g) << 8) | r;
            break;
        case ColorOrder::BRG:
            color = (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(r) << 8) | g;
            break;
    }
    for (uint i = 0; i < NUM_LEDS; i++) {
        ws.set(i, r, g, b);
    }
    ws.write();
}

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
    // PART 1: Individual color channel test (assumes GRB format)
    // ============================================================
    printf("\n--- Part 1: Individual channels (GRB format) ---\n");

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
    // PART 2: Mixed colors (GRB format)
    // ============================================================
    printf("\n--- Part 2: Mixed colors (GRB format) ---\n");

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
    // PART 3: Color format detection - same color, different byte orders
    // Sending pure red (255,0,0) in different formats to see which works
    // ============================================================
    printf("\n--- Part 3: Color format detection ---\n");
    printf("Testing RGB order...\n");
    fill_all(ws2812, ColorOrder::RGB, 255, 0, 0);
    sleep_ms(2000);

    printf("Testing GRB order...\n");
    fill_all(ws2812, ColorOrder::GRB, 255, 0, 0);
    sleep_ms(2000);

    printf("Testing BGR order...\n");
    fill_all(ws2812, ColorOrder::BGR, 255, 0, 0);
    sleep_ms(2000);

    printf("Testing BRG order...\n");
    fill_all(ws2812, ColorOrder::BRG, 255, 0, 0);
    sleep_ms(2000);

    // ============================================================
    // PART 4: Chase pattern - verify all LEDs individually
    // ============================================================
    printf("\n--- Part 4: Single LED scan ---\n");
    for (int cycle = 0; cycle < 3; cycle++) {
        for (uint i = 0; i < NUM_LEDS; i++) {
            // Clear all
            for (uint j = 0; j < NUM_LEDS; j++) ws2812.set(j, 0, 0, 0);
            // Light one LED white
            ws2812.set(i, 255, 255, 255);
            ws2812.write();
            sleep_ms(20);
        }
    }

    // ============================================================
    // FINAL: All LEDs white, stay on forever
    // ============================================================
    printf("\nDone. All LEDs on (white) forever.\n");
    for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 255, 255);
    ws2812.write();

    while (true) {
        tight_loop_contents();
    }

    return 0;
}