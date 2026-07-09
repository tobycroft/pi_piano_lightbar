#pragma once

#include <cstdint>
#include "led/ws2812.h"

namespace led {

enum class LedColor {
    RED,
    GREEN,
    BLUE,
    WHITE,        // R+G+B mixed
    LAKE_BLUE,    // 湖蓝色
    GRASS_GREEN,  // 青草绿色
    PINK,         // 粉色
    PURPLE,       // 紫色
    OFF
};

class LedController {
public:
    LedController(Ws2812& ws2812);

    void setKey(uint index, LedColor color);
    void setAllKeys(LedColor color);
    void update();
    void clear();

    uint numLeds() const;

private:
    struct RGB {
        uint8_t r, g, b;
    };

    static RGB colorToRgb(LedColor color);

    Ws2812& ws_;
};

} // namespace led