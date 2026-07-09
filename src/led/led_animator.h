// =============================================================================
// LedAnimator — LED chase animation (跑马灯)
// =============================================================================
// Single-LED chase animation that sweeps left-to-right, changes color,
// then sweeps right-to-left, and repeats.
// =============================================================================

#pragma once

#include <cstdint>
#include "led/led_controller.h"

namespace led {

class LedAnimator {
public:
    LedAnimator(LedController& controller);

    // Call in main loop when no MIDI device is connected
    // Returns true if the LED strip was updated
    bool tick();

    // Reset animation to initial state
    void reset();

private:
    enum class Direction {
        Forward,   // Left to right (0 → 87)
        Backward   // Right to left (87 → 0)
    };

    static constexpr uint32_t STEP_INTERVAL_MS = 25;  // Speed of chase

    uint32_t now_ms();

    LedController& controller_;
    uint32_t num_leds_;

    int         position_ = 0;
    Direction   direction_ = Direction::Forward;
    int         color_index_ = 0;
    uint32_t    last_tick_ms_ = 0;

    // Chase colors — one per sweep direction
    static constexpr LedColor CHASE_COLORS[] = {
        LedColor::RED,
        LedColor::LAKE_BLUE,
        LedColor::GREEN,
        LedColor::PINK,
        LedColor::PURPLE,
        LedColor::GRASS_GREEN,
        LedColor::BLUE,
        LedColor::WHITE,
    };
    static constexpr int NUM_CHASE_COLORS = 8;
};

} // namespace led