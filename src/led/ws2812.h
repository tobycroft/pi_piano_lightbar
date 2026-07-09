#pragma once

#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

namespace led {

class Ws2812 {
public:
    Ws2812(PIO pio, uint sm, uint pin, uint num_leds);
    ~Ws2812();

    void set(uint index, uint8_t r, uint8_t g, uint8_t b);
    void write();
    void clear();
    void send_wire(uint32_t data);

    uint num_leds() const { return num_leds_; }

    // Pack 3 bytes into wire format (byte0=R, byte1=G, byte2=B)
    static uint32_t rgb_to_wire(uint8_t r, uint8_t g, uint8_t b);

private:
    PIO pio_;
    uint sm_;
    uint pin_;
    uint num_leds_;
    uint8_t* buf_;
    uint32_t freq_;
};

} // namespace led