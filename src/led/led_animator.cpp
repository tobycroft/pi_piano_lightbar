#include "led_animator.h"
#include "pico/time.h"

namespace led {

LedAnimator::LedAnimator(LedController& controller)
    : controller_(controller)
    , phase_(Phase::FlowOn)
    , step_(0)
    , last_tick_(0)
    , interval_ms_(15)
    , color_idx_(0) {}

void LedAnimator::tick() {
    uint32_t now = now_ms();
    if (now - last_tick_ < interval_ms_) {
        return;
    }
    last_tick_ = now;

    switch (phase_) {
        case Phase::FlowOn:
            flow_on();
            break;
        case Phase::FlowOff:
            flow_off();
            break;
        case Phase::FillColor:
            fill_color();
            break;
    }
}

void LedAnimator::reset() {
    phase_ = Phase::FlowOn;
    step_ = 0;
    last_tick_ = 0;
    interval_ms_ = 15;
    color_idx_ = 0;
}

void LedAnimator::flow_on() {
    uint num_leds = controller_.num_leds();
    if (step_ == 0) {
        controller_.clear_all();
    }
    if (step_ < num_leds) {
        controller_.set_led(step_, 255, 255, 255);
        controller_.update();
        ++step_;
    } else {
        step_ = 0;
        phase_ = Phase::FlowOff;
        interval_ms_ = 15;
    }
}

void LedAnimator::flow_off() {
    uint num_leds = controller_.num_leds();
    uint idx = num_leds - 1 - step_;
    if (idx < num_leds) {
        controller_.set_led(idx, 0, 0, 0);
        controller_.update();
        ++step_;
    } else {
        step_ = 0;
        phase_ = Phase::FillColor;
        interval_ms_ = 300;
    }
}

void LedAnimator::fill_color() {
    uint num_leds = controller_.num_leds();
    Color c = kColors[color_idx_];
    for (uint i = 0; i < num_leds; ++i) {
        controller_.set_led(i, c.r, c.g, c.b);
    }
    controller_.update();

    color_idx_ = (color_idx_ + 1) % 4;
    if (color_idx_ == 0) {
        phase_ = Phase::FlowOn;
        interval_ms_ = 15;
        controller_.clear_all();
    }
}

uint32_t LedAnimator::now_ms() {
    return to_ms_since_boot(get_absolute_time());
}

} // namespace led