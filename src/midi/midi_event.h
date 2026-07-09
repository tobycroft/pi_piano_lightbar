#pragma once

#include <cstdint>

namespace midi {

enum class EventType : uint8_t {
    NoteOn = 0,
    NoteOff = 1,
};

struct MidiEvent {
    EventType type;
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
};

} // namespace midi