# pi_piano_lightbar 🎹✨

> 🇬🇧 English version: [README_en.md](./README_en.md)
>
> **全球首个基于 RP2040 的 88 键钢琴 LED 指导系统 —— USB MIDI Host → WS2812 灯条实时可视化**

一个运行在 **Raspberry Pi Pico (RP2040)** 上的硬件项目，能把一台 USB MIDI 键盘（或者任何发出 MIDI 信号的设备）的弹奏动作，实时反映到一整条 88 键 WS2812 灯条上。**这是整个家族里** **唯一** **做"指导系统"的项目**，也是**唯一**支持**黑键和白键使用不同颜色配色方案**的项目。

---

## ✨ 这个项目到底有什么用？

想象一下：

1. **你有一台数码钢琴（MIDI 输出）**
2. **你在琴上贴了一条 88 键的 WS2812 灯带**
3. **插上这个小 Pico → 弹哪个键，哪个键亮！**

用在：
- 🎓 **练琴指导灯条** —— 告诉你接下来弹哪个键（配合外部 MIDI 指令）
- 🎵 **实时演奏可视化** —— 演奏时每个按键被点亮，炫酷夺目
- 🖼️ **展品/演示** —— 让观众直观看到弹奏的位置
- 🎮 **音乐互动装置** —— 键盘成为灯光装置的输入
- 🔧 **MIDI → 灯光协议桥梁** —— 你还可以用它作为一个通用的 USB MIDI Host 解析器

---

## 🔥 核心亮点

| 特性 | 说明 |
|------|------|
| 🎹 **真正的 88 键全覆盖** | A0 (MIDI 21) → C8 (MIDI 108)，一个都不少，完全对齐标准钢琴键位 |
| 🔌 **USB MIDI Host 原生支持** | 用 TinyUSB 直接接入 USB MIDI 设备，不需要电脑中转 |
| 🌈 **黑白键分色配色方案** | **白键和黑键用不同颜色**，循环切换，方案被保存到 Flash |
| 🎲 **随机配色模式** | 每个键随机一个颜色，适合演出氛围 |
| 💾 **Flash 持久化** | 当前配色 + 亮度等级自动保存，重启不丢失 |
| 🎛️ **板载 BOOTSEL 按钮交互** | 单击切配色、双击切亮度、长按 3 秒进入 Bootloader |
| ⚡ **PIO 驱动 WS2812** | 利用 RP2040 的 PIO 精确输出 WS2812 时序，CPU 空闲低延迟 |
| 🚦 **跑马灯动画** | MIDI 设备未接入时自动跑 chase 动画，一接上立即切 MIDI 模式 |
| 🔆 **多档亮度** | 三档亮度，双击 BOOTSEL 循环切换 |
| 💡 **板载 LED 心跳** | MIDI 数据到达时闪烁，直观显示设备工作状态 |
| 🧩 **模块化 C++17 架构** | 每个外设都是一个类，驱动层/协议层/应用层清晰分层 |

---

## 🏗️ 架构与流程

```
 USB MIDI 键盘
        │
        ▼ USB-C
  TinyUSB MIDI Host
  (usb/usb_midi_host.cpp)
        │
        ▼ MIDI 事件
  MIDI Parser
  (midi/midi_parser.cpp)
        │
        ▼ note on / note off
  Key → LED 映射
  (piano/piano_config.h + color_scheme.h)
        │
        ▼ 颜色 + 索引
  LED Controller
  (led/led_controller.h + led_animator.h)
        │
        ▼ PIO 位bang
  WS2812 灯条 (GPIO28)
  (led/ws2812.pio + ws2812.cpp)
```

### 代码目录

```
src/
├── main.cpp                 # 主循环 + BOOTSEL 交互 + 模式切换
├── led/
│   ├── ws2812.pio           # WS2812 时序 PIO 汇编
│   ├── ws2812.h / .cpp      # PIO 驱动封装
│   ├── led_controller.h     # LED 颜色控制（亮度、单键、全键）
│   └── led_animator.h       # chase 跑马灯动画
├── midi/
│   ├── midi_event.h         # MIDI 事件结构
│   └── midi_parser.cpp      # MIDI 字节流 → Event
├── usb/
│   └── usb_midi_host.cpp    # TinyUSB Host MIDI 封装
├── piano/
│   ├── piano_config.h       # 88 键常量 + note↔LED 索引映射
│   └── color_scheme.h       # ⭐ 黑白键配色方案核心
└── system/
    ├── bootsel_button.c     # BOOTSEL 按钮读取
    └── flash_storage.c      # Flash 持久化（配色+亮度）
```

---

## 🎨 配色方案（Profile）

**这是本项目最独特的地方：白键和黑键使用不同的颜色。** 每个 profile 定义了两组颜色：

| Scheme | 白键 | 黑键 | 说明 |
|--------|------|------|------|
| **0** | 湖蓝 `LAKE_BLUE` | 青草绿 `GRASS_GREEN` | 清新自然，默认配色 |
| **1** | 绿 `GREEN` | 红 `RED` | 经典红绿对比，演示感强 |
| **2** | 🎲 随机 | 🎲 随机 | 每个键随机一个颜色 —— 派对模式 |

想加新配色？打开 `src/piano/color_scheme.h:69`，往 `kColorSchemes[]` 里加一行就行，改一个常量自动生效。

> 白键/黑键判定：利用 MIDI note 对 12 取模，白键位置 `{0,2,4,5,7,9,11}`，其余为黑键。见 `color_scheme.h:22`。

---

## 🎛️ 操作

| 动作 | 效果 |
|------|------|
| **单击** BOOTSEL | 循环切换配色方案（0→1→2→0），配色会被保存到 Flash |
| **双击** BOOTSEL | 循环切换亮度等级（LOW → MID → HIGH），亮度会被保存 |
| **长按 3 秒** BOOTSEL | 进入 USB Bootloader，可拖放 UF2 更新固件 |
| **插入 USB MIDI 设备** | 自动退出跑马灯模式，进入实时 MIDI 模式 |
| **拔出 USB MIDI 设备** | 自动恢复跑马灯动画 |

---

## 🔌 硬件接线

| RP2040 (Pico) | 设备 |
|---------------|------|
| GPIO28 | WS2812 DIN（数据） |
| VSYS / VBUS | WS2812 +5V（根据灯条电流决定是否外部供电） |
| GND | WS2812 GND（**必须共地**） |
| USB Host | USB MIDI 键盘（需要 OTG 或 Host 适配器） |
| GPIO25 | 板载 LED（心跳指示） |

> 💡 大电流提示：88 个 WS2812 全亮白色（每颗约 60 mA）峰值可达 **5A+**，建议使用独立 5V 电源给灯条供电，RP2040 只负责信号，并且 **务必共地**。

---

## 🚀 编译与烧录

### 环境要求

- Raspberry Pi Pico SDK 2.3.0+
- CMake 3.13+
- arm-none-eabi-gcc（工具链 13.2 Rel1）
- IntelliJ IDEA（推荐）+ CMake 配置

### 使用 IntelliJ IDEA MCP 构建

1. 打开 `pi_piano_lightbar` 项目
2. 运行 CMake Application 配置
3. 产物：`build/pi_piano_lightbar.uf2`
4. 长按 3 秒 BOOTSEL，把 UF2 拖进 RPI-RP2 盘

### 命令行（也能用，但我们推荐 IDE）

```bash
mkdir build && cd build
cmake ..
make -j
# 得到 build/pi_piano_lightbar.uf2
```

---

## 🔧 定制化

### 想改灯条长度 / 键位范围？

修改 `src/piano/piano_config.h:15` 的 `MIDI_NOTE_MIN / MIDI_NOTE_MAX / NUM_LEDS`。

### 想加新配色？

在 `src/piano/color_scheme.h:69` 的 `kColorSchemes[]` 里加一行即可。

### 想换 LED 数据引脚？

修改 `src/main.cpp:40` 的 `LED_PIN`。

### 想加新的亮度等级？

在 `src/led/led_controller.h` 的 `BrightnessLevel` 枚举里加一个值。

---

## 🏆 为什么这是家族里独一份的项目？

在我的整个 RP2040 固件体系里（pi_tuuzkb_usb / go_tuuzkb / pi_tuuzkb_recv / pi_tuuzkb_test），**其他项目都在围绕键盘输入转发协议、HID、UART、UDP 打转**。

**而 pi_piano_lightbar 是唯一**：

- ✅ **做"指导系统"** 的项目 —— 让输出可视化、反向指导使用者
- ✅ **支持黑键 / 白键独立颜色配色方案** —— 白键一种色、黑键另一种色，一键切换
- ✅ **把 MIDI 变成光** 的项目 —— 从输入协议（MIDI/USB HID）第一次变成**视觉输出**
- ✅ **使用 PIO 驱动 WS2812** —— 这也是我第一次在 RP2040 上用 PIO 驱动灯带

简单说：其他项目都在研究"怎么把键按出去"，**这一个研究"怎么让你看见自己按了什么键"**。🎯

---

## 📝 License

做你想做的。如果这个项目帮你点亮了钢琴，记得给它一张照片 ✨

---

*RP2040 · C++17 · PIO · TinyUSB · WS2812 —— 2026*