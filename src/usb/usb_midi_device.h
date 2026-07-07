#pragma once

#include "usb_midi.h"
#include "midi/midi_parser.h"

namespace usb {

class UsbMidiDevice : public UsbMidiIn {
public:
    UsbMidiDevice();
    ~UsbMidiDevice() override = default;

    bool is_connected() override;
    std::vector<midi::MidiEvent> poll() override;
    void reset() override;

private:
    midi::MidiParser parser_;
    bool connected_;
};

} // namespace usb