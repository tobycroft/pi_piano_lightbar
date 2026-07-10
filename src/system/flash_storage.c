// =============================================================================
// FlashStorage implementation — 带磨损均衡的 Flash 配置持久化
// =============================================================================

#include "flash_storage.h"

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/platform.h"

#include <string.h>

// =============================================================================
// 常量定义
// =============================================================================

// 使用 Flash 最后一个扇区 (4KB)
#define STORAGE_OFFSET  (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

// 每个槽位 = 1 个 flash page (256 bytes)
#define SLOT_SIZE       FLASH_PAGE_SIZE

// 每个扇区可容纳的槽位数
#define SLOT_COUNT      (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)  // 16

// Magic number 用于校验槽位有效性
#define SLOT_MAGIC      0x5049414E  // "PIAN"

// =============================================================================
// 槽位数据结构 (位于 flash 中)
// =============================================================================
// 每个槽位占 256 bytes (1 flash page):
//   [0..3]   magic    — 校验标识
//   [4..7]   sequence — 单调递增的写入序号
//   [8..11]  scheme   — 配色方案索引
//   [12..255] padding — 0xFF (未使用)
// =============================================================================

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    int32_t  scheme;
} flash_slot_t;

// 256-byte 对齐的 RAM 缓冲区，用于 flash_range_program()
// flash_range_program 要求源数据地址 256-byte 对齐
static uint8_t __attribute__((aligned(FLASH_PAGE_SIZE))) ram_page[FLASH_PAGE_SIZE];

// =============================================================================
// 辅助函数
// =============================================================================

// 扫描整个扇区，找到 magic 有效且 sequence 最大的槽位索引
// 返回 -1 表示没有有效槽位
static int find_latest_slot(const uint8_t* sector_base) {
    int best_slot = -1;
    uint32_t best_seq = 0;

    for (int i = 0; i < SLOT_COUNT; i++) {
        const flash_slot_t* slot =
            (const flash_slot_t*)(sector_base + i * SLOT_SIZE);

        if (slot->magic == SLOT_MAGIC && slot->sequence >= best_seq) {
            best_seq = slot->sequence;
            best_slot = i;
        }
    }

    return best_slot;
}

// =============================================================================
// 公开 API
// =============================================================================

bool flash_storage_init(void) {
    // 验证存储偏移量在有效范围内
    if (STORAGE_OFFSET + FLASH_SECTOR_SIZE > PICO_FLASH_SIZE_BYTES) {
        return false;
    }
    return true;
}

bool __no_inline_not_in_flash_func(flash_storage_save_scheme)(int scheme) {
    // 指向 flash 中存储扇区的指针（通过 XIP 映射读取）
    const uint8_t* flash_sector =
        (const uint8_t*)(XIP_BASE + STORAGE_OFFSET);

    // 找到当前最新的槽位
    int latest = find_latest_slot(flash_sector);

    // 计算下一个槽位和序号
    int next_slot = (latest + 1) % SLOT_COUNT;
    uint32_t next_seq = 0;

    if (latest >= 0) {
        const flash_slot_t* slot =
            (const flash_slot_t*)(flash_sector + latest * SLOT_SIZE);
        next_seq = slot->sequence + 1;
    }

    // 如果回到槽位 0，需要先擦除整个扇区
    if (next_slot == 0) {
        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(STORAGE_OFFSET, FLASH_SECTOR_SIZE);
        restore_interrupts(ints);
    }

    // 准备要写入的页数据
    memset(ram_page, 0xFF, FLASH_PAGE_SIZE);
    flash_slot_t* slot = (flash_slot_t*)ram_page;
    slot->magic    = SLOT_MAGIC;
    slot->sequence = next_seq;
    slot->scheme   = scheme;

    // 写入 flash
    uint32_t page_offs = STORAGE_OFFSET + next_slot * SLOT_SIZE;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(page_offs, ram_page, FLASH_PAGE_SIZE);
    restore_interrupts(ints);

    return true;
}

int __no_inline_not_in_flash_func(flash_storage_load_scheme)(int default_scheme) {
    const uint8_t* flash_sector =
        (const uint8_t*)(XIP_BASE + STORAGE_OFFSET);

    int latest = find_latest_slot(flash_sector);

    if (latest < 0) {
        return default_scheme;
    }

    const flash_slot_t* slot =
        (const flash_slot_t*)(flash_sector + latest * SLOT_SIZE);

    return slot->scheme;
}