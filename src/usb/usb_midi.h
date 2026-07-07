#pragma once

#include <cstdint>
#include <vector>
#include "midi/midi_event.h"

namespace usb {

class UsbMidiIn {
public:
    virtual ~UsbMidiIn() = default;

    virtual bool is_connected() = 0;
    virtual std::vector<midi::MidiEvent> poll() = 0;
    virtual void reset() = 0;
};

} // namespace usb