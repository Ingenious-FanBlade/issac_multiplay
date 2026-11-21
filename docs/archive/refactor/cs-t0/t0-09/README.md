# T0-09：双机双进程与网络故障矩阵

tx-03-08（2026-10-21）：T0-09-C 工作设计 **DESIGN_COMPLETE / STATIC_REVIEW_PASS**；生产实现 **NOT_STARTED**、P2 **NOT_RUN**。用户确认 C 的一次统一验收覆盖 F0 10,000 tick 与 F1–F5 各单 seed/5,000 tick；每场由用户重启双端并准备相同基线、runner 保持一次调用；复用普通完整采样，odd-only 明确 gap 且普通样本无遗漏。完整多 seed/C0/C1/C3 仍属 D。见 [C 工作设计](tx-03-08-c-design.md)、[48 项实施前失败方式](tx-03-08-c-failure-modes.md)、[本轮静态审阅](tx-03-08-c-static-review.md)。

tx-03-07（2026-10-21）：Host+VM 双真实游戏 P1/F0 唯一一次实机验收已完成，Orchestrator PASS、Analyzer **38/38 PASS**：起点4553、终点14553，ACK/STATE各10,000，B输入9,989/准入9,981、edge一次，relay零丢失/重复/拒绝，双端正常停止退休、游戏存活，Host catalog **51/51恢复**。原始统一入口因运行中游戏映射 staging DLL、清理删除返回`WinError 5`而机械记为FAIL；用户明确裁定“算正式通过，构建产物我自行删除”。**T0-09-B P1/F0 = FORMAL PASS（用户裁定）；T0-09整体仍IN_PROGRESS**。原始FAIL不篡改、不重跑，见[正式验收记录](tx-03-07-live-f0-formal-acceptance.md)。

tx-03-06（2026-10-21）：生产 AuthorityWireLink 的发布终点锁定已修复，窗口完成后排空但不再接纳远端输入；唯一生产类 + Windows mapping + Remote Endpoint 集成 **73/73 PASS**，10,000 tick 后继续512次调用不扩大发布。当前用户无法准备双机，F0 实机仍待复验，不改变 tx-03-05 的失败结论。见[修复与结果](tx-03-06-publication-boundary.md)。

最新 tx-03-05（2026-10-21）：唯一双机 F0 已执行，**33/38 FAIL**；Host 初始化与 **51/51 槽恢复**、双端正常停止/退休及游戏存活通过，目标14795后继续发布至14819，多24 tick，五个终点条件未通过。无自动重跑，完整 F0/P1 未通过，见[实机结果](tx-03-05-live-f0.md)。下方旧阻塞描述保留对应历史时点。

tx-03-04 更新（2026-10-21）：Native 固定点位与当前脱壳文件一致，镜像准入/精确目标事务/诊断已校正，唯一文件/源码验收 165/165 PASS，相关纯编译通过。当前无游戏进程，未运行内存采集或新的 P1；live-05 的具体失败阶段仍为 UNKNOWN，不能仅凭 Error::Hook 断言 exact-site 拒绝。下文阶段历史按当时记录保留，详见[校正结果](../tx-03-04-hook-calibration-results.md)。

日期：2026-10-20，状态更新：2026-10-21。阶段状态：**DESIGN_COMPLETE / REVIEW_PASSED；T0-09-B 离线 F0 41/41 PASS；tx-03-07 Host+VM 双真实游戏 P1/F0 功能检查 38/38 PASS，并已按用户裁定正式通过；原始统一入口的 staging 清理 FAIL 保留；T0-09 整体 IN_PROGRESS。**

## 当前结论

- T0-08 操作1–9 已通过单真实 Authority + 同进程逻辑 Remote 的唯一整体 E2E（33/33 PASS），因此 T0-09 的协议前置条件已满足。
- T0-08 明确不包含 socket/Steam、双游戏进程、跨进程状态或网络故障矩阵；这些是 T0-09 的新实现范围。
- 2026-10-21 tx-03-07：Host+VM 双真实游戏 P1/F0 已实际完成，Orchestrator PASS、Analyzer 38/38 PASS；用户裁定清理问题不影响功能结论并接受为正式通过。该结论只覆盖 P1/F0。
- 下方“本轮只完成 T0-09 设计”是设计阶段历史陈述，不代表 tx-03-07 后的当前状态；T0-09 完整矩阵仍未完成。

## 已确认设计决策

- Host Windows + 运行于 host 的 Windows VM；两端各运行同 build 真实 `isaac-ng.exe`。
- Host 侧独立 UDP impairment relay；A 经 loopback、B 经 VM 虚拟网络接入；不使用 Steam/原版 Online/Isaac Sync。
- VM 端是完整 observer-simulator：本地重放 A/B 脚本，只有 B canonical input 经 wire 发给 Authority；不应用 snapshot。
- A/B 矩阵输入使用双端确定性脚本输入，共同 canonical bytes 和 tick 表。
- 保留完整 F0–F5 目标，分阶段实现；最终 T0-09 PASS 只由完整矩阵产生。
- VM artifact 通过共享文件夹回流 host；所有 create-new，不自动重跑。
- C2 按现行规范删除；C0/C1 在双机环境新鲜执行，C3 为完整 custom Authority/Remote。
- 不设 gate，不校验任何哈希，不通过原版 Online/Isaac Sync。

## 设计文稿

- [主设计：拓扑、control/data plane、wire IPC、fault relay、observer、状态比较、矩阵与工作分解](tx-03-01-design.md)
- [当前 C 工作设计](tx-03-08-c-design.md)：VM 本地 A/B observer、共享脚本、三种时标、普通采样/odd gap、T09 重复乱序、tick 触发 outage、六场唯一 E2E 与 D 边界；[实施前失败方式](tx-03-08-c-failure-modes.md)、[静态审阅](tx-03-08-c-static-review.md)。
- [实施前失败方式](tx-03-01-failure-modes.md)
- [静态审阅与设计验收](tx-03-01-static-review.md)：设计文稿静态审阅 PASS。
- [T0-09-B 接口与线程边界](tx-03-02-b-interface.md)、[实施前失败方式](tx-03-02-b-preimplementation-failures.md)、[实现说明](tx-03-02-b-implementation.md)：离线双进程 Wire IPC/UDP/control/F0 基础。
- [T0-09-B 离线结果](tx-03-02-b-results.md)：`artifacts/cs-t0/tx-03-02-e2e-09` **41/41 PASS**。
- [tx-03-05 实机失败](tx-03-05-live-f0.md)：历史双机 F0 **33/38 FAIL**，终点后多发布24 tick；原始失败保留。
- [tx-03-06 发布终点修复](tx-03-06-publication-boundary.md)：生产链路/Windows mapping/Remote Endpoint 集成 **73/73 PASS**。
- [tx-03-07 实机正式验收](tx-03-07-live-f0-formal-acceptance.md)：Host+VM 双真实游戏 P1/F0 Analyzer **38/38 PASS**；原始 staging 清理 FAIL 保留；用户裁定正式通过。
- [真实 host+VM P1/F0 接入](tx-03-03-live-p1-integration.md)：Native v7 Authority/Remote、共享启动和 live runner 已实现。
- [首次 live F0 失败 01](tx-03-03-live-failure-01.md)：A Publication、恢复槽数、VM UNC append、relay 窗口修复。
- [第二次 live F0 失败 02](tx-03-03-live-failure-02.md)：A 正常初始化并 51/51 恢复；B init error 已细分。
- [第三次 live F0 失败 03](tx-03-03-live-failure-03.md)：重装后 host 文件变为 9362440 bytes，旧 profile 的 preferred-base 检查拒绝；未运行 VM script。
- [新 build relocated-canonical 实验 profile](tx-03-03-newbuild-experimental-profile.md)：用户要求直接用新版；static/loaded PE 元数据与 approved canonical 一致，但 in-memory ImageBase 被改写为 `0x790000`。新增仅 T09 使用的 experimental profiles；exact hook/site 检查仍为硬失败，结果不自动成为正式 T0-09 PASS。
- [新版实验失败 04](tx-03-03-live-failure-04.md)：profile 3 未覆盖 runtime-rebased header；已新增 canonical-runtime-rebased profile。
- [Steam 脱壳实验失败 05](tx-03-03-live-failure-05.md)：脱壳 image 通过既有 ApprovedUnpacked image check，但 exact hook/site 校验返回 `Error::Hook=3`；只读 SharedV7 mapping 证据已保存。当前脱壳产物不能作为 approved hook baseline，真实 P1 NOT_RUN。

## 分阶段实施状态

| 阶段 | 内容 | 状态 |
| --- | --- | --- |
| T0-09-A | 设计、manifest/control/wire IPC schema 草案、failure modes、静态审阅 | DESIGN_COMPLETE / REVIEW_PASSED |
| T0-09-B | 独立 endpoint、wire IPC v1、UDP relay、control plane、双机 F0 runner | **P1/F0 FORMAL PASS（tx-03-07，用户裁定）**；离线41/41 PASS；Host+VM实机Analyzer38/38 PASS、51/51恢复；原始入口staging清理FAIL保留；旧失败01–05保留 |
| T0-09-C | VM observer-simulator、共享脚本、最小状态比较、F1–F5 | DESIGN_COMPLETE / STATIC_REVIEW_PASS（tx-03-08）；生产实现 NOT_STARTED，P2 NOT_RUN |
| T0-09-D | 双机 C0/C1/C3、完整多 seed 矩阵、最终报告 | NOT_STARTED |

阶段不构成 gate；P1/P2 的专项结果不替代最终 T0-09 完整矩阵 PASS。

## 关键开放问题

- VM 中第二份同 build 游戏、Native/Helper 和 owned virtual B 的稳定性，以及是否产生持久副作用。
- 双端同 seed/角色/room 的可比初始状态；observer 本地 B 与 Authority virtual B 的最小状态可比较性。
- 两端 tick rate/lead 对齐与 control-plane heartbeat 粒度。
- VM 网络本身的 baseline delay/jitter/NAT 行为与配置 impairment 的分离。
- C0 无 Native 的外部只读观察能力，以及 C1 disarmed 的 hook/恢复边界。
- 完整 10,000 tick/5 case/3 seed 双端 artifact 的磁盘预算与流式证据策略。

实现阶段必须在实际观察中收束这些 UNKNOWN；设计阶段不把它们写成已证事实。
