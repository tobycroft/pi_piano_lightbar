// =============================================================================
// MidiParser — USB MIDI 4-byte packet → MidiEvent parser
// =============================================================================
// USB MIDI 4-byte packet format (from wire):
//   Byte 0: bits 7-4 = Cable Number, bits 3-0 = Code Index Number (CIN)
//   Byte 1: MIDI byte 1
//   Byte 2: MIDI byte 2
//   Byte 3: MIDI byte 3
//
// The uint32_t packing in this project:
//   bits 0-7   = wire byte 0 (Cable | CIN)
//   bits 8-15  = wire byte 1 (MIDI byte 1)
//   bits 16-23 = wire byte 2 (MIDI byte 2)
//   bits 24-31 = wire byte 3 (MIDI byte 3)
//
// Two formats are handled:
//   A) Status-byte included: byte1 = 0x9n/0x8n, byte2 = note, byte3 = velocity
//   B) Data-only (TinyUSB-style): byte1 = note, byte2 = velocity
// =============================================================================

#include "midi/midi_parser.h"

namespace midi {

uint8_t MidiParser::get_cin(uint32_t packet) {
    return packet & 0x0F;
}

uint8_t MidiParser::get_cable(uint32_t packet) {
    return (packet >> 4) & 0x0F;
}

uint8_t MidiParser::get_midi_byte1(uint32_t packet) {
    return (packet >> 8) & 0xFF;
}

uint8_t MidiParser::get_midi_byte2(uint32_t packet) {
    return (packet >> 16) & 0xFF;
}

bool MidiParser::parse_packet(uint32_t packet, MidiEvent& event) {
    uint8_t cin = get_cin(packet);
    uint8_t byte1 = get_midi_byte1(packet);
    uint8_t byte2 = get_midi_byte2(packet);

    uint8_t note = 0;
    uint8_t velocity = 0;
    uint8_t channel = 0;

    // Detect format: if byte1 has bit 7 set, it's a status byte
    if (byte1 & 0x80) {
        // Format A: [status+channel, note, velocity]
        channel = byte1 & 0x0F;
        note = byte2 & 0x7F;
        // Velocity is in byte 3 (bits 24-31)
        velocity = (packet >> 24) & 0x7F;
    } else {
        // Format B: [note, velocity, 0]
        channel = 0;
        note = byte1 & 0x7F;
        velocity = byte2 & 0x7F;
    }

    if (cin == CIN_NOTE_ON) {
        if (velocity > 0) {
            event.type = EventType::NoteOn;
            event.channel = channel;
            event.note = note;
            event.velocity = velocity;
            return true;
        } else {
            event.type = EventType::NoteOff;
            event.channel = channel;
            event.note = note;
            event.velocity = 0;
            return true;
        }
    }

    if (cin == CIN_NOTE_OFF) {
        event.type = EventType::NoteOff;
        event.channel = channel;
        event.note = note;
        event.velocity = velocity;
        return true;
    }

    return false;
}

void MidiParser::parse_packets(const uint32_t* packets, size_t count,
                                std::vector<MidiEvent>& events) {
    for (size_t i = 0; i < count; i++) {
        MidiEvent event;
        if (parse_packet(packets[i], event)) {
            events.push_back(event);
        }
    }
}

void MidiParser::reset() {
    // No state to reset for now
}

} // namespace midi