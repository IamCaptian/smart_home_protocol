# KNX 开关键使能位与开关模式矛盾 — 反馈报告

> 提交方: 面板侧（panel_2_86_v / V 版固件）
> 日期: 2026-09-23
> 对象: KNX 模组（HOOCH 核）/ knx_device_key.c 底层转换 / KNX 数据库配置工具
> 结论: 数据源头在底层 KNX 数据流，面板上层按协议如实落地、无逻辑缺陷；面板侧不采用"强行使能"补丁，交由 KNX 侧/底层确认与修正。

---

## 一、现象

真机（KNX 模组，module_type=3）上，开关模式键（协议键 1，名称"开关"）**点击无任何动作**：
- UI 层四宫格短按已触发（`[Grid][CLICK] out ... cb_c=1 cb_s=1`），但因 `sw_en=0` 回调被跳过
- 面板模型层 `set_key_switch` 不被调用，开关命令永不发送

## 二、真机日志关键帧（拉配置过程）

```
[HOOCH] KeyMode key=1, power_on_status=0, type=8
[HOOCH] KeyStatus key=1, state=0, control_item=3, value=0x0001   → [DevBus][Key] enable key=1 data=0x01
[HOOCH] KeyStatus key=1, state=0, control_item=4, value=0x0000   → 设备描述
[HOOCH] KeyMode key=1, power_on_status=0, type=8
[HOOCH] KeyStatus key=1, state=0, control_item=7, value=0x0003   → [DevBus][Key] knx_mode key=1 data=0x03
[HOOCH] KeyStatus key=1, state=0, control_item=2, value=0x0000   → 开关状态
```

开机 [Key][CFG] 快照:

```
[Key][CFG] dev=0 en=0x01 (func=1 sw=0 click=0 dbl=0 long=0 repeat=0 press=0 release=0)
           sw_mode=3 power_on=0 ... cur_sw=0
```

## 三、数据流还原

```
KNX item1(ENABLE_BIT)       data[0]=0x01 → 直传 value=0x01 → 模型 en_data=0x01 (func=1, sw=0)
                              （附带设置 KeyMode type=8，下方第 3 点）
KNX item5(SWITCH_MODE)      data[0]=0x03 → 转换表只收 0/1/2，3 落 default 兜底 (type=8)
                              ↑↑ 但原始值 0x03 仍透传 → KEY_MODE value=0x03 → 面板 sw_mode=3 (TOGGLE_FB)
```

## 四、问题定位

### 上层（面板）— 无逻辑缺陷
- `ENABLE_BIT=0x01` → 按 V3.x 协议位定义独立落地: `func_en=1 / sw_en=0 / click_en=0`，符合协议位映射，行为即"开关功能被协议明确禁用"
- `KEY_MODE=0x03` → 面板协议 `sw_mode` 合法值 3 = 按反馈翻转（TOGGLE_FB），如实存储
- 结论: 面板严格执行协议，UI 短按按 `sw_en` 门控性跳过属**正确行为**

### 底层 / KNX 数据流 — 矛盾源头（需 KNX 侧确认）

**问题点 1: ENABLE_BIT 只有 bit0（开关使能缺失）**
- 该键配置了开关模式（见问题点 2），却同时下发 `ENABLE_BIT=0x01`（bit1 开关使能=0）
- 区间对比（同一次拉配置）：AC=`0x7F`、Light=`0x5F`、FH/Vent/Curtain=`0x07` 均多 bit 使能，唯独按键类（key1/key2）只发 `0x01`
- 判定: 数据库/工具对开关按键的"使能位"配置不完整（只勾总使能、未勾开关），或工具使能位与开关模式未联动

**问题点 2: SWITCH_MODE 值 3 越界且透传与转换结果矛盾**
- `knx_device_key.c` case5U 转换表仅覆盖 0/1/2（OFF/ON/Toggle），`3` 落入 default 兜底为普通开关 type=8
- 但同 case 仍把原始值 `0x03` 透传给面板 → 面板解释为 TOGGLE_FB
- 后果: 同一按键在 KNX 核侧 `s_key_mode=8(NORMAL_SWITCH)` 与面板侧 `sw_mode=3(TOGGLE_FB)` **语义分裂**
- 判定: 底层对值 3 未做一致性处理（要么显式映射/拒绝并给出反馈，要么不兜底直接透传）

**问题点 3（遗留已知）**: case1U(ENABLE) 内 if/else 两分支相同，恒把 KeyMode type 置 `NORMAL_SWITCH`，使能位帧会隐式重置 KNX 核侧键模式——与问题点 2 的 mode 帧相互覆盖（本报告第 2 节可见 type=8 被发了两次）

## 五、向 KNX 侧确认项

1. 该开关键的 **ENABLE 位配置**：是否应勾选"开关使能 (bit1)"？数据库工具是否支持独立勾选？
2. **SWITCH_MODE=3 的来源**：工具允许配置 0-3（3=反馈翻转）？还是配置越界/非法值？
3. 若 3 为合法"反馈翻转"语义：为何使能位不配套 bit1？二者是否应由工具或固件联动保证？

## 六、建议修法（KNX/底层侧）

- **最小修正（数据层）**: 数据库/工具为开关模式键补 ENABLE_BIT bit1=1（使 `sw_en=1`），面板无需改动即可恢复点击发命令
- **底层加固（knx_device_key.c）**:
  1. case5U 显式处理值 3（映射到对应 KeyMode type 或判为非法拒绝），**禁止 default 兜底 + 原始值透传的矛盾行为**；KEY_MODE 透传值须与 type 转换结果语义一致
  2. case1U 修正 if/else 两分支相同问题，使能位帧不应隐式改写键模式
- **面板侧**: 维持现状（不补使能位、不加守卫），保证协议位独立语义不被面板私自改写

## 七、附: 关联文件

| 层 | 文件 |
|----|------|
| 底层转换 | `app/panel_2_86_v/shared/hooch_protocol/knx_smart_screen/knx_device_key.c` |
| 面板下行分发 | `app/panel_2_86_v/shared/hooch_task/hooch_cpu0_protocol_parse_task.c`（KeyStatusDispatch_Process） |
| 面板模型落地 | `app/panel_2_86_v/generated/function/dev_bus/dev_bus_key.c`（key_enable_dev_bus_write_cb / key_knx_mode_dev_bus_write_cb） |
| 面板使能门控 | `app/panel_2_86_v/generated/page_components/page_4grid.c`（grid_run_short_clicked, sw_en 门控） |