# 涂鸦 DP 功能点定义（前端版）

## 1. 文档目标

本文档基于 `涂鸦DP点制定.docx` 整理，面向前端开发人员，重点说明：

- 每个 DP 的业务含义
- 页面应如何展示
- 控件类型建议
- 枚举值和取值范围
- `raw` 类型数据的前端入参/出参结构
- 当前文档中尚未明确、需要后续确认的点

说明：

- `rw`：可下发，也会回传状态
- `ro`：只读，只用于展示
- `Enum` 默认按文档顺序从 `0` 开始编码
- `Bool`：`false=关`，`true=开`

## 2. 页面功能总览

建议前端按以下页面或模块组织：

1. 首页/自定义页面
2. 空调页
3. 新风页
4. 地暖页
5. 场景页
6. 按键配置页
7. 基础设置页
8. 高级设置页

## 3. 标准 DP 定义

### 3.1 场景类

| DP | 标识符 | 名称 | 类型 | 权限 | 前端建议 |
| --- | --- | --- | --- | --- | --- |
| 1 | `scene_1` | 场景1 | Enum | rw | 作为场景按钮或场景卡片 |
| 2 | `scene_2` | 场景2 | Enum | rw | 作为场景按钮或场景卡片 |
| 3 | `scene_3` | 场景3 | Enum | rw | 作为场景按钮或场景卡片 |
| 4 | `scene_4` | 场景4 | Enum | rw | 作为场景按钮或场景卡片 |
| 5 | `scene_5` | 场景5 | Enum | rw | 作为场景按钮或场景卡片 |
| 6 | `scene_6` | 场景6 | Enum | rw | 作为场景按钮或场景卡片 |
| 7 | `scene_7` | 场景7 | Enum | rw | 作为场景按钮或场景卡片 |
| 8 | `scene_8` | 场景8 | Enum | rw | 作为场景按钮或场景卡片 |

补充说明：

Enum默认为0就可以

### 3.2 开关类

| DP | 标识符 | 名称 | 类型 | 权限 | 前端建议 |
| --- | --- | --- | --- | --- | --- |
| 24 | `switch_1` | 继电器1 | Bool | rw | 开关 |
| 25 | `switch_2` | 继电器2 | Bool | rw | 开关 |
| 26 | `switch_3` | 继电器3 | Bool | rw | 开关 |
| 27 | `switch_4` | 继电器4 | Bool | rw | 开关 |
| 28 | `switch_5` | 开关5 | Bool | rw | 开关 |
| 29 | `switch_6` | 开关6 | Bool | rw | 开关 |
| 137 | `switch_7` | 开关7 | Bool | rw | 开关 |
| 138 | `switch_8` | 开关8 | Bool | rw | 开关 |
| 149 | `master_sw` | 总开总关 | Bool | rw | 总控开关 |

### 3.3 空调类

| DP | 标识符 | 名称 | 类型 | 权限 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 112 | `switch` | 空调开关 | Bool | rw | 空调总开关 |
| 105 | `mode` | 空调模式 | Enum | rw | 模式切换 |
| 102 | `fan_speed_enum` | 空调风速 | Enum | rw | 风速切换 |
| 101 | `fan_direction` | 空调风向 | Enum | rw | 风向切换 |
| 106 | `temp_set` | 空调温度 | Value | rw | 设定温度 |

空调枚举定义：

- `mode`
  - `0=cold`
  - `1=hot`
  - `2=dry`
  - `3=fan`
  - `4=auto`
- `fan_speed_enum`
  - `0=low`
  - `1=middle`
  - `2=high`
  - `3=auto`
- `fan_direction`
  - `0=horizontal`
  - `1=vertical`
  - `2=auto`

空调温度定义：

- 标识符：`temp_set`
- 范围：`5~35`
- 步进：`1`
- 单位：`℃`

前端建议：

- 页面展示为“开关 + 温度调节 + 模式切换 + 风速切换 + 风向切换（暂时不加）”
- 温度控件建议使用步进器或滑杆

### 3.4 新风类

| DP | 标识符 | 名称 | 类型 | 权限 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 125 | `fresh_air_valve` | 新风开关 | Bool | rw | 新风开关 |
| 116 | `loop_mode` | 新风模式 | Enum | rw | 新风模式切换 |
| 150 | `fresh_air_speed` | 新风风速 | Enum | rw | 单一新风风速 |
| 123 | `supply_fan_speed` | 送风风速 | Enum | rw | 送风风速 |   （暂时不用）
| 124 | `exhaust_fan_speed` | 排风风速 | Enum | rw | 排风风速 |   （暂时不用）

新风枚举定义：

- `loop_mode`
  - `0=auto`
  - `1=indoor_loop`
  - `2=outdoor_loop`
- `fresh_air_speed`
  - `0=low`
  - `1=mid`
  - `2=high`
- `supply_fan_speed`
  - `0=off`
  - `1=low`
  - `2=mid`
  - `3=high`
- `exhaust_fan_speed`
  - `0=off`
  - `1=low`
  - `2=mid`
  - `3=high`

前端建议：

- 页面展示优先使用“开关 + 模式切换 + 新风风速”
- 若产品需要更细分控制，可扩展展示 `supply_fan_speed` 和 `exhaust_fan_speed`（暂时不需要）
- 若设备侧同时支持单风速 DP 和组合 DP `fan_info`，联调时需确认状态同步优先级（暂时不需要）

### 3.5 地暖类

| DP | 标识符 | 名称 | 类型 | 权限 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 130 | `floor_sw` | 地暖开关 | Bool | rw | 地暖开关 |
| 131 | `floor_temp` | 地暖温度 | Value | rw | 设定温度 |

地暖温度定义：

- 标识符：`floor_temp`
- 范围：`5~35`
- 步进：`1`
- 单位：`℃`

前端建议：

- 页面展示为“开关 + 温度调节”

### 3.6 人感与展示类

| DP | 标识符 | 名称 | 类型 | 权限 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 126 | `pir_state` | 人体感应 | Enum | ro | 只读状态展示 |（仅接受上报，配合情景使用）
| 147 | `disp_param` | 展示参数 | Raw | rw | 温湿度/空气质量展示 |
| 148 | `page_sync` | 页面同步 | Enum | rw | 当前页面同步 |（下发到设备或者上报到app，app切换到某个页面时，需要同步终端设备的页面）

枚举定义：

- `pir_state`
  - `0=pir`
  - `1=none`
- `page_sync`
  - `0=page_1`
  - `1=page_2`
  - `2=air_conditioner`
  - `3=fresh_air`
  - `4=floor_heating`

## 4. 自定义 DP 定义

### 4.1 组合信息 DP

这类 DP 为 `raw` 类型，通常用于一次性同步整组状态。

#### `ac_info` 空调信息

- DP：`134`
- 标识符：`ac_info`
- 权限：`rw`
- 数据结构：
  - Byte0：开关，`0=关`，`1=开`
  - Byte1：模式，枚举同 `mode`
  - Byte2：风速，枚举同 `fan_speed_enum`
  - Byte3：风向，枚举同 `fan_direction`
  - Byte4：温度，范围 `5~35`

适用场景：

- 前端一次下发完整空调状态
- 设备一次回传完整空调状态

#### `fan_info` 新风信息

- DP：`135`
- 标识符：`fan_info`
- 权限：`rw`
- 数据结构：
  - Byte0：开关，`0=关`，`1=开`
  - Byte1：模式，`0=auto`，`1=indoor_loop`，`2=outdoor_loop`
  - Byte2：风速，`0=off`，`1=low`，`2=mid`，`3=high`

说明：

- 原文只定义了一个风速字节，因此如果前端页面区分“送风”和“排风”，需与设备侧确认是否存在单独映射关系。
- 当前已新增独立 DP `fresh_air_speed`，若产品页面只展示单一风速，建议优先使用该 DP。

#### `floor_info` 地暖信息

- DP：`136`
- 标识符：`floor_info`
- 权限：`rw`
- 数据结构：
  - Byte0：开关，`0=关`，`1=开`
  - Byte1：温度，范围 `5~35`

### 4.2 上下限设置

#### `heat_limit` 地暖温度上下限

- DP：`132`
- 标识符：`heat_limit`
- 权限：`rw`
- 数据结构：
  - Byte0：`0=上限`，`1=下限`
  - Byte1：温度值，范围 `5~35`

#### `ac_limit` 空调温度上下限

- DP：`133`
- 标识符：`ac_limit`
- 权限：`rw`
- 数据结构：
  - Byte0：`0=上限`，`1=下限`
  - Byte1：温度值，范围 `5~35`

前端建议：

- 这两个 DP 更适合做成设置页中的表单项
- 若要同时设置上下限，前端需分两次下发

### 4.3 按键配置类

#### `key_mode` 按键模式

- DP：`139`
- 标识符：`key_mode`
- 权限：`rw`
- 数据结构：
  - Byte0：按键通道，`switch_1 ~ switch_8`
  - Byte1：按键模式

按键模式原文定义：

- `switch_1`
- `switch_2`
- `switch_3`
- `switch_4`
- `unreal_switch_1`
- `unreal_switch_2`
- `unreal_switch_3`
- `unreal_switch_4`
- `jog_1`
- `jog_2`
- `jog_3`
- `jog_4`
- `light_?`
- `curtain_?`
- `scene_?`

前端建议：

- 页面上按“按键通道”逐个配置
- 模式选项使用下拉或单选列表
- `light_?`、`curtain_?`、`scene_?` 当前枚举不完整，需补充明确值后再固化前端代码

#### `jog_time` 点动时间

- DP：`141`
- 标识符：`jog_time`
- 权限：`rw`
- 数据结构：
  - Byte0：按键通道，`switch_1 ~ switch_8`
  - Byte1：点动开关，`0=关`，`1=开`
  - Byte2：时间，范围 `1~10s`

#### `key_name` 按键名称

- DP：`145`
- 标识符：`key_name`
- 权限：`rw`
- 数据结构：
  - Byte0：按键通道，`switch_1 ~ switch_8`
  - Byte1：对应图标，当前未定义
  - Byte2：字符串总字节数，包含 `\0`
  - Byte3...N：`utf-8` 字符串

前端建议：

- 提供按键名称编辑框
- 长度校验规则当前未给出，需设备侧补充最大长度
- 图标枚举当前未定义，建议先按“无图标/默认图标”处理，待补充后再接入

### 4.4 页面管理类

#### `page_mgr` 页面管理

- DP：`140`
- 标识符：`page_mgr`
- 权限：`rw`
- 数据结构：
  - Byte0：页面一元素个数，`0=删除`，`1~4=元素个数`
  - Byte1：页面二元素个数，`0=删除`，`1~4=元素个数`
  - Byte2：空调页，`0=删除`，`1=添加`
  - Byte3：新风页，`0=删除`，`1=添加`
  - Byte4：地暖页，`0=删除`，`1=添加`

前端建议：

- 做成“页面开关 + 页面元素数量配置”
- 首页当前仅定义了两个自定义页面：页面一、页面二

#### `page_sync` 页面同步

- DP：`148`
- 标识符：`page_sync`
- 权限：`rw`
- 用途：同步当前正在显示的页面

### 4.5 系统设置类

#### `base_set` 基础设置功能

- DP：`142`
- 标识符：`base_set`
- 权限：`rw`
- 数据结构：
  - Byte0：上电状态，`0=off`，`1=on`，`2=memory`
  - Byte1：振动开关，`0=off`，`1=on`
  - Byte2：人感上报开关，`0=off`，`1=on`
  - Byte3：人感开关，`0=off`，`1=on`
  - Byte4：人感倒计时，`10~60s`
  - Byte5：人感灵敏度，`1~10`
  - Byte6~8：指示灯颜色，`RGB888`
  - Byte9：指示灯亮度（默认），`0~100`
  - Byte10：背光亮度，`10~100`
  - Byte11：关闭指示灯，`0=off`，`1=on`
  - Byte12：屏幕待机样式，原文为 `NULL`

前端建议：

- 适合做成“基础设置”表单页
- 指示灯颜色可使用 RGB 颜色选择器
- `Byte12` 含义未明确，先预留

#### `adv_set` 高级设置功能

- DP：`143`
- 标识符：`adv_set`
- 权限：`rw`
- 数据结构：
  - Byte0：指示灯高亮，`0~100`
  - Byte1~5：页面排序，示例 `12345`

前端建议：

- 页面排序可做拖拽排序
- 下发前需要转成固定长度的排序数组

#### `theme` 主题切换

- DP：`144`
- 标识符：`theme`
- 类型：Enum
- 权限：`rw`
- 说明：原文为 `NULL`，主题枚举值未定义

#### `child_lock` 童锁

- DP：`146`
- 标识符：`child_lock`
- 类型：Bool
- 权限：`rw`

## 5. 展示参数 `disp_param`

- DP：`147`
- 标识符：`disp_param`
- 权限：`rw`

数据结构：

- Byte0~1：温度，`short`，范围 `-32768 ~ 32767`
- Byte2：湿度，`char`，范围 `0~100`
- Byte3~4：空气质量，`ushort`，范围 `0~65535`

示例：

- `21.5℃` 传值为 `2150`

前端展示建议：

- 温度按 `value / 100` 显示
- 湿度按百分比显示
- 空气质量按整数显示

## 6. 前端页面建议映射

### 首页

- 显示 `switch_1 ~ switch_8`
- 显示 `master_sw`
- 显示 `scene_1 ~ scene_8`
- 根据 `page_mgr` 决定页面入口和模块显示

### 空调页

- `switch`
- `temp_set`
- `mode`
- `fan_speed_enum`
- `fan_direction`

### 新风页

- `fresh_air_valve`
- `loop_mode`
- `fresh_air_speed`
- 可选扩展：`supply_fan_speed`
- 可选扩展：`exhaust_fan_speed`

### 地暖页

- `floor_sw`
- `floor_temp`

### 设置页

- `base_set`
- `adv_set`
- `theme`
- `child_lock`
- `heat_limit`
- `ac_limit`
- `key_mode`
- `jog_time`
- `key_name`

