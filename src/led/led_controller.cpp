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

// =============================================================================
// 颜色映射表 — 所有颜色 RGB 值集中定义在此
// 添加新颜色: 在 LedColor 枚举加一项，在此函数加一个 case
// 硬件: WS2812 GRB 3-byte 灯条, 88 键
// =============================================================================
LedController::RGB LedController::colorToRgb(LedColor color) {
    switch (color) {
        case LedColor::RED:         return {255, 0, 0};
        case LedColor::GREEN:       return {0, 255, 0};
        case LedColor::BLUE:        return {0, 0, 255};
        case LedColor::WHITE:       return {255, 255, 255};   // R+G+B 三色混合
        case LedColor::LAKE_BLUE:   return {0, 180, 255};     // 湖蓝色: 青+蓝
        case LedColor::GRASS_GREEN: return {60, 255, 30};     // 青草绿: 绿为主+少量红
        case LedColor::PINK:        return {255, 80, 180};    // 粉色: 红+蓝
        case LedColor::PURPLE:      return {100, 0, 255};     // 紫色: 蓝为主+少量红
        case LedColor::OFF:
        default:                    return {0, 0, 0};
    }
}

} // namespace led