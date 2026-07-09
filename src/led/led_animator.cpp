// =============================================================================
// LedAnimator implementation — chase animation
// =============================================================================

#include "led/led_animator.h"
#include "pico/time.h"

namespace led {

constexpr LedColor LedAnimator::CHASE_COLORS[];

LedAnimator::LedAnimator(LedController& controller)
    : controller_(controller)
    , num_leds_(controller.numLeds())
{
}

uint32_t LedAnimator::now_ms() {
    return to_ms_since_boot(get_absolute_time());
}

bool LedAnimator::tick() {
    uint32_t now = now_ms();

    if (now - last_tick_ms_ < STEP_INTERVAL_MS) {
        return false;
    }
    last_tick_ms_ = now;

    // Clear previous position
    controller_.setKey(static_cast<uint>(position_), LedColor::OFF);

    // Advance position
    if (direction_ == Direction::Forward) {
        position_++;
        if (position_ >= static_cast<int>(num_leds_)) {
            position_ = static_cast<int>(num_leds_) - 2;
            direction_ = Direction::Backward;
            color_index_ = (color_index_ + 1) % NUM_CHASE_COLORS;
        }
    } else {
        position_--;
        if (position_ < 0) {
            position_ = 1;
            direction_ = Direction::Forward;
            color_index_ = (color_index_ + 1) % NUM_CHASE_COLORS;
        }
    }

    // Set new position with current color
    LedColor color = CHASE_COLORS[color_index_];
    controller_.setKey(static_cast<uint>(position_), color);

    controller_.update();
    return true;
}

void LedAnimator::reset() {
    position_ = 0;
    direction_ = Direction::Forward;
    color_index_ = 0;
    last_tick_ms_ = 0;

    controller_.setAllKeys(LedColor::OFF);
    controller_.update();
}

} // namespace led