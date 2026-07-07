#include "led_controller.h"

namespace led {

LedController::LedController(Ws2812& ws2812)
    : ws2812_(ws2812) {}

void LedController::set_led(uint index, uint8_t r, uint8_t g, uint8_t b) {
    ws2812_.set(index, r, g, b);
}

void LedController::clear_led(uint index) {
    ws2812_.set(index, 0, 0, 0);
}

void LedController::clear_all() {
    ws2812_.clear();
}

void LedController::update() {
    ws2812_.write();
}

uint LedController::num_leds() const {
    return ws2812_.num_leds();
}

} // namespace led