#pragma once

#include <cstdint>
#include "led_controller.h"

namespace led {

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class LedAnimator {
public:
    LedAnimator(LedController& controller);

    void tick();
    void reset();

private:
    enum class Phase : uint8_t {
        FlowOn = 0,
        FlowOff = 1,
        FillColor = 2,
    };

    LedController& controller_;
    Phase phase_;
    uint step_;
    uint32_t last_tick_;
    uint32_t interval_ms_;
    uint color_idx_;

    static constexpr Color kColors[4] = {
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255},
        {255, 255, 255},
    };

    void flow_on();
    void flow_off();
    void fill_color();
    uint32_t now_ms();
};

} // namespace led