// =============================================================================
// Ws2812 — WS2812/SK6812 LED 灯条底层驱动 (PIO-based)
// =============================================================================
// 通过 RP2040 PIO 状态机驱动 WS2812 / SK6812 可寻址 LED 灯条。
// 支持 3-byte (RGB/GRB) 和 4-byte (RGBW/GRBW) 两种模式。
//
// 当前硬件配置: GRB 3-byte, 88 键, pin=28
//
// 上层应通过 LedController 调用，一般不需要直接使用本类。
// =============================================================================

#pragma once

#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

namespace led {

// 灯条颜色字节顺序
enum class ColorOrder {
    RGB,    // WS2812: 3 bytes, byte0=R, byte1=G, byte2=B
    GRB,    // WS2812 variant: 3 bytes, byte0=G, byte1=R, byte2=B
    RGBW,   // SK6812: 4 bytes, R-G-B-W
    GRBW,   // SK6812 variant: 4 bytes, G-R-B-W
};

class Ws2812 {
public:
    // pio: PIO 实例 (pio0/pio1)
    // sm:  状态机编号 (0~3)
    // pin: GPIO 数据引脚
    // num_leds: LED 数量
    // order: 颜色字节顺序 (默认 GRBW)
    Ws2812(PIO pio, uint sm, uint pin, uint num_leds,
           ColorOrder order = ColorOrder::GRBW);
    ~Ws2812();

    // 设置指定 LED 的 RGB(W) 值到缓冲区（不立即输出）
    void set(uint index, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);

    // 将缓冲区全部数据通过 PIO 发送到灯条
    void write();

    // 通过 PIO 发送全零数据清零全部 LED
    void clear();

    // 直接发送一个 32-bit wire 值到 PIO FIFO（用于调试）
    void send_wire(uint32_t data);

    uint num_leds() const { return num_leds_; }
    uint bytes_per_led() const { return bytes_per_led_; }

    // 将 RGB 值打包为 PIO 发送用的 32-bit wire 格式
    // 注意: 24-bit autopull 取高 24 位，因此数据需左移 8 位对齐
    static uint32_t rgb_to_wire(uint8_t r, uint8_t g, uint8_t b,
                                 ColorOrder order);
    static uint32_t rgbw_to_wire(uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                                  ColorOrder order);

private:
    PIO pio_;
    uint sm_;
    uint pin_;
    uint num_leds_;
    uint bytes_per_led_;
    ColorOrder color_order_;
    uint8_t* buf_;
    uint32_t freq_;
};

} // namespace led