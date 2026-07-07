#pragma once

#include <cstdint>
#include "ws2812.h"

namespace led {

class LedController {
public:
    LedController(Ws2812& ws2812);

    void set_led(uint index, uint8_t r, uint8_t g, uint8_t b);
    void clear_led(uint index);
    void clear_all();
    void update();
    uint num_leds() const;

private:
    Ws2812& ws2812_;
};

} // namespace led