#pragma once

#include <cstdint>
#include "hardware/pio.h"

namespace led {

class Ws2812 {
public:
    Ws2812(PIO pio, uint sm, uint pin, uint num_leds);
    ~Ws2812();

    void set(uint index, uint8_t r, uint8_t g, uint8_t b);
    void clear();
    void write();
    uint num_leds() const { return num_leds_; }

private:
    PIO pio_;
    uint sm_;
    uint pin_;
    uint num_leds_;
    uint8_t* buf_;

    void put_pixel(uint8_t r, uint8_t g, uint8_t b);
};

} // namespace led