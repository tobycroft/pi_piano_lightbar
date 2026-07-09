#include "led/led_controller.h"

namespace led {

LedController::LedController(Ws2812& ws2812)
    : ws_(ws2812)
{
}

void LedController::setKey(uint index, LedColor color) {
    auto rgb = colorToRgb(color);
    ws_.set(index, rgb.r, rgb.g, rgb.b);
}

void LedController::setAllKeys(LedColor color) {
    auto rgb = colorToRgb(color);
    for (uint i = 0; i < ws_.num_leds(); i++) {
        ws_.set(i, rgb.r, rgb.g, rgb.b);
    }
}

void LedController::update() {
    ws_.write();
}

void LedController::clear() {
    ws_.clear();
}

uint LedController::numLeds() const {
    return ws_.num_leds();
}

LedController::RGB LedController::colorToRgb(LedColor color) {
    switch (color) {
        case LedColor::RED:         return {255, 0, 0};
        case LedColor::GREEN:       return {0, 255, 0};
        case LedColor::BLUE:        return {0, 0, 255};
        case LedColor::WHITE:       return {255, 255, 255};
        case LedColor::LAKE_BLUE:   return {0, 180, 255};
        case LedColor::GRASS_GREEN: return {60, 255, 30};
        case LedColor::PINK:        return {255, 80, 180};
        case LedColor::PURPLE:      return {160, 0, 255};
        case LedColor::OFF:
        default:                    return {0, 0, 0};
    }
}

} // namespace led