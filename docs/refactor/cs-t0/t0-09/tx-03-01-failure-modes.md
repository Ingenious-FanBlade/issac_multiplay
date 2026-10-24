# tx-03-01：T0-09 实施前失败方式

日期：2026-10-20（Asia/Shanghai）。本文件先于 T0-09 实现登记。设计主文见 [tx-03-01-design.md](tx-03-01-design.md)。当前没有运行游戏、构建或测试；本轮是文档设计。

## A. 拓扑、环境与双进程

1. 错误地把同进程逻辑 Remote 或单游戏进程结果当作 T0-09 双进程 PASS。
2. Host 或 VM 启动多个游戏窗口/进程，控制器按旧“唯一 `isaac-ng.exe`”规则选错目标或把输入发给错误进程。
3. VM 游戏不是同一 approved unpacked build，Native/Helper、input/state schema 或角色/seed/room 不一致，仍被当作可比较 baseline。
4. A/B 使用同一个 mapping/session/artifact 目录，导致串线、覆盖或旧 generation 复活。
5. 游戏上次残留 DLL、UDP relay、control socket 或旧 artifact；未完整重启便注入，形成混合状态。
6. 第二份游戏触发 Steam/原版 Online/Isaac Sync 或共享 save/achievement/unlock/统计持久副作用。
7. Host/VM 焦点、最小化、VM 暂停或用户同时操作导致输入/采样被错误归因于协议。
8. VM 共享文件夹延迟、权限、原子性问题使 manifest/脚本读取半成品或 artifact 丢失。
9. 用户预置 safe run 的 seed/room/角色不是实际 manifest 声明值，比较结果被伪造为可比。

## B. 控制面与启动屏障

10. `RUN_START` 前就开始发送数据包/输入，或在屏障前设置 script tick0。
11. 两端 ready/armed 条件未包含游戏 identity、Native 状态、script schema、artifact 权限和 relay 配置。
12. 控制消息无界、重放、旧 sequence、错误 run_id/role 被接受；连接断线后自动重连重跑。
13. HELLO/首 ACK 在 impairment 开启后才发送，导致矩阵把 bootstrap 丢包误归因于 F1–F5。
14. Established 被误当作 Native Authority active、exact pair ready 或本地 observer 已准备好。
15. STOP 后仍从 control/relay/wire IPC 接受 packet；或 stop 清理失败覆盖最初错误。

## C. Wire、IPC 与线程

16. 跨进程 UDP 承载时拼接多包、拆分单包、改变长度或直接改写 wire bytes；接收端未按单 datagram 精确长度校验。
17. UDP socket 在 worker/network 线程直接改 Player、Lua、canonical queue 或 scheduler。
18. Wire IPC 槽在复制前 release/覆盖，或发布 consumed 后仍读共享字节；ring 满时静默丢证据。
19. Shared wire IPC 版本/尺寸/role/session 校验不完整，旧 endpoint 或错误 role 接入新 session。
20. ACK/STATE 与 INPUT 共用错误 lane/顺序；duplicate/reorder 允许修改 accepted/processed 高水位。
21. 输入在 decoder/admission 前已写 queue，或 packet loss 被当作成功 apply。
22. Target/generation/sequence 校验不完整，旧代包或 late target 在下一秒被重新提交。
23. 网络线程使用墙钟/渲染帧生成 Authority tick；或由 UDP 到达推进 gameplay tick。

## D. UDP relay 和故障注入

24. Relay 修改 payload、重编码、重新计算 CRC、合并/拆分 packet，导致被测对象不再是 T0-08 wire。
25. Loss/duplicate/reorder 决策依赖真实到达交错或非固定 PRNG，同一 seed 无法重放。
26. Drop 后仍推进 endpoint send sequence/watermark；duplicate 被 endpoint 当成新 input identity 或新 edge。
27. Delay/jitter/reorder 只影响日志，不影响实际 forward；或 outage 在错误方向/窗口生效。
28. Relay/VM NAT/防火墙自身引入不可解释的丢失、重复或重排，未与配置 impairment 分开记录。
29. 大包导致 UDP 分片但没有记录；或 >1200 包进入 relay。
30. F5 在 tick2000 前/后不足 1000 tick 就 stop；或 2 s outage 的实际触发时间和 tick 范围缺失。
31. Relay 自己创建 session、ACK、包或输入，把 transport 故障变成伪造的协议行为。

## E. 输入和 observer-simulator

32. 共享脚本在两端 canonical bytes 不同，或 B 本地 replay 与 wire 发送使用两份不同来源。
33. B 本地 observer 把 ACK/STATE 应用到本地世界，违反“不应用 snapshot”。
34. 同一 B 输入在 VM 本地 apply 一次、wire 又在本地 apply 第二次；或 B 本地 replay 的 A 输入被错误发到 Authority。
35. A 本地脚本绕过 canonical/queue/admission 旁路；或 B wire input 被 receiver 直接写世界。
36. 本地 observer 和 Authority 的 tick 偏差超过 lead/future 窗口，B 输入总是 late 但报告为成功。
37. Observer 用渲染帧、墙钟或 UI 状态替代 Authority tick；没有完整来源时伪造状态字段。
38. 将 observer 本地状态名称/entity id/pointer 当作跨进程稳定 identity。
39. 输入脚本覆盖重复 edge、迟发 recovery、hold/release 或 1/5/30/120 静默时只跑其中一部分。
40. 测试中自动补发过期 edge、滑动 target 或把 loss 后输入重放成新 sequence。

## F. 状态比较与 first-divergence

41. 把 partial/gap 观测当成完整状态相等，或用单一 checksum mismatch 报告。
42. 比较了 address/vptr/Player/device、SteamID、user path 等不可比或隐私字段。
43. 采集两端的 tick/room/capture token 不一致，仍拼接成同一 tick 状态。
44. 将 F1–F5 的任意差异都归因为网络；没有关联 input arrival/apply 或已知未复制状态。
45. F0 出现未知 divergence 时删除/跳过 tick 或放宽 subset 以制造通过。
46. first-divergence 没有前后窗口、双端 frame、relay 因果链和重放入口。
47. 计算或校验 hash 作为 gate/证据代替实际字段比较。

## G. C0/C1/C3 与恢复

48. C0 安装/注入 T0 Native，或 C1 disarmed 实际建立 C/S session/发送 packet/写 canonical。
49. C0/C1 使用短外部键序列后声称与 C3 canonical 脚本状态相等。
50. C2 按旧计划触碰 Original Online/Isaac Sync；或把“未触碰”写成通过。
51. 任一端 Hook 未恢复、DLL 被强制卸载、游戏被杀死或旧进程复用仍报正常。
52. 恢复只验证“若干槽”而不核对每端实际完整 installed catalog，或把恢复失败当可解释退役。
53. 矩阵失败后自动重跑、仅换编号重复同一验收、覆盖历史失败 artifact 或删除未覆盖项。

## H. 资源、隐私与验收

54. 完整矩阵事件/包/状态全量无界保存耗尽磁盘，随后静默丢证据。
55. 公共 artifact 泄露 SteamID、用户名、完整路径、peer identity 或无关进程信息。
56. 使用旧 gate/解锁/hash 字段作为实现或验收前置条件。
57. 只运行 F0 或单个 seed 就宣称 T0-09 完整 PASS。
58. 文档/报告把 observer-simulator 写成权威同步、Replica World 或可玩双端。
59. 没有用户环境准备确认/实际游戏身份就启动双机操作。
60. 测试期间并发操作同一 host/VM 游戏、调试器或另一矩阵 case，造成不可归因混跑。

以上任一项出现时，保留原始错误、tick/窗口、双端状态和已恢复项，不自动重跑；后续修复作为新的推进。
