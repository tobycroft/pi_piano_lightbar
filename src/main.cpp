#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "led/ws2812.h"

static constexpr uint LED_PIN = 28;
static constexpr uint NUM_LEDS = 88;
static constexpr uint PICO_ONBOARD_LED = 25;

void send_all(led::Ws2812& ws, uint32_t wire_data) {
    for (uint i = 0; i < ws.num_leds(); i++) {
        ws.send_wire(wire_data);
    }
    sleep_us(300);
}

// SK6812 RGBW wire format: byte0=G, byte1=R, byte2=B, byte3=W
uint32_t wire_grbw(uint8_t g, uint8_t r, uint8_t b, uint8_t w) {
    return (static_cast<uint32_t>(g) << 24) |
           (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(b) << 8)  |
           static_cast<uint32_t>(w);
}

int main() {
    stdio_init_all();

    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);

    printf("=== SK6812 RGBW GRBW Order ===\n");

    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS, led::ColorOrder::GRBW);

    while (true) {
        printf("RED\n");
        send_all(ws2812, wire_grbw(0, 255, 0, 0));
        sleep_ms(1000);

        printf("GREEN\n");
        send_all(ws2812, wire_grbw(255, 0, 0, 0));
        sleep_ms(1000);

        printf("BLUE\n");
        send_all(ws2812, wire_grbw(0, 0, 255, 0));
        sleep_ms(1000);

        printf("WHITE\n");
        send_all(ws2812, wire_grbw(0, 0, 0, 255));
        sleep_ms(1000);

        printf("YELLOW\n");
        send_all(ws2812, wire_grbw(255, 255, 0, 0));
        sleep_ms(1000);

        printf("CYAN\n");
        send_all(ws2812, wire_grbw(255, 0, 255, 0));
        sleep_ms(1000);

        printf("MAGENTA\n");
        send_all(ws2812, wire_grbw(0, 255, 255, 0));
        sleep_ms(1000);

        printf("OFF\n");
        send_all(ws2812, 0);
        sleep_ms(1000);
    }

    return 0;
}