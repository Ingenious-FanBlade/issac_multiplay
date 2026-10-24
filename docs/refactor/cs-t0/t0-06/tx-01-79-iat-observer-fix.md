# tx-01-79：按实际槽类型修正外部原字节取证

日期：2026-10-19。目标是修正 tx-01-78 的注入前观察器错误，不改生产实现或二进制。

- 原生 loop_runtime.cpp 的 kPeekIatRva=0x718714、kTranslateIatRva=0x718650、kDispatchIatRva=0x7185B8，Prepare 中通过 FindIatSlot 及 IsAddressOwnedBy(USER32.dll) 检查。
- 外部观察器对上述三个 4 字节槽要求当前指针唯一落入已枚举 USER32.dll 范围；其它 4 字节槽仍要求归属 isaac-ng.exe。记录 pointer-owners-before.jsonl，错误包含具体 RVA/期望/实际 owner。
- 调用与入口仍对照已知原始指令；所有 70 槽恢复字节一律和本次进程注入前的实际快照逐字节比较。模块基址与系统指针允许各自正常 ASLR；不以旧进程绝对指针替代本次原值。
- 外部观察器权限保持只读，原生安装事务完整保留自身 expected bytes/owner/目标检查。未放宽未知 owner，不增加写游戏、调函数或主动修复。

tx-01-77 与 tx-01-78 的原始 FAIL 分别保存，未来新 artifact 会复制两份报告说明本次实质修正。控制器沿用已纯编译的 tx-01-77 元数据版本，Native/Helper 沿用 tx-01-75。场景与故障范围见[原取证设计](tx-01-77-live-remaining.md)。修正期间没有另行游戏查询、试跑或测试；等待用户明确同意后执行新的唯一一次整体验收。

运行更新：用户确认后，一次整体运行已完成，**INCOMPLETE：21 PASS / 0 FAIL / 12 NOT_COVERED**；补齐八类字段和独立失联恢复，消费暂停得到部分证据。原始两个 FAIL 保留，见[完整结果](tx-01-79-e2e-results.md)。
