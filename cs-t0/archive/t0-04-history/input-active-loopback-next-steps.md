# T0-04 Active Loopback 实施与完成记录

更新时间：2026-08-27

## 当前执行快照

最后一个 primary source `original-online-disarmed live-46` 完成后，20 份 source 已执行全量 provenance replay，并创建正式 18+2 matrix、repeatability comparison 与 final decision。T0-04 正式状态现为 `PASS`：`t004Unlocked=true`、`activeInputAllowed=true`，证据阶段已经闭合。

- Schema-4 Native runtime、active Helper、45 槽事务、offline attestation v2、Controller token CAS、18 场景 analyzer 与 verification v2 均已实现。
- 正式包位于 `out/cs-t0/formal-active-gate-20260827-r1`。`active-matrix-v2.json` 已登记 primary `18/18`、extra repeatability `2/2`，SHA-256 `54BDD4F7E0070D14DA913C6993C97272D97A91B193BDA5C04E2FCAE3E9C6889D`。
- `repeatability.json` 为 4/4 `PASS`、`eligibleForActiveGate=true`，SHA-256 `AFA254C0E06BAFC53728F9D4CCCF42C1999B9B5CD68533449DBF2E92B57775CC`。
- `verification-v2.json` 为 27/27，SHA-256 `2ABBA008B319C973E8424C740BCCE00CE6D9002B913B7ABB3C2DA1C790243317`；Rust 255/255、cross-language 1/1、baseline Native Debug/Release 各 39/39、Active Native Debug/Release-LTCG 各 262/262、MCP 16/16。该 prerequisite 文件按设计仍携带 `activeInputAllowed=false`，最终授权由 gate decision 给出。
- `active-gate-decision.json` 为 schema 4 `PASS`，SHA-256 `927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C`；18 个 analysis binding、2 个 repeatability binding、零 missing case，`semanticContractComplete=true`、`runtimeContractComplete=true`、`t004Unlocked=true`、`activeInputAllowed=true`。
- 20/20 当前 analyzer 重放全部 `PASS` 且 `eligibleForActiveGate=true`。其中 17 份 analysis SHA 逐字节一致；`keyboard`、`simultaneous`、`rapid-tap` 仅迁移 `companion-loop-context` 的说明文字，所有语义字段、trace hash、check ID/结果、manifest 与 events 均不变。原 analysis 未覆盖，完整记录见正式包内 `PROVENANCE.md`。
- `loading live-32` 的原始 analysis 保持 20/21 `FAIL`；修复后的 create-new 重分析与独立 exact replay 均为 21/21 `PASS`，analysis SHA-256 `22BA86D061B816DD45B5B0476B98F407BEFB4690F27F965CFE4E9F8D2B39E70E`。正式登记必须引用当前语义重放结果，不得覆盖原 analysis。
- `room live-33` 没有产生 room-change transaction，不能登记。该运行仍完成 Active `0x1FFF/0x1FFF`、45/45 槽和 before/after 字节安全恢复，未观察到 Unsafe；其失败原因是缺少场景证据，不是恢复失败。
- `room live-34` 由用户手动穿过普通门，companion V5 捕获唯一完整 room-change transaction 并 18/18 `PASS`；Active runtime 却把 9 次 owner attempt 与 1 次嵌套 NewRoom 都发布为 `ROOM_ENTER/EXIT`，共 10 对，因而违反冻结 analyzer 的唯一派生标签要求。主 artifact 与 `.finalize-failed` 诊断均保留，45/45 槽完整恢复、零 overwrite、无模块/进程残留；该运行不能登记为 source。
- 修复后 owner-only room guard 使用既有 `Lifecycle::None` generic clear 继续推进 owner generation，只有 `NewRoom` 发布 `ROOM_ENTER/EXIT`；schema-4 数字码本、共享内存 ABI 和冻结 analyzer 契约均未改变。回归同时验证多个 owner-only attempt 合法、第二对 Active 标签 fail closed，以及 generic clear 的代际取消语义。
- `room live-35` 实机验证修复成立：Active 47,272 条 event、恰好一对 room 标签、18 个 generic clear，21/21 `PASS`；companion V5 72,081 条 event、9 次 owner attempt、唯一完整 NewRoom/simulation transaction，18/18 `PASS`。45/45 槽逐字节恢复，零 overwrite、无 runtime 残留；独立 QA exact replay 得到相同 analysis SHA `DA0DFFAD19A6745238E386166EF91B572F398D6114872853C42F9706EAA292CB`，该 source 可进入后续正式 matrix replay。
- `menu live-36` 实机由用户手动完成：Active 121,832 条 event、唯一 `RUN_TEARDOWN` 后立即 `CLIENT_RETIRED`，21/21 `PASS`；companion V5 180,170 条 event、同 loop 唯一 `RUN_TEARDOWN_ENTER/EXIT`，18/18 `PASS`。45/45 槽逐字节恢复，零 overwrite、无 runtime 残留；独立 QA exact replay 得到相同 analysis SHA `F9C95A9D5BB2764B36A5EFDC35C59FBEB9BFD3BA158BFA96B7D831E69070F4F6`。
- `loopback live-37` 捕获三组 applied-neutral handoff：tick `4→5`、`12→13`、`20→21` 均先以 `original=1.0/effective=0` 屏蔽物理 movement，再由预先 accepted 的 pending non-neutral command 在下一目标 tick apply 并得到 `effective=1.0`。原 20/21 FAIL analysis 保留；严格 handoff 修复要求 accept/apply/query 顺序、同 action/device/consumer/generation/result，正反回归与 Release exact replay 为 21/21 `PASS`，analysis SHA `D4D6B02A37BD0FDFE56A9EDA8EA9BE29F701ABDE59BE7FAC824BFFD202790CAF`。45/45 槽恢复、零 overwrite、无 runtime 残留。
- `control-no-native live-38` 只有 1 条 `control-observation` event，accept/reject/apply 全为 0，installed/restored mask 均为 `0x0`；未创建 companion、IPC session、mapping、grant、claim、arm token 或输入脚本。源 manifest/events/analysis SHA-256 分别为 `DC7589DC6DDF5F1F499BB120BFA80753C722DAC59B4567A234BF41531A42711B`、`81EE020DB08089155E9BB909150521FC3687B7C58CAF265F02DB7CFBD62C6AD0`、`E701AF3F73577B37AA1D18429B69502FC13AC1B166CDE323A9D26DE4C9DBDFF6`；独立 exact replay 仍为 21/21 `PASS`，游戏继续响应且无模块/进程残留。
- `controller live-39` 保留为不可覆盖的 `FAILED` artifact：末尾 sequence 60855 为 `CLIENT_RETIRED`，60856 为 error 12 `PROTOCOL_VIOLATION`；失败 manifest 为 installed `0x1FFF`、restored `0x0`、0 slot、ProducerClosed false、ClientRetired true。Active manifest/events SHA-256 为 `8A966B3E4690ED99D4F540148A6E353F31799B3276D4B60581EC72D890059908`、`3B5097FD05132D425B0EC79583C89A1E8D6CBAB9029FD32BBC308B8F4815D11A`，companion manifest/events 为 `26F2871E3CF9D0B1A49D4B482B02BB2EE3366B1F78F4552DAC04695556C259CF`、`13A1BDDF6BDB778965A825BFBA0DF75D2830DC6CCBF30CBD101733E2DADB19E0`。正常游戏退出后未留进程级残留；该运行不计入候选。
- 修复使 physical exit、authority、top-update exit、lifecycle 四个 embedded callback 统一参与 `HookScope` 计数，并将停止顺序固定为关闭准入 → 排空已准入 callback → 退休 dispatcher/adapter → 恢复 Active hooks → 二次排空/target verify → 停止 V5 → 核对 45 槽。排空超时仍 fail closed；确定性 Native 回归证明 retirement 不得越过 drain。
- `controller live-40` 的 prerequisite/grant/claim 均 create-new 且一次性 token 已消费；Active 只发布 `INITIALIZED`、`HOOKS_INSTALLED`、`PROTOCOL_VIOLATION` 三条事件，没有到达人工操作、canonical/accept/apply。Active manifest/events SHA-256 为 `44692D689EE7AACFCD25DE0EACC434EB75FD27130E1A58295943E5DC2D1371FF`、`583C64F211BE1A44EAC44510C343174399A2C167CAB6BF790849587B51938B2D`，companion manifest/events 为 `FF968D01E9328131394C21D6B7AFEE63288F16DB80FEAB492D7D781C17D18FF2`、`180C1200DAF046BDA6E0BA232A60C89596AE70F10EB53B6C210C5783CB176C0F`。QPC/事件顺序将故障限定在首个 `PhysicalExit → BuildRawSample`，但现有 codebook 无法区分快照不一致、投影非 Ok 与 valid mask 不完整；不得猜测子原因。用户正常退出后无进程级残留。
- 修复统一要求 Native owner binding 先于 projection：unbound callback 直接返回，不读取 device state、不更新 history、不创建 command；bound 后仍核对 selected key 并执行原有严格 `BuildRawSample`。全部 16 个 Active scenario 的正反回归、Debug/Release-LTCG 和 verification v2 均通过。当前 Release Controller/Native/Helper SHA-256 分别为 `44DB89923CC2DFC7461C2026B6B4426F4ACE15847B07181A712DA159FFE14C16`、`B74E95688E1CD4F893D47AEECD06A2E5DD50FCA381F8A4672974DC6BECD729D7`、`FC3B862C180501638A956BBD6781CE8BF2F2AA3A6971C1D1BBBFBED6553E7E06`。
- `controller live-41` 使用 fresh prerequisite/grant/token，用户仅用手柄手动完成移动和射击。Active 157,594 条 event、5,149 accept、0 reject、5,070 apply，21/21 `PASS`；companion V5 18/18 `PASS`。Active `0x1FFF/0x1FFF`、45/45 槽与 companion 32 槽安全恢复，零 overwrite、ProducerClosed、ClientRetired；独立 `out/cs-t0/qa/live-41-controller-exact-replay-20260826` replay 的 analysis SHA 与 source 同为 `1889A038ACCC590A47359C6873093B766CD610F5F656DA8102882D9F08B72FBD`。Controller/Helper 无残留，游戏继续运行。
- `device-switch live-42` 使用 fresh prerequisite/grant/token。Active 197,229 条 event、6,425 accept、0 reject、6,348 apply；唯一 `OWNER_SWITCH` 前 6,249 次 apply 全为 Gamepad，切换后 99 次全为 Keyboard 且使用新 generation 4。Active 21/21、companion V5 18/18 `PASS`，Active `0x1FFF/0x1FFF`、45/45 槽与 companion 32 槽安全恢复，零 overwrite、ProducerClosed、ClientRetired；独立 `out/cs-t0/qa/live-42-device-switch-exact-replay-20260826` replay 的 analysis SHA 与 source 同为 `CF3E07489E004D69445D71E3ABCB3B28D2CF83425B29988CE0B57A775F5725E0`。Controller/Helper 无残留，游戏继续运行。
- `original-online-disarmed live-46` 位于 `artifacts/cs-t0/20260826T161503Z-t0-04-input-disarmed-original-online-disarmed-t0-04-original-online-disarmed-live-46`。两次只读 positive observation 在相同 game root/manager/Steam backend/2 人 distinct roster 下取得 frame `6052→6056`；未创建 arm/token、IPC、Native、Helper、suppression 或 synthetic input，installed/restored mask 均为 `0x0`，45/45 槽完整不变。source manifest/events/analysis SHA-256 分别为 `56F38929626542E96C80D10BA2C5901E97B124466536FB64FCA570323F2E8B6B`、`4606D8A18BE4828F6A6BCE4B093EDCE0A6624CBB1FC581C428209F3D152E5D10`、`58DAE197680F5ECF01F26FBBFE196804ABF257C70D97C52D2E72DCFB7BCE72DD`；独立 `out/cs-t0/qa/live-46-original-online-disarmed-exact-replay-20260827` 三文件哈希完全相同，21/21 `PASS`、`eligibleForActiveGate=true`。Controller/Helper 无残留，游戏 PID 26960 继续响应。
- 第一轮 extra A 重试 `live-43-r1` 保留为 20/21 `FAIL`：25 个采样帧均为 neutral，说明授权后窗口焦点未回到游戏，冻结脚本没有形成输入状态切换；通用不变量、Active `0x1FFF/0x1FFF`、45/45 槽恢复、零 overwrite 与进程清理均通过。该 source 不得登记，也不自动改判。
- 新鲜 `loopback live-44` 使用 60 callback prerequisite、一次性 grant/token 与 10 秒前台切换窗口。Active 781 条 event、31 accept、0 reject、23 apply，21/21 `PASS`、`eligibleForActiveGate=true`；semantic/canonical/decision/apply trace SHA-256 分别为 `B90ED34DE85F510F1BED83C443B5F9039AB383C6787EA6D0FADC19EBC5CBAD34`、`151E29A874C702727F457A7D5CB73827338B70B20FB319BAE057153199C57E12`、`19E478C10290D9CF227754541BEE1CF4E678718517E714582C634952BC27A3F3`、`8F92050F9A1BF4CAEEE4B8B1445B8330B656FF6B9D626C1C5866528EACFD9CE6`。Active `0x1FFF/0x1FFF`、45/45 槽与 target bytes 完整恢复，双方零 overwrite、ProducerClosed、ClientRetired；独立 `out/cs-t0/qa/live-44-loopback-extra-a-exact-replay-20260826` 的 analysis SHA 与 source 同为 `E3A4F5B30E0E40D8DB012AEB5B5A07ABEBD42637E59E5B7DD776F02F1E195142`。Controller/Helper/Native 无残留，游戏继续响应。
- 第二份 `loopback live-45` 同样使用 fresh 60 callback prerequisite 与一次性 grant/token。Active 776 条 event、30 accept、0 reject、23 apply，21/21 `PASS`、`eligibleForActiveGate=true`；semantic/canonical/decision/apply trace SHA-256 分别为 `71B156BFBEE4F0B8EA6DB1EE435A7E819F89EBB7F446C1943B0149BE3783812D`、`B19242AEF4B2A5BF68E7B8F77669F0D73091E6DB8A569B758E47802DA602458E`、`8C4DFF750463BA79DA2E204A6CFCF1ABAD282529526C8703DFB13A9081ED5C21`、`8F92050F9A1BF4CAEEE4B8B1445B8330B656FF6B9D626C1C5866528EACFD9CE6`。Active `0x1FFF/0x1FFF`、45/45 槽与 target bytes 完整恢复，双方零 overwrite、ProducerClosed、ClientRetired；独立 `out/cs-t0/qa/live-45-loopback-extra-b-exact-replay-20260826` 的 analysis SHA 与 source 同为 `06A94C29983A91719DA4374A7534E06CF62C5F08802F66AC1B814408B29D71C2`。Controller/Helper/Native 无残留，游戏继续响应。
- 首次 create-new comparison `out/cs-t0/live-comparisons/live-44-live-45-loopback-extra-repeatability-20260826.json` 保留为历史 `FAIL`，SHA-256 `5A88A47F2C39B5FE4C07148981F2E898E43AE21F68FE4F24AB90EF7B4183D6A7`。`independent-runs`、`repeatability-scope`、`source-analysis-pass` 均通过，仅 `semantic-trace-equal` 失败。事件级差异只发生在归一化 ordinal 775：两边前 774 项完全一致，`live-44` 在 authority tick 25 后额外记录一个 target tick 27、未 resolve/apply 的 neutral command 五阶段尾巴，随后 lifecycle 与 `live-45` 一致；两份 apply trace 完全相同。该旧输出不覆盖、不改判。
- comparator 修复只允许剥离紧邻最终 `HOOKS_RESTORED → PRODUCER_CLOSED` 的严格五阶段 neutral command 尾巴，并验证连续序号、相同 session/client/generation/source/action、零 edge、next-loop、target tick、最后一次 neutral apply 以及不存在 resolve/apply；任一语义字段变化仍必须失败。正反回归均已加入。新 comparison `out/cs-t0/live-comparisons/live-44-live-45-loopback-extra-repeatability-tail-normalized-20260826.json` 为 4/4 `PASS`、`eligibleForActiveGate=true`，SHA-256 `AFA254C0E06BAFC53728F9D4CCCF42C1999B9B5CD68533449DBF2E92B57775CC`。

剩余新 source 测试为 **0**；primary、extra source 与 pair comparison 均已闭合：

| 类别 | 当前状态 |
| --- | --- |
| primary | 正式登记 `18/18` |
| extra repeatability | 正式登记 source `2/2`；pair comparison `1/1` |
| final decision | `1/1 PASS`；`activeInputAllowed=true` |

Active lifecycle、loopback applied-neutral handoff、Native/Rust 回归门、verification v2、全部 18 个 primary、extra A/B `live-44`/`live-45` source、repeatability comparison、正式 matrix 与 final `input-gate` 均已完成；历史 FAIL 保留且不覆盖。T0-04 不再有剩余 source、replay、登记或门禁测试。

## 2026-08-25 实机前状态（历史）

- T0-02 mode gate 保持 `PASS`；decision SHA-256 为 `8C882CB0985D55D5FBA379C2BDC118D6280C90B6017A961A7F729E3A281AB185`。
- T0-03R V5 loop gate 保持 7/7 `PASS`；gate SHA-256 为 `CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA`。
- observation-only passive raw/apply 四场景为 4/4 `PASS`。它们只满足 active 前置证据，不具 active-gate 资格，历史 FAIL artifact 保持不变。
- `input-raw-prereq` 已只读重放四份 source 并生成 4/4 `PASS` decision，SHA-256 为 `99BA518991FEA36FE78D91AEA6D91D80634D4FD3B314399C4C4203C1BA27D5AC`。Decision 永久 `eligibleForActiveGate=false`、`activeInputAllowed=false`。
- 正式 active evidence 保持 primary `0/18`、extra repeatability `0/2`；全局 `activeInputAllowed=false`，未执行 active 注入、suppression、synthetic apply、active artifact 采集或最终 gate decision。
- 工作包 1 的 additive passive observer 已实现为独立 `IsaacCsT0-ActiveOfflineAttestation-v1` IPC：仅安装 `lua_pcallk`、`lua_close`、`SteamInternal_ContextInit` 三个 IAT 和 `SteamNetworking006` 三个虚表槽。`20260824T172923Z...-05` 已取得 60 callback、四类在线信号全零、六槽恢复的历史 `PASS`；其 prerequisite 固定 5 分钟且现已过期，不能用于后续 active CAS。
- 工作包 2 已实现 `IsaacCsT0-ActiveArmGrant-v1` 与 create-new token claim CAS。Grant 绑定 prerequisite、target、PID/creation、opaque generation、全部 gate/plan SHA、case/scenario 和有效期；并发双消费者只能一个成功，claim 一旦创建即使进程崩溃也永久视为已消费。生产 `input-smoke --probe loopback` 在 active verification 之前只校验 grant，不消费 token。
- 工作包 4 已完成 18 个场景 predicate、companion V5 manifest/events/analysis SHA 绑定和 semantic repeatability。pause/loading/room/menu 直接从 V5 的 bracketed steady gap、alternate transaction、room-change transaction 和 `RUN_TEARDOWN` 边界推导。`semanticContractComplete=true`。
- 工作包 3 已闭合 Native schema-4 runtime、active Helper 与 45 槽事务，并完成 Active IPC 身份收口：mapping 精确绑定 PID/creation、canonical/observed/equivalence、raw prerequisite 与全部 gate/catalog/plan/script SHA；Native 只接受 approved-unpacked profile。Active Native Debug 与 Release/LTCG 独立 harness 各固定 262 case。
- loopback CLI 强制 offline prerequisite、arm grant 与 verification v2，并保持顺序：静态 bundle → read-only preflight → session/generation opaque binding → grant → verification v2 → CAS → create-new artifact/mappings → Helper。当前没有未过期 prerequisite 或逐场景 active 授权，因此未进入生产 CAS、mapping、Helper 或 artifact 路径。
- 当前已安装 `isaac-ng.exe` SHA-256 `1FBA9A37...B89C` 已由 SHA-256 `8B3FB097...51F138` 的 create-new equivalence document 精确证明为 canonical `C4118519...AE30` 的 approved-unpacked 目标。它不再被描述为普通 target mismatch；Controller 与 Native 仍同时绑定三份摘要，任一变化即 fail closed。
- 身份收口后的 create-new `out/cs-t0/verification-v2-active-identity-final.json` SHA-256 为 `32ED209C467B1B2520C4AFED8A71D5C1FD3A82EA7BC26A98ABF8170FC997C0BD`，27/27 子门通过，`runtimeContractComplete=true`，同时固定 `activeInputAllowed=false`。

冻结输入为：

| 文件 | SHA-256 |
| --- | --- |
| `input-active-catalog-v1.json` | `AAB3A81D70BB5F8495C88811E37D6005D5CF9B313D76CDC96ED7F96F2DF0AC83`（`FROZEN_REJECTED_ADDRESS_MAP`） |
| `input-active-plan-v1.json` | `2F3F45CE2D27A67BCA5F0345825CF6371759738974D64EB5C75FBBBF7B16AA25`（随 v1 catalog 拒绝） |
| `input-active-catalog-v2.json` | `A89A74A251EEAA2510FC0C681247ABD6B0A31A7CC3ED60833C78EF36C9B0DC77` |
| `input-active-plan-v2.json` | `FE4FC7ABDE09CBB280DA342BD384AB50107CA948A052AC02BA6ADE37BB1DF3BA` |
| `input-raw-prereq-matrix-v1.json` | `292A52C48DA17C8546157A404EF1628BF517C85DE3C4F8476EA5D7A442A3EE29` |
| `input-active-repeatability-script-v1.json` | `7F2DAC1FF124B7DB2BC71FE886B6EE3AE49ADE23D5C464AE14973F05A9E53E33` |
| `artifacts/cs-t0/20260823-t0-04-input-raw-prerequisite-4of4.json` | `99BA518991FEA36FE78D91AEA6D91D80634D4FD3B314399C4C4203C1BA27D5AC` |

Active v1 catalog 错把 Player owner target 写为 `0x00382450`，并把六个 Player consumer call target 写成 logical-query RVA。既有 PASS target snapshots、passive v4 catalog 与 x32dbg 只读复核一致否定该地址图。v1 文件和 SHA 保持不变；create-new v2 只修正 ordinal 38–44 的七个 target，ordinal 0–37 及所有行为边界不变。

## 冻结边界

- Active adapter 只支持 steady-standard `main+0x2FBA4D -> Room::Update`。alternate、loading、room lifecycle、run teardown、menu 和无法唯一确认 owner/device generation 的路径全部 fail closed。
- 复用 V5 的 standard authority hook，在原始 `Room::Update` 前 drain/resolve/publish immutable synthetic snapshot，返回后清除 transaction scope；不新增第二个 authority call patch。
- 45 槽事务由 V5 32 槽和 active 13 槽组成。active 只含六个 logical-query vtable 槽、Player owner 槽和六个 Player consumer call；不安装 setter。
- Query wrapper 必须调用 original 恰好一次，仅在 main thread、steady transaction、目标 Player、匹配 consumer/device/owner 且 action `0..11` 时覆盖 `AL` 或 x87 `ST0`；其它情况 passthrough。
- 任一安装、运行、恢复或证据条件不确定时锁存 `Unsafe` 并禁止卸载。停止先关闭 publication/override 准入并排空所有已经进入的 embedded active callback，再退休 dispatcher/adapter、恢复 active 13 槽并二次 drain/verify，随后恢复 V5 32 槽并 drain，最后逐字节核对 45 槽；任一 drain 超时都 fail closed。
- 当前 Rust observer 对三个 IAT 槽的语义明确限定为“disk import symbol 映射到固定 IAT RVA，live pointer owner 为 `USER32.dll`”；它不能解析远端 x86 USER32 export 来证明 live pointer 仍等于精确 symbol 地址。报告固定写出 `iatSymbolResolution=DISK_IMPORT_SLOT_PLUS_LIVE_MODULE_OWNER_ONLY`，每槽 identity 标记 `SYMBOL_ADDRESS_UNRESOLVED`，不得把它表述为 exact remote-export proof。
- 逆向工作和测试工作继续分别由独立子智能体承担。IsaacTest 只有在逐场景明确授权后才能发送该场景必需的输入，不能代替 active 授权；当前暂停期间只允许只读检查。

## 实现里程碑（历史完成顺序）

1. **已完成（2026-08-23）**：扩展 `input-smoke --probe loopback`，强制接收 `--mode-gate`、`--loop-gate`、`--raw-prereq`、`--active-plan`、一次性 `--arm-token`；仅 loopback repeatability 场景额外接收冻结 `--input-script`。CLI 已只读验证静态 bundle 和 script SHA，并在 token 消费、进程枚举、mapping、artifact 或 Helper 之前停下；schema-2/3 passive 参数和行为保持不变。
2. **已完成（2026-08-24）**：`input-analyze`、`input-compare` 和 `input-gate` 按 manifest schema/evidence 与 matrix identity fail-closed 分派到 schema 4；matrix v2 固定按顺序分离 18 个 primary case 与两份额外 loopback repeatability source，并拒绝路径逃逸、重复 source/artifact/token、跨 schema、错误 plan/script/hash/eligibility 绑定和既有输出覆盖。
3. **已完成可观测部分（2026-08-24）**：在现有静态 bundle 重放之上接入 Rust active controller read-only preflight。它使用现有 query/read-only 句柄，核对精确 target、单一进程、x86/path/creation、remote PE identity、完整 module set、blocker/debugger/denylist（含 Tractor Beam hook）、45 槽语义与双快照稳定性，以及 RPM 的唯一 Player/concrete-device owner。keyboard/controller 场景还必须匹配 `KEYBOARD`/`GAMEPAD`；game root、run-active 与 online-manager-zero 只作为必要 negative screen。
4. **实现与自动门完成，历史 live PASS 已过期**：同一会话被动 offline/local attestation、60 callback、四类零信号、opaque identity 和六槽恢复均已接线；`-05` 历史 source/decision 保留，但新的 active 场景必须重新取得同会话 prerequisite。
5. **实现完成**：持久化 arm grant/token CAS、并发/重复/过期/错身份/崩溃恢复测试均已完成；生产路径仍要求匹配目标、正式 offline prerequisite、verification v2 和逐场景授权，当前未进入 CAS。
6. **实现完成**：Native schema-4 runtime、`IsaacCsT0InputActiveInitialize/Shutdown` 和 Helper active inject/unload 模式；45 槽单事务安装、故障 rollback、authority pre/post、synthetic snapshot、edge planner、reject、generation retirement 与恢复锁存均已接线。
7. **实现完成**：18 个专用 analyzer predicate、V5 companion replay、semantic repeatability、完整正向 fixture，以及 stage/reason/target/canonical/edge/V5 hash mutation 负例。
8. **实现完成**：独立 Active Native target 覆盖 contract、PID/creation 与九份固定摘要 mutation、45 槽 transaction/fault、AL/x87 ST0/ESP/非易失寄存器、pipeline、generation、shutdown、Helper 与跨语言 mapping；Debug 与 Release/LTCG 使用同一 262-case 合约。
9. **实现完成**：`verify-active.ps1` 统一运行 Rust fmt/check/clippy/test、baseline Native 39、Active Native Debug/Release、MCP 与 fixture，并按真实 exit、case count 和二进制 SHA create-new 生成 27/27 verification v2。
10. 自动门通过后停下，重新确认正式 active execution window。当前继续实施授权不改变 `activeInputAllowed=false`，也不替代每个 artifact 的新 arm token、case id、输出目录和场景前检查。

## 历史设计记录：Active Offline Attestation v1

状态：该段保留 2026-08-24 的 v1 设计与当时停点。当前实现已演进到 offline attestation v2，并已经过多次逐场景 live prerequisite 验证；与本段冲突时，以文首“当前执行快照”和后文“七个工作包状态”为准。

### 目标与证据身份

- 新增 additive、passive-only 的 `IsaacCsT0-ActiveOfflineAttestation-v1` 通道和 create-new prerequisite decision；不得修改 T0-02 冻结 IPC、ledger、artifact 或 gate。
- 证明必须来自即将接受 active preflight 的同一 `isaac-ng.exe`：target SHA、PID、process creation time 和当前 volatile game-root/generation identity 必须一致。game-root/generation 原始地址只允许在控制器内存中短暂比较，不写入 artifact。
- 复用 T0-02 已验证的 passive mode signals，而不是以历史 mode gate、窗口标题或瞬时 RPM 代替：同一 Lua generation 至少 60 个连续 main-thread simulation callback；整个观察窗 manager identity、stable roster、Steam channel `0` 与 nonzero channel 事件均为零；generation 不得退休、切换或复活。

### 固定执行顺序

1. 先执行现有 static bundle 与 observable preflight 的廉价检查：精确 target、唯一进程、path/x86/creation/remote PE、blocker/debugger/模块、45 槽和唯一 Player/device owner；任一失败立即停止。
2. 通过新的独立 passive attestation IPC 启动 T0-02 mode observer 的最小只读子集。该 observer 只记录 mode/lifecycle 信号并恰好调用 originals；不加载 schema-4 active runtime，不创建 input mapping，不 suppress、inject、queue、resolve 或 apply 输入。
3. 在同一 active Run 中等待一个 Lua generation 达到至少 60 个连续 simulation callback，并持续确认 manager/roster/channel 四类在线信号为零；线程变化、generation 变化、menu/teardown、Lua close、online signal、drop/overwrite 或超时全部 fail closed。
4. 安全停止 passive observer，等待 wrapper drain，逐槽恢复并卸载；随后立即以同一 PID/creation/game-root identity 重新打开目标，重复 debugger/module/45-slot/owner/negative-screen 检查，并确认磁盘 SHA 前后不变。
5. 只有上述步骤全部通过，才允许 create-new 写出 `IsaacCsT0-ActiveOfflinePrerequisite-v1` decision。Decision 必须固定 `eligibleForActiveGate=false`、`activeInputAllowed=false`、`readyForArmTokenCas=false`、`armTokenConsumed=false`，只解除 `BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION` 这一项实现缺口。
6. Controller 的 loopback CLI 后续必须显式接收并 hash-bind 该 decision；旧文件、重复路径、错误 PID/creation/target/generation、历史 artifact 或跨会话 decision 全部拒绝。当前 arm token 仍只做语法/静态输入检查，不传入 attestation 或 preflight。

### 自动测试与人工停点

- Rust 正向 fixture 必须覆盖 60 callback、同 generation/main thread、四类在线信号全零、安全卸载、同 PID/creation/game-root 和 45 槽复核；59/61 边界、sequence gap、wrong thread、generation retirement/reuse、manager、roster、channel-0、nonzero-channel、teardown、hash/path/identity、overwrite 和 existing-output mutation 必须全部 fail closed。
- Native Debug 与 Release/LTCG harness 必须覆盖 original-once、observer-only payload、schema fail closed、partial-install rollback、wrapper drain 和全部被动槽恢复；不得把旧 Native harness 当作本 schema 的通过证据。
- 独立逆向子智能体只读确认复用 callsites/ABI；x32dbg 不新增 attach、断点、暂停、patch、寄存器或内存写入。独立测试子智能体运行 Rust/Native/MCP 自动门；`isaac_test` 只允许 `inspect_game`。
- 自动门完成后必须停止并请求一次新的“passive attestation 实机窗口”授权。未经该授权，不启动 Helper、不加载 passive observer、不创建 prerequisite decision；该授权也不构成后续 active execution 授权。

### 本增量完成条件

本增量只有在“实现与自动门全部通过、生产实机仍未自动启动、arm/token/mapping/active Native/artifact 均未触碰、文档和 CLI 明确停在 passive attestation 人工授权前”时才可记为 implementation-complete。正式 prerequisite decision 必须等新的被动实机授权与唯一 create-new run；它完成后仍需再次停在 token CAS 设计前。

## 七个工作包状态

七个主要工作包现已全部闭合：

| 工作包 | 当前进度 | 完成标志 |
| --- | --- | --- |
| 1. Same-session passive offline attestation | 完成 | 每个 active 场景均使用 fresh prerequisite；prerequisite 本身不单独解锁 active |
| 2. 一次性 arm token CAS | 实现完成；已在已执行 active 场景中按次消费 | consume-once、过期/重复/错 session 全部 fail closed；失败后 token 不复活 |
| 3. Native schema-4 active runtime + Helper + 45 槽事务 | 实现完成 | Debug/Release ABI、安装、回滚、恢复自动门全部通过 |
| 4. 18 场景 analyzer semantics + repeatability | 实现完成 | `semanticContractComplete=true`；V5/hash/mutation fixture 全通过 |
| 5. Native/Rust/MCP 综合自动门 | 完成；verification v2 27/27 | `semanticContractComplete=true`、`runtimeContractComplete=true` |
| 6. 正式 active evidence matrix | 完成；primary 18/18、extra 2/2、comparison 1/1 | 20/20 provenance replay 后 create-new 登记 |
| 7. 最终 active gate decision | 完成；正式 1/1 `PASS` | `t004Unlocked=true`、`activeInputAllowed=true` |

代码工作包、source 采集、全量 provenance replay、正式 matrix 和最终 gate decision 均已闭合。第一次正式 replay 在 `keyboard` 报告说明文字差异处按规则中止并保留为不可用审计包；完整 `r1` 包只允许这一已核实的 detail-only 迁移，不放宽语义比较。

从正式通过标准看，active matrix 已为 `18/18 + 2/2`，comparison `1/1`，final decision `1/1 PASS`。T0-04 已从候选覆盖推进到正式登记与授权完成。

## 已完成的实机与登记顺序

matrix 的逻辑顺序仍固定为：`control-no-native`、`keyboard`、`controller`、`simultaneous`、`rapid-tap`、`device-switch`、`pause`、`loading`、`room`、`menu`、`queue-full`、`expired`、`too-far`、`stale`、`conflict`、`duplicate`、`original-online-disarmed`、`loopback`。18 个 primary、两份 extra 与 pair comparison 已全部通过；matrix 生成器负责按冻结顺序登记，不以采集先后替代规范顺序。

每个场景均在执行前重新验证进程、owner、gate hashes、目标字节和 debugger/模块状态；controller 场景先完成 RPM unique-owner 前检并由用户手动操作。失败 artifact 不覆盖，未进入正式 matrix。

## 完成条件

完成条件已满足：18 个 primary 全部通过，两份 loopback source 与 comparison 通过，45 槽完整恢复，全部自动门通过，且没有 Unsafe/overwrite/wrong-thread/lifecycle 泄漏。正式 gate decision 已 create-new 写出 `PASS`；冻结 ledger 与历史 artifact 仍不覆盖。
