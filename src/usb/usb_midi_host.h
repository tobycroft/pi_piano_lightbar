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

    void on_mount(uint8_t daddr, tusb_desc_device_t const* desc);
    void on_umount(uint8_t daddr);

private:
    friend void xfer_callback_internal(tuh_xfer_t* xfer);

    midi::MidiParser parser_;

    bool connected_ = false;
    bool inited_ = false;

    uint8_t dev_addr_ = 0;
    uint8_t ep_in_ = 0;
    uint8_t ep_in_size_ = 0;

    static constexpr size_t RX_BUFFER_SIZE = 64;
    uint8_t rx_buffer_[RX_BUFFER_SIZE] = {};
    volatile bool xfer_done_ = false;
    volatile uint32_t xfer_len_ = 0;

    void start_receive();
    void process_packet(const uint8_t* packet, uint32_t len);
};

} // namespace usb