#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "led/ws2812.h"

static constexpr uint LED_PIN = 28;
static constexpr uint NUM_LEDS = 88;
static constexpr uint PICO_ONBOARD_LED = 25;

// Send same 24-bit wire data to all LEDs
void send_all(led::Ws2812& ws, uint32_t wire_data) {
    for (uint i = 0; i < ws.num_leds(); i++) {
        ws.send_wire(wire_data);
    }
    sleep_us(300);
}

int main() {
    stdio_init_all();

    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);

    printf("=== RGB Byte Position Test ===\n");

    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS);

    // Wire format: 24 bits per LED
    // 0x00RRGGBB where:
    //   bits 23-16 = byte0
    //   bits 15-8  = byte1
    //   bits 7-0   = byte2
    //
    // Known from previous tests:
    //   [255, 0, 0] -> RED   (byte0=255)
    //   [0, 255, 0] -> BLUE  (byte1=255)
    //   [0, 0, 255] -> OFF   (byte2=255)
    //   [255, 255, 0] -> PINK (byte0+byte1 = RED+BLUE)
    //
    // This tells us: byte0=Red, byte1=Blue, byte2=???
    // The user says green is physically fine.
    // Let's try sending 4 bytes per LED (32 bits) to see if it's RGBW.

    while (true) {
        // === Test 1: 3-byte RGB ===
        printf("--- 3-byte tests ---\n");

        // [255, 0, 0] -> byte0=255 -> RED (should be RED)
        printf("R\n");
        send_all(ws2812, 0x00FF0000);
        sleep_ms(2000);

        // [0, 255, 0] -> byte1=255 -> BLUE (should be BLUE)
        printf("G\n");
        send_all(ws2812, 0x0000FF00);
        sleep_ms(2000);

        // [0, 0, 255] -> byte2=255 -> OFF previously
        printf("B\n");
        send_all(ws2812, 0x000000FF);
        sleep_ms(2000);

        // All off
        send_all(ws2812, 0x00000000);
        sleep_ms(1000);

        // === Test 2: Try 4-byte (32-bit) per LED ===
        // If strip is RGBW with format [R, G, B, W], 
        // sending 4 bytes with byte3=0 should work.
        // But we need to see if the PIO handles 32-bit autopull...
        // For now, let's just try putting 255 in different positions
        // using the 3-byte format, but with byte2 as different values.

        // [255, 0, 128] -> byte0=255, byte2=128 -> RED + something
        printf("R + byte2=128\n");
        send_all(ws2812, 0x00FF0080);
        sleep_ms(2000);

        // [0, 255, 128] -> byte1=255, byte2=128 -> BLUE + something
        printf("B + byte2=128\n");
        send_all(ws2812, 0x0000FF80);
        sleep_ms(2000);

        // All off
        send_all(ws2812, 0x00000000);
        sleep_ms(1000);

        // === Test 3: Try putting green at byte0, byte1, and byte2 ===
        // byte0 only: [255, 0, 0] -> RED
        // byte1 only: [0, 255, 0] -> BLUE
        // byte2 only: [0, 0, 255] -> OFF
        //
        // Now try COMBINATIONS:
        // [255, 255, 0] -> RED + BLUE -> PINK (confirmed)
        // [255, 0, 255] -> RED + byte2 -> ???
        // [0, 255, 255] -> BLUE + byte2 -> ???

        // [255, 0, 255] -> RED + byte2=255
        printf("R + byte2\n");
        send_all(ws2812, 0x00FF00FF);
        sleep_ms(2000);

        // [0, 255, 255] -> BLUE + byte2=255
        printf("B + byte2\n");
        send_all(ws2812, 0x0000FFFF);
        sleep_ms(2000);

        // [255, 255, 255] -> ALL 3 bytes = 255
        printf("ALL\n");
        send_all(ws2812, 0x00FFFFFF);
        sleep_ms(2000);

        // All off
        send_all(ws2812, 0x00000000);
        sleep_ms(2000);

        // === Test 4: Ramp byte2 to see if it has any effect ===
        printf("Ramp byte2\n");
        for (int v = 0; v <= 255; v += 16) {
            send_all(ws2812, 0x00FF0000 | static_cast<uint32_t>(v));
            sleep_ms(200);
        }
        sleep_ms(1000);
    }

    return 0;
}