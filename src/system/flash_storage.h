// =============================================================================
// FlashStorage — 带磨损均衡的 Flash 配置持久化
// =============================================================================
// 使用 RP2040 Flash 最后一个扇区存储配置，通过轮转写入实现磨损均衡。
//
// 扇区布局:
//   Flash 末扇区 (4096 bytes) 被划分为 16 个 256-byte 槽位
//   每个槽位: magic(4B) + sequence(4B) + scheme(4B) + padding(244B, 0xFF)
//
// 写入策略:
//   - 找到 sequence 最大且 magic 有效的槽位
//   - 写入下一个槽位，sequence + 1
//   - 所有槽位用完时擦除整个扇区，重新从槽位 0 开始
//   - 16 次写入仅触发 1 次擦除，显著降低磨损
//
// 读取策略:
//   - 扫描所有槽位，返回 sequence 最大且 magic 有效的槽位数据
//   - 未找到有效数据时返回默认值
//
// 使用方式:
//   flash_storage_init();                    // 初始化（可选）
//   int scheme = flash_storage_load_scheme(0); // 读取，默认 0
//   flash_storage_save_scheme(2);            // 保存
// =============================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 flash 存储（验证偏移量有效性）
bool flash_storage_init(void);

// 保存配色方案索引到 flash
// 返回 true 表示成功
bool flash_storage_save_scheme(int scheme);

// 从 flash 读取配色方案索引
// 如果 flash 中没有有效数据，返回 default_scheme
int flash_storage_load_scheme(int default_scheme);

#ifdef __cplusplus
}
#endif