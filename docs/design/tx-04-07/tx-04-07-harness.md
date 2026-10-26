# tx-04-07 第 3.4 节 harness 编写记录

> **当前状态：tx-04-07 完成，e2e-11 整体验收通过（用户正式接受，D31，2026-10-26）**。用户明确将本次 E2E 视为全部通过；原自动 FAIL、实际停止/未运行与 V12 PASS 事实保留，不再补测。见[验收记录](../../artifacts/repentogon/tx-04-07-e2e-11/acceptance.md)。下文为各时点历史记录。

> e2e-08 单启动执行到 S20，Continue 因 A 死亡而不可选；第二轮收尾及前三轮严格网络对账通过，V12 有正对照与提前发射/输入空档证据，另发现 loop 快照偏移。见[结果](../../artifacts/repentogon/tx-04-07-e2e-08/README.md)。用户要求下一次开局输入 debug 10，已新增一次性控制台步骤并纯构建，尚未实测，见[准备](../../artifacts/repentogon/tx-04-07-debug10-preparation/README.md)。

> 2026-10-26 中继/V12 修复已编写并统一纯构建：收尾 10054/10052 单列通知，native 有界真实收包；S15 增加同角色普通松键对照、180 tick 按住和独立 laser-evidence。原始缺输入仍按真实失败记录，没有新 E2E，见[说明与限制](tx-04-07-relay-v12-fix.md)。

> 最新e2e-06（2026-10-26）：**FAIL**，S6等待回归修复、V9通过，S0–S15已执行；第二轮Bye收尾的UDP relay receive报10054，后续未执行。角色7的Index=46/player_index=1及动画读取正常，V12仍缺所需激光证据。正常退出、无新dump、已清理，无自动重跑。见[结果](tx-04-07-results.md)与[修复](tx-04-07-pause-wait-fix.md)。

> 最新 e2e-05（2026-10-26）**FAIL**：焦点维护修改后，S6 主动退出暂停的 stable 等待被新增 Active 检查过早终止，属于 harness 回归。无失焦/恢复样本；游戏正常退出、无新 dump、已清理、无自动重跑。前一轮 e2e-04 的身份/loop/投弹修复使 S4 移动、skip 与 V8 通过对应检查，本轮再次观察到这些结果。见[结果](tx-04-07-results.md)与[修改说明](tx-04-07-focus-fix.md)。

> 退休调用实质修复后第三次 E2E（2026-10-25）：**FAIL**。S13 Stop 正常退休、无新崩溃，S0–S14 完成；S15 前因夹具将实体 Index=17 当作 ordinal=1 的否定证据，缺失 B 动画字段而超时。未自动重跑或修改旧结果。见[修复](tx-04-07-retirement-fix.md)与[结果](tx-04-07-results.md)；下方保持原时点。

> 后续实质修复后的第二次 E2E（2026-10-25）：**FAIL**，启动 trace 丢失未重现；S0–S12 完成，S13 Stop 退休时崩溃。V11 PASS，其余实际结论见[结果](tx-04-07-results.md)。本次一次启动、未自动重跑，原失败与编码记录保留。修复见[说明](tx-04-07-trace-fix.md)。

> 编写后唯一整体 E2E 已执行（2026-10-25，America/Los_Angeles），结果 **FAIL**：首轮开局发现既有 trace 丢失，尚未 Join；未重跑。见[结果](tx-04-07-results.md)。以下“尚未测试”是本次编码交付时点的记录。

日期：2026-10-26。本次完成第 3.4 节代码，属于同一次 tx-04-07 推进。没有部署、启动游戏、运行 harness、单元测试或任何阶段测试；V1–V16 未验收。统一纯编译链接的实际结果在本文末尾记录，唯一整体 E2E 仍待单独安排。

依据：[实现方案](tx-04-07-implementation-plan.md)、[工程细化](tx-04-07-engineering-changes.md)、[状态与 trace 接口](tx-04-07-state-trace.md)及[现行规范](../refactor/project-conventions.md)。保留现有工作区的 3.1–3.3 实现与历史记录。

## 入口与编排

`tools/harness/src/main.rs` 接入 `tx-04-07`，复用现有部署、第二档、新局、临时 Download Helper 禁用、控制口、失败现场、清理与存档首尾快照。新增 `tx07.rs` 与 `scenarios/tx-04-07.json`。

唯一 E2E 入口（仅说明用法，本次没有执行）：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game <game-root> --out <new-artifact-directory>
```

一次启动四轮 Host，S0–S21 顺序执行，结束原因依次 Stop、PeerBye、PeerTimeout、RunTeardown。Continue 按实际菜单 ID 与选中项导航，进入游戏菜单后选择 Continue(1)，不走新局角色确认链。部署生成 tx07 Lua token、端口、动作序列与预先固定的超时。

F8 动作按配置 step 匹配唯一 request/终态，顺序为 skip、room、kill_b、revive_b、stage、room。失败不补按、不重试；后续场景保留 NOT_RUN。死亡期间 A 移动、远端持续发送，保持至少 8 个完整 Authority tick；复活后先观察移动，等待实际动画结束后才观察射击。S15 使用阿萨泻勒与 clean relay，切房前预置下一代次源，实际松开点另存检查点。

Lua 的必要只读配对改动：Player 视图增加逐点检查后的 `movement_options`；run-exit 记录 should_save 与保存请求时的人数，run-started 记录实际 continued 与加载后、Join 前的人数和 controller。保存请求不等同于磁盘结果，编码存档中的记录数仍单列 UNKNOWN。

## relay 与远端

`relay/mod.rs` 每轮两个 loopback socket、两个独立 xorshift64-v1 随机源、有界控制通道与发送队列。远端 socket 绑定后、Hello 发送前，经控制消息登记确切地址；不由未知首包抢占。主机 socket 固定连接本轮主机。

clean、moderate、outage_short、handshake_loss、outage_long 均按方案参数实现；预先检查场景的档位、容量、种子、端口与时限。切换回复含实际生效的控制序号、profile_epoch 和 relay 单调时间。已经排定的副本保留原决定，切 clean 后等待有界 drain。长断网保留至双方实际超时和本轮收尾，主机的三份 Bye 继续丢弃。

每次接收以方向内 occurrence 编号，同序号 Hello/Bye 重发不合并。保留原始 bytes、codec 解码、四个随机值、决定、计划时刻，以及每份 copy 的实际发送结果。收尾包含 received/policy_drop/copies_scheduled/send_ok/send_error/pending/shutdown_drop；失败不重新抽签。

`remote/mod.rs` 补齐每个新可用代次的 +1 启动样本、+2 连续量补偿（edge=0）、ArmNextGeneration 一次性源及失效清理。旧代次样本使用新 packet/command sequence 和本 session 真正收到的旧 Ack；归约必须命中 ack_known 的 old_generation 丢弃，ack_unknown 或 stale 不算通过。

S18 可显式期望 peer_timeout；保留真实 error、terminal_cause、最后合法接收以来的实际时长，握手超时/协议/IO 错误仍失败。正常 Bye 接收与主动 Bye 发出后各保留 100 ms 的有界收包窗口。DLL WorkerStatus 配对增加只读 `terminal_at_ms`，在网络线程发布终态前记录；net stop 同时输出该时刻与 last_receive_ms，超时判定不用主线程稍后轮询时刻。

## 归约与产物

`tx07/evidence.rs` 独立归约 V1–V16，不复用 tx06 的旧零写入断言。清单数量来自本次生成 manifest；session generation 与 participant generation 分开；非中性 Resolve 对齐 Authority-pre 的 input_generation、Accepted、sequence 与 target。

新增主要产物：

| 文件 | 内容 |
| --- | --- |
| relay-N.jsonl / remote-N.jsonl | 原包、副本/故障决定、样本/换代/终态 |
| network-ledger.json | 两方向三方最终计数、逐 occurrence/copy 对账与真实差额 |
| fault-evidence.json | 实际丢包/重复/乱序命中、clean/fault DLL 时间轴推进率与固定 0.8–1.2 容差 |
| life-evidence.json | ghost、Lost 完整 tick、Invalidate→Bind→Ack→fresh、旧代次探针及同一个 K |
| loading-query-evidence.json | S12/S20 owner、无 tick/Submit/Resolve、生命周期变更归属、逐键覆盖差值 |
| retirement-evidence.json / tick-integrity.json | 当前关联退休、常驻 K、网络线程停止、双 ordinal/owner/tick 完整性 |
| file-notifications.jsonl | 原始目录通知，含真实路径/类型/时间、监听错误或溢出 |
| file-windows/Sxx.before、after | 保存根的只读窗口快照、存在/长度/时间、采样区间与 possibly_writing |
| file-window-changes.json / persistent-evidence.json | 窗口字节变化、D18/D20/D27、入口返回与跨 loop 窗口关联、未归属/UNKNOWN |

文件监听采用 ReadDirectoryChangesW/OVERLAPPED，每个冻结根独立线程、64 KiB 缓冲与事件上限；退出先取消并等待在途 IO，再释放句柄/缓冲。Documents 子树覆盖已发现的 REPENTOGON sidecar，另监听实际 Steam remote 根。窗口与首尾均复制原始文件，不做哈希、不翻 readonly、不回滚、不扩大 achievements JSON 等许可范围。窗口中第一/三档继续按历史关闭配置样本的 Counter/Checksum 字段范围判断；不因首尾净变化相同而略过窗口变化。原始窗口存档在 .gitignore 中排除，仅元数据可进入版本管理。

报告区分 PASS、FAIL、NOT_OBSERVED、NOT_RUN。S7 实际故障未命中、S17 没有自然 Hello 重发/First Ack 副本、加载无自然受控查询、动作/查询类型无自然重复时，均不能冒记 PASS。加载的 432 个分类键逐项保留零覆盖；不足时 V13 保留 NOT_OBSERVED，同时独立列出已经观察的不变量结果。缺观察不自动延长或重跑。

文件通知不证明调用者或写入成功，最终相同字节不能排除中间写回；ESSM binary/ex JSON、VirtualRoom、ItemPool、achievements 与云端提交结果按可见边界逐项记录。未知间接闭包不变成新的推进前置条件。

## 编写后的纯构建

Win32/v143/Release DLL 与 i686 Rust harness 已统一纯编译链接成功，最终日志见 [build-04.log](../../artifacts/repentogon/tx-04-07-harness/build-04.log)。Rust 最终无 warning；完整 DLL 构建中的 SDK/RapidJSON、生成 wrapper 与队列对齐警告保留在 build-02。第一次因沙箱拒绝 SDK 目录读取而失败，原始日志未覆盖，见[构建记录](../../artifacts/repentogon/tx-04-07-harness/README.md)。

纯构建只生成 DLL、harness 和 Hook 清单，没有部署或运行程序/测试；不代表 V1–V16 通过。实机验收前仍须按现行规范告知用户操作与接管时机，需要手动配合时先确认已准备好。
