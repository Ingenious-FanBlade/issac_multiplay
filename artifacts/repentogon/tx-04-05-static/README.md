# tx-04-05 静态证据

日期：2026-10-24。覆盖 T1–T8：Odd 普通调用点、设备身份与查询、屏蔽、暂停/失焦、new-room 时序、Player 字段和 consumer action。用户指定 GPT-6 Luna / max 子智能体负责逆向，主智能体独立取证并审阅。最终结论以[静态确认报告](../../../docs/research/tx-04-05-static-confirmation.md)及[实现方案](../../../docs/design/tx-04-05-implementation-plan.md)为准。

## 输入与范围

- 当前 IDB：`isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，基址 `0x400000`；完整输入路径记录在导出头部。
- 官方源码：`F:\projects_isaac\REPENTOGON-1.1.3`。
- 原型语义来源：本仓库 `cs-t0/native/src/coop_input.cpp`、`coop_active_runtime.cpp` 等，只作为已采纳语义的实现依据，不复用旧版本设备布局。
- 全部工作为文件/源码阅读、只读 IDA 反编译和反汇编及文档编辑，没有构建、实机操作、注入或测试。未改 IDB 名称、类型、注释或字节；不计算/校验哈希。

## 阅读顺序

| 文件 | 内容 |
| --- | --- |
| [luna-findings.md](luna-findings.md) | 子智能体 T1–T8 草稿，已保留其修正；不代替后续主审 |
| [candidacy-corrections.md](candidacy-corrections.md) | 候选文件误配、宏遗漏、地址和语义更正历史 |
| [primary-ida-evidence.txt](primary-ida-evidence.txt) | 主智能体直接读取 IDB 的汇编、字节和函数伪代码；含 WM_ACTIVATE→焦点回调链、回调前后位置写入、Lua 切房参数→过渡对象字段的完整链 |
| [T6-supplement.md](T6-supplement.md) | Luna 切房补充经主审修正后的结论，区分 Player 与 type=3 实体写入、普通 animation=0 与特殊分支 |
| [t6-transition-position-evidence.txt](t6-transition-position-evidence.txt) | Luna 补充时的原始推导；原文误认保留，开头追加主审勘误，最新结论读 T6-supplement.md |
| [source-evidence.txt](source-evidence.txt) | 官方 Hook/菜单/ImGui/ABI/字段源码片段和原型每 target 输入语义，保留原行号 |
| [source-index.txt](source-index.txt) | 子智能体分批导出的来源及函数索引 |
| `t1_*.txt`–`t8_*.txt` | 原始反编译与反汇编、重点调用链和窗口；文件名误认以更正记录和实际函数头为准 |

主审的增量结论：

1. 确认 T1 的普通点 `0x2F9559`，纠正 Room 列表误称 PlayerManager、遗漏宏生成的 Player Update Hook；确定使用调用点 shim 的设计，不读取 ZHL 桩帧。
2. 补查设备 raw query 的下层调用和公共更新的 Poll 后派生，采用公共更新成功返回后采样。
3. 补齐失焦静态链：WndProc `0x629010` 的 WM_ACTIVATE → 窗口 `+0x1F4` 回调 → `0x5E4910` → `0x581AC0` → Manager 焦点字段 → Game/Odd 分支。子智能体草稿原来未闭合的 Windows 来源由此补充。
4. 补查 T6 条件路径：`0x3395B6 → 0x592A80 → 0x39EA20` 在 new-room 回调前写 Player 位置/速度；animation=11/20/3 特定分支仍有回调后 Player 定位。Lua `StartRoomTransition → native 0x2F9710 → setter 0x423990` 的 animation→`this+0x1C` 已对齐，S9 显式选 0。纠正两处批量写入实际是 type=3、variant=238 实体；所有路径最终定位次序仍 UNKNOWN，回调后不立即重绑/apply。
5. 读取 T8 动作常量表，确认 movement Triggered 使用 `[0,1,2,3]`、shooting 特殊 Triggered 使用 `[4,6,5,7]`；确认主动道具默认 action 9、特殊条件用 10。
6. 从原型发现并修正方案中的持续样本去重错误：held 值不变也要为后续 target tick 提交新鲜样本；只对同一轮重复发布去重。

## 导出脚本

- [primary_ida_review.py](primary_ida_review.py)：通过 ida-mcp 的 `py_exec_file` 在当前目标 IDB 中读取重点函数/指令及 xref，生成主审证据。脚本不写 IDB。
- [luna_export_ida.py](luna_export_ida.py)：按当前基址导出 T1–T8 函数与窗口；`TX0405_STATIC_OUT` 可指定输出目录，默认写入本目录的 `reexport/`。本轮分批原始导出保留在根目录。
- [export_sources.ps1](export_sources.ps1)：只复制带原行号的官方源码与原型片段；`-SdkRoot` 可指定 tag1.1.3 源码目录。

这三个入口只生成静态阅读材料，不能给出功能通过/失败结果。本轮没有调用 tx-04-05 的 E2E，也未使用静态脚本代替整体验收。
