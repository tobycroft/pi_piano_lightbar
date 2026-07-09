// =============================================================================
// UsbMidiHost — USB MIDI Host driver (TinyUSB custom class driver)
// =============================================================================
// Implements a TinyUSB host class driver for USB MIDI (Audio class,
// MIDI Streaming subclass).  Uses the usbh_app_driver_get_cb mechanism
// to register with the TinyUSB host stack.
//
// Usage:
//   UsbMidiHost host;
//   host.init();                    // Initialize USB host
//   while (true) {
//       tuh_task();                 // Process USB events
//       if (host.is_connected()) {
//           auto events = host.poll();  // Get MIDI events
//       }
//   }
// =============================================================================

#pragma once

#include <cstdint>
#include <vector>
#include "tusb.h"
#include "midi/midi_event.h"
#include "midi/midi_parser.h"

namespace usb {

class UsbMidiHost {
public:
    UsbMidiHost() = default;

    // Initialize USB host stack.  Registers the MIDI host class driver.
    // Returns true on success.
    bool init();

    // Check if a MIDI device is connected and fully enumerated
    bool is_connected() const;

    // Check if MIDI data was received since last call (clears flag)
    bool has_midi_activity();

    // Poll for accumulated MIDI events
    std::vector<midi::MidiEvent> poll();

    // Reset state (on disconnect)
    void reset();

    // Called from the TinyUSB class driver callbacks
    void on_mount(uint8_t daddr, uint8_t itf_num);
    void on_umount(uint8_t daddr);
    void on_rx_data(const uint8_t* data, uint32_t len);

private:
    static constexpr size_t MAX_PACKETS = 16;

    uint8_t  dev_addr_ = 0;
    bool     connected_ = false;
    bool     midi_activity_ = false;
    midi::MidiParser parser_;

    uint32_t rx_packets_[MAX_PACKETS];
    size_t   rx_count_ = 0;
};

// Global pointer for use by the C-linkage class driver callbacks
extern UsbMidiHost* g_usb_midi_host;

} // namespace usb