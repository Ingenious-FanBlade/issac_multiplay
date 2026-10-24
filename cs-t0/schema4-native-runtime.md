# T0-04 Schema-4 Native Runtime 模块与安全边界

更新时间：2026-08-27

## 实现状态

Schema-4 active runtime 已按独立模块闭合 Native、V5 bridge、45 槽事务、Helper、Rust Controller 与 verification v2；正式 18+2 evidence matrix、repeatability comparison 和 final gate 也已完成。

- 最终 gate 为 schema 4 `PASS`，`t004Unlocked=true`、`activeInputAllowed=true`。
- 正式 matrix 已登记 primary `18/18`、extra repeatability `2/2`；20/20 根 analysis 均 `PASS` 且 `eligibleForActiveGate=true`。
- repeatability comparison 为 4/4 `PASS`；最终 gate decision 为 `1/1`。
- 当前安装目标 SHA-256 `1FBA9A37...B89C` 已由 create-new equivalence document 精确绑定到 canonical `C4118519...AE30`；Active Native 只接受该 `ApprovedUnpacked` profile。
- 所有 active source 均在逐场景授权、fresh prerequisite/grant/token 与 create-new artifact 约束下完成；失败 source 未进入正式 matrix，历史结论不覆盖。

历史 `IsaacCsT0-ActiveVerification-v1` 原样保留 `runtimeContractComplete=false`。create-new `IsaacCsT0-ActiveVerification-v2` 表达自动化 runtime 完成；其 prerequisite 文件按职责仍固定 `activeInputAllowed=false`，不能单独授权 active。最终授权只由绑定 verification、18+2 matrix 与 repeatability 的 schema-4 gate decision 给出。

正式包中的 clean/full verification 为 `out/cs-t0/formal-active-gate-20260827-r1/verification-v2.json`，SHA-256 `2ABBA008B319C973E8424C740BCCE00CE6D9002B913B7ABB3C2DA1C790243317`；27/27 自动子门通过。Rust 为 255/255、cross-language 1/1、baseline Native Debug/Release 各 39/39、Active Native Debug/Release-LTCG 各 262/262、MCP 16/16。matrix、repeatability 与 final decision SHA-256 分别为 `54BDD4F7E0070D14DA913C6993C97272D97A91B193BDA5C04E2FCAE3E9C6889D`、`AFA254C0E06BAFC53728F9D4CCCF42C1999B9B5CD68533449DBF2E92B57775CC`、`927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C`。

`live-32 loading` 的原始 analysis 保持 20/21 `FAIL`；create-new 重分析与独立 replay 在修复后均为 21/21 `PASS`，没有覆盖原始 manifest/events/analysis。`live-34 room` 的重复标签 FAIL 保持不变；修复后的 `live-35 room` 为 Active 21/21、companion 18/18 `PASS`。`live-36 menu`、`live-37 loopback`、`live-38 control-no-native`、`live-41 controller`、`live-42 device-switch` 与 `live-46 original-online-disarmed` 的通过结论和历史 FAIL provenance 均保持不变。20 份正式 replay 中 17 份 analysis SHA 完全一致；`keyboard`、`simultaneous`、`rapid-tap` 只迁移 companion-loop-context 的报告说明文字，所有语义与 trace 字段不变。runtime 当前停在安全未加载状态。

## 功能模块

| 模块 | Native / Rust 入口 | 职责与边界 |
| --- | --- | --- |
| Active IPC/Contract | `native/include/isaac_cs_t0/active_ipc.hpp`, `native/include/isaac_cs_t0/active_identity.hpp`, `src/active_ipc.rs` | 固定 PID/creation、canonical/observed/equivalence、prerequisite/gate/catalog/plan/script identity、`sessionId[16]`、288B event、events offset 720、18,875,088B shared block，以及场景、阶段、query、consumer、lifecycle、witness、error codebook。loop mapping name 容量为 96 UTF-16 单元，生产最坏只需 52；未知值、零摘要、错位旧 mapping 全部 fail closed。 |
| V5 Active Bridge | `native/include/isaac_cs_t0/loop_runtime.hpp`, `native/src/loop_runtime.cpp` | 通过 `EmbeddedObserverHooks` 提供 physical-exit、authority pre/post、top-update-exit、lifecycle 与 companion health；不增加第 46 槽，空回调保持 V5 行为不变。 |
| Active Targets | `native/include/isaac_cs_t0/active_targets.hpp`, `native/src/active_targets.cpp` | 验证六个 consumer REL32 与七个 image pointer 的 expected bytes、target、COL、owner 和 ABI，并在恢复后逐字节复核。 |
| 45 槽事务 | `native/src/active_runtime.cpp`, `native/src/hooks.cpp` | 以 suspended-thread transaction 一次安装 V5 REL32 27 → Active REL32 6 → V5 pointer 5 → Active pointer 7；失败逆序 rollback，不确定状态锁存 Unsafe/result 8。 |
| Dispatcher | `native/include/isaac_cs_t0/active_dispatcher.hpp`, `native/src/active_dispatcher.cpp` | physical projection → canonicalize → edge plan → loopback transport → decode/accept/resolve；实现五个公开 reject、duplicate/conflict 和 authority tick 提交规则。 |
| Synthetic Adapter | `native/include/isaac_cs_t0/active_apply.hpp`, `native/src/active_apply.cpp` | authority pre 发布不可变 snapshot、post 清除；logical wrapper original exactly-once，且只在完整 eligibility 下 suppression/override。它不调用 setter，也不写 Player/device/cache/world。 |
| Player Witness | `native/include/isaac_cs_t0/active_witness.hpp`, `native/src/active_witness.cpp` | TLS 绑定瞬时 Player owner/consumer family，生成 32B before/after witness；不把指针写入 IPC，跨 family 或 owner ABA 锁存 Unsafe。 |
| Runtime FSM/C ABI | `native/include/isaac_cs_t0/active_runtime.hpp`, `native/src/active_runtime.cpp`, `native/src/dllmain.cpp` | 提供 `IsaacCsT0InputActiveInitialize` / `IsaacCsT0InputActiveShutdown`；管理 generation、publication、retirement、drain、分段恢复和 Unsafe 禁止卸载。 |
| Helper | `native/include/isaac_cs_t0/helper_operations.hpp`, `native/src/helper.cpp` | 使用精确操作表分派 `--inject-input-active` / `--unload-input-active`；不使用字符串包含匹配，result 8 保持 DLL 加载。 |
| Controller/Artifact | `src/active_commands.rs`, `src/active_live.rs`, `src/active_artifact.rs` | 按 static → preflight → offline continuity → grant → verification v2 → CAS → create-new run → mappings → Helper 顺序编排；CAS 后失败写 FAILED artifact，Unsafe 禁止自动 unload。 |
| Original Online disarmed collector | `src/active_commands.rs`, `src/input_rpm.rs`, `src/active_artifact.rs` | 仅用 read-only process observer 重放 approved-unpacked/static bundle，两次稳定读取 game root、manager、Steam backend、2–4 peer roster 与 online frame，并复核 45 槽不变。布局与原 IsaacSync 和 Schema-4 Native 交叉核对；不创建 IPC，不加载 Native/Helper，不保存原始 peer identity。 |
| Automated Verification | `native/tests/active_main.cpp`, `src/active_verification.rs`, `verify-active.ps1` | Active Native Debug 与 Release/LTCG 各固定 262 case，并绑定二进制 SHA；同时运行 baseline Native 39、Rust、MCP 16 和 analyzer fixtures，create-new 生成 verification v2。 |

## 关键运行时不变量

1. Controller 在 CAS 后创建 mapping 时写入真实 process creation time，以及 canonical `C411...AE30`、observed `1FBA...B89C`、equivalence `8B3F...F138`、raw prerequisite `99BA...D5AC` 和全部冻结 gate/catalog/plan/script 摘要。Native 在安装任何 hook 前逐项比较，并以 `GetProcessTimes` 复核 creation；目标 PE 只允许 `Profile::ApprovedUnpacked`。
2. `authorityTick` 的 pre candidate 为 `lastCompleted+1`，只在 authority post 正常返回后提交；溢出永久退休。
3. `applyGeneration` 在一次 Native load 内固定；Player/device/lifecycle epoch 改变时递增 `ownerGeneration`，并同步清 transport、queue、planner 与 snapshot。room owner/lifecycle guard 使用 `Lifecycle::None` generic clear 保持该代际语义，只有实际 `NewRoom` enter/exit 发布场景级 `ROOM_ENTER/EXIT`。
4. physical projection 必须先由 Native `PlayerUpdate` wrapper 绑定唯一 Player 和 selected-device key；未绑定首帧只跳过，不读取 device、不更新 history、不创建 command。绑定后任何 key、双快照、projection 或完整 valid-mask 不匹配仍 fail closed。六个 logical query 与六个 consumer wrapper 始终调用 original 恰好一次；retired、Unsafe、错误线程、非 steady-standard、错误 generation/owner/device/family 或 action `12+` 只能 passthrough。
5. 五个公开 reject 仅为 queue-full、expired、too-far、stale、identity-conflict；其它协议错误发布 terminal Failure/ClientRetired，不冒充 reject。
6. 正常停止顺序固定为 publication/override off → drain all admitted embedded active callbacks → retire/neutralize → Active 13 restore → secondary callback drain/target verify → V5 32 restore/drain → 45 槽 byte equality → ProducerClosed/Stopped → unload。5 秒排空超时进入 Unsafe，绝不越过 drain 退休客户端或卸载。
7. 任一 restore、drain、owner、thread、ring、companion health 或 slot 复核不确定都进入 Unsafe；Controller 只保存诊断，不自动卸载或重试。

## 正式工作完成与后续边界

原先缺少的 `original-online-disarmed`、两份 extra loopback、18+2 replay、matrix v2、comparison 和 final `input-gate` 均已完成；正式结果见文首和 [T0-04 逆向交接](../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md)。T0-04 不再有待补 source 或登记步骤。

后续 T0-05 若新增 local co-op/virtual-device owner、slot 或 ABI，必须创建新的 T0-05 ledger/catalog/verification，并纳入单一 patch/restore 事务。不得修改 T0-04 catalog、复用旧 arm/token/session，或把 T0-04 的单 Player ownership 外推成双玩家 mapping。
