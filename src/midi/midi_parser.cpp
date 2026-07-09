#include "midi_parser.h"

namespace midi {

void MidiParser::feed(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        parse_byte(data[i]);
    }
}

std::vector<MidiEvent> MidiParser::poll() {
    std::vector<MidiEvent> result;
    result.swap(pending_);
    return result;
}

void MidiParser::reset() {
    running_status_ = 0;
    buffer_len_ = 0;
    pending_.clear();
}

void MidiParser::parse_byte(uint8_t byte) {
    if (buffer_len_ >= MAX_BUFFER) {
        buffer_len_ = 0;
    }

    if (byte & 0x80) {
        running_status_ = byte;
        buffer_len_ = 0;
    }

    buffer_[buffer_len_++] = byte;

    if (running_status_ == 0) {
        buffer_len_ = 0;
        return;
    }

    try_parse_message();
}

bool MidiParser::try_parse_message() {
    uint8_t status = running_status_;
    uint8_t msg_type = status & 0xF0;

    if (msg_type != NOTE_ON && msg_type != NOTE_OFF) {
        buffer_len_ = 0;
        return false;
    }

    size_t needed = 2;
    if (buffer_len_ < needed) {
        return false;
    }

    uint8_t note = buffer_[0] & 0x7F;
    uint8_t velocity = buffer_[1] & 0x7F;

    MidiEvent event;
    event.note = note;
    event.velocity = velocity;
    event.channel = status & 0x0F;

    if (msg_type == NOTE_ON && velocity > 0) {
        event.type = EventType::NoteOn;
    } else {
        event.type = EventType::NoteOff;
    }

    pending_.push_back(event);
    buffer_len_ = 0;
    return true;
}

} // namespace midi