#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_MCU            OPT_MCU_RP2040

#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE

#define CFG_TUD_ENABLED         1
#define CFG_TUH_ENABLED         0

#define CFG_TUD_MIDI            1
#define CFG_TUD_MIDI_RX_BUFSIZE 64
#define CFG_TUD_MIDI_TX_BUFSIZE 64

#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_HID             0
#define CFG_TUD_VENDOR          0

#define CFG_TUH_CDC             0
#define CFG_TUH_MSC             0
#define CFG_TUH_HID             0
#define CFG_TUH_VENDOR          0

#define CFG_TUSB_DEBUG          0

#ifdef __cplusplus
}
#endif