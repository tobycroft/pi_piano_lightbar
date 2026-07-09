// =============================================================================
// MidiEvent — MIDI event data structure
// =============================================================================

#pragma once

#include <cstdint>

namespace midi {

enum class EventType : uint8_t {
    NoteOn,
    NoteOff,
    Unknown
};

struct MidiEvent {
    EventType type;
    uint8_t   channel;
    uint8_t   note;
    uint8_t   velocity;
};

} // namespace midi