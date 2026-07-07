#pragma once

#include <cstdint>

namespace piano {

constexpr uint8_t MIDI_NOTE_A0 = 21;
constexpr uint8_t MIDI_NOTE_C8 = 108;
constexpr uint8_t NUM_LEDS = 88;

inline bool is_valid_note(uint8_t note) {
    return note >= MIDI_NOTE_A0 && note <= MIDI_NOTE_C8;
}

inline int note_to_index(uint8_t note) {
    if (!is_valid_note(note)) {
        return -1;
    }
    return static_cast<int>(note - MIDI_NOTE_A0);
}

} // namespace piano