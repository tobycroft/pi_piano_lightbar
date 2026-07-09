// =============================================================================
// ColorScheme — 白键/黑键配色方案
// =============================================================================
// 88 键钢琴中，白键与黑键使用不同颜色区分。
// 通过 BOOTSEL 短按循环切换配色方案。
//
// 白键/黑键判定:
//   MIDI note % 12 → 在八度中的位置
//   白键: C(0), D(2), E(4), F(5), G(7), A(9), B(11)
//   黑键: C#(1), D#(3), F#(6), G#(8), A#(10)
// =============================================================================

#pragma once

#include <cstdint>
#include "led/led_controller.h"

namespace piano {

// 判断指定 MIDI note 是否为白键
inline bool is_white_key(uint8_t midi_note) {
    uint8_t pos = midi_note % 12;
    // 白键在八度中的位置: 0=C, 2=D, 4=E, 5=F, 7=G, 9=A, 11=B
    return pos == 0 || pos == 2 || pos == 4 || pos == 5 ||
           pos == 7 || pos == 9 || pos == 11;
}

// 判断指定 LED index 对应的键是否为白键
inline bool is_white_key_by_index(int index) {
    if (index < 0 || index >= NUM_LEDS) return true;
    return is_white_key(index_to_note(index));
}

// 根据 LED index 和当前配色方案获取对应颜色
inline led::LedColor key_color(int index, int scheme_index);

// =============================================================================
// 配色方案定义
// =============================================================================

struct ColorScheme {
    led::LedColor white_key;
    led::LedColor black_key;
};

static constexpr ColorScheme kColorSchemes[] = {
    // 方案 0: 白键=白, 黑键=浅绿
    { led::LedColor::WHITE, led::LedColor::GRASS_GREEN },
    // 方案 1: 白键=白, 黑键=湖蓝
    { led::LedColor::WHITE, led::LedColor::LAKE_BLUE },
};

static constexpr int kNumColorSchemes =
    sizeof(kColorSchemes) / sizeof(kColorSchemes[0]);

inline led::LedColor key_color(int index, int scheme_index) {
    if (scheme_index < 0 || scheme_index >= kNumColorSchemes) {
        scheme_index = 0;
    }
    const auto& scheme = kColorSchemes[scheme_index];
    return is_white_key_by_index(index) ? scheme.white_key : scheme.black_key;
}

} // namespace piano