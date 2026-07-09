#include "usb_midi_device.h"
#include "tusb.h"

namespace usb {

UsbMidiDevice::UsbMidiDevice()
    : connected_(false) {}

bool UsbMidiDevice::is_connected() {
    if (tud_midi_mounted()) {
        connected_ = true;
    }
    return connected_;
}

std::vector<midi::MidiEvent> UsbMidiDevice::poll() {
    std::vector<midi::MidiEvent> events;

    uint8_t packet[4];
    while (tud_midi_available()) {
        if (tud_midi_packet_read(packet)) {
            uint8_t cin = packet[0] & 0x0F;
            uint8_t cable = (packet[0] >> 4) & 0x0F;
            (void)cable;

            uint8_t raw[3] = {packet[1], packet[2], packet[3]};

            switch (cin) {
                case 0x08:
                case 0x09:
                    parser_.feed(raw, 3);
                    break;
                case 0x02:
                case 0x03:
                    parser_.feed(raw, 2);
                    break;
                default:
                    break;
            }
        }
    }

    auto parsed = parser_.poll();
    events.insert(events.end(), parsed.begin(), parsed.end());
    return events;
}

void UsbMidiDevice::reset() {
    parser_.reset();
    connected_ = false;
}

} // namespace usb