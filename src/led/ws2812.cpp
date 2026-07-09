#include "ws2812.h"
#include <cstring>
#include "hardware/clocks.h"
#include "pico/time.h"

namespace led {

static const uint16_t ws2812_program_instructions[] = {
    0x6221, 0x1123, 0x1400, 0xa042
};

static const struct pio_program ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length = 4,
    .origin = -1,
};

static uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(r) << 8) |
           static_cast<uint32_t>(b);
}

Ws2812::Ws2812(PIO pio, uint sm, uint pin, uint num_leds)
    : pio_(pio), sm_(sm), pin_(pin), num_leds_(num_leds) {
    buf_ = new uint8_t[num_leds_ * 3]();

    uint offset = pio_add_program(pio_, &ws2812_program);
    pio_sm_claim(pio_, sm_);

    pio_gpio_init(pio_, pin_);
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin_, 1, true);

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset, offset + ws2812_program.length - 1);
    sm_config_set_sideset(&c, 1, false, false);
    sm_config_set_out_shift(&c, false, true, 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    float div = clock_get_hz(clk_sys) / (800000.0f * 10.0f);
    sm_config_set_clkdiv(&c, div);

    sm_config_set_set_pins(&c, pin_, 1);
    sm_config_set_sideset_pins(&c, pin_);

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
    std::memset(buf_, 0, num_leds_ * 3);
    write();
}

void Ws2812::write() {
    for (uint i = 0; i < num_leds_; ++i) {
        size_t offset = i * 3;
        uint32_t grb = rgb_to_grb(buf_[offset], buf_[offset + 1], buf_[offset + 2]);
        pio_sm_put_blocking(pio_, sm_, grb);
    }
    sleep_us(60);
}

void Ws2812::put_pixel(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t grb = rgb_to_grb(r, g, b);
    pio_sm_put_blocking(pio_, sm_, grb);
}

} // namespace led