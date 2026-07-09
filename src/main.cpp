#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "led/ws2812.h"

static constexpr uint LED_PIN = 28;
static constexpr uint NUM_LEDS = 88;
static constexpr uint PICO_ONBOARD_LED = 25;

// Send same 32-bit wire data to all LEDs over 4-byte PIO
void send_all(led::Ws2812& ws, uint32_t wire_data) {
    for (uint i = 0; i < ws.num_leds(); i++) {
        ws.send_wire(wire_data);
    }
    sleep_us(300);
}

// Build 32-bit wire value for RGBW GRBW order (byte0=G, byte1=R, byte2=B, byte3=W)
uint32_t wire_grbw(uint8_t g, uint8_t r, uint8_t b, uint8_t w) {
    return (static_cast<uint32_t>(g) << 24) |
           (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(b) << 8)  |
           static_cast<uint32_t>(w);
}

// Build 32-bit wire value for RGBW RGBW order (byte0=R, byte1=G, byte2=B, byte3=W)
uint32_t wire_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return (static_cast<uint32_t>(r) << 24) |
           (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(b) << 8)  |
           static_cast<uint32_t>(w);
}

int main() {
    stdio_init_all();

    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);

    printf("=== RGBW 4-Byte Test ===\n");

    // Default: GRBW order (most common for SK6812 RGBW)
    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS, led::ColorOrder::GRBW);

    printf("Bytes per LED: %u\n", ws2812.bytes_per_led());

    while (true) {
        // === Test 1: GRBW order ===
        printf("--- GRBW order tests ---\n");

        // RED:    G=0, R=255, B=0, W=0
        printf("RED\n");
        send_all(ws2812, wire_grbw(0, 255, 0, 0));
        sleep_ms(2000);

        // GREEN:  G=255, R=0, B=0, W=0
        printf("GREEN\n");
        send_all(ws2812, wire_grbw(255, 0, 0, 0));
        sleep_ms(2000);

        // BLUE:   G=0, R=0, B=255, W=0
        printf("BLUE\n");
        send_all(ws2812, wire_grbw(0, 0, 255, 0));
        sleep_ms(2000);

        // WHITE:  G=0, R=0, B=0, W=255
        printf("WHITE\n");
        send_all(ws2812, wire_grbw(0, 0, 0, 255));
        sleep_ms(2000);

        // ALL OFF
        send_all(ws2812, 0);
        sleep_ms(1000);

        // === Test 2: RGBW order ===
        printf("--- RGBW order tests ---\n");

        // RED:    R=255, G=0, B=0, W=0
        printf("RED (RGBW)\n");
        send_all(ws2812, wire_rgbw(255, 0, 0, 0));
        sleep_ms(2000);

        // GREEN:  R=0, G=255, B=0, W=0
        printf("GREEN (RGBW)\n");
        send_all(ws2812, wire_rgbw(0, 255, 0, 0));
        sleep_ms(2000);

        // BLUE:   R=0, G=0, B=255, W=0
        printf("BLUE (RGBW)\n");
        send_all(ws2812, wire_rgbw(0, 0, 255, 0));
        sleep_ms(2000);

        // WHITE:  R=0, G=0, B=0, W=255
        printf("WHITE (RGBW)\n");
        send_all(ws2812, wire_rgbw(0, 0, 0, 255));
        sleep_ms(2000);

        send_all(ws2812, 0);
        sleep_ms(2000);

        // === Test 3: Mixed colors ===
        printf("--- Mixed ---\n");

        // Yellow = R+G
        printf("YELLOW GRBW\n");
        send_all(ws2812, wire_grbw(255, 255, 0, 0));
        sleep_ms(2000);

        // Cyan = G+B
        printf("CYAN GRBW\n");
        send_all(ws2812, wire_grbw(255, 0, 255, 0));
        sleep_ms(2000);

        // Magenta = R+B
        printf("MAGENTA GRBW\n");
        send_all(ws2812, wire_grbw(0, 255, 255, 0));
        sleep_ms(2000);

        // White via RGB
        printf("WHITE RGB\n");
        send_all(ws2812, wire_grbw(255, 255, 255, 0));
        sleep_ms(2000);

        send_all(ws2812, 0);
        sleep_ms(2000);
    }

    return 0;
}