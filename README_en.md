# pi_piano_lightbar 🎹✨

> 🇨🇳 中文版本: [README.md](./README.md)
>
> **The world's first RP2040-based 88-key Piano LED Guidance System — USB MIDI Host → WS2812 strip, real-time visualization**

A hardware project running on a **Raspberry Pi Pico (RP2040)**, that mirrors what you play on a USB MIDI keyboard (or anything that outputs MIDI) onto a full 88-key WS2812 LED strip in real time. **This is the ONLY project in the whole family that builds a "guidance system"**, and the ONLY one that supports **separate color profiles for white keys and black keys**.

---

## ✨ What is this good for?

Imagine:

1. **You have a digital piano (MIDI out)**
2. **You tape an 88-key WS2812 strip onto it**
3. **Plug in this little Pico → press a key, that key lights up!**

Use cases:
- 🎓 **Practice guidance light bar** — tells you which key to play next (driven by external MIDI)
- 🎵 **Real-time performance visualization** — every key lights up as you play
- 🖼️ **Exhibits / demos** — audiences see exactly where you're playing
- 🎮 **Musical interactive installations** — keyboard becomes input for a light show
- 🔧 **MIDI → light protocol bridge** — also useful as a generic USB MIDI Host parser

---

## 🔥 Key Features

| Feature | Details |
|---------|---------|
| 🎹 **True full 88-key coverage** | A0 (MIDI 21) → C8 (MIDI 108), every key mapped exactly to a standard piano |
| 🔌 **Native USB MIDI Host** | Uses TinyUSB to talk directly to USB MIDI devices — no PC required in the middle |
| 🌈 **Separate colors for white / black keys** | **White keys and black keys use different colors**, cyclable, saved to Flash |
| 🎲 **Random color mode** | Each key gets a random color — great for stage atmosphere |
| 💾 **Flash persistence** | Current color scheme + brightness level saved automatically, survives reboot |
| 🎛️ **On-board BOOTSEL button UI** | Single-click: cycle color scheme. Double-click: cycle brightness. Hold 3s: enter Bootloader |
| ⚡ **PIO-driven WS2812** | RP2040 PIO generates the exact WS2812 bit-banged timing — CPU stays idle, low latency |
| 🚦 **Chase animation** | Runs automatically when no MIDI device is connected; instantly switches to MIDI mode on plug-in |
| 🔆 **Multi-level brightness** | Three levels, double-click BOOTSEL to cycle |
| 💡 **On-board LED heartbeat** | Flashes when MIDI data arrives — visual confirmation the device is alive |
| 🧩 **Modular C++17 architecture** | Each peripheral is its own class; clear driver / protocol / app layering |

---

## 🏗️ Architecture & Data Flow

```
 USB MIDI Keyboard
        │
        ▼ USB-C
  TinyUSB MIDI Host
  (usb/usb_midi_host.cpp)
        │
        ▼ MIDI events
  MIDI Parser
  (midi/midi_parser.cpp)
        │
        ▼ note on / note off
  Key → LED Mapping
  (piano/piano_config.h + color_scheme.h)
        │
        ▼ color + index
  LED Controller
  (led/led_controller.h + led_animator.h)
        │
        ▼ PIO bit-bang
  WS2812 Strip (GPIO28)
  (led/ws2812.pio + ws2812.cpp)
```

### Source Layout

```
src/
├── main.cpp                 # main loop + BOOTSEL interaction + mode switching
├── led/
│   ├── ws2812.pio           # WS2812 timing — PIO assembly
│   ├── ws2812.h / .cpp      # PIO driver wrapper
│   ├── led_controller.h     # LED color control (brightness, single-key, all-key)
│   └── led_animator.h       # chase animation
├── midi/
│   ├── midi_event.h         # MIDI event structure
│   └── midi_parser.cpp      # MIDI byte stream → Events
├── usb/
│   └── usb_midi_host.cpp    # TinyUSB Host MIDI wrapper
├── piano/
│   ├── piano_config.h       # 88-key constants + note↔LED index mapping
│   └── color_scheme.h       # ⭐ white-key / black-key color profiles
└── system/
    ├── bootsel_button.c     # BOOTSEL button read
    └── flash_storage.c      # Flash persistence (scheme + brightness)
```

---

## 🎨 Color Schemes (Profiles)

**The most unique part of this project: white keys and black keys use different colors.** Each profile defines two color sets:

| Scheme | White Key | Black Key | Notes |
|--------|-----------|-----------|-------|
| **0** | Lake Blue `LAKE_BLUE` | Grass Green `GRASS_GREEN` | Fresh & natural, the default |
| **1** | Green `GREEN` | Red `RED` | Classic red-green contrast — great for demos |
| **2** | 🎲 Random | 🎲 Random | Every key gets a random color — party mode |

Want more profiles? Open `src/piano/color_scheme.h:69`, add one line to `kColorSchemes[]` — one constant change and it works.

> White/black key detection: uses MIDI note mod 12. White key positions `{0,2,4,5,7,9,11}`, everything else is black. See `color_scheme.h:22`.

---

## 🎛️ Controls

| Action | Effect |
|--------|--------|
| **Single-click** BOOTSEL | Cycle color schemes (0→1→2→0), saved to Flash |
| **Double-click** BOOTSEL | Cycle brightness levels (LOW → MID → HIGH), saved to Flash |
| **Hold 3s** BOOTSEL | Enter USB Bootloader — drag-and-drop UF2 to update firmware |
| **Plug in USB MIDI device** | Auto-exits chase animation, enters real-time MIDI mode |
| **Unplug USB MIDI device** | Auto-resumes chase animation |

---

## 🔌 Hardware Wiring

| RP2040 (Pico) | Device |
|---------------|--------|
| GPIO28 | WS2812 DIN (data) |
| VSYS / VBUS | WS2812 +5V (use external power depending on strip current) |
| GND | WS2812 GND (**common ground is mandatory**) |
| USB Host | USB MIDI keyboard (OTG or Host adapter required) |
| GPIO25 | On-board LED (heartbeat indicator) |

> 💡 High-current tip: 88 WS2812 pixels all lit white (~60 mA each) can peak at **5A+**. Consider a dedicated 5V supply for the strip, let the RP2040 handle signalling only, and **make sure grounds are tied together**.

---

## 🚀 Build & Flash

### Requirements

- Raspberry Pi Pico SDK 2.3.0+
- CMake 3.13+
- arm-none-eabi-gcc (toolchain 13.2 Rel1)
- IntelliJ IDEA (recommended) + CMake Application config

### Building with IntelliJ IDEA MCP

1. Open the `pi_piano_lightbar` project
2. Run the CMake Application configuration
3. Output: `build/pi_piano_lightbar.uf2`
4. Hold BOOTSEL for 3 seconds, drag the UF2 into the RPI-RP2 drive

### Command line (also works, but the IDE flow is recommended)

```bash
mkdir build && cd build
cmake ..
make -j
# produces build/pi_piano_lightbar.uf2
```

---

## 🔧 Customization

### Want a different strip length / key range?

Edit `MIDI_NOTE_MIN / MIDI_NOTE_MAX / NUM_LEDS` in `src/piano/piano_config.h:15`.

### Want more color schemes?

Add a line to `kColorSchemes[]` in `src/piano/color_scheme.h:69`.

### Want a different LED data pin?

Edit `LED_PIN` in `src/main.cpp:40`.

### Want more brightness levels?

Add a value to the `BrightnessLevel` enum in `src/led/led_controller.h`.

---

## 🏆 Why this project is one-of-a-kind in the family

Across my whole RP2040 firmware family (pi_tuuzkb_usb / go_tuuzkb / pi_tuuzkb_recv / pi_tuuzkb_test), **every other project is about forwarding keyboard input, HID, UART, and UDP**.

**pi_piano_lightbar is the ONLY one that**:

- ✅ **builds a "guidance system"** — output visualizes and guides the user
- ✅ **supports independent color profiles for white keys and black keys** — white keys one color, black keys another, switchable at the click of a button
- ✅ **turns MIDI into light** — for the first time, an input protocol (MIDI / USB HID) becomes a **visual output**
- ✅ **drives WS2812 via PIO** — also my first RP2040 project using PIO for LED strips

In short: all the other projects study "how to press a key outwards." **This one studies "how to see the keys you're pressing."** 🎯

---

## 📝 License

Do whatever you want. If this project lights up your piano, consider taking a picture of it ✨

---

*RP2040 · C++17 · PIO · TinyUSB · WS2812 — 2026*