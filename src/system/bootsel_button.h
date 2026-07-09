// =============================================================================
// BootselButton — RP2040 BOOTSEL 按键检测
// =============================================================================
// BOOTSEL 按键连接在 QSPI flash 的 CS 引脚上。
// 正常运行期间读取该按键需要临时接管 QSPI 引脚控制权。
// 实现位于 bootsel_button.c，强制运行在 RAM 中以确保安全。
//
// 用法:
//   if (bootsel_button_is_pressed()) { ... }
//
// 注意: 此函数会短暂禁用中断，不应在中断服务例程中调用。
// =============================================================================

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 返回 true 表示 BOOTSEL 按键当前被按下
// 注意: 此函数必须运行在 RAM 中（实现中已标记 __no_inline_not_in_flash_func）
bool bootsel_button_is_pressed(void);

#ifdef __cplusplus
}
#endif