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

    printf("=== SK6812 GRB 3-Byte ===\n");

    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS, led::ColorOrder::GRB);

    while (true) {
        printf("RED\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 0, 0);
        ws2812.write();
        sleep_ms(1000);

        printf("GREEN\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 255, 0);
        ws2812.write();
        sleep_ms(1000);

        printf("BLUE\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 0, 255);
        ws2812.write();
        sleep_ms(1000);

        printf("YELLOW\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 255, 0);
        ws2812.write();
        sleep_ms(1000);

        printf("CYAN\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 255, 255);
        ws2812.write();
        sleep_ms(1000);

        printf("MAGENTA\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 255, 0, 255);
        ws2812.write();
        sleep_ms(1000);

        printf("OFF\n");
        for (uint i = 0; i < NUM_LEDS; i++) ws2812.set(i, 0, 0, 0);
        ws2812.write();
        sleep_ms(1000);
    }

    return 0;
}