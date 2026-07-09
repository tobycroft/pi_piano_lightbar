// =============================================================================
// UsbMidiHost implementation — TinyUSB custom host class driver
// =============================================================================
// Since TinyUSB does not ship a MIDI host class driver, we implement one
// using the usbh_app_driver_get_cb() extension mechanism.
//
// The driver:
//   1. Matches interfaces with bInterfaceClass==AUDIO(1) and
//      bInterfaceSubClass==MIDI_STREAMING(3)
//   2. Opens the IN endpoint (bulk or interrupt) found in the descriptor
//   3. Submits a receive transfer during set_config
//   4. On each xfer completion, parses USB MIDI 4-byte packets and
//      re-submits the receive transfer
// =============================================================================

#include "usb/usb_midi_host.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/audio/audio.h"
#include "class/midi/midi.h"

#include <cstdio>
#include <cstring>

namespace usb {

// =============================================================================
// Global instance pointer
// =============================================================================
UsbMidiHost* g_usb_midi_host = nullptr;

// =============================================================================
// MIDI Host Class Driver State
// =============================================================================
#ifndef CFG_TUH_MIDI
#define CFG_TUH_MIDI 1
#endif

// Per-interface state
struct MidiHInterface {
    uint8_t daddr;
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;
    uint16_t epin_size;
    bool mounted;
    uint8_t rx_buf[64];
};

static MidiHInterface _midi_itf[CFG_TUH_MIDI];

// Find an unused interface slot
static MidiHInterface* find_new_itf(void) {
    for (int i = 0; i < CFG_TUH_MIDI; i++) {
        if (_midi_itf[i].daddr == 0) return &_midi_itf[i];
    }
    return nullptr;
}

// Find interface by device address
static MidiHInterface* find_itf_by_daddr(uint8_t daddr) {
    for (int i = 0; i < CFG_TUH_MIDI; i++) {
        if (_midi_itf[i].daddr == daddr) return &_midi_itf[i];
    }
    return nullptr;
}

// Find interface by device address + endpoint address
static MidiHInterface* find_itf_by_ep(uint8_t daddr, uint8_t ep_addr) {
    for (int i = 0; i < CFG_TUH_MIDI; i++) {
        if (_midi_itf[i].daddr == daddr && _midi_itf[i].ep_in == ep_addr) {
            return &_midi_itf[i];
        }
    }
    return nullptr;
}

// =============================================================================
// Class Driver Callbacks
// =============================================================================
static bool midih_init(void) {
    for (int i = 0; i < CFG_TUH_MIDI; i++) {
        _midi_itf[i].daddr = 0;
    }
    return true;
}

static bool midih_open(uint8_t rhport, uint8_t daddr,
                       tusb_desc_interface_t const* desc_itf, uint16_t max_len) {
    (void) rhport;
    (void) max_len;

    // Match: Audio class + MIDI Streaming subclass
    if (desc_itf->bInterfaceClass != TUSB_CLASS_AUDIO) return false;
    if (desc_itf->bInterfaceSubClass != AUDIO_SUBCLASS_MIDI_STREAMING) return false;

    printf("MIDIH: opening interface %d on device %d\n",
           desc_itf->bInterfaceNumber, daddr);

    MidiHInterface* p_itf = find_new_itf();
    if (!p_itf) return false;

    p_itf->daddr = daddr;
    p_itf->itf_num = desc_itf->bInterfaceNumber;

    // Walk endpoint descriptors
    uint8_t const* p_desc = (uint8_t const*) desc_itf;
    p_desc = tu_desc_next(p_desc); // skip interface descriptor

    for (int i = 0; i < desc_itf->bNumEndpoints; i++) {
        tusb_desc_endpoint_t const* desc_ep =
            (tusb_desc_endpoint_t const*) p_desc;

        if (desc_ep->bDescriptorType != TUSB_DESC_ENDPOINT) {
            p_desc = tu_desc_next(p_desc);
            continue;
        }

        if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_IN) {
            p_itf->ep_in = desc_ep->bEndpointAddress;
            p_itf->epin_size = tu_edpt_packet_size(desc_ep);
        } else {
            p_itf->ep_out = desc_ep->bEndpointAddress;
        }

        // Open the endpoint with the host stack
        tuh_edpt_open(daddr, desc_ep);

        p_desc = tu_desc_next(p_desc);
    }

    return true;
}

static bool midih_set_config(uint8_t daddr, uint8_t itf_num) {
    MidiHInterface* p_itf = nullptr;
    for (int i = 0; i < CFG_TUH_MIDI; i++) {
        if (_midi_itf[i].daddr == daddr && _midi_itf[i].itf_num == itf_num) {
            p_itf = &_midi_itf[i];
            break;
        }
    }
    if (!p_itf) return false;

    // Device is fully enumerated
    p_itf->mounted = true;

    printf("MIDIH: device %d mounted (ep_in=0x%02x)\n", daddr, p_itf->ep_in);

    if (g_usb_midi_host) {
        g_usb_midi_host->on_mount(daddr, itf_num);
    }

    // Start receiving if we have an IN endpoint
    if (p_itf->ep_in != 0) {
        usbh_edpt_xfer(daddr, p_itf->ep_in,
                       p_itf->rx_buf, p_itf->epin_size);
    }

    return true;
}

static bool midih_xfer_cb(uint8_t daddr, uint8_t ep_addr,
                          xfer_result_t result, uint32_t xferred_bytes) {
    MidiHInterface* p_itf = find_itf_by_ep(daddr, ep_addr);
    if (!p_itf) return false;

    if (result == XFER_RESULT_SUCCESS && xferred_bytes > 0) {
        if (g_usb_midi_host) {
            g_usb_midi_host->on_rx_data(p_itf->rx_buf, xferred_bytes);
        }
    }

    // Re-submit receive to keep the pipe alive
    if (p_itf->ep_in != 0) {
        usbh_edpt_xfer(daddr, p_itf->ep_in,
                       p_itf->rx_buf, p_itf->epin_size);
    }

    return true;
}

static void midih_close(uint8_t daddr) {
    MidiHInterface* p_itf = find_itf_by_daddr(daddr);
    if (!p_itf) return;

    printf("MIDIH: device %d closed\n", daddr);

    if (g_usb_midi_host) {
        g_usb_midi_host->on_umount(daddr);
    }

    p_itf->daddr = 0;
    p_itf->mounted = false;
}

// =============================================================================
// Class Driver Registration
// =============================================================================
static usbh_class_driver_t _midih_driver = {
    "MIDIH",        // .name
    midih_init,     // .init
    nullptr,        // .deinit
    midih_open,     // .open
    midih_set_config, // .set_config
    midih_xfer_cb,  // .xfer_cb
    midih_close     // .close
};

// Override TinyUSB weak function to register our custom driver
extern "C" usbh_class_driver_t const* usbh_app_driver_get_cb(
    uint8_t* driver_count) {
    *driver_count = 1;
    return &_midih_driver;
}

// =============================================================================
// UsbMidiHost Public API
// =============================================================================
bool UsbMidiHost::init() {
    g_usb_midi_host = this;

    if (!tuh_init(BOARD_TUH_RHPORT)) {
        printf("UsbMidiHost: tuh_init failed\n");
        return false;
    }

    printf("UsbMidiHost: initialized, waiting for MIDI device...\n");
    return true;
}

bool UsbMidiHost::is_connected() const {
    return connected_;
}

bool UsbMidiHost::has_midi_activity() {
    bool activity = midi_activity_;
    midi_activity_ = false;
    return activity;
}

std::vector<midi::MidiEvent> UsbMidiHost::poll() {
    std::vector<midi::MidiEvent> events;

    if (rx_count_ > 0) {
        parser_.parse_packets(rx_packets_, rx_count_, events);
        rx_count_ = 0;
    }

    return events;
}

void UsbMidiHost::reset() {
    connected_ = false;
    dev_addr_ = 0;
    rx_count_ = 0;
    midi_activity_ = false;
    parser_.reset();
}

void UsbMidiHost::on_mount(uint8_t daddr, uint8_t itf_num) {
    dev_addr_ = daddr;
    connected_ = true;
    rx_count_ = 0;
    midi_activity_ = false;

    printf("UsbMidiHost: MIDI device mounted (addr=%d, itf=%d)\n",
           daddr, itf_num);
}

void UsbMidiHost::on_umount(uint8_t daddr) {
    printf("UsbMidiHost: MIDI device unmounted (addr=%d)\n", daddr);
    reset();
}

void UsbMidiHost::on_rx_data(const uint8_t* data, uint32_t len) {
    midi_activity_ = true;

    // USB MIDI packets are 4 bytes each
    size_t num_packets = len / 4;
    if (num_packets > MAX_PACKETS) {
        num_packets = MAX_PACKETS;
    }

    for (size_t i = 0; i < num_packets; i++) {
        // Pack as uint32_t in little-endian order (byte0=cable, byte1=cin, byte2=midi1, byte3=midi2)
        uint32_t packet = data[i * 4]
                        | (static_cast<uint32_t>(data[i * 4 + 1]) << 8)
                        | (static_cast<uint32_t>(data[i * 4 + 2]) << 16)
                        | (static_cast<uint32_t>(data[i * 4 + 3]) << 24);

        if (rx_count_ < MAX_PACKETS) {
            rx_packets_[rx_count_++] = packet;
        }
    }
}

} // namespace usb