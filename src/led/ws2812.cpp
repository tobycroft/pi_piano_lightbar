#include "led/ws2812.h"
#include "ws2812.pio.h"

namespace led {

Ws2812::Ws2812(PIO pio, uint sm, uint pin, uint num_leds)
    : pio_(pio)
    , sm_(sm)
    , pin_(pin)
    , num_leds_(num_leds)
    , buf_(nullptr)
    , freq_(0)
{
    buf_ = new uint8_t[num_leds_ * 3]();

    uint offset = pio_add_program(pio_, &ws2812_program);
    freq_ = clock_get_hz(clk_sys);

    pio_sm_claim(pio_, sm_);
    pio_gpio_init(pio_, pin_);

    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin_);
    sm_config_set_out_shift(&c, false, true, 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    // 9 SM cycles per bit, 24 bits per LED, 800 kHz
    // sys_clk / (9 * 24 * 800000) ≈ sys_clk / 17280000
    float div = static_cast<float>(freq_) / 17280000.0f;
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio_, sm_, offset, &c);
    pio_sm_set_enabled(pio_, sm_, true);
}

Ws2812::~Ws2812() {
    delete[] buf_;
}

void Ws2812::set(uint index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= num_leds_) return;
    size_t offset = index * 3;
    buf_[offset]     = r;
    buf_[offset + 1] = g;
    buf_[offset + 2] = b;
}

void Ws2812::write() {
    for (uint i = 0; i < num_leds_; i++) {
        size_t offset = i * 3;
        uint32_t wire = rgb_to_wire(buf_[offset], buf_[offset + 1], buf_[offset + 2]);
        pio_sm_put_blocking(pio_, sm_, wire);
    }
    sleep_us(300);
}

void Ws2812::send_wire(uint32_t data) {
    pio_sm_put_blocking(pio_, sm_, data);
}

void Ws2812::clear() {
    for (uint i = 0; i < num_leds_; i++) {
        pio_sm_put_blocking(pio_, sm_, 0);
    }
    sleep_us(300);
}

// Pack 3 bytes into 32-bit wire format: byte0=R, byte1=G, byte2=B
// This is a simple pass-through, no reordering.
uint32_t Ws2812::rgb_to_wire(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8)  |
           static_cast<uint32_t>(b);
}

} // namespace led