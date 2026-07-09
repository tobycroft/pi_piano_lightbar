// =============================================================================
// PianoConfig — 88-key piano constants and note-to-index mapping
// =============================================================================
// Standard 88-key piano: A0=21, C8=108
// LED index = note - 21 → range 0~87
// =============================================================================

#pragma once

#include <cstdint>

namespace piano {

// MIDI note range for 88-key piano
constexpr uint8_t MIDI_NOTE_MIN = 21;   // A0
constexpr uint8_t MIDI_NOTE_MAX = 108;  // C8
constexpr uint8_t NUM_LEDS     = 88;    // 88 keys

// Convert MIDI note number to LED index
// Returns -1 if note is outside valid range
inline int note_to_index(uint8_t note) {
    if (note < MIDI_NOTE_MIN || note > MIDI_NOTE_MAX) {
        return -1;
    }
    return static_cast<int>(note - MIDI_NOTE_MIN);
}

// Convert LED index to MIDI note number
inline uint8_t index_to_note(int index) {
    return static_cast<uint8_t>(index + MIDI_NOTE_MIN);
}

} // namespace piano