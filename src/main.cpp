#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "led/ws2812.h"

static constexpr uint LED_PIN = 28;
static constexpr uint NUM_LEDS = 88;
static constexpr uint PICO_ONBOARD_LED = 25;

int main() {
    stdio_init_all();

    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);

    printf("=== WS2812 GRB 3-Byte ===\n");

    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS, led::ColorOrder::GRB);

    while (true) {
        // --- Channel 1: R only ---
        printf("R channel only\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 0, 0);
        ws2812.write();
        sleep_ms(3000);

        printf("OFF\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 0, 0);
        ws2812.write();
        sleep_ms(1500);

        // --- Channel 2: G only ---
        printf("G channel only\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 255, 0);
        ws2812.write();
        sleep_ms(3000);

        printf("OFF\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 0, 0);
        ws2812.write();
        sleep_ms(1500);

        // --- Channel 3: B only ---
        printf("B channel only\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 0, 255);
        ws2812.write();
        sleep_ms(3000);

        printf("OFF\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 0, 0);
        ws2812.write();
        sleep_ms(2000);
    }

    return 0;
}