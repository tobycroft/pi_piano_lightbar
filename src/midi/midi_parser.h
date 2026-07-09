// =============================================================================
// MidiParser — USB MIDI packet → MidiEvent parser
// =============================================================================
// Parses USB MIDI 4-byte packets into MidiEvent structs.
// USB MIDI packet format:
//   Byte 0: Cable Number (low nibble)
//   Byte 1: Code Index Number (CIN)
//   Byte 2: MIDI byte 1
//   Byte 3: MIDI byte 2
// CIN: 0x8=Note Off, 0x9=Note On
// =============================================================================

#pragma once

#include <cstdint>
#include <vector>
#include "midi/midi_event.h"

namespace midi {

class MidiParser {
public:
    MidiParser() = default;

    // Parse a USB MIDI packet (4 bytes) and return a MidiEvent if valid
    // Returns true if a valid event was produced
    bool parse_packet(uint32_t packet, MidiEvent& event);

    // Convenience: parse a batch of packets
    void parse_packets(const uint32_t* packets, size_t count,
                       std::vector<MidiEvent>& events);

    void reset();

private:
    static constexpr uint8_t CIN_NOTE_OFF = 0x8;
    static constexpr uint8_t CIN_NOTE_ON  = 0x9;

    static uint8_t get_cin(uint32_t packet);
    static uint8_t get_cable(uint32_t packet);
    static uint8_t get_midi_byte1(uint32_t packet);
    static uint8_t get_midi_byte2(uint32_t packet);
};

} // namespace midi