#include "led/ws2812.h"
#include "ws2812.pio.h"

namespace led {

Ws2812::Ws2812(PIO pio, uint sm, uint pin, uint num_leds,
               ColorOrder order)
    : pio_(pio)
    , sm_(sm)
    , pin_(pin)
    , num_leds_(num_leds)
    , bytes_per_led_(0)
    , color_order_(order)
    , buf_(nullptr)
    , freq_(0)
{
    bytes_per_led_ = (order == ColorOrder::RGBW || order == ColorOrder::GRBW) ? 4 : 3;
    buf_ = new uint8_t[num_leds_ * bytes_per_led_]();

    uint offset = pio_add_program(pio_, &ws2812_program);
    freq_ = clock_get_hz(clk_sys);

    pio_sm_claim(pio_, sm_);
    pio_gpio_init(pio_, pin_);
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin_, 1, true);

    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin_);
    sm_config_set_set_pins(&c, pin_, 1);
    sm_config_set_out_shift(&c, false, true, bytes_per_led_ * 8);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    float div = static_cast<float>(freq_) / (800000.0f * 9.0f);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio_, sm_, offset, &c);
    pio_sm_set_enabled(pio_, sm_, true);
}

Ws2812::~Ws2812() {
    delete[] buf_;
}

void Ws2812::set(uint index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    if (index >= num_leds_) return;
    size_t offset = index * bytes_per_led_;
    buf_[offset]     = r;
    buf_[offset + 1] = g;
    buf_[offset + 2] = b;
    if (bytes_per_led_ >= 4) {
        buf_[offset + 3] = w;
    }
}

void Ws2812::write() {
    for (uint i = 0; i < num_leds_; i++) {
        size_t offset = i * bytes_per_led_;
        uint32_t wire;
        if (bytes_per_led_ == 4) {
            wire = rgbw_to_wire(buf_[offset], buf_[offset + 1],
                                buf_[offset + 2], buf_[offset + 3],
                                color_order_);
        } else {
            wire = rgb_to_wire(buf_[offset], buf_[offset + 1],
                               buf_[offset + 2], color_order_);
        }
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

uint32_t Ws2812::rgb_to_wire(uint8_t r, uint8_t g, uint8_t b,
                              ColorOrder order) {
    switch (order) {
        case ColorOrder::GRB:
            return (static_cast<uint32_t>(g) << 16) |
                   (static_cast<uint32_t>(r) << 8)  |
                   static_cast<uint32_t>(b);
        case ColorOrder::RGB:
        default:
            return (static_cast<uint32_t>(r) << 16) |
                   (static_cast<uint32_t>(g) << 8)  |
                   static_cast<uint32_t>(b);
    }
}

uint32_t Ws2812::rgbw_to_wire(uint8_t r, uint8_t g, uint8_t b, uint8_t w,
                               ColorOrder order) {
    switch (order) {
        case ColorOrder::RGBW:
            return (static_cast<uint32_t>(r) << 24) |
                   (static_cast<uint32_t>(g) << 16) |
                   (static_cast<uint32_t>(b) << 8)  |
                   static_cast<uint32_t>(w);
        case ColorOrder::GRBW:
        default:
            return (static_cast<uint32_t>(g) << 24) |
                   (static_cast<uint32_t>(r) << 16) |
                   (static_cast<uint32_t>(b) << 8)  |
                   static_cast<uint32_t>(w);
    }
}

} // namespace led