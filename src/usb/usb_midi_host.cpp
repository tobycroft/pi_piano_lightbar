#include "usb_midi_host.h"

#include "tusb.h"
#include "pico/time.h"

#include <cstdio>
#include <cstring>

namespace usb {

UsbMidiHost* s_instance = nullptr;

void xfer_callback_internal(tuh_xfer_t* xfer) {
    if (!s_instance) return;

    if (xfer->result == XFER_RESULT_SUCCESS) {
        s_instance->xfer_len_ = xfer->actual_len;
        s_instance->xfer_done_ = true;
    }

    s_instance->start_receive();
}

UsbMidiHost::UsbMidiHost() {
    s_instance = this;
}

bool UsbMidiHost::init() {
    if (inited_) return true;

    tusb_rhport_init_t host_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUSB_SPEED_FULL
    };
    if (!tuh_rhport_init(TUH_OPT_RHPORT, &host_init)) {
        printf("USB Host: init failed\n");
        return false;
    }

    inited_ = true;
    printf("USB Host: initialized\n");
    return true;
}

bool UsbMidiHost::is_connected() {
    return connected_;
}

std::vector<midi::MidiEvent> UsbMidiHost::poll() {
    std::vector<midi::MidiEvent> events;

    if (!connected_) return events;

    if (xfer_done_) {
        process_packet(rx_buffer_, xfer_len_);
        xfer_done_ = false;
    }

    auto parsed = parser_.poll();
    events.insert(events.end(), parsed.begin(), parsed.end());
    return events;
}

void UsbMidiHost::reset() {
    parser_.reset();
    connected_ = false;
    dev_addr_ = 0;
    ep_in_ = 0;
    ep_in_size_ = 0;
    xfer_done_ = false;
    xfer_len_ = 0;
}

void UsbMidiHost::start_receive() {
    if (!connected_ || ep_in_ == 0) return;

    xfer_done_ = false;
    xfer_len_ = 0;

    tuh_xfer_t xfer = {
        .daddr = dev_addr_,
        .ep_addr = ep_in_,
        .buflen = ep_in_size_,
        .buffer = rx_buffer_,
        .complete_cb = xfer_callback_internal,
        .user_data = 0
    };

    if (!tuh_edpt_xfer(&xfer)) {
        printf("USB Host: xfer submit failed\n");
    }
}

void UsbMidiHost::process_packet(const uint8_t* packet, uint32_t len) {
    for (uint32_t i = 0; i + 4 <= len; i += 4) {
        uint8_t cin = packet[i] & 0x0F;
        uint8_t raw[3] = {packet[i + 1], packet[i + 2], packet[i + 3]};

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

void UsbMidiHost::on_mount(uint8_t daddr, tusb_desc_device_t const* desc) {
    printf("USB Host: device mounted, addr=%d, VID=%04x PID=%04x\n",
           daddr, desc->idVendor, desc->idProduct);

    uint8_t cfg_buf[256];
    uint8_t const* p_cfg = cfg_buf;

    uint8_t result = tuh_descriptor_get_configuration_sync(daddr, 0, cfg_buf, sizeof(cfg_buf));
    if (result != XFER_RESULT_SUCCESS) {
        printf("USB Host: failed to get config descriptor\n");
        return;
    }

    tusb_desc_configuration_t const* cfg = (tusb_desc_configuration_t const*) p_cfg;
    uint16_t total_len = cfg->wTotalLength;
    uint8_t const* end = p_cfg + total_len;
    p_cfg += cfg->bLength;

    bool found = false;
    uint8_t ep_out = 0, ep_in = 0;
    uint16_t ep_in_size = 0;

    while (p_cfg < end) {
        uint8_t desc_len = p_cfg[0];
        uint8_t desc_type = p_cfg[1];

        if (desc_len == 0) break;

        if (desc_type == TUSB_DESC_INTERFACE) {
            tusb_desc_interface_t const* itf = (tusb_desc_interface_t const*) p_cfg;

            if (itf->bInterfaceClass == TUSB_CLASS_AUDIO &&
                itf->bInterfaceSubClass == AUDIO_SUBCLASS_MIDI_STREAMING) {
                found = true;
            }
        }

        if (found && desc_type == TUSB_DESC_ENDPOINT) {
            tusb_desc_endpoint_t const* ep = (tusb_desc_endpoint_t const*) p_cfg;

            if (ep->bEndpointAddress & 0x80) {
                ep_in = ep->bEndpointAddress;
                ep_in_size = ep->wMaxPacketSize;
            } else {
                ep_out = ep->bEndpointAddress;
            }
        }

        p_cfg += desc_len;
    }

    if (found && ep_in != 0) {
        connected_ = true;
        dev_addr_ = daddr;
        ep_in_ = ep_in;
        ep_in_size_ = ep_in_size;

        tusb_desc_endpoint_t ep_desc = {};
        ep_desc.bLength = sizeof(tusb_desc_endpoint_t);
        ep_desc.bDescriptorType = TUSB_DESC_ENDPOINT;
        ep_desc.bEndpointAddress = ep_in;
        ep_desc.bmAttributes = {.xfer = TUSB_XFER_BULK};
        ep_desc.wMaxPacketSize = ep_in_size;

        if (tuh_edpt_open(daddr, &ep_desc)) {
            printf("USB Host: MIDI IN endpoint opened, ep=0x%02x size=%d\n", ep_in, ep_in_size);
            start_receive();
        } else {
            printf("USB Host: failed to open endpoint\n");
            connected_ = false;
        }
    } else {
        printf("USB Host: no MIDI device found\n");
    }
}

void UsbMidiHost::on_umount(uint8_t daddr) {
    if (daddr == dev_addr_) {
        printf("USB Host: device removed\n");
        reset();
    }
}

} // namespace usb

extern "C" {

static tusb_desc_device_t s_dev_desc;

void tuh_mount_cb(uint8_t daddr) {
    uint8_t result = tuh_descriptor_get_device_sync(daddr, &s_dev_desc, sizeof(s_dev_desc));
    if (result != XFER_RESULT_SUCCESS) {
        printf("USB Host: failed to get device descriptor\n");
        return;
    }
    usb::s_instance->on_mount(daddr, &s_dev_desc);
}

void tuh_umount_cb(uint8_t daddr) {
    usb::s_instance->on_umount(daddr);
}

} // extern "C"