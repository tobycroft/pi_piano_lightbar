// =============================================================================
// LedController — 88-key LED 颜色控制器
// =============================================================================
// 封装 Ws2812 底层驱动，向上层提供"按键序号 + 颜色名"的调用接口。
// 所有颜色定义集中在 colorToRgb() 中，外部调用者无需接触 RGB 色值。
//
// 使用方式:
//   led::LedController leds(ws2812);
//   leds.setKey(0, led::LedColor::RED);        // 第 0 号键亮红色
//   leds.setKey(60, led::LedColor::LAKE_BLUE);  // 第 60 号键亮湖蓝色
//   leds.setAllKeys(led::LedColor::OFF);        // 全部灭灯
//   leds.update();                               // 推送到硬件
//
// 添加新颜色:
//   1. 在 LedColor 枚举中加一项
//   2. 在 led_controller.cpp 的 colorToRgb() 中加一个 case
// =============================================================================

#pragma once

#include <cstdint>
#include "led/ws2812.h"

namespace led {

// 颜色预设枚举 — 外部调用只需传颜色名，不需传 RGB 数值
enum class LedColor {
    RED,          // 红色    R=255
    GREEN,        // 绿色    G=255
    BLUE,         // 蓝色    B=255
    WHITE,        // 白色    R+G+B 全亮
    LAKE_BLUE,    // 湖蓝色  青+蓝混合
    GRASS_GREEN,  // 青草绿色 绿为主+少量红
    PINK,         // 粉色    红+蓝混合
    PURPLE,       // 紫色    红+蓝混合，蓝偏多
    OFF           // 灭灯
};

class LedController {
public:
    // 传入已初始化的 Ws2812 实例（需已配置好 ColorOrder::GRB, NUM_LEDS=88）
    LedController(Ws2812& ws2812);

    // 设置指定按键的颜色（不立即输出，需调用 update()）
    // index: 0~87, 对应 A0~C8
    void setKey(uint index, LedColor color);

    // 设置全部 88 键为同一颜色
    void setAllKeys(LedColor color);

    // 设置全局亮度 (0~255)，影响后续所有 setKey/setAllKeys 调用
    // 默认 255 = 全亮
    void setBrightness(uint8_t brightness);
    uint8_t brightness() const;

    // 将缓冲区数据推送到 LED 硬件
    void update();

    // 直接通过 PIO 清零全部 LED（不经过缓冲区）
    void clear();

    uint numLeds() const;

private:
    struct RGB {
        uint8_t r, g, b;
    };

    // 颜色名 → RGB 色值映射（唯一需要修改颜色定义的地方）
    static RGB colorToRgb(LedColor color);

    Ws2812& ws_;
    uint8_t brightness_ = 255;
};

} // namespace led