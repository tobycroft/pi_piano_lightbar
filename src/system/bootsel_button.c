// =============================================================================
// BootselButton 实现 — 强制运行在 RAM 中
// =============================================================================
// 读取 BOOTSEL 按键需要临时接管 QSPI 控制寄存器。
// 由于 Pico 从 Flash 执行代码 (XIP)，操控 QSPI 期间必须确保
// 所有代码都在 RAM 中运行，否则取指令会失败。
//
// __no_inline_not_in_flash_func 将函数放入 .time_critical 段，
// 该段在启动时被复制到 RAM。
// =============================================================================

#include "system/bootsel_button.h"
#include "hardware/gpio.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/sync.h"
#include "pico/platform.h"

bool __no_inline_not_in_flash_func(bootsel_button_is_pressed)(void) {
    const uint32_t CS_PIN_INDEX = 1;

    uint32_t flags = save_and_disable_interrupts();

    // 将 QSPI_SS 引脚设为 Hi-Z (禁用输出使能)
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    // 等待引脚电平稳定
    for (volatile int i = 0; i < 1000; ++i) {
        __asm__ volatile("nop");
    }

    // 读取 QSPI_SS 引脚状态 (低电平 = 按下)
    bool pressed = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));

    // 恢复 QSPI 正常控制
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    restore_interrupts(flags);

    return pressed;
}