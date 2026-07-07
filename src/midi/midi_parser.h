#pragma once

#include <cstdint>
#include <vector>

#include "midi_event.h"

namespace midi {

class MidiParser {
public:
    MidiParser() = default;

    void feed(const uint8_t* data, size_t len);
    std::vector<MidiEvent> poll();
    void reset();

private:
    static constexpr uint8_t NOTE_ON = 0x90;
    static constexpr uint8_t NOTE_OFF = 0x80;
    static constexpr size_t MAX_BUFFER = 256;

    uint8_t running_status_ = 0;
    uint8_t buffer_[MAX_BUFFER] = {};
    size_t buffer_len_ = 0;
    std::vector<MidiEvent> pending_;

    void parse_byte(uint8_t byte);
    bool try_parse_message();
};

} // namespace midi