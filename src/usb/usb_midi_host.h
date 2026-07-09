#pragma once

#include "tusb.h"
#include "usb_midi.h"
#include "midi/midi_parser.h"

#include <cstdint>

namespace usb {

class UsbMidiHost : public UsbMidiIn {
public:
    UsbMidiHost();
    ~UsbMidiHost() override = default;

    bool is_connected() override;
    std::vector<midi::MidiEvent> poll() override;
    void reset() override;

    bool init();

    // Called by global C callbacks from TinyUSB
    void on_mount(uint8_t daddr, tusb_desc_device_t const* desc);
    void on_umount(uint8_t daddr);

    // Debug flags for visual feedback via LED strip
    // These are set from callbacks, cleared by main loop
    volatile uint32_t debug_flags = 0;
    static constexpr uint32_t DEBUG_MOUNT      = 1 << 0;  // Device detected
    static constexpr uint32_t DEBUG_CONFIG_DONE = 1 << 1;  // Endpoint opened
    static constexpr uint32_t DEBUG_DATA_RX     = 1 << 2;  // Data received

private:
    friend void xfer_callback_internal(tuh_xfer_t* xfer);
    friend void config_complete_cb(tuh_xfer_t* xfer);

    midi::MidiParser parser_;

    bool connected_ = false;
    bool inited_ = false;

    uint8_t dev_addr_ = 0;
    uint8_t ep_in_ = 0;
    uint8_t ep_in_size_ = 0;

    // Saved endpoint descriptor from the device
    tusb_desc_endpoint_t ep_in_desc_ = {};

    static constexpr size_t RX_BUFFER_SIZE = 64;
    uint8_t rx_buffer_[RX_BUFFER_SIZE] = {};

    void start_receive();
    void process_packet(const uint8_t* packet, uint32_t len);
};

} // namespace usb