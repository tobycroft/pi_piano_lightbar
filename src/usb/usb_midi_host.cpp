#include "usb_midi_host.h"

#include "tusb.h"
#include "pico/time.h"

#include <cstdio>
#include <cstring>

namespace usb {

UsbMidiHost* s_instance = nullptr;

// These are friend functions of UsbMidiHost, declared with C++ linkage in the header.
// They are passed as function pointers to TinyUSB, so linkage type is compatible.
void xfer_callback_internal(tuh_xfer_t* xfer) {
    if (!s_instance) return;

    if (xfer->result == XFER_RESULT_SUCCESS && xfer->actual_len > 0) {
        // Process received data immediately, then start the next receive.
        // This avoids a race where start_receive() clears the xfer_done_ flag
        // before poll() can read it.
        s_instance->process_packet(xfer->buffer, xfer->actual_len);
    }

    s_instance->start_receive();
}

void config_complete_cb(tuh_xfer_t* xfer) {
    if (!s_instance) return;

    if (xfer->result == XFER_RESULT_SUCCESS) {
        printf("USB Host: configuration set complete, opening endpoint...\n");
        // Configuration is now set; open the IN endpoint with the saved descriptor
        if (tuh_edpt_open(xfer->daddr, &s_instance->ep_in_desc_)) {
            printf("USB Host: MIDI IN endpoint opened, ep=0x%02x attr=0x%02x size=%d\n",
                   s_instance->ep_in_desc_.bEndpointAddress,
                   s_instance->ep_in_desc_.bmAttributes,
                   s_instance->ep_in_desc_.wMaxPacketSize);
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

UsbMidiHost::UsbMidiHost() {
    s_instance = this;
}

bool UsbMidiHost::init() {
    if (inited_) return true;

    // TinyUSB is initialized globally via tusb_init() in main()
    // Host stack events are processed via tuh_task() in the main loop
    inited_ = true;
    printf("USB Host: ready\n");
    return true;
}

bool UsbMidiHost::is_connected() {
    return connected_;
}

std::vector<midi::MidiEvent> UsbMidiHost::poll() {
    std::vector<midi::MidiEvent> events;

    if (!connected_) return events;

    // Data is processed directly in the xfer callback (xfer_callback_internal),
    // so there's no stale xfer_done_ flag to check here.
    // Just drain any parsed events from the MIDI parser.
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
}

void UsbMidiHost::start_receive() {
    if (!connected_ || ep_in_ == 0) return;

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
        // USB MIDI Event Packet:
        // Byte 0: [CIN (4 bits) | CableNumber (4 bits)]
        // Byte 1: MIDI status byte (or data)
        // Byte 2: MIDI data 1
        // Byte 3: MIDI data 2
        uint8_t cin = (packet[i] >> 4) & 0x0F;
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
                itf->bInterfaceSubClass == 0x03) { // AUDIO_SUBCLASS_MIDI_STREAMING
                found = true;
                printf("USB Host: found MIDI interface (class=%02x, subclass=%02x)\n",
                       itf->bInterfaceClass, itf->bInterfaceSubClass);
            }
        }

        if (found && desc_type == TUSB_DESC_ENDPOINT) {
            tusb_desc_endpoint_t const* ep = (tusb_desc_endpoint_t const*) p_cfg;

            if (ep->bEndpointAddress & 0x80) {
                ep_in = ep->bEndpointAddress;
                ep_in_size = ep->wMaxPacketSize;
                printf("USB Host: found IN endpoint 0x%02x, size=%d\n", ep_in, ep_in_size);
            } else {
                ep_out = ep->bEndpointAddress;
                printf("USB Host: found OUT endpoint 0x%02x\n", ep_out);
            }
        }

        p_cfg += desc_len;
    }

    if (found && ep_in != 0) {
        connected_ = true;
        dev_addr_ = daddr;
        ep_in_ = ep_in;
        ep_in_size_ = ep_in_size;

        // Save the actual endpoint descriptor from the device
        // (including correct transfer type: interrupt vs bulk)
        // We need to search for it again since we only have the pointer
        // from the parsing loop above
        p_cfg = cfg_buf + cfg->bLength;
        bool ep_found = false;
        while (p_cfg < end && !ep_found) {
            uint8_t desc_len = p_cfg[0];
            uint8_t desc_type = p_cfg[1];
            if (desc_len == 0) break;
            if (desc_type == TUSB_DESC_ENDPOINT) {
                tusb_desc_endpoint_t const* ep = (tusb_desc_endpoint_t const*)p_cfg;
                if (ep->bEndpointAddress == ep_in) {
                    memcpy(&ep_in_desc_, ep, sizeof(tusb_desc_endpoint_t));
                    ep_found = true;
                    printf("USB Host: saved endpoint desc: addr=0x%02x attr=0x%02x size=%d interval=%d\n",
                           ep_in_desc_.bEndpointAddress,
                           ep_in_desc_.bmAttributes,
                           ep_in_desc_.wMaxPacketSize,
                           ep_in_desc_.bInterval);
                }
            }
            p_cfg += desc_len;
        }

        // Set configuration with callback.
        // When the callback fires, we open the endpoint and start receiving.
        // This ensures the device is configured before we try to use endpoints.
        uint8_t config_value = cfg->bConfigurationValue;
        printf("USB Host: setting configuration %d...\n", config_value);
        if (!tuh_configuration_set(daddr, config_value, config_complete_cb, (uintptr_t)this)) {
            printf("USB Host: set configuration request failed\n");
            connected_ = false;
            return;
        }
        // Endpoint will be opened in config_complete_cb
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

// --------------------------------------------------------------------
// TinyUSB Host callbacks (called from tuh_task())
// --------------------------------------------------------------------
extern "C" {

static tusb_desc_device_t s_dev_desc;

void tuh_mount_cb(uint8_t daddr) {
    printf("USB Host: tuh_mount_cb addr=%d\n", daddr);
    printf("USB Host: tuh_mount_cb received, calling tuh_descriptor_get_device_sync...\n");
    uint8_t result = tuh_descriptor_get_device_sync(daddr, &s_dev_desc, sizeof(s_dev_desc));
    printf("USB Host: tuh_descriptor_get_device_sync result=%d\n", result);
    if (result != XFER_RESULT_SUCCESS) {
        printf("USB Host: failed to get device descriptor\n");
        return;
    }
    printf("USB Host: device VID=%04x PID=%04x\n", s_dev_desc.idVendor, s_dev_desc.idProduct);
    usb::s_instance->on_mount(daddr, &s_dev_desc);
}

void tuh_umount_cb(uint8_t daddr) {
    printf("USB Host: tuh_umount_cb addr=%d\n", daddr);
    printf("USB Host: device unmounted\n");
    usb::s_instance->on_umount(daddr);
}

} // extern "C"