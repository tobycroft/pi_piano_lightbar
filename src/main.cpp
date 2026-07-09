#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "led/ws2812.h"
#include "led/led_controller.h"

static constexpr uint LED_PIN = 28;
static constexpr uint NUM_LEDS = 88;
static constexpr uint PICO_ONBOARD_LED = 25;

int main() {
    stdio_init_all();

    gpio_init(PICO_ONBOARD_LED);
    gpio_set_dir(PICO_ONBOARD_LED, GPIO_OUT);
    gpio_put(PICO_ONBOARD_LED, 1);

    printf("=== LedController Color Test ===\n");

    led::Ws2812 ws2812(pio0, 0, LED_PIN, NUM_LEDS, led::ColorOrder::GRB);
    led::LedController leds(ws2812);

    while (true) {
        printf("RED\n");
        leds.setAllKeys(led::LedColor::RED);
        leds.update();
        sleep_ms(1500);

        printf("GREEN\n");
        leds.setAllKeys(led::LedColor::GREEN);
        leds.update();
        sleep_ms(1500);

        printf("BLUE\n");
        leds.setAllKeys(led::LedColor::BLUE);
        leds.update();
        sleep_ms(1500);

        printf("WHITE\n");
        leds.setAllKeys(led::LedColor::WHITE);
        leds.update();
        sleep_ms(1500);

        printf("LAKE_BLUE\n");
        leds.setAllKeys(led::LedColor::LAKE_BLUE);
        leds.update();
        sleep_ms(1500);

        printf("GRASS_GREEN\n");
        leds.setAllKeys(led::LedColor::GRASS_GREEN);
        leds.update();
        sleep_ms(1500);

        printf("PINK\n");
        leds.setAllKeys(led::LedColor::PINK);
        leds.update();
        sleep_ms(1500);

        printf("PURPLE\n");
        leds.setAllKeys(led::LedColor::PURPLE);
        leds.update();
        sleep_ms(1500);

        printf("OFF\n");
        leds.setAllKeys(led::LedColor::OFF);
        leds.update();
        sleep_ms(1500);
    }

    return 0;
}