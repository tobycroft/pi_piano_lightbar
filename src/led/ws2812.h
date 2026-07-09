#pragma once

#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

namespace led {

enum class ColorOrder {
    RGB,    // WS2812: 3 bytes per LED
    GRB,    // WS2812 variant
    RGBW,   // SK6812: 4 bytes per LED, R-G-B-W order
    GRBW,   // SK6812 variant: G-R-B-W order
};

class Ws2812 {
public:
    Ws2812(PIO pio, uint sm, uint pin, uint num_leds,
           ColorOrder order = ColorOrder::GRBW);
    ~Ws2812();

    void set(uint index, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);
    void write();
    void clear();
    void send_wire(uint32_t data);

    uint num_leds() const { return num_leds_; }
    uint bytes_per_led() const { return bytes_per_led_; }

    static uint32_t rgb_to_wire(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t rgbw_to_wire(uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                                  ColorOrder order);

private:
    PIO pio_;
    uint sm_;
    uint pin_;
    uint num_leds_;
    uint bytes_per_led_;
    ColorOrder color_order_;
    uint8_t* buf_;
    uint32_t freq_;
};

} // namespace led