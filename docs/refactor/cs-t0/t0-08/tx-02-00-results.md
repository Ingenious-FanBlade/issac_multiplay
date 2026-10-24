# tx-02-00：整体 E2E 执行记录

## 最新结果：用户明确要求继续后的实机失败

2026-10-20 21:29（Asia/Shanghai），`tx-02-00-e2e-03`：**整体 FAIL，controller exit 1**。用户在本会话明确要求继续，并先后确认安全新局已准备好、已手动聚焦游戏；这些是本轮再次执行的会话授权，不是自动重跑，也没有改编号隐藏失败。离线 **19,022/19,022 PASS**，17,373 条 corpus 向量。真实 Authority 已启动、B 原生创建成功，随后控制器因 **`protocol trace overflow`** 退出。

| 执行目录 | 实际观察 | 结果 |
| --- | --- | --- |
| `tx-02-00-e2e-01`，21:21 | 未找到 isaac-ng.exe；未启动控制器。原始记录见下文。 | FAIL，实机 NOT_RUN |
| `tx-02-00-e2e-02`，21:25–21:26 | 离线通过；物理输入等待 90 秒超时，controller never launched。截图服务 native pipe 不可用；键盘工具报告 FOREGROUND_DENIED。ready 文件在 21:28 才写入，晚于 21:26:41 的失败结果，不能当作有效输入已开始。 | FAIL，实机 NOT_RUN |
| `tx-02-00-e2e-03`，21:29 | 用户手动聚焦后，4 步初始闭合移动及 64 步连续移动均由工具成功执行；一次注入进入 Authority，日志环溢出后正常停止 helper 返回成功。 | FAIL，实机已开始但未完成 |

第三次使用同一 `tx-02-00-build-ready-02`，批准 unpacked PE，游戏 PID 9900，创建时间 `134369762443717724`，路径为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`。显式 wire v2、IPC v6、lead8、seed131080、delay/jitter1、loss5%、duplicate/reorder15%、outage64..83。没有修改生产代码或再构建。

实机原始文件保存 **1,206** 条事件，最后已落盘 commit 为 tick **20**；协议文件保存连续 sequence **1..312**，最后记录 tick **19**。这是已保存范围，不能外推为退出瞬间的最终真实 tick。控制器退出后，同次执行器确认同一个游戏进程仍存活。尚未到达指定 outage，不能宣称 A 持续推进、B 中性/恢复、全部 edge、逐 commit ACK/STATE 配对或整体实机通过。

控制器主错误为 `protocol trace overflow`。Rust 的 ResidentCleanup 在错误返回时执行了一次正常 `--stop-coop-active`；[helper.log](../../../../artifacts/cs-t0/tx-02-00-e2e-03/capture/helper.log)有初始化和停止两次成功返回。根据 Shutdown 源码，正常成功返回包含停止与原生恢复检查；这是 helper 返回值证据。控制器提前返回，**没有生成 restoration.json，也没有保存 EventKind15 逐槽恢复事件**，故不宣称本次 70 槽逐项产物验收通过。DLL 按设计常驻，未强制卸载、重注入或杀游戏。

统一执行器随后直接打开缺失的 `capture/restoration.json`，导致顶层 [result.json](../../../../artifacts/cs-t0/tx-02-00-e2e-03/result.json)记录 FileNotFoundError、live=STARTED。该次要报表错误不能替代 Native/controller 的实际溢出原因；原始文件均保留。

静态风险定位（未修复、未重测）：`coop_active_ipc.hpp` 的协议环容量为 256；Native trace producer 在 `next-consumed>256` 时增加 overwritten；Rust `drain_protocol` 发现 overwritten 后直接返回错误。消费者逐条写未缓冲 File，直到整个 produced 快照写完才发布 consumed，且与普通事件/loop/world 输出共用控制器主循环。长输出窗口可能导致生产者按旧 consumed 判断已满；具体时序尚未由本次保存范围证明，不能把该推断写成已验证根因。错误路径也未保证先落盘恢复结果再返回，分析器没有处理缺少恢复文件的情况。

### 本轮原始产物

- [第三次统一结果](../../../../artifacts/cs-t0/tx-02-00-e2e-03/result.json)、[控制器实际错误](../../../../artifacts/cs-t0/tx-02-00-e2e-03/controller.stdout.log)和[控制器结果](../../../../artifacts/cs-t0/tx-02-00-e2e-03/capture/result.json)。
- [实际调用/游戏身份/授权/配置](../../../../artifacts/cs-t0/tx-02-00-e2e-03/invocation.json)、[controller 命令](../../../../artifacts/cs-t0/tx-02-00-e2e-03/controller-command.json)、[输入请求及成功结果](../../../../artifacts/cs-t0/tx-02-00-e2e-03/physical-input-tool-result.json)。
- [真实事件](../../../../artifacts/cs-t0/tx-02-00-e2e-03/capture/events.jsonl)、[原始协议包](../../../../artifacts/cs-t0/tx-02-00-e2e-03/capture/protocol.jsonl)、`capture/world/`，以及源码/二进制/构建日志/corpus/离线轨迹副本。
- [第二次等待超时结果](../../../../artifacts/cs-t0/tx-02-00-e2e-02/result.json)和[输入工具拒绝记录](../../../../artifacts/cs-t0/tx-02-00-e2e-02/physical-input-tool-result.json)。

本轮停止测试，不自动修改后重跑。下一步应针对协议日志消费、失败收尾产物和报告错误处理作实质修正，再按新的完整推进安排验收；需要用户准备新鲜进程时先询问。当时T0-08仍为 **IN_PROGRESS**，没有 hash 校验、原版 Online/Isaac Sync 操作或新增 gate。

## 历史第一次执行（原事实保留）

2026-10-20 21:21（Asia/Shanghai）。用户明确要求开始 CST0-08 E2E，统一入口已执行一次，退出码 **1 / FAIL**。离线部分 **19,022/19,022 PASS**，生成 **17,373** 条 corpus 向量；随后枚举游戏进程得到空列表，实机部分 **NOT_RUN**。当时T0-08整体仍为 **IN_PROGRESS**。

实际错误原文：`exactly one prepared game required; observed []`。这表示执行器当时未找到 `isaac-ng.exe`，不能据此判定游戏协议、Hook 或恢复失败，也不能把未运行部分写成通过。

| 清单 | 本次结果 | 依据与范围 |
| --- | --- | --- |
| P01–P07 | PASS（离线范围） | 独立 corpus 与生产 Participant/Endpoint/FaultTransport 生命周期，19,022 项全部通过，场景退出码 0。 |
| L01 | FAIL（未找到进程） | 唯一运行进程列表为空；未继续读取游戏 PE。安全新局、进程新鲜度及冲突模块未检查。 |
| L02–L07 | NOT_RUN | 未启动 controller、未注入 DLL、未聚焦或发送输入，没有本轮实机包、世界采集或 Hook 恢复记录。 |
| L08 | 已记录 | 保留统一失败结果、离线原始记录、源码、二进制、构建日志及本说明。 |

## 调用与产物

只执行了下面这一次入口；没有自动重跑、游戏启动或其它测试：

```powershell
python cs-t0/accept-protocol-e2e.py --build-dir out/cs-t0/tx-02-00-build-ready-02 --artifact-dir artifacts/cs-t0/tx-02-00-e2e-01 --authorization "User explicitly requested starting CST0-08 E2E on 2026-10-20; execute prepared tx-02-00 unified acceptance with bounded physical movement and owned controller injection."
```

- [原始统一结果](../../../../artifacts/cs-t0/tx-02-00-e2e-01/result.json)：FAIL、RuntimeError、live=NOT_RUN、automatic_retry=false、controller_stop_sent=false。
- [离线逐项检查](../../../../artifacts/cs-t0/tx-02-00-e2e-01/offline-checks.jsonl)：19,022 项全部通过。
- [离线原始轨迹](../../../../artifacts/cs-t0/tx-02-00-e2e-01/offline-trace.jsonl)与[离线摘要](../../../../artifacts/cs-t0/tx-02-00-e2e-01/offline-summary.json)：failed=0、game_access=false。
- [离线实际调用](../../../../artifacts/cs-t0/tx-02-00-e2e-01/offline-invocation.json)与[文件来源记录](../../../../artifacts/cs-t0/tx-02-00-e2e-01/provenance.json)：保存实际程序、参数、来源长度及时间。
- 同目录 `corpus/` 保存输入向量；`sources/`、`build/` 保存执行时源码、二进制和构建日志。stdout/stderr 均为空。实机前停止，因此没有 `capture/`、`invocation.json` 或实机分析输出。

统一入口及构建说明仍保留在[实现文档](tx-02-00-implementation.md)，为后续复现提供入口，不表示允许自动重复本次验收。失败不覆盖、不改编号重跑；后续安排须遵守现行规范，并明确本次未完成的真实 Authority 范围。未计算或校验任何哈希，未触及原版 Online/Isaac Sync、socket/Steam、双游戏或 Replica。
