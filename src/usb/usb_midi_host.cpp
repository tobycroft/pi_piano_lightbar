#include "usb_midi_host.h"

#include "tusb.h"
#include "pico/time.h"

#include <cstdio>
#include <cstring>

namespace usb {

UsbMidiHost* s_instance = nullptr;

// --------------------------------------------------------------------
// Friend functions (C++ linkage, passed as C function pointers to TinyUSB)
// --------------------------------------------------------------------

void xfer_callback_internal(tuh_xfer_t* xfer) {
    if (!s_instance) return;

    if (xfer->result == XFER_RESULT_SUCCESS && xfer->actual_len > 0) {
        s_instance->debug_flags |= UsbMidiHost::DEBUG_DATA_RX;
        s_instance->process_packet(xfer->buffer, xfer->actual_len);
    }

    s_instance->start_receive();
}

void config_complete_cb(tuh_xfer_t* xfer) {
    if (!s_instance) return;

    if (xfer->result == XFER_RESULT_SUCCESS) {
        printf("USB Host: configuration set complete, opening endpoint...\n");
        if (tuh_edpt_open(xfer->daddr, &s_instance->ep_in_desc_)) {
            printf("USB Host: MIDI IN endpoint opened, ep=0x%02x attr=0x%02x size=%d\n",
                   s_instance->ep_in_desc_.bEndpointAddress,
                   s_instance->ep_in_desc_.bmAttributes,
                   s_instance->ep_in_desc_.wMaxPacketSize);
            s_instance->debug_flags |= UsbMidiHost::DEBUG_CONFIG_DONE;
            s_instance->start_receive();
        } else {
            printf("USB Host: failed to open endpoint\n");
            s_instance->connected_ = false;
        }
    } else {
        printf("USB Host: set configuration failed (result=%d)\n", xfer->result);
        s_instance->connected_ = false;
    }
}

// --------------------------------------------------------------------
// UsbMidiHost implementation
// --------------------------------------------------------------------

UsbMidiHost::UsbMidiHost() {
    s_instance = this;
}

bool UsbMidiHost::init() {
    if (inited_) return true;
    inited_ = true;
    printf("USB Host: ready\n");
    return true;
}

bool UsbMidiHost::is_connected() {
    return connected_;
}

std::vector<midi::MidiEvent> UsbMidiHost::poll() {
    if (!connected_) return {};
    return parser_.poll();
}

void UsbMidiHost::reset() {
    parser_.reset();
    connected_ = false;
    dev_addr_ = 0;
    ep_in_ = 0;
    ep_in_size_ = 0;
    memset(&ep_in_desc_, 0, sizeof(ep_in_desc_));
}

void UsbMidiHost::start_receive() {
    if (!connected_ || ep_in_ == 0) return;

    tuh_xfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.daddr = dev_addr_;
    xfer.ep_addr = ep_in_;
    xfer.buffer = rx_buffer_;
    xfer.buflen = ep_in_size_;
    xfer.complete_cb = xfer_callback_internal;
    xfer.user_data = 0;

    if (!tuh_edpt_xfer(&xfer)) {
        printf("USB Host: xfer submit failed\n");
    }
}

void UsbMidiHost::process_packet(const uint8_t* packet, uint32_t len) {
    for (uint32_t i = 0; i + 4 <= len; i += 4) {
        // USB MIDI Event Packet:
        // Byte 0: [CIN (4 bits) | CableNumber (4 bits)]
        // Byte 1: MIDI status byte
        // Byte 2: MIDI data 1
        // Byte 3: MIDI data 2
        uint8_t cin = (packet[i] >> 4) & 0x0F;
        uint8_t raw[3] = {packet[i + 1], packet[i + 2], packet[i + 3]};

        switch (cin) {
            case 0x08: // Note Off
            case 0x09: // Note On
                parser_.feed(raw, 3);
                break;
            case 0x02: // Program Change
            case 0x03: // Channel Pressure
                parser_.feed(raw, 2);
                break;
            default:
                break;
        }
    }
}

void UsbMidiHost::on_mount(uint8_t daddr, tusb_desc_device_t const* desc) {
    printf("USB Host: device mounted, addr=%d, VID=%04x PID=%04x\n",
           daddr, desc->idVendor, desc->idProduct);

    // Read configuration descriptor
    uint8_t cfg_buf[256];
    uint8_t const* cfg_start = cfg_buf;

    uint8_t result = tuh_descriptor_get_configuration_sync(daddr, 0, cfg_buf, sizeof(cfg_buf));
    if (result != XFER_RESULT_SUCCESS) {
        printf("USB Host: failed to get config descriptor\n");
        return;
    }

    tusb_desc_configuration_t const* cfg = (tusb_desc_configuration_t const*)cfg_start;
    uint16_t total_len = cfg->wTotalLength;
    uint8_t const* end = cfg_start + total_len;
    uint8_t const* p_cfg = cfg_start + cfg->bLength;

    bool found = false;
    uint8_t ep_in = 0;
    uint16_t ep_in_size = 0;

    // First pass: find MIDI interface and IN endpoint
    while (p_cfg < end) {
        uint8_t desc_len = p_cfg[0];
        uint8_t desc_type = p_cfg[1];

        if (desc_len == 0) break;

        if (desc_type == TUSB_DESC_INTERFACE) {
            auto const* itf = (tusb_desc_interface_t const*)p_cfg;
            if (itf->bInterfaceClass == TUSB_CLASS_AUDIO &&
                itf->bInterfaceSubClass == 0x03) { // MIDI Streaming
                found = true;
                printf("USB Host: found MIDI interface\n");
            }
        }

        if (found && desc_type == TUSB_DESC_ENDPOINT) {
            auto const* ep = (tusb_desc_endpoint_t const*)p_cfg;
            if (ep->bEndpointAddress & 0x80) { // IN endpoint
                ep_in = ep->bEndpointAddress;
                ep_in_size = ep->wMaxPacketSize;
                printf("USB Host: IN endpoint 0x%02x size=%d attr=0x%02x\n",
                       ep_in, ep_in_size, ep->bmAttributes);
            }
        }

        p_cfg += desc_len;
    }

    if (!found || ep_in == 0) {
        printf("USB Host: no MIDI IN endpoint found\n");
        return;
    }

    // Second pass: save the IN endpoint descriptor
    p_cfg = cfg_start + cfg->bLength;
    bool ep_saved = false;
    while (p_cfg < end && !ep_saved) {
        if (p_cfg[0] == 0) break;
        if (p_cfg[1] == TUSB_DESC_ENDPOINT) {
            auto const* ep = (tusb_desc_endpoint_t const*)p_cfg;
            if (ep->bEndpointAddress == ep_in) {
                memcpy(&ep_in_desc_, ep, sizeof(tusb_desc_endpoint_t));
                ep_saved = true;
            }
        }
        p_cfg += p_cfg[0];
    }

    if (!ep_saved) {
        printf("USB Host: failed to save endpoint descriptor\n");
        return;
    }

    // Set configuration then open endpoint in callback
    dev_addr_ = daddr;
    ep_in_ = ep_in;
    ep_in_size_ = ep_in_size;
    connected_ = true;

    printf("USB Host: setting configuration %d...\n", cfg->bConfigurationValue);
    if (!tuh_configuration_set(daddr, cfg->bConfigurationValue,
                                config_complete_cb, (uintptr_t)this)) {
        printf("USB Host: set configuration failed\n");
        connected_ = false;
    }
    // Endpoint will be opened in config_complete_cb
}

void UsbMidiHost::on_umount(uint8_t daddr) {
    if (daddr == dev_addr_) {
        printf("USB Host: device removed\n");
        reset();
    }
}

} // namespace usb

// --------------------------------------------------------------------
// TinyUSB Host callbacks (C linkage)
// --------------------------------------------------------------------
extern "C" {

static tusb_desc_device_t s_dev_desc;

void tuh_mount_cb(uint8_t daddr) {
    printf("USB Host: tuh_mount_cb addr=%d\n", daddr);

    uint8_t result = tuh_descriptor_get_device_sync(daddr, &s_dev_desc, sizeof(s_dev_desc));
    if (result != XFER_RESULT_SUCCESS) {
        printf("USB Host: failed to get device descriptor\n");
        return;
    }

    printf("USB Host: device VID=%04x PID=%04x\n",
           s_dev_desc.idVendor, s_dev_desc.idProduct);

    if (usb::s_instance) {
        usb::s_instance->debug_flags |= usb::UsbMidiHost::DEBUG_MOUNT;
        usb::s_instance->on_mount(daddr, &s_dev_desc);
    }
}

void tuh_umount_cb(uint8_t daddr) {
    printf("USB Host: tuh_umount_cb addr=%d\n", daddr);
    if (usb::s_instance) {
        usb::s_instance->on_umount(daddr);
    }
}

} // extern "C"