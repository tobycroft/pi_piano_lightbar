#include "tusb.h"

#define USB_VID         0xCafe
#define USB_PID         0x4000
#define USB_BCD         0x0200

#define STRID_LANG      0x00
#define STRID_MANUF     0x01
#define STRID_PRODUCT   0x02
#define STRID_SERIAL    0x03

enum {
    ITF_NUM_MIDI = 0,
    ITF_NUM_MIDI_STREAMING,
    ITF_NUM_TOTAL
};

#define EPNUM_MIDI_OUT  0x01
#define EPNUM_MIDI_IN   0x81

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = STRID_MANUF,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN,
                          0x00, 100),

    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 0, EPNUM_MIDI_OUT,
                        EPNUM_MIDI_IN, 64),
};

static const uint16_t string_desc_lang[] = {
    (TUSB_DESC_STRING << 8) | 4,
    0x0409,
};

static const uint16_t string_desc_manuf[] = {
    (TUSB_DESC_STRING << 8) | (2 * sizeof("PianoLightbar")),
    'P', 'i', 'a', 'n', 'o', 'L', 'i', 'g', 'h', 't', 'b', 'a', 'r',
};

static const uint16_t string_desc_product[] = {
    (TUSB_DESC_STRING << 8) | (2 * sizeof("88-Key Piano LED MIDI")),
    '8', '8', '-', 'K', 'e', 'y', ' ', 'P', 'i', 'a', 'n', 'o', ' ',
    'L', 'E', 'D', ' ', 'M', 'I', 'D', 'I',
};

static const uint16_t string_desc_serial[] = {
    (TUSB_DESC_STRING << 8) | (2 * sizeof("000001")),
    '0', '0', '0', '0', '0', '1',
};

const uint8_t* tud_descriptor_device_cb(void) {
    return (const uint8_t*)&desc_device;
}

const uint8_t* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    switch (index) {
        case STRID_LANG:    return string_desc_lang;
        case STRID_MANUF:   return string_desc_manuf;
        case STRID_PRODUCT: return string_desc_product;
        case STRID_SERIAL:  return string_desc_serial;
        default:            return NULL;
    }
}