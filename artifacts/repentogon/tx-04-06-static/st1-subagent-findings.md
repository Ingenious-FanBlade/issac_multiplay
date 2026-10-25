# ST1：InputDeviceBase 38 槽静态确认

## 范围、目标与取证状态

**STATIC_CONFIRMED**：当前 IDA 数据库输入文件为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，IDA imagebase 为 `0x400000`，架构 32-bit；`inf_get_min_ea()` 是 `0x401000`，不是 imagebase。未读取或计算哈希。基类 vtable 在 VA `0xB32F04` / RVA `0x732F04`，共 38 槽；基类大小 `0x3F8`。

**取证纠错记录**：

- 第一轮导出误把 `min_ea=0x401000` 当作 imagebase，故该轮 RVA 比正确值小 `0x1000`。已用 `ida_nalt.get_imagebase()=0x400000` 重导并覆盖错误的 `ida-st1-raw.json`；错误版本没有保留，不能作为证据。
- 旧 `ida-st1-ui-functions.json` 的请求点 RVA `0x4B95C0` 实际落在函数起点 RVA `0x4B8C70`，不应据此命名 KeyConfig Update。现已在正确 VA `0x8B9AD0` / RVA `0x4B9AD0` 直接导出 `sub_8B9AD0`，文件为 `ida-st1-keyconfig-confirmed.json`；旧 candidate 保留且不强行归属。
- 我第一遍阅读 KeyConfig 伪码时漏看了等待按键分支的设备 slot11。主审给出调用点后，我再次直接读取 IDA 汇编，确认 RVA `0x4B9D1A` 调用 `[edx+0x2C]`、RVA `0x4B9D1F` 与 `-1` 比较；更正后的证据已经保存。此处是设备基类 slot11，不是 InputManager vtable 槽号。

未修改 IDB 类型、名称、注释、补丁或字节。本文只陈述静态证据、推断和设计建议，不代表运行时验证。

## 38 槽原始表

下表为 `ida-st1-raw.json` 中读取的 vtable 指针目标 RVA 与 IDA 名称。目标名保留 IDB 原样；除已单独分析的项外，`sub_*` 不推定语义。

| 设备槽 | 目标 RVA | IDA 名称 |
|---:|---:|---|
| 0 | `0x5ec660` | `sub_9EC660` |
| 1 | `0x5ec6a0` | `sub_9EC6A0` |
| 2 | `0x5ec7b0` | `sub_9EC7B0` |
| 3 | `0x68801d` | `_purecall` |
| 4 | `0x5ec840` | `sub_9EC840` |
| 5 | `0x5ecb80` | `sub_9ECB80` |
| 6 | `0x5ecc40` | `sub_9ECC40` |
| 7 | `0x5ecd00` | `sub_9ECD00` |
| 8 | `0x5ecd80` | `sub_9ECD80` |
| 9 | `0x5ecde0` | `sub_9ECDE0` |
| 10 | `0x5ece50` | `sub_9ECE50` |
| 11 | `0x5eced0` | `sub_9ECED0` |
| 12 | `0x5ecf50` | `sub_9ECF50` |
| 13 | `0x5ecf90` | `sub_9ECF90` |
| 14 | `0x5ed070` | `sub_9ED070` |
| 15 | `0x5ed0b0` | `sub_9ED0B0` |
| 16 | `0x5ed0d0` | `sub_9ED0D0` |
| 17 | `0x5ed130` | `sub_9ED130` |
| 18 | `0x5ed170` | `sub_9ED170` |
| 19 | `0x5c4e70` | `UserMathErrorFunction` |
| 20 | `0x5ed190` | `sub_9ED190` |
| 21 | `0x5ed210` | `sub_9ED210` |
| 22 | `0x5ed2a0` | `sub_9ED2A0` |
| 23 | `0x28390` | `sub_428390` |
| 24 | `0x28390` | `sub_428390` |
| 25 | `0x5ed330` | `sub_9ED330` |
| 26 | `0x5ed3a0` | `sub_9ED3A0` |
| 27 | `0x5ed4e0` | `sub_9ED4E0` |
| 28 | `0x5ed600` | `sub_9ED600` |
| 29 | `0x5ed610` | `sub_9ED610` |
| 30 | `0x5ed620` | `sub_9ED620` |
| 31 | `0x5ed6a0` | `sub_9ED6A0` |
| 32 | `0x5ed6c0` | `sub_9ED6C0` |
| 33 | `0x5ed700` | `sub_9ED700` |
| 34 | `0x5ed730` | `sub_9ED730` |
| 35 | `0x68801d` | `_purecall` |
| 36 | `0x27ee0` | `nullsub_8` |
| 37 | `0x5ed850` | `sub_9ED850` |

## 消费者与调用路径

**STATIC_CONFIRMED — InputManager 更新**：RVA `0x5EDCC0` 更新设备时调用公共更新 RVA `0x5EC900`；公共更新通过 vptr+`0x8C` 调设备 slot35。Poll 返回 false 时，InputManager 执行回调和移除，按设备 vptr 调 slot2，再以参数 1 调 slot0，最后从设备向量删除。Poll 返回 true 时，更新路径还调用设备 slot26（vptr+`0x68`）进行活动/设备处理。单独 RemoveDevice RVA `0x5EEBB0` 也走 slot2→slot0(1)。主审另从全局 shutdown RVA `0x5EDAA0` 确认同一 slot2→slot0(1) 路径。

**STATIC_CONFIRMED — Join pass**：Join pass RVA `0x591360` 在调用点 RVA `0x591794` 对查到的设备调用 vptr+`0x44`，即设备 slot17，参数为十进制 19；返回值在该调用点后未读取。slot17 基类实现 RVA `0x5ED130` 在索引有效时对两个已映射值调用 slot16，并清理对应的每项状态。**STATIC_INFERENCE**：这会改动基类映射/状态缓存；若 K 的逻辑 Action 查询和原始查询都走自有状态，slot17 不会替代 K 的 Action 状态，暂不列为必须覆写项。它是否应屏蔽仍取决于 K 的最终数据模型。

**STATIC_CONFIRMED — MenuManager**：MenuManager Update RVA `0x561CB0` 的设备枚举在 RVA `0x56207D` 直接调用设备 slot27，按 AL 结果更新全局菜单控制器索引。该函数另在 RVA `0x561FDA` 调 InputManager wrapper RVA `0x5EE6C0`；wrapper 查设备后调设备 slot26（vptr+`0x68`）。InputManager vtable 的槽33是这个 wrapper；不要把它与设备基类 slot33 混淆。

**STATIC_CONFIRMED — KeyConfig**：正确函数 RVA `0x4B9AD0` / VA `0x8B9AD0` 的等待按键分支在 RVA `0x4B9D1A` 调设备 slot11（vptr+`0x2C`），RVA `0x4B9D1F` 把返回值与 `-1` 比较。获得有效输入后，它在 RVA `0x4B9D58` 调设备 slot33（vptr+`0x84`）写映射。RVA `0x4B9D00` 在 reset 路径调设备 slot30（vptr+`0x78`）。导航另经 InputManager wrapper RVA `0x5EE200`（InputManager vtable 槽12）调用设备 slot9；在 RVA `0x4B9B1A` 也直接对查到设备调用 slot9。

InputManager 自身的 vtable 槽7/8/9/10 对应 RVA `0x5EDEA0/0x5EDF60/0x5EE020/0x5EE040`，它们分发设备原始 slot5/6/7；InputManager vtable 槽11/12/14 对应 action wrappers RVA `0x5EE140/0x5EE200/0x5EE2E0`，分发设备逻辑 slot8/9/10。这里的管理器槽号与 38 槽设备接口索引是不同的编号空间。

消费者结论只针对以上已核实函数与调用点。保存的广泛 `[reg+0xC]` 搜索命中很多未分类 vtable（包括锁对象），不能据此断言它们属于设备 slot3；也不能把已找到路径外推为全映像无其他消费者。slot3 的未知类型菜单分支/其他间接调用仍未完成设备对象归属确认。

## ABI 与返回值

以下 ABI 以 32-bit 原始汇编的参数准备、寄存器返回和 `ret/retn` 为准；未更改 IDB 类型。Hex-Rays 对个别类槽的声明有省略/歧义，不以声明替代汇编。

| 槽 | STATIC_CONFIRMED 语义/ABI | K 的约束 |
|---:|---|---|
| 0 | 基类删除析构路径：`this` 在 ECX，stack 上一个 flags 参数；汇编在 RVA `0x5EC66C` 直接 `call 0x5EC7B0`（与 slot2 指向同一函数），检查 flags bit0，并在 bit0 为 1 时按 `0x3F8` 大小释放；EAX 返回原对象指针，`retn 4`。 | NEVER 必须覆写：不要释放 K 对象，不调用基类析构释放；返回 `this`，只记录/执行 K 自己允许的退役状态。 |
| 2 | `void __thiscall(this)`，无显式参数、`retn`。基类释放 `this+0x1C` 的映射数组及其中指针，并释放 `+0xC8`、`+0xCC` 缓冲。 | NEVER 必须 no-op 覆写；不能透传基类释放。 |
| 3 | 基类为 `_purecall`。Keyboard 实现 RVA `0x633C50` 将字符串 `"Keyboard"` 地址放入 EAX；Gamepad RVA `0x632F60` 从 `this+0x3FC` 读指针。均为无显式参数的 thiscall，EAX 返回 C 字符串指针，`retn`。这不是整数设备类型码。 | 必须覆写为稳定、NUL 结尾的设备名指针。主审另核实官方 Initialize hook 会读 `+0x3FC`；应在 Initialize 前将尾部字段设为有效稳定名称或安全空值，不得拿它覆盖状态字段。 |
| 35 | 基类为 `_purecall`。Keyboard/Gamepad Poll 实现均为无显式参数 thiscall，AL 返回 bool，`retn`；公共更新在每轮调用该槽，false 进入移除/析构链。 | NEVER 必须覆写；活动期间返回 true。只允许在有意退役流程中返回 false，并保证 slot2/0 不释放 K。 |

关于 slot2 的反例：InputManager 的 false-poll、主动 RemoveDevice 和 shutdown 都会走 slot2→slot0(1)。仅覆写 slot0 仍会先被 slot2 释放基类资源；仅覆写 slot2 又会让 slot0 按 flags 位释放整个 `0x3F8` 对象，因此 NEVER 需要两者都截断。

## K 的覆写建议与中性策略

这是根据静态调用路径提出的 **DESIGN**，不是原生二进制事实。推荐的 13 槽集合：

- **生命周期/类型必要覆写**：0、2、3、35。行为见上表；slot35 活动态为 true。
- **K 的 Action 接口**：8、9、10 使用 K 自己的同步 Action 状态；查询不到有效动作时中性返回 false/0、false/0、浮点 0.0。不要透传到基类物理按键/轴缓存。
- **原始物理输入防旁路**：5、6 返回 false；7 返回 ABI 对应的浮点 0.0。InputManager 原始 wrappers 会直达这些槽，基类 setter/初始化也能写缓存；直接覆写可确保 K 不成为额外本地控制器。
- **按键捕获/映射读取**：11 返回 `-1`。基类实现会枚举映射并经 slot6 探测输入；InputManager 公共更新还有 pending-remap 路径调用 slot11，KeyConfig 也用它判断是否捕获到物理输入。-1 是已确认调用点使用的无输入哨兵。
- **菜单活动路径**：26、27 返回 false，避免 wrapper 和 MenuManager 选择 K 为菜单活动控制器或更新菜单索引。

**DESIGN — 暂留基类**：slot30 与 slot33 在 KeyConfig 中用于 reset/set mapping；但 slot30 也由基类 Initialize 在 RVA `0x5EC6A0` 通过 vptr+`0x78` 调用，负责初始化映射槽，因此当前建议保留基类实现。slot33 写的是基类映射数组；当 slot5/6/7/11 都屏蔽且 K 的 Action 查询读取独立状态时，该数组不进入 K 的查询输出。若设计决定 K 必须禁止所有配置 UI 对其映射缓存写入，可再把 slot30/33 改为受控 no-op，但要另行处理 Initialize 所需初始化语义。slot17 在 Join 中确实被调用；当前暂留基类是基于 K 不读基类 action/raw cache 的推断，若 K 复用这些缓存，应重新评估。

**DESIGN — 中性默认**：slot18 保留基类实现（主审独立确认其 sret 输出两个 DWORD 的全局当前值为零），slot36 保留基类 `nullsub_8`（`ret 8`）。不要为这两个槽增加与 K 无关的状态。

## STATIC_INFERENCE / UNKNOWN 边界与文件索引

- **STATIC_INFERENCE**：13 槽集合是在 NEVER 生命周期、同步 Action 与禁止本地设备输入旁路的设计约束下推导；需由 K 最终数据模型确认各 Action 返回是否读取自有事件。
- **UNKNOWN**：除列出的已核实 callsite 外，未声称全映像不存在其他设备槽调用；通用槽偏移候选未完成对象类型分类。KeyConfig/MenuManager 对设备名/未知类型菜单分支的完整逻辑仍有未决处。
- **STATIC_CONFIRMED 风险 / UNKNOWN 完整路径**：旧候选文件中的函数起点 RVA `0x4B8C70`（不是 KeyConfig Update）在 `g_MenuControllerIndex != 0` 且设备 lookup 成功分支直接读 `device+0x48C` 与 `device+0x498`；这两个偏移超出 `InputDeviceBase` 的 `0x3F8`。slot26/27 返回 false 是防止正常菜单选择 K 的设计防线，但不能证明所有 UI 都不会以其他状态选中 K，也不能让该尾部读取变安全。完整排除/适配路径留给 ST9；ST1 不扩展追踪。
- **UNKNOWN**：未做运行、构建或测试；本报告只证明静态结构与调用链。

原始与汇总证据（均为只读 IDA MCP 输出；`st1-subagent` 目录由本子任务写入）：

- `ida-st1-raw.json`：目标 image 元数据、完整 38 槽、选定设备 vtable、slot0/1/2/35 相关原始指令与伪码、公共更新/InputManager/Join 等导出。
- `ida-st1-consumers.json`：48 槽 InputManager vtable 与消费者候选；其中全局偏移候选保持未分类。
- `ida-st1-keyconfig-confirmed.json`：正确 KeyConfig Update RVA `0x4B9AD0`、完整函数反汇编/伪码/xrefs，slot11/30/33 的实现，MenuManager wrapper slot26 以及直接调用点汇编；附 imagebase 和 min_ea 原始读数。
- `ida-st1-ui-functions.json`：保留原来的候选导出；其中 `0x4B95C0` 项不是 KeyConfig Update，勿按请求 RVA 名称归属。
- `ida-st1-slot3-candidate-scan.json`：过窄匹配结果为空，非“没有消费者”的证据。

