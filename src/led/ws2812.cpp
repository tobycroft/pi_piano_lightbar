#include "ws2812.h"
#include "ws2812.pio.h"
#include "hardware/clocks.h"
#include "pico/time.h"

namespace led {

static uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(r) << 8) |
           static_cast<uint32_t>(b);
}

Ws2812::Ws2812(PIO pio, uint sm, uint pin, uint num_leds)
    : pio_(pio), sm_(sm), pin_(pin), num_leds_(num_leds) {
    buf_ = new uint8_t[num_leds_ * 3]();

    // Load PIO program
    uint offset = pio_add_program(pio_, &ws2812_program);
    pio_sm_claim(pio_, sm_);

    // Configure GPIO for PIO
    pio_gpio_init(pio_, pin_);
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin_, 1, true);

    // Configure state machine
    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_out_shift(&c, false, true, 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    // SM clock: target ~7.2MHz (9 cycles per bit → ~800KHz bit rate)
    float div = clock_get_hz(clk_sys) / (800000.0f * 9.0f);
    sm_config_set_clkdiv(&c, div);
    sm_config_set_set_pins(&c, pin_, 1);
    sm_config_set_sideset_pins(&c, pin_);

    // Start state machine
    pio_sm_init(pio_, sm_, offset, &c);
    pio_sm_set_enabled(pio_, sm_, true);
}

Ws2812::~Ws2812() {
    pio_sm_set_enabled(pio_, sm_, false);
    pio_sm_unclaim(pio_, sm_);
    delete[] buf_;
}

void Ws2812::set(uint index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= num_leds_) return;
    size_t offset = index * 3;
    buf_[offset] = r;
    buf_[offset + 1] = g;
    buf_[offset + 2] = b;
}

void Ws2812::clear() {
    for (uint i = 0; i < num_leds_; i++) {
        pio_sm_put_blocking(pio_, sm_, 0);
    }
    sleep_us(300);
}

void Ws2812::write() {
    for (uint i = 0; i < num_leds_; i++) {
        size_t offset = i * 3;
        uint32_t grb = rgb_to_grb(buf_[offset], buf_[offset + 1], buf_[offset + 2]);
        pio_sm_put_blocking(pio_, sm_, grb);
    }
    // Reset pulse: >280us for WS2812, >100us for SM16703
    sleep_us(300);
}

} // namespace led