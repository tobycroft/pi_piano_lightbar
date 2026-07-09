#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_MCU            OPT_MCU_RP2040

#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_HOST)

// Host-only mode: RP2040 acts as USB Host for MIDI devices
#define CFG_TUD_ENABLED         0
#define CFG_TUH_ENABLED         1

// No device class needed - host only
#define CFG_TUD_MIDI            0

#define CFG_TUH_MIDI            1
#define CFG_TUH_MIDI_RX_BUFSIZE 64
#define CFG_TUH_MIDI_TX_BUFSIZE 64

#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_HID             0
#define CFG_TUD_VENDOR          0

#define CFG_TUH_CDC             0
#define CFG_TUH_MSC             0
#define CFG_TUH_HID             0
#define CFG_TUH_VENDOR          0

#define CFG_TUSB_DEBUG          2

// TinyUSB host stack uses audio class constants for MIDI device detection,
// even in host-only mode. Include the audio header to define them.
#include "class/audio/audio.h"

#ifdef __cplusplus
}
#endif