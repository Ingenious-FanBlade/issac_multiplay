# tx-03-16 Luna：Sprite draw 字段与特殊 flags 静态逆向

日期：2026-10-23（Asia/Shanghai）  
范围：独立 Sprite 展示所需的 native draw 数据流；限定于 `0x409120`、`0x4098A0`、`0x409CB0`、`0x40A0D0`、`0x40A140` 及解释这些路径所需的直接 helper 和字段访问器。证据只来自当前 IDA IDB 的静态读取。

完整原始导出见 [Luna draw 字段 artifact](../../artifacts/cs-t0/tx-03-16-static/luna-draw-fields.json)，导出脚本见 [只读 IDA exporter](../../artifacts/cs-t0/tx-03-16-static/luna-export-sprite-fields.py)。artifact 收录目标及直接 helper 的完整指令 bytes、逐条 VA/RVA、反汇编、Hex-Rays 伪代码、目标入口 xrefs、IDA 输入路径和 imagebase；导出失败也保留为字段。`imagebase=0x400000`，故 RVA=VA-0x400000。没有做哈希检查或任何运行时操作。

## 调用链、索引与可见性

**STATIC_CONFIRMED — `0x40A0D0`（RVA `0xA0D0`）与 `0x40A140`（RVA `0xA140`）是两个 draw wrapper。** 两者先要求 Sprite `this+0x109` 为非零，再分别进入遍历组 helper `0x40A030` 或选层 helper `0x40A080`。Sprite `this+0x70` 决定先后调用内嵌描述符区域 `this+0x30`、`this+0x50` 的顺序：为零时先 `+0x30` 后 `+0x50`，非零时相反。此处是两个渲染组的调用顺序；字段的原生构造默认值未从这些函数中得到。

**STATIC_CONFIRMED — `0x409120`（RVA `0x9120`）的 ECX 是视觉 track descriptor。** `0x40A030` 从描述符表按 `0x10` stride 遍历 track，并以该 track 的 selected-key index 调用 `0x409120`；关键 raw 片段为 `0x40A058: 8B 4A 18`（从 track 表取基址）、`0x40A05E: 03 CB`（加 `0x10*i`）、`0x40A06F: 83 C3 10`（下一个 track）。`0x409120` 对 ECX descriptor 读取：`+0` 全局 Sprite layer ordinal、`+4` frame-record 数组指针、`+8` key 数、`+0xC` track active byte。它要求 active、`0 <= selectedKeyIndex < keyCount`，再以 `*(track+4)+0x6C*selectedKeyIndex` 取当前 frame record；record `+0x2C` 必须非零。对应 raw 指令包括 `0x409161: 80 79 0C 00`、`0x409188: 3B 41 08` 与 `0x40919D: 80 78 2C 00`。

`0x409120` 使用 `track+0` 的 ordinal 选 Sprite layer：`*(Sprite+0x7C) + ordinal*0xA0`。raw `0x4091A7` 起先取 track `+0`，再乘 `0xA0` 并加到 Sprite `+0x7C` 字段所存指针；layer 的 draw-enable/visibility byte 位于 `+0x74`，`0x4091B8: 80 7F 74 00` 为非零检查。由此必须分别识别 track index、selected-key index、全局 layer ordinal 和资源 ID；它们不是同一个数。`0x40A140` 的 selected-layer 路径还会先检查 ordinal 是否小于 Sprite `+0x80`，再用映射表 `+0x38+4*ordinal` 取得 track index，拒绝负映射后读取该 track 的 selected-key index。

**STATIC_CONFIRMED — 入口至少有四项独立 draw 检查。** 除 wrapper 的 Sprite `+0x109` 外，`0x409120` 还检查 track `+0x0C`、选中 frame record `+0x2C`、实例 layer `+0x74`。它取得当前 layer image/control pair 后才继续几何与 image virtual call。pair 位于 layer `+0x98/+0x9C`；该路径不从 ANM path 或 resource ID 自动重载图片。`sub_408590` 会检查并同步已有 pair 的资源参数缓存，参数变化时调用资源虚函数并更新缓存，所以 draw 路径可能触碰 renderer/resource 状态。

**STATIC_DERIVED — 每次 draw 还依赖调用者给的外部参数。** `0x40A030`／`0x40A080` 把 wrapper 入参透传给 `0x409120`；除 selected-key index 与 Sprite 指针外，`0x409120` 再把其中的向量/指针与两项变换参数传给 `0x4098A0`、`0x409CB0`。这些值来自 `0x40A0D0/0x40A140` 的上游 render caller，而不是由 track 自身算出；本报告未沿多个场景 caller 扩展，因此其具体坐标系、默认值和逐帧来源为 `UNKNOWN`。Replica 必须从稳定 renderer owner 提供这些上下文，不能把它们误当作 Sprite 字段或网络 frame。

## transform、flip 与颜色字段

**STATIC_CONFIRMED — Sprite property accessors 将字段映射到以下 offset。** 对应 setter/getter 的完整 bytes 和伪代码在 artifact 中。

| 属性 | Sprite offset | 访问器观察 | draw 中的使用 |
| --- | --- | --- | --- |
| Offset | `+0x94/+0x98` | `0x456340` 写两 float，`0x49CB40` 返回 `this+0x94` | `0x409CB0` 将其加到图像坐标，作为平移输入 |
| Scale | `+0x9C/+0xA0` | `0x420170` 写两 float，`0x417200` 返回 `this+0x9C` | `0x4098A0`/`0x409CB0` 组合 frame、layer 和 Sprite scale；Sprite scale 零会退化，crop 除法分母则是 frame×layer scale |
| Rotation | `+0xA4` | `0x43EE80` 写 float，`0x417210` 读回 | `0x409CB0` 把它送入 `0xA10760` 旋转 helper |
| Color | `+0xA8..+0xD3` | `0x4073D0` 复制 44 bytes，`0x407410` 返回字段地址 | 非活动颜色栈时，`0x409120` 从 `Sprite+0xA8` 复制 44 bytes 到 draw-local 状态 |
| FlipX / FlipY | `+0x100/+0x101` | setter `0x435C60/0x505B50`；getter `0x407420/0x407430` | `0x4098A0` 与 `0x409CB0` 均读取并参与几何/变换 |

**STATIC_CONFIRMED — 方向由 Sprite、layer 与 scale 符号共同决定。** `0x4098A0` 读取 Sprite flip bytes `+0x100/+0x101`，也读取实例 layer `+0x32/+0x33`；再比较实际合成后的 X/Y scale 正负，必要时重排 quad 顶点。`0x409CB0` 也用 Sprite flip 和 scale 符号修正坐标、scale/rotation 的方向。只复制 Sprite 的两个 flip bool 而忽略 ANM layer 的 flip 和变换符号，不能保证得到相同朝向。

**STATIC_CONFIRMED — Color 不是已证明的 4-byte RGBA 字段。** setter 复制完整 44-byte block。`0x409120` 在 `dword_C78DC0==0` 路径从 `Sprite+0xA8` 取该 block；颜色栈非空时则由 `0x40C6F0` 取当前栈项。直接 helper `0x683060` 将该 block 的子区传给 `"Colorize"`（从 `this+0x10`）和 `"ColorOffset"`（从 `this+0x20`）着色器参数；所以最小状态不能擅自把这个 native block 截成 RGBA。确切 11 个 float 的语义与构造默认值仍为 `UNKNOWN`。

`0x409120` 在 `byte_C7977D` 为真时修改 draw-local block 的第四个 float，并可乘 renderer 全局 scale；其具体语义未由此处伪代码确定。`0x409120` 的 bit `0x10` 特殊分支还从 Sprite `+0xD4` 读取 `ChampionColor` 相关数据并提前走另一 shader pass。不要把 `+0xD4` 与上述 44-byte Color block 合并。

## special flags 与 renderer 外部状态

**STATIC_CONFIRMED — 两个 flag 字段会按位 OR 使用：** `layer+0x8C | Sprite+0x110`，raw `0x4091EB`/`0x4091F1`。本次范围确认这些消费点：

| bit | 静态效果 |
| --- | --- |
| `0x01` | 选择 layer-local Color；另一分支选择 Sprite Color 或当前颜色栈项，再与 frame Color 组合。`4071C0` 复制的是 44-byte Color，并非矩阵。 |
| `0x02` | `0x4098A0` 对 quad 四角加入基于共享 `C33914` 的偏移。该分支在 `0x409AF0` 与 `0x409B7D` 两次写回 `C33914`；这是非纯读、有状态的 draw 路径。下游用途未知，不能由此推断为 gameplay RNG，也不能断言它不影响同步。 |
| `0x04` | 若全局 `byte_C7977E` 为零则跳过绘制。raw `0x4091FD: F6 C1 04`。 |
| `0x08` | `0x409CB0` 使用另一组 layer/frame 维度与 Sprite scale 组合，再继续坐标修正。 |
| `0x10` | `ChampionColor` 特殊参数/shader 分支，使用 Sprite `+0xD4` 并提前结束普通 pass。 |
| `0x20` | 选择 palette-index shader pass；`0x40` 决定 index 从哪个 game/render global 读取。 |
| `0x40` | 与 `0x20/0x80` 分支共同决定 palette/source 值，归一化到 0..1 传入渲染状态。 |
| `0x80` | 选择另一特殊 shader pass。 |
| `0x200` | 当更高优先分支未命中时，把 draw-local 四个 float 直接送入 image virtual draw pass。 |

未列 bits 的含义未知。`layer+0x8C` 和 `Sprite+0x110` 的构造/默认值没有在本次范围内追到。

即使两个 flag 字段均为零，结果仍依赖 renderer 外部状态：`dword_C78DC0` 当前颜色栈、`C7977D/C7977F` 全局开关、`C33910` 坐标模式、`n3_2/dword_C7B644` 坐标偏移，以及 `0x409120` 接收的 caller 参数。`C7977E` 的上述可见性分支只有 bit `0x04` 置位时才读取。`C7977F` 还触发额外 quad center 处理。故“特殊 flags 全零”只能限定 native per-Sprite/per-layer 特殊分支，不能表示像素输出只由 Sprite 字段决定。

## 最小字段必要性与默认建议

以下默认是独立 Replica 的**设计建议**，不是已观察到的原生构造默认。

| 状态 | 必要性 | 建议默认 / 限制 |
| --- | --- | --- |
| 每视觉 track 的 active、key 数组、selected-key index、track→global-layer ordinal | 直接决定是否绘制、当前 frame record 与对应 layer | 固定姿态也要按 track 保存 selected-key index；不能用 global frame、track index 或 resource ID 替代 layer ordinal。 |
| Frame record 可绘制状态、layer draw-enable、image/control pair | 各自是独立检查／来源 | 局部 draw 要求它们满足；逻辑描述可以显式表示无帧／不可见，不据此猜测资源成功。原生默认未知。 |
| Offset、Scale、Rotation | 参与最终几何和坐标 | 建议 `(0,0)`、`(1,1)`、`0`；Scale 必须有限且非零。原生默认未经确认。 |
| FlipX/FlipY | 参与 quad 顶点顺序及变换 | 建议都为 false；layer 自带 flip 仍须尊重资源数据或明确限制资源子集。 |
| Color 44-byte block | 非活动外部颜色栈时直接进入 draw state，并含 shader 参数区 | 原生默认和 11 个 float 的完整语义未知；必须选择已知的固定 identity block 或完整承载字段，不要把 44 bytes 清零/截断后声称等价。 |
| `+0x109`、track/frame/layer 的可绘制条件 | wrapper 与 inner draw 分别检查 | 显示资格与资源就绪分开；隐藏状态可显式描述，初始化默认未知。 |
| `+0x70` 双组顺序 | 改变两个 descriptor set 的绘制先后 | 可在首版固定 `false`（先 `+0x30` 后 `+0x50`）作为设计约束；原生默认未知。 |
| layer `+0x8C` 与 Sprite `+0x110` special flags | 影响矩阵、抖动、可见性和 shader 路径 | 保守支持集固定两者为零；不复制非零值，也不承诺其 renderer side effects。 |
| 每次 draw 的 caller/context 向量与 pivot/matrix 输入 | 由 wrapper caller 透传到几何和坐标 helper | 必须由一个稳定 renderer owner 提供；本报告未得出通用缺省值。 |

这些字段之外，目标 draw 路径没有读取单独的 layer priority 字段；排序至少受双组 `+0x70` 顺序和外部 renderer pass 影响，priority 的完整上游处理不在本范围内。资源加载与 ANM 所有权、SetFrame/Play 状态重建、事件语义由相邻研究处理；本报告不把 draw helper 当作独立可运行的资源创建接口。

表中同时列出本地资源依赖和实例状态，不表示全部应上网传输。track／keyframe 表及其可见状态可由声明的相同 ANM 资源派生，image/control pair 只在本地管理；网络候选只承载所需逻辑身份／姿态，不发送这些数组指针或控制块。

## 主审修订记录

初步消息曾把 `409120` 的 receiver 称为 animation state，raw 调用流已更正为 visual track。终稿曾把步进指令误记为 `40A06E`，实际是 `40A06F`；前者只有 `inc esi`。主审还修正 bit `0x01` 的 matrix 解释为 Color 选路、说明指针字段解引用，并区分 flags 全零下仍读取的全局与仅 bit `0x04` 使用的 `C7977E`。主审新增 [primary-color-composition.json](../../artifacts/cs-t0/tx-03-16-static/primary-color-composition.json)确认 Color 分量组合；原始 Luna JSON 保留，不覆盖为新观察。

## 未知与范围

- 原生 constructor/Load 的 Sprite transform、Color、active、visibility、flip 与 flags 默认值未知；本报告中的 identity 值仅为设计 proposal。
- `Color+0xA8` 44 bytes 的完整字段语义、外部颜色栈的 owner/lifetime 与 caller-specific matrix/pivot 输入未闭合。
- 非零 special flags 的 shader 资源、全局 RNG/state 的所有消费者及 draw virtual target 未追；共享 `C33914` 写入的用途未知。
- 静态伪代码不证明实际像素、显隐默认值、线程边界或所有全局 context 在 Replica 环境中的有效性。

本次不改 IDB 与生产代码；仅新增本报告和 `artifacts/cs-t0/tx-03-16-static/luna-*` 静态研究产物。
