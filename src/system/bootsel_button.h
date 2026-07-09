// =============================================================================
// BootselButton — RP2040 BOOTSEL 按键检测
// =============================================================================
// BOOTSEL 按键连接在 QSPI flash 的 CS 引脚上。
// 正常运行期间读取该按键需要临时接管 QSPI 引脚控制权。
//
// 用法:
//   if (bootsel_button_is_pressed()) { ... }
//
// 注意: 此函数会短暂禁用中断，不应在中断服务例程中调用。
// =============================================================================

#pragma once

#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/sync.h"

inline bool bootsel_button_is_pressed() {
    constexpr uint32_t CS_PIN_INDEX = 1;

    uint32_t flags = save_and_disable_interrupts();

    // 将 QSPI_SS 引脚设为 Hi-Z (禁用输出使能)，以便读取按键状态
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