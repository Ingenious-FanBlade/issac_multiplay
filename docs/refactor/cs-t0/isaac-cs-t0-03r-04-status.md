# T0-03R / T0-04 当前状态与恢复工作说明

> 2026-10-12 执行约定更新：先读[现行项目规范](../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

更新时间：2026-08-27  
目标构建：Win32 `isaac-ng.exe`，SHA-256 `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`

本文是当前执行状态与历史停点索引；任务目标和通过标准仍以 [isaac-cs-t0-task-plan.md](./t0-04/isaac-cs-t0-task-plan.md) 为准。截至 T0-04 的可复用逆向事实、反证和 T0-05 输入统一见 [T0-04 逆向交接](./t0-04/isaac-cs-t0-04-reverse-handoff.md)。下方第 1 节起保留按时间累积的历史停点；与文首快照或逆向交接冲突时，历史文字只解释当时状态。

## 2026-08-27 T0-04 执行快照

最后一个 primary source `original-online-disarmed live-46` 完成后，20 份 source 已执行全量 provenance replay。正式 18+2 matrix、repeatability comparison 和 final decision 均已 create-new 写出；T0-04 正式为 `PASS`，`t004Unlocked=true`、`activeInputAllowed=true`。

`live-46` 全程只读，没有 arm/token、IPC/mapping、Native、Helper、suppression、synthetic input 或 companion runtime。artifact 只保存 nonce 盐化的 game/manager/backend/roster 摘要、roster count 与 frame 范围，不保存原始 peer identity 或地址；Controller/Helper 无残留，游戏 PID 26960 继续响应。

### 硬状态

| 项目 | 当前结论 |
| --- | --- |
| canonical target | `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30` |
| observed approved-unpacked target | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| target equivalence | `PASS_EQUIVALENT_READ_ONLY`；document SHA-256 `8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138` |
| mode / V5 loop gate | `PASS`；SHA-256 分别为 `8C882CB0985D55D5FBA379C2BDC118D6280C90B6017A961A7F729E3A281AB185`、`CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA` |
| Schema-4 自动合约 | verification：`semanticContractComplete=true`、`runtimeContractComplete=true`；final gate：`activeInputAllowed=true` |
| 最新 verification v2 | `out/cs-t0/live-verifications/t0-04-repeatability-shutdown-tail-normalization-fix-v2.json`，27/27，SHA-256 `2ABBA008B319C973E8424C740BCCE00CE6D9002B913B7ABB3C2DA1C790243317` |
| 自动门明细 | Rust 255/255、cross-language 1/1、baseline Native Debug/Release 39/39、Active Native Debug/Release-LTCG 262/262、MCP 16/16 |
| 当前 Release 二进制 | Controller `EADB7B2934668F93733315B989442D0D273C155935E539482D1F1697029F2A2B`；Native `B74E95688E1CD4F893D47AEECD06A2E5DD50FCA381F8A4672974DC6BECD729D7`；Helper `FC3B862C180501638A956BBD6781CE8BF2F2AA3A6971C1D1BBBFBED6553E7E06` |
| 正式 matrix / final decision | primary `18/18` + extra `2/2`；final decision `1/1 PASS` |
| 正式 primary source | `18/18`；20/20 根分析均 `PASS` 且 gate-eligible |
| 正式 extra source / comparison | 单份 `2/2`；A/B comparison `1/1` |

这里必须区分三种计数：

- **正式 primary `18/18`**：全部 primary 均已进入 create-new `IsaacCsT0-ActiveMatrix-v2`。
- **正式 extra source `2/2`**：`loopback live-44`、`live-45` 均已登记；严格尾部归一化后的 pair comparison 为 `1/1`。
- **最终 gate `1/1 PASS`**：18 个 analysis binding、2 个 repeatability binding、零 missing case，`t004Unlocked=true`、`activeInputAllowed=true`。

正式包位于 `out/cs-t0/formal-active-gate-20260827-r1`。matrix、repeatability、verification 与 final decision SHA-256 分别为 `54BDD4F7E0070D14DA913C6993C97272D97A91B193BDA5C04E2FCAE3E9C6889D`、`AFA254C0E06BAFC53728F9D4CCCF42C1999B9B5CD68533449DBF2E92B57775CC`、`2ABBA008B319C973E8424C740BCCE00CE6D9002B913B7ABB3C2DA1C790243317`、`927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C`。

20/20 重放中有 17 份 analysis SHA 逐字节一致；`keyboard`、`simultaneous`、`rapid-tap` 仅发生 `companion-loop-context` 的报告说明文字迁移，语义字段、trace hashes、check IDs/结果、manifest 与 events 均不变。原 analysis 未覆盖。首次正式 replay 在该差异处停止并保留为 incomplete 审计包；完整规则与哈希记录见 `PROVENANCE.md`。

### 可重放 primary 候选

| 场景 | source | 当前结果 |
| --- | --- | --- |
| `control-no-native` | `live-38` | 21/21 `PASS`；`DISARMED_CONTROL`；零 IPC/Native/arm |
| `keyboard` | `live-07` | 21/21 `PASS` |
| `controller` | `live-41` | 21/21 `PASS`；companion V5 18/18 `PASS`；独立 exact replay 一致 |
| `simultaneous` | `live-09` | 21/21 `PASS` |
| `rapid-tap` | `live-10` | 21/21 `PASS` |
| `device-switch` | `live-42` | 21/21 `PASS`；唯一 owner switch；companion V5 18/18 `PASS` |
| `queue-full` | `live-14` | 21/21 `PASS` |
| `expired` | `live-16` | 21/21 `PASS` |
| `too-far` | `live-17` | 21/21 `PASS` |
| `stale` | `live-18` | 21/21 `PASS` |
| `conflict` | `live-20` | 21/21 `PASS` |
| `duplicate` | `live-21` | 21/21 `PASS` |
| `original-online-disarmed` | `live-46` | 21/21 `PASS`；`DISARMED_CONTROL`；2 人 roster；frame `6052→6056`；独立 exact replay 一致 |
| `pause` | `live-23r1` | 21/21 `PASS` |
| `loading` | `live-32` | 当前 analyzer create-new 重分析 21/21 `PASS`；正式登记前仍需按下述 provenance 规则复核 |
| `room` | `live-35` | 21/21 `PASS`；companion V5 18/18 `PASS` |
| `menu` | `live-36` | 21/21 `PASS`；companion V5 18/18 `PASS` |
| `loopback` | `live-37` | 当前 analyzer create-new 重放 21/21 `PASS`；原 20/21 `FAIL` analysis 保留 |

`live-32` 原始 manifest、events 与 analysis SHA-256 分别为 `31B7B1828239D15AA11290169F72C362F69B07AE13B4594B1AA18FDCA8F9DF72`、`F78C747143F827E55EF3E1E0F3C9F26878CDF5DE964285582469429BD78A2B83`、`0F0797BE93CD2925098A922B9CA51994A0AAFD7132750D86FAD0CB7F9B4EAE6A`。原 analysis 的 20/21 `FAIL` 不覆盖；修复后的 analyzer 将 `resolve.sourceDevice` 解释为协议内生、必须为 `null`，其余 command stage 仍必须为 `KEYBOARD`。create-new 重分析位于 `out/cs-t0/live-32-reanalysis-resolve-source-fix`，analysis SHA-256 `22BA86D061B816DD45B5B0476B98F407BEFB4690F27F965CFE4E9F8D2B39E70E`；独立 exact-replay QA 位于 `out/cs-t0/qa/live-32-exact-replay-audit-20260826`，结果与摘要相同。正式 matrix 只能引用这份不改写原始证据的当前语义重放结果。

### 最近停点：extra A/B source 与 repeatability comparison 全部 `PASS`

- Fresh prerequisite 位于 `artifacts/cs-t0/20260826T130714Z-t0-04-offline-attestation-loopback-t0-04-active-loopback-extra-b-live-45/prerequisite.json`：同一 PID 26960 / creation `134322115495116295` / generation 1，60 个连续 simulation callback，SHA-256 `413B2779A78FD7274CE336B266AFF7968C1CC613C4FF73F627281AECD6E5F1FC`。Grant SHA-256 `ADD94FCCAAA30A71826487D759F76711CF12308D502D9883335A1446A82964AE`，claim SHA-256 `D5D2E4018B9343344923A523630DF646CC7973E74EF79E69D135A6C227232424`；一次性 token 已消费。
- Active source 位于 `artifacts/cs-t0/20260826T130751Z-t0-04-input-active-loopback-t0-04-active-loopback-extra-b-live-45`，artifact identity 为 `active-runtime-85143B15A9A1334228409D03DF4CFAAD96ED34893C3DB1797CF271D06CB330FA`。运行产生 776 条 event、30 accept、0 reject、23 apply，全部 21 项 Active 检查及专用 loopback predicate 均通过，`eligibleForActiveGate=true`。
- Active manifest/events/analysis SHA-256 分别为 `15B48FA8D0F1A977D5557B9F8F2203654E53F1BAF0891CB1BDADF40FC8275211`、`AC054EE9787B515B26E7F17D54FEE1113AAD5E1BEF2E3933143614FF94C603EC`、`06A94C29983A91719DA4374A7534E06CF62C5F08802F66AC1B814408B29D71C2`；semantic/canonical/decision/apply trace SHA-256 分别为 `71B156BFBEE4F0B8EA6DB1EE435A7E819F89EBB7F446C1943B0149BE3783812D`、`B19242AEF4B2A5BF68E7B8F77669F0D73091E6DB8A569B758E47802DA602458E`、`8C4DFF750463BA79DA2E204A6CFCF1ABAD282529526C8703DFB13A9081ED5C21`、`8F92050F9A1BF4CAEEE4B8B1445B8330B656FF6B9D626C1C5866528EACFD9CE6`。
- Companion V5 为 1,158 条 event、50/50 个完整 loop；17/18 仅缺冻结七场景之外的 `scenario-evidence`，符合 Active 对 loopback 的 companion scope policy。其 manifest/events/analysis SHA-256 分别为 `B2AF76BADF8B5A35C0B36AE0CCEC13AF8EA6CBBAC3C7FFF7DEB1A16A451E381C`、`C690D71597472A43A421D05CA91EDD66286E52C3EBBEA093B887008CB41AD614`、`B1B1B1267B7431E26112FF6AEAB15D65BC7EF762ED991B93880435981EAB0440`。
- 独立 replay 位于 `out/cs-t0/qa/live-45-loopback-extra-b-exact-replay-20260826`，重新得到 21/21 `PASS`，analysis SHA 与 source 完全相同。Active `0x1FFF/0x1FFF`、45/45 槽、before/after target bytes 完整恢复，双方 overwrite 为 0，ProducerClosed、ClientRetired；Controller/Helper/Native 无残留，游戏 PID 26960 继续响应。
- 首次 create-new comparison 位于 `out/cs-t0/live-comparisons/live-44-live-45-loopback-extra-repeatability-20260826.json`，SHA-256 `5A88A47F2C39B5FE4C07148981F2E898E43AE21F68FE4F24AB90EF7B4183D6A7`，保留为历史 `FAIL`。两份 source 的独立身份、冻结 scope 与 replay PASS 均通过，仅 `semantic-trace-equal` 失败。
- 事件级归一化确认两边前 774 个事件完全相同；差异从 ordinal 775 开始，`live-44` 在 authority tick 25 后额外记录一个 target tick 27、没有 resolve/apply 的 neutral command 五阶段尾巴（`sample → canonical → send → receive → accept`），随后双方 lifecycle 一致。两份 apply trace SHA 均为 `8F92050F...FD9CE6`。该旧 comparison 不覆盖、不改判。
- comparator 现只剥离紧邻最终 `HOOKS_RESTORED → PRODUCER_CLOSED` 的严格五阶段 neutral command 尾巴，并校验连续 event sequence、同 session/client/generation/source/action、零 edge、next-loop、target tick、最后一次 neutral apply 以及不存在 resolve/apply。canonical state、target tick、loop、sample component 或后续 stage 任一变化仍在负向回归中保持语义可见。
- 新 create-new comparison 位于 `out/cs-t0/live-comparisons/live-44-live-45-loopback-extra-repeatability-tail-normalized-20260826.json`，SHA-256 `AFA254C0E06BAFC53728F9D4CCCF42C1999B9B5CD68533449DBF2E92B57775CC`；4/4 `PASS`、`eligibleForActiveGate=true`。最新 verification v2 为 27/27，Rust 255/255，完整 Native/cross-language/MCP 门均通过。

### 前一停点：`loopback live-44` extra A `PASS`

- Fresh prerequisite 位于 `artifacts/cs-t0/20260826T125118Z-t0-04-offline-attestation-loopback-t0-04-active-loopback-extra-a-live-44/prerequisite.json`：同一 PID 26960 / creation `134322115495116295` / generation 1，60 个连续 simulation callback，SHA-256 `8998075B58950FE014DA053F1D22EE8223C43734189DFC2F5B4880ECFF8D8A5B`。Grant SHA-256 `65C0862E20C14872BC5DD86CFA207AC2A06614A4111F5868BD5406F178C329C3`，claim SHA-256 `B872FE9542D9E86582D8B89A42BEF4F734B87257F0D697959B523B4E4B201298`；一次性 token 已消费。
- Active source 位于 `artifacts/cs-t0/20260826T125202Z-t0-04-input-active-loopback-t0-04-active-loopback-extra-a-live-44`，artifact identity 为 `active-runtime-B60F33BD36617638DCF56B602BE70D14BE9B5B13C288BE2581FB435B15BE3C57`。运行产生 781 条 event、31 accept、0 reject、23 apply，全部 21 项 Active 检查及专用 loopback predicate 均通过，`eligibleForActiveGate=true`。
- Active manifest/events/analysis SHA-256 分别为 `358718F23AEAE01C2B2DDEEC1E67E3163368395FE5943CC0B5A5B2F553AE47CC`、`C8CEC7D7BCA329DAAEA91D60BEA97C3B2579E9B4727377C3075A5DC2B280DEE2`、`E3A4F5B30E0E40D8DB012AEB5B5A07ABEBD42637E59E5B7DD776F02F1E195142`；semantic/canonical/decision/apply trace SHA-256 分别为 `B90ED34DE85F510F1BED83C443B5F9039AB383C6787EA6D0FADC19EBC5CBAD34`、`151E29A874C702727F457A7D5CB73827338B70B20FB319BAE057153199C57E12`、`19E478C10290D9CF227754541BEE1CF4E678718517E714582C634952BC27A3F3`、`8F92050F9A1BF4CAEEE4B8B1445B8330B656FF6B9D626C1C5866528EACFD9CE6`。
- Companion V5 为 1,158 条 event、50/50 个完整 loop；17/18 仅缺冻结七场景之外的 `scenario-evidence`，符合 Active 对 loopback 的 companion scope policy。其 manifest/events/analysis SHA-256 分别为 `AD9349014819A00258BDE3027FC71E84EFD7770C7079A1CB021EC43D413A4492`、`B6E5B9869ECBA30C364C947EF405017AC1174CC88165923F740385D1773DE5D2`、`8B1AFEEB3F6EEFAF993E46939D53FB1A365F423FB12D1BDF27F9EAC2447660E7`。
- 独立 replay 位于 `out/cs-t0/qa/live-44-loopback-extra-a-exact-replay-20260826`，重新得到 21/21 `PASS`，analysis SHA 与 source 完全相同。Active `0x1FFF/0x1FFF`、45/45 槽、before/after target bytes 完整恢复，双方 overwrite 为 0，ProducerClosed、ClientRetired；Controller/Helper/Native 无残留，游戏 PID 26960 继续响应。
- 更早的 `live-43-r1` 保留为 20/21 `FAIL`：25 个采样帧均为 neutral，说明授权后焦点没有回到游戏，脚本未形成输入切换；它的通用不变量与安全恢复均通过，但不得登记或改判。`live-44` 通过增加授权后的 10 秒前台切换窗口消除了该执行条件问题。

### 最近停点：`original-online-disarmed live-46 PASS`

- 旧路径会进入 `manual_input_checkpoint`，随后由 canonical-only inspect 拒绝 approved-unpacked 目标，且没有 schema-4 `DISARMED_CONTROL` source producer。修复后 `control-no-native` 与 `original-online-disarmed` 共用 approved-unpacked/static bundle 验证，后者改由 read-only RPM positive screen 创建 source。
- positive screen 必须在两次观察中稳定得到 active Run、`OnlineManager + 0x4B3D8`、Steam backend vtable `base + 0x7A2440`、2–4 个 distinct `device + 0x374 → identity + 8` 身份，以及不回退 frame。这些布局已与原 IsaacSync 源码和当前 Schema-4 Native 实现逐项交叉核对；旧项目的 session、channel `0xFD` 和可变 runtime 未复用。
- artifact 只保存单次 nonce 盐化的 game/manager/backend/roster 摘要、roster count 与 frame 范围，不保存原始 peer identity 或地址。同时要求进程/module 和 45 槽前后不变，全程禁止 arm/token、IPC/mapping、Native、Helper、suppression 与 synthetic input。
- Source 位于 `artifacts/cs-t0/20260826T161503Z-t0-04-input-disarmed-original-online-disarmed-t0-04-original-online-disarmed-live-46`，artifact identity 为 `disarmed-control-A0BBC2D3925BBCC409CBF6F4AF7735F4F33CCC7A36B7AF641FA999FE863D69EC`。两次 observation 的 roster count 为 2，online frame `6052→6056`；21/21 `PASS`、`eligibleForActiveGate=true`。
- manifest/events/analysis SHA-256 分别为 `56F38929626542E96C80D10BA2C5901E97B124466536FB64FCA570323F2E8B6B`、`4606D8A18BE4828F6A6BCE4B093EDCE0A6624CBB1FC581C428209F3D152E5D10`、`58DAE197680F5ECF01F26FBBFE196804ABF257C70D97C52D2E72DCFB7BCE72DD`；独立 replay `out/cs-t0/qa/live-46-original-online-disarmed-exact-replay-20260827` 的三份哈希完全相同。
- installed/restored mask 均为 `0x0`，restored slot count 为 45，arm token 未消费，只有 1 条 `control-observation`，accept/reject/apply 均为 0。Controller/Helper 无残留，游戏 PID 26960 继续响应。
- 当前 Release Controller/Native/Helper SHA-256 分别为 `EADB7B2934668F93733315B989442D0D273C155935E539482D1F1697029F2A2B`、`B74E95688E1CD4F893D47AEECD06A2E5DD50FCA381F8A4672974DC6BECD729D7`、`FC3B862C180501638A956BBD6781CE8BF2F2AA3A6971C1D1BBBFBED6553E7E06`。候选现为 18/18。

### 前一停点：`live-42 device-switch PASS`

- Fresh prerequisite 位于 `artifacts/cs-t0/20260826T101301Z-t0-04-offline-attestation-device-switch-t0-04-active-device-switch-live-42/prerequisite.json`：同一 PID 26960 / creation `134322115495116295` / generation 1，60 个连续 simulation callback，SHA-256 `D7DF0B2EDB9F9F85D26872A515F65F36D1BB575EEF4C58FE3D1A559D53763812`。Grant SHA-256 `8638DF05B222AEAA4D415F5F2618AFB31FEADA2C49677474674FE95CC9D84FEB`，claim SHA-256 `173ADFFB9F6AC5E06454401B7E6791C97704441554DE3970EFAA861B12DD3E89`；一次性 token 已消费。
- Active source 位于 `artifacts/cs-t0/20260826T101334Z-t0-04-input-active-device-switch-t0-04-active-device-switch-live-42`，artifact identity 为 `active-runtime-8F6F4EFD812D1870EECA3E52CCC136FFCFF492D7BD2CAA5E49B8483518B873C0`。用户先用手柄操作，再经设置将 Player 1 切到键盘，并在切换后只用键盘移动/射击；197,229 条 event、6,425 accept、0 reject、6,348 apply，Active 21/21、companion V5 18/18 `PASS`。
- Active stream 恰好只有一条 `OWNER_SWITCH`（sequence 194053，owner key `1 → UINT32_MAX`，generation 4）。切换前 6,249 条 apply 的 source 全为 `GAMEPAD`；切换后 99 条全为 `KEYBOARD`，且 `WrongGenerationAfter=0`。这同时满足 owner-switch isolation、新代际 apply 与 companion 双设备设置刷新，不以“看见两类 poll”冒充真实切换。
- 正常停止完成 Active `0x1FFF/0x1FFF`、45/45 槽、companion `0xFFFFFFFF` 32 槽和 before/after target bytes 恢复；双方 overwrite 为 0，Active ProducerClosed、ClientRetired，Controller/Helper 无残留，游戏继续运行。
- Active manifest/events/analysis SHA-256 分别为 `45A1357B3E25DB599A32DA767708D31729F9D69AC1DFF3E4A6649FF970172418`、`9732BC714A2F813F8105CAFB520C89913AC8F338D503126C6C09740FC2C61BD3`、`CF3E07489E004D69445D71E3ABCB3B28D2CF83425B29988CE0B57A775F5725E0`；companion manifest/events/analysis 分别为 `E81B20F38984F32BFEDA93575427E2816A391D1FB404537DC5500CAA452A858A`、`A9DED1F1E62D77F6CD8AD45E69ABE26B4F12C5C30990CB0DAB544E9A1D6271FE`、`3120D03B04895B08B3DB1271D59EF9F0B7E2B59DE796973E81F52E857EB28533`。独立 replay 位于 `out/cs-t0/qa/live-42-device-switch-exact-replay-20260826`，重新得到 21/21 `PASS`，analysis SHA 与 source 完全相同。

### 前一停点：`live-41 controller PASS`

- Fresh prerequisite 位于 `artifacts/cs-t0/20260826T095709Z-t0-04-offline-attestation-controller-t0-04-active-controller-live-41/prerequisite.json`：PID 26960 / creation `134322115495116295` / generation 1，60 个连续 simulation callback，SHA-256 `245AC72BAA47AA0281EF9C53E25458656808A83D551D49F00FD64D1B059ADAE7`。Grant SHA-256 `1204FB8B62B66E24C9F2A4FFA300B37B489B424EE354FB4F7EDDF4A2C1AFB27C`，claim SHA-256 `D3C70975FA22F9AFAC2925AACAE0E636BDF6343C019BF12240F63C61459357C7`；一次性 token 已消费。
- Active source 位于 `artifacts/cs-t0/20260826T095813Z-t0-04-input-active-controller-t0-04-active-controller-live-41`，artifact identity 为 `active-runtime-DA660655469A8C4849CE0E28F8C1E5121CD50E6D466E1B341429ADAAE5B77897`。用户仅用手柄手动完成移动和射击；157,594 条 event 中有 5,149 次 accept、0 reject、5,070 次 apply，专用 controller predicate 与全部通用不变量均通过，Active 21/21、companion V5 18/18 `PASS`。
- 正常停止完成 Active `0x1FFF/0x1FFF`、45/45 槽、companion `0xFFFFFFFF` 32 槽和 before/after target bytes 恢复；双方 overwrite 为 0，Active ProducerClosed、ClientRetired、initial snapshot neutral，未锁存 Failure。Controller/Helper 均退出，游戏 PID 26960 继续运行。
- Active manifest/events/analysis SHA-256 分别为 `D91CA81CBD630CE0712ACBE3C4805FB11B31797E64087471D2F0B76F9B51B376`、`B381D4E85A0676933D25E31A4F838C79BDFB2DBA071695D21919D9879BE6CB4D`、`1889A038ACCC590A47359C6873093B766CD610F5F656DA8102882D9F08B72FBD`；companion manifest/events/analysis 分别为 `1C0DE9C9FF515AA4E72B27C293A1A3ACB9636DB5B55DC835D2B6FDD53D99F497`、`172780A44C314D6EFF8F130832E4BB80E62E9DCF1979655DAD786059F84C90C4`、`006635CA4E87602FF07E06517247ACEC621C654254D8152286BC477C2AF9E4B7`。独立 replay 位于 `out/cs-t0/qa/live-41-controller-exact-replay-20260826`，重新得到 21/21 `PASS` 且 analysis SHA 与 source 完全相同。

### 前一停点：`live-40 controller FAIL`，首个 gamepad projection 未就绪

- Fresh prerequisite 位于 `artifacts/cs-t0/20260826T093230Z-t0-04-offline-attestation-controller-t0-04-active-controller-live-40/prerequisite.json`：同一 PID 19460 / creation `134322092748108550` / generation 1，60 个连续 simulation callback并完整恢复被动 observer，SHA-256 `7F8793BA36917A5062F4B0A1ADEA3E273C957D6AECEE3D2FB5578C5AF7C18017`。Grant SHA-256 `C54EFD6933A00C8F9B764399B5E6B09AE3FAFEE0AC4755B5D543DD74D4912D9A`，claim SHA-256 `69749E61D2EF5993CBEA68B8B92DDAF035AD0B3FCF2A1BA82FAD67F9509FDB9D`；一次性 token 已消费且不得复用。
- Active artifact 位于 `artifacts/cs-t0/20260826T093327Z-t0-04-input-active-controller-t0-04-active-controller-live-40`。它只有三条事件：controller 线程发布 `INITIALIZED`、`HOOKS_INSTALLED`，随后 main thread/loop 1 立即发布 error 12 `PROTOCOL_VIOLATION`；没有 `READY` 后人工动作、physical sample、canonical、accept、apply 或 dispatcher retirement trace。
- Active Failure QPC `2705866624653` 位于 companion `INPUT_PHYSICAL_POLL_EXIT` 发布 QPC `2705866624655` 之前，符合 embedded physical-exit callback 的调用顺序。Initialize 已在安装前构造 dispatcher，而 stream 中没有 `SubmitPhysical` 会产生的 canonical 或 `CLIENT_RETIRED`，因此故障被限定在 `BuildRawSample` 返回 false：`ReadGamepadProjection` 的 bindings/state 双快照不一致、`ProjectGamepad` 非 Ok，或最终 valid mask 不等于 `0x0FFF`。当前 error code 没有持久化该子原因，不能把其中任一项单独写成事实。
- 失败 manifest 为 installed `0x1FFF`、restored `0x0`、0 slot、ProducerClosed false、overwrite 0；自动卸载按 Unsafe 规则被禁止。用户从游戏内正常退出后，`isaac-ng.exe`、Active Native、Helper/Controller 与调试器进程均不存在，不再有进程级残留。该清理只证明 OS 进程退出，不把失败 artifact 改判为安全恢复。
- Active manifest/events SHA-256 分别为 `44692D689EE7AACFCD25DE0EACC434EB75FD27130E1A58295943E5DC2D1371FF`、`583C64F211BE1A44EAC44510C343174399A2C167CAB6BF790849587B51938B2D`；companion manifest/events SHA-256 分别为 `FF968D01E9328131394C21D6B7AFEE63288F16DB80FEAB492D7D781C17D18FF2`、`180C1200DAF046BDA6E0BA232A60C89596AE70F10EB53B6C210C5783CB176C0F`。原始失败证据保持不覆盖、不重放成 PASS。
- 修复新增 `PhysicalOwnerReadyForSubmission`：Rust preflight 的运行前唯一 owner 证明不再替代 Native runtime binding；只有 `ActivePlayerUpdateHook` 已观察同一唯一 Player 后，physical callback 才能读取 selected device 并构造 sample。全部 16 个 Active scenario 的确定性回归都验证 unbound=false、bound=true；旧有 loading、simultaneous/device-switch bootstrap 合并到同一更强不变量。owner 绑定后的任何 key、快照、projection 或 valid-mask 异常仍锁存 Unsafe，没有放宽 fail-closed。
- 修复后的 create-new verification v2 位于 `out/cs-t0/live-verifications/t0-04-active-owner-before-projection-bootstrap-fix-v2.json`，SHA-256 `2ABBA008B319C973E8424C740BCCE00CE6D9002B913B7ABB3C2DA1C790243317`：27/27，Rust 250/250、cross-language 1/1、baseline Native Debug/Release 各 39/39、Active Native Debug/Release-LTCG 各 262/262、MCP 16/16；Release Controller/Native/Helper SHA-256 分别为 `44DB89923CC2DFC7461C2026B6B4426F4ACE15847B07181A712DA159FFE14C16`、`B74E95688E1CD4F893D47AEECD06A2E5DD50FCA381F8A4672974DC6BECD729D7`、`FC3B862C180501638A956BBD6781CE8BF2F2AA3A6971C1D1BBBFBED6553E7E06`。`activeInputAllowed=false` 保持不变，自动门不能替代新的 live run。

### 前一停点：`live-39 controller FAIL`，停止竞态已修复待实机复测

- Active artifact 位于 `artifacts/cs-t0/20260826T090507Z-t0-04-input-active-controller-t0-04-active-controller-live-39`。它保留为 schema-4 `PIPELINE_RUNTIME` / `FAILED`，不得登记到正式 matrix，也不改变当前 17/18 候选覆盖。
- Active stream 的最后两条事件为 sequence 60855 `CLIENT_RETIRED` 和 60856 `FAILURE` / error 12 `PROTOCOL_VIOLATION`。停止线程关闭 override 后立即退休 dispatcher；一个此前已通过 `OverrideEnabled()` 的 embedded physical-exit callback 随后恢复并提交，因 dispatcher 已退休而锁存 Unsafe。失败 manifest 固定 `installedActiveHooksMask=0x1FFF`、`restoredActiveHooksMask=0x0`、`restoredSlotCount=0`、`producerClosed=false`、`clientRetired=true`；这是停止竞态，不是 controller 场景通过证据。游戏之后正常退出，进程级模块与 Controller/Helper 残留均已清除。
- Active manifest/events SHA-256 分别为 `8A966B3E4690ED99D4F540148A6E353F31799B3276D4B60581EC72D890059908`、`3B5097FD05132D425B0EC79583C89A1E8D6CBAB9029FD32BBC308B8F4815D11A`；companion loop manifest/events SHA-256 分别为 `26F2871E3CF9D0B1A49D4B482B02BB2EE3366B1F78F4552DAC04695556C259CF`、`13A1BDDF6BDB778965A825BFBA0DF75D2830DC6CCBF30CBD101733E2DADB19E0`。原始失败证据保持不覆盖、不改判。
- 修复为四个 V5→Active embedded callback（physical exit、authority、top-update exit、lifecycle）统一进入 `HookScope`；Shutdown 顺序改为 override off → 已准入 callback drain → dispatcher/adapter retire/neutralize → Active restore → secondary drain/target verify → V5 stop/restore → 45-slot verify。确定性 Native 回归同时证明 drain 必须先于 retirement，drain 失败时绝不退休；5 秒超时仍返回 `DrainTimeout` 并禁止卸载。
- 修复后的 create-new verification v2 位于 `out/cs-t0/live-verifications/t0-04-active-callback-drain-shutdown-race-fix-v2.json`，SHA-256 `3AF6CAC129BC0853562CC66A98159D87EFCC0B77618F8520B69F5C6AC8D66353`：27/27，Rust 250/250、cross-language 1/1、baseline Native Debug/Release 各 39/39、Active Native Debug/Release-LTCG 各 262/262、MCP 16/16；`activeInputAllowed=false` 保持不变。该自动回归不能替代 fresh prerequisite、grant/token 与新的 controller 实机授权。

### 前一可用停点：`live-38 control-no-native PASS`

- Source 位于 `artifacts/cs-t0/20260826T084528Z-t0-04-input-disarmed-control-no-native-t0-04-control-no-native-live-38`。Controller 只以 read-only observer 权限验证目标、PID/creation、debugger/forbidden modules、冻结 bundle 和 45 个 slot；没有加载 Active Native/Helper，没有创建共享 mapping、IPC session、grant、claim 或 arm token，也没有发送输入。
- Artifact 固定为 schema 4 / `DISARMED_CONTROL`，只有 1 条 `control-observation` event；accept/reject/apply 均为 0，installed/restored mask 均为 `0x0`，45 个 before/after snapshot 完全相同，ProducerClosed、ClientRetired，21/21 `PASS`、`eligibleForActiveGate=true`。artifact identity 为 `disarmed-control-6335DDB3E879A41CB044C23D9A5CA5660C0B4F902B627C3F78E8627F40EA6C7B`。
- Manifest/events/analysis SHA-256 分别为 `DC7589DC6DDF5F1F499BB120BFA80753C722DAC59B4567A234BF41531A42711B`、`81EE020DB08089155E9BB909150521FC3687B7C58CAF265F02DB7CFBD62C6AD0`、`E701AF3F73577B37AA1D18429B69502FC13AC1B166CDE323A9D26DE4C9DBDFF6`。独立 create-new replay 位于 `out/cs-t0/qa/live-38-control-no-native-exact-replay-20260826`，重新得到 21/21 `PASS` 与相同 trace；源 analysis 未覆盖。
- 停止后 PID 32992 仍响应；未发现 IsaacCsT0、x32dbg 或 x64dbg 模块/进程残留。这是 disarmed control evidence，不证明 suppression、synthetic apply 或 active pipeline 行为。

### 前一停点：`live-37 loopback` 当前语义 `PASS`

- Fresh prerequisite 位于 `artifacts/cs-t0/20260826T052712Z-t0-04-offline-attestation-loopback-t0-04-active-loopback-live-37/prerequisite.json`，SHA-256 `5ED976B2ADBD94E32A1351C2A91F3B7C7938C416CD43A307BF4E15E3F4180AF5`；61 个连续 simulation callback、六槽完整恢复。Grant SHA-256 为 `0A4358EF86608E8733F309A01D2B5DBE4D8553DA15384A23007FB2BA89C49D0C`，token 已 CAS 消费。
- Active artifact 位于 `artifacts/cs-t0/20260826T052838Z-t0-04-input-active-loopback-t0-04-active-loopback-live-37`：787 条 event、32 个 accept、23 个 apply。三次 handoff 分别位于 authority tick `4→5`、`12→13`、`20→21`：已应用 neutral command 以 `original=1.0/effective=0` 屏蔽 movement 输入，预先 accepted 的 pending non-neutral command 在下一目标 tick 恰好一次 apply，并以同 action/device/consumer/generation 得到 `effective=1.0`。
- 原 analysis 只接受同一 command trace 内的 future `suppression`，因此保持 20/21 `FAIL`，SHA-256 `0502FE012877FD19D1D1A02CC46D2156E5AD8389F977D7D210088E73AAECD103`。修复保留该规则并新增严格跨命令 handoff：pending accept 必须先于屏蔽，neutral apply 必须先于 query，下一 tick 恰好一次 pending apply，action/device/query/consumer/owner/apply generation 与 result bits 必须一致；乱序、错 action 或缺少 neutral apply 的回归均 fail closed。
- Release exact replay 位于 `out/cs-t0/qa/live-37-loopback-release-exact-replay-20260826`，得到 21/21 `PASS`、`eligibleForActiveGate=true`，analysis SHA-256 `D4D6B02A37BD0FDFE56A9EDA8EA9BE29F701ABDE59BE7FAC824BFFD202790CAF`。源与 replay 的 manifest/events SHA-256 均分别为 `D3C061F34179B3FA87A0CC78C881913197AE93B0C87A27BC0913B410ECDAA43C`、`FA7ED3B233208D3D2668456070D5E5D7F6611B5F380263A9BB9E6C0763D4122F`；四条 trace SHA 与原采集完全相同。
- Companion V5 为 1,155 条 event、50/50 个完整 loop；17/18 仅缺其冻结七场景之外的 `scenario-evidence`，符合 Active 对非 lifecycle 场景的 companion scope policy。其 manifest/events/analysis SHA-256 分别为 `27BC2E53DFD321F2B3B6003AE22BE3D0DFE8699CE2EC6B237B04DC02FD698BE2`、`B411F54CE5CAF654F065D19A0ED3DE941F91B7BF727E4FE2B1E3B14768838682`、`042142E68BB3286E8D0B8AED6E32380FFB2B055BE7A99CCBD728D7AD23C7D13A`。Active/V5 零 overwrite，45/45 槽逐字节恢复，ProducerClosed、ClientRetired；停止后游戏仍响应，T0 module 与进程均为零。

### 前一停点：`live-36 menu PASS`

- 首次 prerequisite 尝试在安装前因 Player/device ownership 不唯一而安全拒绝；没有 artifact、Hook、grant 或 token 消费。用户将游戏恢复前台、确认仍在可移动单人 Run 并轻触移动键后，fresh offline prerequisite 位于 `artifacts/cs-t0/20260826T051449Z-t0-04-offline-attestation-menu-t0-04-active-menu-live-36/prerequisite.json`，SHA-256 `84EA9F7CBD2EB1E4C37E38152FE13998D89278AD7702502C3238E5E97A89C355`；60 个 simulation callback、六槽完整恢复。Grant SHA-256 为 `207C1F3E915BFD4E1FE03AA2467476D52382BC1008E0E3445F5E031676FEBFB8`，token 已 CAS 消费。
- Active artifact 位于 `artifacts/cs-t0/20260826T051525Z-t0-04-input-active-menu-t0-04-active-menu-live-36`：121,832 条 event、3,931 个 accept、3,929 个 apply，21/21 checks `PASS`、`eligibleForActiveGate=true`。Active 在 loop 7923 恰好一个 `RUN_TEARDOWN`，随后紧邻 `CLIENT_RETIRED`，retirement 后无 active command。
- Companion V5 为 180,170 条 event、8,581/8,581 个完整 loop，18/18 `PASS`、`eligibleForActiveGate=true`；同一 main thread/loop 7923 恰好一对 `RUN_TEARDOWN_ENTER/EXIT`，exit 记录 original exactly once。
- Active manifest/events/analysis SHA-256 分别为 `A23EA4C519D77301FE66BF544DACF774274AE1CC137039BB4A4BD714AE547815`、`D0C2AF2AB620326515EAD020CC212535EC02E061DDE336866636254A6C3C3BEA`、`F9C95A9D5BB2764B36A5EFDC35C59FBEB9BFD3BA158BFA96B7D831E69070F4F6`。Companion manifest/events/analysis SHA-256 分别为 `5CD9D6AB666ADD3D7A4215123B724FE6BB4ED8C2EC32927A0D5B8781D2AFF440`、`FD53F32C6401699A0BBCFD6951D30A6B426D3F2606668A2BC2B404562FC654E4`、`A8DBC47FB6083E6474C08B459098A48D7C27F124255E4926DA9005B8BB88576E`。
- 独立 exact replay 位于 `out/cs-t0/qa/live-36-menu-exact-replay-20260826`，重新得到相同的 21/21 `PASS`、trace 摘要和三文件 SHA。Active `0x1FFF/0x1FFF`、V5 `0xFFFFFFFF`、45/45 槽逐字节恢复，双方 overwrite 均为零，ProducerClosed、ClientRetired；没有 `.finalize-failed`。停止后游戏 PID 32992 仍响应，T0 module 与 Controller/Helper 进程均为零。

### 更早停点：`live-35 room PASS`

- Fresh offline prerequisite 位于 `artifacts/cs-t0/20260826T045827Z-t0-04-offline-attestation-room-t0-04-active-room-live-35/prerequisite.json`，SHA-256 `0A03AEF864016CFBDF618E3ACABA08357A5F2597938A1CF6363259D63353BAF9`；60 个 simulation callback、六槽完整恢复。Grant SHA-256 为 `0E179F99510D1EFC724CD3841A22A64A4FCE30A70A8C2A653C13F523C8269C26`，token 已 CAS 消费。
- Active artifact 位于 `artifacts/cs-t0/20260826T045918Z-t0-04-input-active-room-t0-04-active-room-live-35`：47,272 条 event、1,534 个 accept、1,519 个 apply，21/21 checks `PASS`、`eligibleForActiveGate=true`。Active 恰好一对 `ROOM_ENTER/EXIT`；9 次 V5 owner attempt 对应 18 个 `Lifecycle::None` generic clear，generation timeline 连续且无 stale 复现。
- Companion V5 为 72,081 条 event、3,393/3,393 个完整 loop，18/18 `PASS`、`eligibleForActiveGate=true`；恰有 9 对 `ROOM_CHANGE_OWNER`、1 对 `NEW_ROOM` 和 1 对 nested `ROOM_CHANGE_SIMULATION`，完整 transaction 位于 loop 2720。
- Active manifest/events/analysis SHA-256 分别为 `F9C1D38935E78F02DE747B6E6E237208677BA4D969A53ED7E244D428CDF1511D`、`8E2BA7AC19BD606DF68AC8E46FB082BEA90F002DE60FD1A6F04B8B6C445D9AF8`、`DA0DFFAD19A6745238E386166EF91B572F398D6114872853C42F9706EAA292CB`。Companion manifest/events/analysis SHA-256 分别为 `F41013266F2BEC70EB9DF0C217AFD498C878A40224C848C3A82A7CFC21247A89`、`C117E87A43AD90DA843BC35FD0FCFBB4086B97847164CEC2D81B5EC94344B247`、`268E56E75E801339599FA0B438E33B0DF5D56A46F2A19221B53E55D5E5D6217B`。
- 独立 exact replay 位于 `out/cs-t0/qa/live-35-room-exact-replay-20260826`，重新得到相同的 21/21 `PASS`、trace 摘要和三文件 SHA；源目录已有 analysis 时 analyzer 按 create-new 规则拒绝覆盖，源 analysis SHA 前后保持 `DA0D...92CB`。
- Active `0x1FFF/0x1FFF`、V5 `0xFFFFFFFF`、45/45 槽逐字节恢复，双方 overwrite 均为零，ProducerClosed、ClientRetired；没有 `.finalize-failed`。停止后游戏 PID 32992 仍响应，T0 module 与 Controller/Helper 进程均为零。

### 保留的失败停点：`live-34 room`

- 用户在 `READY` 后手动穿过已开启的普通门；本轮没有调用 IsaacTest 或发送自动键盘输入。Companion V5 捕获到唯一完整的 `ROOM_CHANGE_OWNER → NewRoom → ROOM_CHANGE_SIMULATION → EntityList` transaction，18/18 `PASS`、`eligibleForActiveGate=true`，因此场景动作本身成立。
- Active stream 同时为 9 次 `RoomChange` owner attempt 和其中嵌套的 1 次 `NewRoom` 发布 `ROOM_ENTER/EXIT`，共 10 对 lifecycle 标签。冻结 `lifecycle_transaction_matches` 要求恰好一对由完整 V5 transaction 派生的标签，finalization 因 `active lifecycle labels are not derived from the required V5 transaction shape` 正确 fail closed。该 source 不得登记。
- 主 artifact 位于 `artifacts/cs-t0/20260826T042821Z-t0-04-input-active-room-t0-04-active-room-live-34`，共 7,362 条 Active event；manifest/events SHA-256 分别为 `86B51812CCFA47C07B1DDBD37144C2440C51AAA17F264C2CDA79E0898265C708`、`FA8EE5CE488EBA70652EE5C12910C97A457F29B017F1133106947F17A554CDD9`。Companion manifest/events/analysis SHA-256 分别为 `1B537CC82A773E758D676389758A0F1BD232AF5B41264A6E2B1145735ABA5F44`、`A72A8972FAD47D5CCF60962951083CF89E615330AB2E572CD03F0534F129DD9E`、`3D8CA3F7908A8BC04703A43355838B614F849C40C6A826904CED5CF13AAF7DC6`。
- 不可覆盖的失败诊断位于同名 `.finalize-failed` 目录，manifest SHA-256 为 `15FCD8A59884D64967A1CD7A280FCF5B0A09CFD039F4134BB33A84A5DF7B48E5`。安全恢复成立：Active `0x1FFF/0x1FFF`、45/45 槽、before/after 字节相同、零 overwrite、ProducerClosed、ClientRetired；停止后未发现 T0 module 或 T0 process 残留，游戏 PID 32992 仍响应。
- 后续代码修复没有重分析或改写该 artifact。修复后的自动回归覆盖“多个 owner-only attempt + 单个嵌套 NewRoom”、重复 Active room 标签拒绝和 generic clear 代际推进；最新 verification v2 为 27/27。它只消除了已知实现缺陷，不能替代新的实机 `room` source。

### 前一停点：`live-33 room`

- fresh Active Run 已进入 `Ready`，token 已经 CAS 消费；IsaacTest 只向用户指定的 `DOWN` 方向按住 700 ms，并由 RAII 立即释放，没有其它键盘输入或任何手柄输入。
- 运行中没有出现 `ROOM_ENTER/EXIT`，companion V5 也没有 owner → NewRoom → nested Room update → EntityList 的 room-change transaction；V5 为 17/18，仅 `scenario-evidence` 失败。因此 `room` source 不成立，不能登记为候选或正式 source。
- active artifact 位于 `artifacts/cs-t0/20260825T190822Z-t0-04-input-active-room-t0-04-active-room-live-33`；manifest/events SHA-256 分别为 `30ABCAB951B302C2B8ACB63BC9A4DA26D84F3B656C5451CACF555754E64D4F71`、`2E39CDEDE44EEDEC52BC85E135449278D5DA4A2EBF45CA9F0C41600140EE044B`。不可覆盖诊断副本保留在同名 `.finalize-failed` 目录。
- 安全恢复成立：Active mask `0x1FFF/0x1FFF`、45/45 槽恢复、before/after 字节相同、overwrite 为 0、ProducerClosed、ClientRetired，未观察到 Unsafe 或残留。fresh passive prerequisite 自身为 60/60 `PASS`，六槽安全恢复；其 SHA-256 为 `25D9052CEB5DBB7AC1BEEEA60B796A7AFCEACA9238052A785A6641B63CC49146`，且已随该次执行失去后续复用资格。
- 当前证据只能说明“这次按键没有触发被 runtime 观察到的房间切换”，不能据此断言 hook/runtime 有缺陷，也不能把安全恢复当作 `room PASS`。原计划中的只读逆向诊断已在暂停指令后中止。

### 正式完成路径（已闭合）

新 source 测试、全量 provenance replay、正式 matrix 和最终 gate 均已完成：primary `18/18`、extra `2/2`、A/B repeatability comparison `1/1`、final decision `1/1 PASS`。

恢复工作时按以下顺序推进：

1. **已完成**：修正 Active lifecycle 发布语义，使 ordinary-door 的 owner-only attempt 仍保持输入隔离/代际清理，但只有实际完整 room-change transaction 派生唯一可登记的 `ROOM_ENTER/EXIT`；已补充“多个 owner-only attempt + 单个嵌套 NewRoom”的 Native/Rust 回归测试。
2. **已完成**：Rust、cross-language、Native Debug/Release-LTCG、MCP 与 verification v2 自动门已重新通过；`live-34` 原始 FAIL 保留，未用新语义覆盖或改判。
3. **已完成**：在新的逐场景明确授权下以 fresh prerequisite、grant、token、CAS 和 artifact 重做 `room`；`live-35` 直接得到 Active 21/21、companion 18/18 `PASS` 并安全恢复。
4. **已完成**：在新的明确确认下执行 `menu live-36`，用户手动返回主菜单；Active 21/21、companion 18/18 `PASS` 并安全恢复。
5. **已完成**：执行 deterministic `loopback live-37`；保留原 20/21 FAIL analysis，修正过窄谓词后以 Release create-new exact replay 得到 21/21 `PASS`，无需改写或重采运行时事件。
6. **已完成**：执行 `control-no-native live-38`；只读 observer 生成零 IPC/Native/arm 的 `DISARMED_CONTROL` source，源与独立 exact replay 均为 21/21 `PASS`，45 个 slot 前后相同且无残留。
7. **已完成**：`live-40` 首个 gamepad projection 改为 owner-before-projection bootstrap，verification v2 27/27；随后 `live-41 controller` 与 `live-42 device-switch` 均以 fresh prerequisite/grant/token 取得 Active 21/21、companion 18/18 `PASS` 和一致 exact replay。`live-39`、`live-40` 仍只保留为 FAIL。
8. **已完成**：extra A/B `live-44`/`live-45` 均已采集并 exact replay；首次 semantic repeatability comparison 保留为历史 `FAIL`，严格停止边界尾部归一化的正反回归与新 4/4 `PASS` comparison 已完成。
9. **已完成**：`original-online-disarmed live-46` 以 read-only RPM positive screen 取得 21/21 `PASS`，独立 exact replay 三文件哈希一致；primary 候选达到 18/18。
10. **已完成**：当前 analyzer 对 18 个 primary 与 2 个 extra 完成全量 replay；17 份 analysis SHA 完全一致，3 份只迁移报告说明文字，语义/trace/check/source 均不变；正式 matrix v2 已 create-new 写出。
11. **已完成**：`input-gate` 已绑定 verification v2、正式 matrix 与 repeatability comparison，创建 schema-4 `PASS` decision；`t004Unlocked=true`、`activeInputAllowed=true`。

T0-04 已正式 `PASS`，T0-05 的任务依赖已满足。新的 Local Co-op / Virtual Device 工作仍需建立 T0-05 自己的 ledger、只读前检、自动门和逐次实机授权；T0-04 的通过不自动授权新的写入型实验。

## 1. 2026-08-24 历史停点

2026-08-21 V5 `room`、`keyboard`、`controller`、`device-switch`、`pause`、`menu` 与 `loading` 均已取得可重放 `PASS`。最终 `device-switch` 实验以切换前后两份 RPM snapshot 确认 Player assignment 从 Gamepad unique 严格反转为 Keyboard unique；中间 runtime artifact 捕获同一 main thread/update/loop 内的精确消息分发与 Keyboard/Gamepad 直接刷新序列，18/18 checks PASS 并完整恢复 32 槽。由七份源码 artifact 生成的 V5 loop gate 为 `PASS`，矩阵 SHA-256 `30F2FCE8E5E3684B19967ACFA8CC3A0D21A3ACCE6516911A157FFC71F1CFD05A`，gate SHA-256 `CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA`。该 gate 内的 `activeInputAllowed=true` 只表示 loop 前置条件满足；raw sample/apply 被动闭环和第二次主动授权尚未取得，因此主动输入抑制、合成和游戏内/runtime loopback 继续禁止。

2026-08-22 的无断点只读逆向进一步冻结 InputManager primitive dispatch ABI、per-device binding table 和一个 Keyboard action `0..11` 单快照，同时证明 primitive code 会与 action `12+` alias，不能由 primitive 反推动作。后续磁盘静态分析又冻结 physical poll → common-device-update 返回后的 action-indexed sample、Player owner envelope 与六个 natural consumer call；它们随后进入 schema-3 observation-only runtime，并由后述 v3/v4 四份实机 artifact 取得 passive 4/4 PASS，详见历史归档的 [`input-consumer-research.md`](../../../cs-t0/archive/t0-04-history/input-consumer-research.md)。同轮 3 秒 RPM 主菜单诊断得到 149/149 coherent、0 drop、8/8 checks PASS，但 Player vector 为空，证据类仍为 `RPM_DIAGNOSTIC_ONLY`。主动门状态不变。

2026-08-23 的 active 前置实施保留了错误的 `input-active-catalog-v1.json` / plan v1，并将其状态固定为 `FROZEN_REJECTED_ADDRESS_MAP`：v1 的 Player owner 与六个 natural consumer target 与既有 PASS target snapshots 不符。create-new catalog v2 只修正 ordinal 38–44，SHA-256 为 `A89A74A251EEAA2510FC0C681247ABD6B0A31A7CC3ED60833C78EF36C9B0DC77`；plan v2 SHA-256 为 `FE4FC7ABDE09CBB280DA342BD384AB50107CA948A052AC02BA6ADE37BB1DF3BA`。随后 `input-raw-prereq` 只读重放两份 v3 Keyboard 与两份 v4 Controller PASS source，生成 decision `artifacts/cs-t0/20260823-t0-04-input-raw-prerequisite-4of4.json`，SHA-256 `99BA518991FEA36FE78D91AEA6D91D80634D4FD3B314399C4C4203C1BA27D5AC`。该 decision 只表示 passive prerequisite 满足，固定 `eligibleForActiveGate=false`、`activeInputAllowed=false`；没有执行任何 active runtime 或游戏输入。

同日下一增量闭合 `input-smoke --probe loopback` 的静态 CLI bundle：16 个 active-runtime scenario 必须提供 mode gate、V5 loop gate、raw prerequisite、active plan v2 与非零 64-hex arm token；仅 `scenario=loopback` 的 repeatability source 额外要求冻结 input script。CLI 会只读重放并绑定 target/loop-ledger，严格核对 plan/script SHA，然后输出 `activeInputAllowed=false`、`gameStarted=false`、`nativeInjected=false`、`armTokenConsumed=false` 的安全停点。缺失/重复参数、probe/scenario 错配、v1 或 mutation 均 fail closed。该段记录的是当时的停点；owner/debugger/module observable preflight 后续已完成，token CAS、schema-4 runtime 与 active artifact 仍未实现。

2026-08-24 的下一增量完成 schema-4 公开分析门分派。`input-analyze` 与 `input-compare` 以 manifest schema/evidence class 分类，跨 schema、未知 class 和 passive raw/apply compare 均拒绝；`input-gate` 同时核对 matrix schema 与 `IsaacCsT0-ActiveMatrix-v2` identity，active 路径强制 mode/loop/raw/plan，旧路径拒绝 active-only 参数。matrix v2 固定有序 18 个 primary case 加两份额外 loopback repeatability source，拒绝重复/逃逸路径、primary/repeatability 重用、artifact identity 重用，以及任一 plan/script/hash/eligibility/replay 不一致；两份 repeatability source 的 manifest/events/analysis SHA 也进入 decision provenance。active analysis、comparison 与 gate 输出均为 create-new。该增量只闭合静态 dispatch 与结构门，没有创建 active artifact、comparison 或 decision，也没有进入动态 preflight 或游戏进程；decision 固定 `semanticContractComplete=false`，即使 18+2 结构齐全也返回 `BLOCKED_BY_UNIMPLEMENTED_ACTIVE_SEMANTICS`，完整 18 场景语义与 semantic repeatability 闭合前不能正式解锁。

上一段记录的是本日较早的历史停点；以下状态取代其中的 `semanticContractComplete=false` 和 “offline attestation 未实现”描述。

2026-08-24 本轮继续完成了除工作包 3 以外的实现。工作包 1 新增独立 passive-only `IsaacCsT0-ActiveOfflineAttestation-v1` Rust/C++ IPC、六槽 observer、Helper inject/unload、60 callback analyzer 与 5 分钟 prerequisite；工作包 2 新增 hash-bound arm grant 和 create-new claim CAS；工作包 4 完成 18 场景专用 predicate、companion V5 hash replay 与 semantic repeatability。工作包 5、6、7 的 verification/matrix/gate 编排也已完成，但机器可读 verification 固定 `semanticContractComplete=true`、`runtimeContractComplete=false`、`BLOCKED_BY_DEFERRED_SCHEMA4_NATIVE`。延期的工作包 3——schema-4 active Native、active Helper 与 45 槽事务——没有实现，也没有用 schema-2/3 harness 冒充。

Offline observer 只安装 `lua_pcallk`、`lua_close`、`SteamInternal_ContextInit` 三个 IAT 和 `SteamNetworking006` Send/Available/Read 三个虚表槽；只有 descriptor/import/owner 验证与三槽事务完成后才设置 `steamObserverArmed=true`。正常停止先禁用发布，再恢复 Steam 三槽并 drain，随后恢复三个 IAT 并 drain，最后比较六槽 snapshot、owner 与 read-only preflight。Decision 仅持久化 salted opaque game-root/Lua generation digest 和 salt，不持久化地址；后续 loopback 会重新加载同一 observer、复用 salt 并比较 opaque identity，任一 PID/creation/target/thread/generation/digest 漂移都拒绝。

Arm grant 固定绑定 prerequisite SHA、target、PID/creation、opaque generation、mode/loop/raw/plan SHA、case/scenario 和不晚于 prerequisite 的过期时间。Grant 同目录的 `claim-<TOKEN_SHA>.json` 使用 create-new 原子消费；并发消费者只有一个成功，claim 创建后崩溃也不允许重试复活。当前 Controller 顺序为 static bundle → read-only preflight → passive continuity recheck → grant verification → active verification → CAS；runtime verification 固定 false，因此真实 token 在 CAS 前停止。

自动语义门现在直接从 V5 transaction 推导 pause/loading/room/menu，不信任合成 lifecycle label；pipeline source 强制 companion V5 manifest/events/analysis SHA 与 scenario/thread/loop/QPC 关联，两个 disarmed source 禁止 active IPC/companion 身份。ordered 18+2 template 还要求唯一路径、artifact 和 token，并逐 source 重放 hash。正式 evidence 仍为 `0/18 + 0/2`，没有创建 active source、comparison 或最终 decision。

当前安装的 `isaac-ng.exe` SHA-256 仍为 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`，不等于冻结 `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`，所以本轮真实 CLI 只允许 target mismatch/read-only fail closed；没有恢复或替换游戏文件，没有创建正式 prerequisite/matrix/source/decision，没有消费 token，也没有发送输入。

| 工作项 | 当前状态 | 可以得出的结论 |
| --- | --- | --- |
| T0-01 / T0-02 | `PASS` | 既有目标绑定和 ModeLatch 保持不变。 |
| T0-03 scoped boundary | `PASS` | 已证明原 scoped callback/boundary 的排序、线程、恢复和场景门；不能解释为完整游戏 loop 或真实输入 apply。 |
| T0-03R V5 静态与离线实现 | `PASS_STATIC` | V5 IPC、32 槽 passive runtime、analyzer、恢复和 ABI 门均已实现。 |
| T0-03R V5 实机 loop gate | `PASS`，7/7 | 七个必需 V5 场景均有 gate-eligible PASS；派生 gate 已从源码 artifact 重放并绑定各 manifest/events/analysis SHA。历史 FAIL 与 V1–V4 artifact 未填入 V5 gate。 |
| T0-04 codec/queue/planner | 已实现，自动测试通过 | 只证明纯内存规范和状态机，不证明游戏内 raw sample、target tick 或 apply。 |
| T0-04 witness / RPM | 诊断证据已取得 | 已取得 keyboard 与 stable Gamepad unique-owner 诊断；仍只证明 query/ownership 包络和用户目视相关，永不解锁主动门。 |
| T0-04 observation-only input runtime | keyboard/controller 及 rapid-tap input `PASS` | schema-2 input ring、11 个 input 槽与 companion V5 32 槽已在 Keyboard/Gamepad unique-owner offline/local Run 完成组合安装、采集和恢复；rapid-tap companion V5 仅因场景不在冻结七项内而为预期 17/18 scope FAIL，首次 controller pre-Ready 权限 FAIL 也保留。只观察，不是 active gate 证据。 |
| T0-04 raw/apply passive runtime | v4 静态/自动门 PASS，实机 PASS 4/4 | keyboard/controller held-release 与 edge 四场景均 strict raw 7/7 PASS；edge companion V5 各为预期17/18 scope FAIL，不改变既有V5 gate。所有证据仍不具主动资格。 |
| T0-04 raw prerequisite decision | `PASS`，4/4 source replay | 绑定 v3 Keyboard 与 v4 Controller 的原始 provenance 和 manifest/events/analysis SHA；decision 本身永久不具 active-gate 资格。 |
| T0-04 offline attestation | 实现与自动门完成，正式 live 0/1 | 当前 target mismatch；未创建正式 prerequisite。 |
| T0-04 arm grant / token CAS | 实现完成，生产未消费 | 并发、重复、过期、错 session、既有 claim/output 和崩溃恢复均 fail closed。 |
| T0-04 analyzer semantics | `semanticContractComplete=true` | 18 场景 fixture/V5 binding/repeatability 完成；不代表 active runtime 存在。 |
| T0-04 active verification | `BLOCKED_BY_DEFERRED_SCHEMA4_NATIVE` | `runtimeContractComplete=false`；工作包 3 未实现。 |
| T0-04 runtime input gate | primary 0/18 + extra 0/2 | 尚无 gate-eligible 的 raw → canonical → queue → resolve → apply 实机流水线证据。 |

硬状态分层记录为：

- 冻结 loop ledger：仍为 `BLOCKED_BY_EVIDENCE`，文件和 SHA 不变；它不追写后续实机结果
- 派生 V5 loop gate：`PASS`，7/7；gate-local `activeInputAllowed=true`
- T0-04 全局主动权限：`activeInputAllowed=false`
- 主动输入 suppression、synthetic apply 和游戏内/runtime loopback：`FORBIDDEN`
- V5 loop ledger SHA-256：`8E4C5676358EBBDA831BCC847DE129DF1A9C0D5470F01FAC90F4A298E2E83858`

## 2. 历史实施进展

### 2.1 T0-03R 已完成

1. 已建立独立 `loop-inspect / loop-smoke / loop-analyze / loop-gate` 路径，并与 T0-02 ModeLatch 隔离。
2. V5 passive runtime 固定安装 32 个观察槽：
   - 27 个 rel32 callsite；
   - 2 个 concrete physical-poll vtable slot；
   - 3 个 USER32 IAT slot。
3. hook mask 使用 `u64`，完整值为 `0x00000000FFFFFFFF`；IPC version 为 5。
4. analyzer 已严格检查 sequence、QPC、线程、loop iteration、phase、return RVA、enter/exit 深度、original exactly once、事务顺序、ring overwrite、artifact hash 和卸载恢复。
5. schema-3 首次 canary 暴露并修复两项 Rust 合约缺陷后，全仓 `167/167 PASS`，fmt 与 `clippy --all-targets -- -D warnings` 均通过。
6. Win32 Debug、Release/LTCG Native/Helper 构建及两配置 Native harness 均通过；Release wrapper 已反汇编核对 ECX、ESP、非易失寄存器、plain `ret` 和 original exactly once。
7. V3 keyboard foreground 历史 artifact 已确认稳态 loop/input envelope 和 standard Authority transaction。
8. V4 room artifact 已作为不可覆盖的失败证据，否定错误的普通门 owner 假设；V5 随后换用 `0x2FB51C/0x2FE37C` 候选锚点，并已由第三次 canary 实机确认。
9. V5 首个完整实机窗口已证明 32 槽安装/恢复、V5 IPC、主线程 loop、physical-poll 和 message-pump 在固定目标上可连续运行十分钟；该窗口没有发生 room scenario，不能证明或证伪两个 V5 room-change 锚。
10. V5 第三次 `room` canary 已运行时确认普通门 transaction：同一 main thread/loop 中 owner → NewRoom → nested Room::Update → EntityList → simulation exit → owner exit 严格闭合，18/18 analyzer checks PASS，可进入 V5 gate 的 `room` 槽。
11. V5 `keyboard` baseline 已取得 4,837 个完整 loop、4,837 组 manager/device/keyboard-poll 包络和 2,419 条 steady standard transaction；18/18 analyzer checks PASS，可进入 V5 gate 的 `keyboard` 槽。
12. XInput ownership 已由 305/305 coherent RPM 样本稳定确认；V5 `controller` artifact 又取得键盘/手柄各 4,606 组 poll 包络和 2,303 条 steady standard transaction，18/18 analyzer checks PASS，可进入 V5 gate 的 `controller` 槽。
13. `device-switch` 实机 trace 已定位两种精确合法形状：controller → keyboard 的连续 12 事件直接刷新，以及 keyboard → controller 的 23 事件 manager sample、第二次 Peek 与 direct refresh 组合。Analyzer 仅在 `device-switch` 场景、同一 main thread/update/loop 且 device identity 一致时接受这两种形状；所有历史 FAIL 保持原样。最终 gamepad → keyboard 新采集直接得到 18/18 PASS，可进入 `device-switch` 槽。
14. V5 `pause` analyzer 已从被实机否定的“input-query 缺口”修正为前后夹闭的 steady-standard transaction 缺口，并要求至少 30 次 attempt、完整 passive loop/poll 持续且无 lifecycle 混入；新 artifact 18/18 PASS，可进入 `pause` 槽。
15. `loop-analyze` 已改为真正只读重放；采集侧 `analysis.json` 使用 create-new，已有历史报告不再覆盖。对应回归测试验证重放前后报告字节不变。
16. V5 `menu` artifact 取得 2,795 个完整 loop、885 条 steady standard transaction 和一对严格平衡的 Run teardown，18/18 PASS，可进入 `menu` 槽。
17. 紧接主菜单 Continue 的 V5 `loading` artifact 取得 6,321 个完整 loop、116 条 steady standard transaction 和一对 NewRoom，18/18 PASS，可进入 `loading` 槽。
18. 七份 V5 source artifact 已写入 create-new matrix 并生成 create-new loop gate；gate 状态为 `PASS`、missing scenarios 为空，matrix/gate SHA 分别为 `30F2FCE8E5E3684B19967ACFA8CC3A0D21A3ACCE6516911A157FFC71F1CFD05A` 与 `CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA`。

当前正式产物 SHA-256：

- Controller：`63EEB616E43181C15ED7CAF272962652DD45A45050A2846EA315C2105136FD6D`
- Native：`79FC27358B7A11F0FE86BAEC211D43CF4DC9D0D1141AB7A8DAA7352B0BD6A725`
- Helper：`717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`

### 2.2 T0-04 2026-08-22 被动阶段（历史）

截至该历史检查点，纯内存和自动化部分已经实现：

- `InputStateDraftV1`：24B LE；
- `InputCmdDraftV1`：64B；
- Q15 轴、整数 radial dead-zone `4096`、向零截断、线性重标和单位圆限制；
- unknown bits、`-32768`、非有限 raw、非零 reserved、shoot/aim conflict 的严格拒绝；
- lead `2`、future window `64`、transport/queue capacity `256`、drain budget `64`、dedupe history `4096`；
- duplicate 幂等、conflict 安全退休、stale/expired/too-far/full 不进入纯内存 resolve/apply；这不是对真实游戏世界不变的实机证明；
- 同 target 连续量选择、不同 edge 合并、相同 edge collision 拒绝和快速第二次 rising edge 顺延；
- Rust/Native golden bytes 一致。Golden vectors SHA-256 为 `46D093CCF61439E4D7A25C7B1A058F1B39995BC72E0AE23024B122E51DAF69A0`。

被动人工阶段已观察：

- keyboard `c=0` 的 ownership/query envelope，以及 controller `c=1` 的 witness/query 相关；新增 305/305 coherent RPM capture 已独立建立 Gamepad device key `1` → Player assignment key `1` 的 stable unique join；
- 移动、射击、同时移动射击和快速双点的用户目视相关；
- pause/resume、房间过渡、run teardown/menu 的 envelope 相关；
- keyboard RPM 静置样本为 287/287 coherent、0 drop、唯一 keyboard owner match。

这些均是 `WITNESS_QUERY_ONLY` 或 `RPM_DIAGNOSTIC_ONLY`，不能证明 raw value、canonical bytes、edge、Authority tick、queue decision 或游戏内 apply。

2026-08-22 已实现新的 observation-only passive runtime：

- `ICSI04V1` 保留独立 family magic、升级为 schema 2；固定 80B event、65,536 槽 ring 与 `0x7FF` input hook mask；
- `IsaacCsT0InputInitialize` 在同一 DLL 内启动 companion V5 observer，V5 32 槽和 input 11 槽在同一 patch transaction 中安装并在安装失败时整体回滚；正常卸载先恢复 11 个 input 槽并等待 input wrapper drain，再恢复/排空 V5 32 槽；V5 IPC/事件/分析语义不变；
- 11 槽覆盖 InputManager 三个 primitive query、Keyboard/Gamepad 各三个 logical query，以及两个只作 `LOGICAL_OVERRIDE_DIAGNOSTIC` 的 setter；
- logical 仅持久化 action `0..11`。manager 对最多 32 个 action、每项 4 个 primitive 的 binding table 做 SEH 保护双快照；只有不与 action `12+` alias 的唯一 gameplay 映射可以落盘，其余只增加脱敏 redaction 计数；
- wrapper 只调用 original 一次并原样返回 bool、x87 float 或 void，不修改参数、设备、queue、world 或 `ModeLatch`；bootstrap 未知只透传，冻结主线程后的 wrong-thread 立即 fail closed，但仍不改写 original 调用；
- 每个 run 绑定 input stream、嵌套 `loop-context`、双方 analyzer/hash、11+32 槽 before/after 恢复快照。证据类固定为 `PASSIVE_INPUT_RUNTIME`，永久 `eligibleForActiveGate=false`，`input-gate` 显式拒绝；
- `input-smoke --probe passive` 要求重放派生 V5 gate 并核对 target/ledger identity，不需要也不接受 arm token。其它 probe 的 fail-closed 规则不变。

keyboard、controller 与随后单独确认的 rapid-tap canary 均已在 x32dbg/x64dbg 完全关闭且游戏处于对应 unique-owner 的可操作 offline/local Run 后完成。没有运行或改写既有 V5 7/7 gate artifacts，也没有运行任何主动 probe。下一次实机运行仍需新的单独确认。

### 2.3 T0-04 observation-only keyboard canary PASS

create-new artifact：`artifacts/cs-t0/20260822T073431Z-t0-04-input-passive-keyboard-t0-04-passive-keyboard-canary-01`。

- input stream 为 457,570 条连续事件，21/21 analyzer checks PASS；其中 457,566 条为安全 logical/manager query，另有 4 条 lifecycle；无 action `12+`、wrong-thread、Failure、非零错误、canonical、queue、resolve、apply 或 world mutation；
- input hook 安装/恢复 mask 均为 `0x7FF`，overwrite 为零，producer 已关闭；11 槽 before/after 逐项一致，SHA-256 均为 `D3F8F6A8D41F3097884A8DB1B70318E2E5AE6E476C02F1C7F8CFC6DCA2D7D08D`；
- companion V5 为 271,755 条事件，18/18 PASS，mask 为 `0xFFFFFFFF`，overwrite 为零，末事件为 `HOOKS_RESTORED`；32 槽 before/after 逐项一致；
- 双方 main thread 均为 4976。companion manifest/events/analysis SHA-256 分别为 `F98404508D1F1070713628D779C195290903F768BDDBD79A36621633F813A7CF`、`D04CA1530A7D9A8F2CFDD1EF4BA10FEE488E9FEA8D60A38719EFDC060D688FD5`、`007D5FF88ADB96F184663AB88E16344CBEAF7696A00852126ABCF495C14C2990`，与 input manifest 绑定完全一致；
- Native 正常卸载后游戏进程仍响应，目标进程中无 T0/冲突模块残留。证据类固定为 `PASSIVE_INPUT_RUNTIME`，`eligibleForActiveGate=false`。

### 2.4 T0-04 controller canary pre-Ready FAIL

create-new artifact：`artifacts/cs-t0/20260822T080816Z-t0-04-input-passive-controller-t0-04-passive-controller-canary-01`。

- 前检确认唯一 PID 18460、固定目标/三工件/gate SHA 均匹配，无 x32dbg/x64dbg、debugger attached、冲突模块或 T0 Native 残留；
- RPM 双样本均 coherent，Gamepad platform controller `0`、device key `1` 与 Player device key `1` 为 `UNIQUE_OWNER_MATCH`，Keyboard 不归属 Player；
- 唯一一次 Helper `--inject-input` 以 exit 4 失败，failure reason 固定为 `FAIL independent T0 helper error=4`；源码语义是无法取得精确注入权限的 `OpenProcess`，或随后 `ValidateProcess` 失败；
- input/loop 事件均为零，input install/restore mask 为 `0/0`、V5 mask 为零、main thread 为零，说明没有进入 Native initialization 或安装任何 hook；
- input 11 槽 before/after SHA 均为 `D3F8F6A8D41F3097884A8DB1B70318E2E5AE6E476C02F1C7F8CFC6DCA2D7D08D`；V5 32 槽 before/after SHA 均为 `E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`；两侧均标记 restored；
- 失败后游戏仍响应，模块复查无残留。该 artifact 永久保持 `FAILED`，不得覆盖、改判或自动重试。

同日文档回填期间出现 Codex Windows sandbox `setup refresh had errors`。sandbox 日志将根因固定为对 `D:\projects\isaac_hook_reserach\.git` 应用保护性 deny ACE 时 `SetNamedSecurityInfoW` 返回错误 5；该目录当时由 `BUILTIN\Administrators` 所有且 ACL 继承受保护。修复仅把 `.git` owner 改回当前工作区用户，没有修改 Git 内容或递归重写 ACL；随后普通 sandbox 命令与 `apply_patch` 均恢复。该工具环境事件没有接触游戏或改变任何 artifact。

### 2.5 T0-04 observation-only controller canary PASS

用户重新单独确认并在 sandbox 外授予 Helper 精确注入权限后，create-new artifact `artifacts/cs-t0/20260822T081545Z-t0-04-input-passive-controller-t0-04-passive-controller-canary-02` 完成：

- 前检确认 Gamepad platform controller `0`、device key `1` 与唯一 Player device key `1` 严格匹配，Keyboard 为 no-player-match；固定目标、三工件、gate、debugger 和模块条件全部满足；
- input stream 为 118,812 条事件，21/21 checks PASS，其中 118,808 条为安全 query；state 为 `COMPLETE`、producer closed，安装/恢复 mask 均为 `0x7FF`；
- companion V5 为 170,955 条事件、7,170 个完整 loop，18/18 PASS；mask 为 `0xFFFFFFFF`，最后事件为 `HOOKS_RESTORED`；
- input 11 槽与 V5 32 槽 before/after 均逐项一致；双方 main thread 均为 20256，overwrite 为零，无 sequence gap、wrong-thread、Failure 或非零 error；
- Input manifest/events/analysis SHA-256 分别为 `3E3DFEF719AADCA41FDD5F738D125121394AC924B039D7E91AD9FFE7970B346D`、`A6CF3FB3A6530E4D99D0E1CE7B0DFBAFF699DBAD3A0E263234FD38B19A3411D7`、`B7B750AE3C8F4409F9D2AFB73036F44EBB5C86C9E1AF9F31E70FC5E665038DA3`；companion manifest/events/analysis 分别为 `8CB4621B2F4909509BDFDB4E09A2D5400A10EA106560268698CB24D6A8748E5F`、`9D1D7AD3C4251A36047188CE105B0FE1DFE960BDFC21C89772C38E90F8871C33`、`97FB79D8367877E51613F5CA55E3850C063422B170EE59266940C98C147C27B2`；
- Native 正常卸载后游戏 PID 33176 仍响应，无调试器或 T0/冲突模块残留。没有 canonical、decision、queue、resolve、apply 或 world mutation，`eligibleForActiveGate=false`。

### 2.6 T0-04 observation-only rapid-tap input PASS

Keyboard unique-owner create-new artifact `artifacts/cs-t0/20260822T083151Z-t0-04-input-passive-rapid-tap-t0-04-passive-rapid-tap-canary-01` 完成：

- input stream 为 122,240 条事件，21/21 checks PASS，其中 122,236 条为安全 query；state 为 `COMPLETE`、producer closed，安装/恢复 mask 均为 `0x7FF`；
- 分组快速点按得到 Bomb/action 8 Triggered true 1 次；Item/action 9 Triggered true 4 次、Pressed true 26 次；PillCard/action 10 Triggered true 4 次；Drop/action 11 Triggered true 8 次、Pressed true 77 次；
- companion V5 为 223,306 条事件、11,508 个完整 loop，mask 为 `0xFFFFFFFF`；只读重放 17/18 FAIL，唯一失败为 `scenario-evidence`。rapid-tap 不属于冻结 V5 七场景，因此这是预期 scope FAIL；其 sequence、thread、lifecycle、restore 和 hash checks 均 PASS，不修改既有 V5 gate；
- input 11 槽与 V5 32 槽 before/after 均逐项一致；双方 main thread 均为 20256，overwrite 为零，无 sequence gap、wrong-thread、Failure、非零 error 或模块残留；
- Input manifest/events/analysis SHA-256 分别为 `ACA4F41E5DC71FB607085229C82859DE427AEBF387AE73C9BAA3B9E8ED7D9296`、`2BD2C6426460ED790AD35F4D52D3BE49E00898273A0EAF2D4A396A79175448AE`、`73B03A3B02A313140027A4E0AB41A391481BF8CE1AB0EAC17C334502F69C726B`；companion manifest/events/analysis 分别为 `0DCEA9879CAC51FD743F1879D5C2E411019B475D9478CA0C88D0E4401B1D3489`、`2B068AE34A45472FBD000722F80A42BD235EB92705EDE8AF02AAD00566671A66`、`366936B880DB8EE13169B66F100437C04D260B729093ECD764663D789FC0A2DF`；
- 该 artifact 没有 canonical、decision、queue、resolve、apply 或 world mutation，`eligibleForActiveGate=false`。

### 2.7 T0-04 schema-3 passive raw/apply runtime 自动门完成

固定目标的磁盘静态分析（未启动调试器、未设置断点）最初冻结了 v1 假设：

- Keyboard/Gamepad physical poll slot RVA `0x007A16F0/0x00782CF8` 只建立 sample generation；
- common device update RVA `0x0061F280` 在 `0x0061F307` 调 concrete poll；v1 假设返回后 `device+0xC0/+0xC4[0..11]` 是 action-indexed physical projection；
- Player owner envelope 为 vtable slot RVA `0x0076BDDC` → `Player::Update` RVA `0x00382AF0`；六个 natural consumer callsite 为 `0x00385BC2/0x00385BC9/0x00385BD2/0x00385BD9/0x00385BE0/0x00385BE7`，分别覆盖 movement、bomb、item、pill/card、shooting intent 与 drop；
- offline/local proof 需要同一 Player consumer transaction 内的 concrete-device logical query，不能由 manager primitive、setter 或 OriginalOnline oracle 替代。movement/shooting/bomb/drop 还需批准字段的 before/after witness；action 9/10 没有伪造通用 world witness。

冻结 catalog `cs-t0/archive/t0-04-history/raw-apply-catalog-v1.json` SHA-256 为 `C01E325A592876D0D2C0EC1354EFF1ED72C3B7E4D25E3B4FFAA0212F447A2D0C`；capture plan `cs-t0/archive/t0-04-history/raw-apply-capture-plan-v1.json` SHA-256 为 `DEE4C87298045174F76689D4D5DB4D97AE6F4977303D1E89E84AC598C6847805`。canary-03 后两者保持不可改写，状态为 `FROZEN_REJECTED_HYPOTHESIS`；后续修正均使用 create-new 版本与新 hash。

历史 v2 catalog `cs-t0/archive/t0-04-history/raw-apply-catalog-v2.json` SHA-256 为 `F64FA3FFF87F1B0D1AEA8C4FAE8C849E229459222CAB7CB1B9B1D232E2489266`；capture plan `cs-t0/archive/t0-04-history/raw-apply-capture-plan-v2.json` SHA-256 为 `6A7BC25ECDDCA0D556DE64BECAE8F2799B4AFC1E3D3618DF00E494CC6C3633CE`。历史 v3 catalog/capture plan SHA-256 为 `258549E9B107316FDE02C44580741862476AAB7CCABE57AA1958F06729304F51` / `8908DABFF44E299185438CBC6092F7A3A6B5E308FB95C75BDFA795F0E0042C93`。当前 create-new active v4 catalog SHA-256 为 `E22323047082E6289891393DE0B467CA5038E15EBD45310F39B5051FD0EB13D6`；capture plan v4 SHA-256 为 `203F64825EA24EE4EFA21B731FD2CB8E15AB11DA87145F7654C545B676A45562`。

实现保持 `ICSI04V1` family magic，以 schema 3 增加 128B event 与 65,536 槽 ring；schema 2 和 V5 IPC/stream/analyzer 不变。单次 `--inject-input` 在同一事务内安装 V5 32、legacy input 11、Player owner 1 和 relative consumer 6，共 50 槽。停止顺序为 schema-3 18 槽恢复/drain → V5 32 槽恢复/drain；任何不确定恢复锁存 `Unsafe` 并禁止卸载。

v1 `RAW_VALUE` 曾仅发布双读稳定、finite 的 `+0xC0/+0xC4` action `0..11` 值，primitive 始终 sentinel；canary-03 证明这些值只是 logical setter override/cache。v2 完全移除旧 reader/发布路径，并在 main-thread `InputQuery`、physical-poll original 返回、return RVA `0x0061F30C` 从 concrete cache 与稳定 gameplay binding 双快照重建 action `0..11`。canary-04 又证明 v2 错误复用了 manager 的全 `0..31` alias classifier。v3 将 action-indexed projection owner 检查限制为 gameplay `0..11`：outside alias 不读取/不持久化且不阻断，gameplay↔gameplay alias 仍全部脱敏；manager classifier 继续完整扫描 `0..31`。Keyboard/Gamepad OOB、`0xCDCC/0xCDCD`、非法/负/非 finite axis、非法 deadzone 与不稳定快照仍按 action fail closed；primitive、完整设备状态、指针和 handle 不落盘。

Controller 新增 `input-raw-smoke --probe passive` 四场景；不接受 arm token，必须重放固定 V5 gate 并精确绑定 target、ledger、catalog 和 capture plan。每个 run create-new 保存 schema-3 stream、嵌套 `loop-context`、18+32 target before/after、双 producer close 和 analyzer/hash binding。证据类固定为 `PASSIVE_RAW_APPLY_RUNTIME`、永久 `eligibleForActiveGate=false`；canonical、command identity、queue、resolve、applied state 与 world-write 字段必须为空，`input-gate` 显式拒绝两种 passive evidence class。

初始自动验证结果为 Rust `164/164 PASS`；canary-01/02 后为 167 项；索引化 closure、v2 concrete projection 与 v3 split classifier 为 `168/168 PASS`。v4 cardinality/identity 回归后为 `171/171 PASS`，fmt、全目标 clippy、Win32 Debug、Release/LTCG 与两套 Native harness 全部通过。v4 Release Controller/Native/Helper SHA-256 分别为 `442064DE4D4CEAE30CF57BAE18D37A797B8061E544A775812167DABF9CBF753F`、`03751409B5BAD0D21717317030717E36B9EDEB0905E277C616CC07D5248FA954`、`717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`；v3 build identity 保持历史冻结，不冒充 v4 输出。

受限 Codex filesystem sandbox 内的 MSBuild 复验因读取工作区外 `C:\Users\114514\AppData\Local\Microsoft SDKs` 被拒绝而报 `GetLatestSDKTargetPlatformVersion` 失败；同一命令经明确批准在 sandbox 外运行即通过。该限制不是源码、SDK 安装或工作区 ACL 损坏，不应通过递归改权限或复制 SDK 规避。Rust 验证仍可留在 sandbox 内。

v4 自动实现轮次未启动游戏、未注入目标、未使用 x32dbg；v1/v2/v3 实机结果见后续小节。当前停在新的 v4 `controller-held-release` 场景独立人工确认前，禁止自动开始。后续仍要求 x32dbg/x64dbg 已关闭或分离、游戏位于可操作 offline/local Run，并由用户逐次单独确认。

### 2.8 Schema-3 keyboard-held-release canary-01 历史 FAIL

用户单独确认后，create-new artifact `artifacts/cs-t0/20260822T101325Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-01` 在固定目标、Keyboard unique owner、无 debugger/T0 残留前检通过后启动，并在首个 movement velocity `WorldWitnessBefore` 上 fail closed；未自动重试。

- 根因一：capture plan 已批准 movement 的 `Player+0x1624` direction 与 `Player+0x360/+0x364` velocity，Native 正确发送两类 witness，但 Rust `witness_allowed(0x385BC2)` 只允许 direction。修复仅为该 site 增加 `PlayerVelocityF32x2`；错误 site、action `12+` 和持久 primitive 继续拒绝。
- 根因二：`requiredRaw=0x3` 是 Keyboard/Gamepad 两个 source 的安装契约，不表示 Keyboard-only 场景必须实机观察 Gamepad。本次 `observedRaw=0x1` 正确。修复后 Keyboard 场景要求 owner bit `0x1`，Controller 场景要求 `0x2`，允许额外已知 source、拒绝未知位；catalog/plan/Native 不变。
- raw artifact 持久化 95,866 条 event；events/native 序列因 converter 拒绝批次保留一处 63 序号缺口，overwrite、raw wrong-thread、Native Failure/error 均为零。所有 RAW_VALUE 仍为动作发生前的零值，没有 logical pair，因此 action `0..7` 闭环为 0/8，绝不能从用户操作外推为已证明。
- companion V5 持久化 126,444 条连续 event，最后为 `HOOKS_RESTORED`。schema-3 18 槽 before/after SHA 均为 `6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`；V5 32 槽均为 `E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`，合计 50 槽逐字节恢复。raw producer closed 可证明；失败 companion manifest 无 producer-closed 字段，因此 loop producer close 记为 `UNPROVEN`，不冒充 PASS。
- 游戏 PID 9372 保持响应，目标内 T0 module 与 debugger process 均为零。raw analyzer 仅 field-scope（已持久化部分）和 50-slot restoration PASS，其余 manifest/lifecycle/LIFO/offline closure/companion binding FAIL；证据永久 `eligibleForActiveGate=false`。

关键 SHA-256：raw manifest `B9661481384225938CC80A8B14161B068D20B17759AD2AA9D534697DB9E1C78B`、events `793E51AAE684D46875787BD5C989CAEEA10B4B12583EBF5962B21F301FF99323`、analysis `D6CD8918A3C6A23F9F69C16C45A79AA6346FAD5C2F84D4AB6C7323B221646077`；loop manifest `11C2D3447984BDC082693F57F68F434682FF6F5751FEAB7585B33C824DB30239`、events `750D2E7AC468E9CAC98DD15F0E7C2844ED0CF8EFECCAEC7491ADE92C7594B698`。loop analysis 不存在，因为该 run 不是 complete V5 artifact。

### 2.9 Schema-3 keyboard-held-release canary-02 历史 FAIL

用户再次单独确认后，create-new artifact `artifacts/cs-t0/20260822T104912Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-02` 使用修复后的合约启动。它实机确认 `observedRaw=0x1` 对 Keyboard 场景有效，movement direction/velocity field scope、六个 consumer pair 与批准 witness pair 均能持久化；随后 raw ring 真实 overwrite 4 次并自行 fail closed。Controller 在用户完成回复前已退出，因此没有发送停止 Enter，也没有重试。

- raw 持久化 85,242 条连续 event，loop 持久化 39,428 条连续 event；raw overwrite=4，loop overwrite=0，已持久化部分没有 wrong-thread、Failure 或非零 error。raw producer closed 可证明，失败 companion producer close 仍为 `UNPROVEN`。
- action `0..7` 各有 1,079 条 RAW_VALUE，但均为零，rising/falling 与 logical true 均为零；用户动作发生在窗口退出后，闭环仍为 0/8。六个 consumer site 均为 1,079 对，movement direction/velocity、bomb intent、shoot direction/aim vector/aim state、drop counter 均为 1,079 对；field-scope 与 LIFO PASS。
- schema-3 18 槽 before/after SHA 均为 `6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`，V5 32 槽均为 `E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`；游戏 PID 29008 继续响应，无 T0/冲突模块。
- overwrite 不是阈值或 cursor 假阳性。85,242 条 raw 约 87.9 MiB，事件 QPC 跨度 18.013 秒、约 4,732 event/s；原 writer 直接对 `File` 执行 serde 的大量细碎写，文件落盘约 31 秒，导致下一次 drain 延迟超过 ring 容量。修复将 schema-3 events writer 改为显式 4 MiB `BufWriter<File>`，finish/fail 保持强制 flush 和错误传播；未扩大 ring、未放宽 overwrite 或改 cursor。

关键 SHA-256：raw manifest `F3B3B3EE5CCE90B08DADB88CB78B195F4177EB9D602F14F90E0AA9AAF7A84920`、events `B8C50E17A3ED3E639D4A2364981B354D0AE9A8418A40BF3FF6EE85D2CD663696`、analysis `B740796F0DC1BEFE8940954A6CF283CC32CD3E40DF37ECEB3435570DBAB8C4C2`；loop manifest `29D97C518F66FE695EC5CEF1830512E30D57728D001D5EEF1B8E81FE37350FAC`、events `B07BC0800C7F959B54D24BC55BA519DD152AA8C70935A98207DCFC28F1F2463B`。loop analysis 不存在。

### 2.10 Schema-3 keyboard-held-release canary-03：安全完成并推翻 v1 raw-source 假设

用户第三次单独确认后，create-new artifact `artifacts/cs-t0/20260822T110518Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-03` 在 Keyboard unique-owner offline/local Run 中完成 action `0..7` 的 neutral/held/release 操作，并正常停止：

- raw manifest 与 companion manifest 均为 `COMPLETE`，producer closed，overwrite=0；raw 703,040 条、728,454,396 bytes，companion V5 201,036 条、57,622,597 bytes、9,160 个完整 loop；双方序列完全连续；
- schema-3 18 槽 before/after SHA 均为 `6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`，V5 32 槽均为 `E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`；50 槽全部恢复，Native 已卸载，游戏继续响应；
- 无 wrong-thread、Failure、非零 error、action `12+`、primitive 或 canonical/queue/resolve/apply/world-write 字段。companion V5 analyzer 18/18 PASS；
- action `0..11` 各有 9,160 条 `RAW_VALUE`，总计 109,920 条，但 value、rising、falling 全为零。与此同时 action `0..7` 均观察到 logical true；六个 consumer site 均为 8,843 对；movement direction changed 16、Player velocity changed 2,313、shoot direction changed 16、aim vector changed 16。这证明用户动作、逻辑查询与 natural consumer witness 均真实存在，但 v1 raw source 未反映它们；
- 静态复核确认 common update 先把 `+0xC0` 搬到 `+0xC4` 并清零，Keyboard/Gamepad concrete poll 不写这两组字段；logical pressed 直接按 binding 查询 primitive 且不回写，logical setter 才明确写 `+0xC0[action]`。因此 `+0xC0/+0xC4` 是 override/cache，不是 physical projection。

旧 analyzer 在 70 万条流上出现 O(n²) 退化并且只检查 raw 事件存在，可能把全零 raw 错绑到 logical true。停止 controller 前已确认两个 producer complete、所有槽恢复、文件稳定且 Native 不驻留；随后只终止挂起的离线 analyzer，未触碰游戏。修正版把 poll/witness/consumer/raw binding 建立索引，要求 finite non-zero、同 action/sample/thread/loop/device/controller/selected-device 且早于 logical 的 raw；held/release 还要求同 owner/action 的 inactive→active→inactive，generation 回退、重复或 flag/value 矛盾全部 fail closed。168 项 Rust tests 覆盖全零、NaN、future raw、duplicate pair 与八个 action transition。

索引化 strict analyzer 对该流的离线读取/分析/写入约 1.95 秒，create-new `analysis.json` 明确得到：manifest binding、field scope、native lifecycle、native LIFO、target restoration 与 companion binding PASS，仅 offline-local closure FAIL；总结果 FAIL、`eligibleForActiveGate=false`。关键 SHA-256：raw manifest `4D072807A3304026FD3C62DE6BC373CED1E7986C309674F448E6BD8B6F6F4D48`、events `8A10C592386EE8448791EC569EE744CA3FA43BECAABB3C410A754FBD7ADD615F`、analysis `9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`；loop manifest `747FD2BA4A7E3CD7ECBFB0E4E76AA8328E25F62C3961CB462972F372DC51720E`、events `3A0B1CB84BE399D9A29ACF28A020B0020BAC26F56BBDE3D72E335F10B31AAD8D`、analysis `ADB0B61593B7658EDD7D82E4147286698A6F1E0618AFE52A65F679669BEE815E`。

### 2.11 v2 keyboard-held-release canary-04：movement 闭合，shoot projection 被脱敏

用户新的单独确认后，首次 v2 create-new artifact `artifacts/cs-t0/20260822T121256Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-v2-canary-04` 完成并正常停止；没有自动重试或启动其它场景：

- raw/loop manifest 均 `COMPLETE`；raw 627,338 条、loop 225,849 条、10,555 个完整 loop；event/native sequence 完全连续，overwrite、wrong-thread、Failure、非零 error 为零；raw redaction 计数 168,127；
- 50 槽 before/after 精确一致：schema-3 SHA 均为 `6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`，V5 SHA 均为 `E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`；Native 已卸载、游戏继续响应；
- action `0..3` 各有 10,555 条 `RAW_VALUE`；non-zero 分别为 153/157/171/178，且每项均有 2 rising、2 falling、2 个完整 proof cycle。对应 logical scalar true、consumer transaction 与 movement witness change 全部对齐；
- action `4..7` 没有任何 `RAW_VALUE`，但 logical pressed/scalar true 分别为 172+172、165+165、168+168、185+185；shoot consumer 完整，shoot direction 与 aim vector 各有16次 change。这把缺口限定为 shooting binding 的 v2 projection classification/redaction，不是用户动作、physical poll、logical query、consumer 或 witness 缺失；
- strict raw analyzer 的 manifest/field/lifecycle/LIFO/restoration/companion 六项 PASS，仅 offline-local closure FAIL；companion V5 18/18 PASS。artifact 永久 `eligibleForActiveGate=false`。

关键 SHA-256：raw manifest `7B6C4BFFE03704FF205A7E006390C8057D23DBABB71349C8958970B1D2D324D8`、events `87AA7BD72CC3E3B221590234ABB93339CAD35626149CFDA909A1FE6B5C45DCB9`、analysis `9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`；loop manifest `798BC76A40795073688D72AE9378C49B174E824B9A4F2614CAAC5CE2A95E0A1B`、events `BB4F141972F58759386CE9E19E902730EBBBC8F911E58B308FC5316D4B276C1C`、analysis `B60E0DE64C95F14BD9C15D12F82F71F365762F472BE55F8D11EB0CFDCD5D8B64`。

同构建 binding correlation 显示 shoot action `4..7` 的 primitive `0x107/0x106/0x109/0x108` 分别与 UI action `20..23` alias；edge action `8..11` 也分别与 outside action alias。v2 `ProjectionBindingUniqueToAction` 调用通用 `ClassifyPrimitiveAction` 扫描完整 `0..31`，因此清除这些 gameplay valid bit。该分类器对 manager primitive→action 脱敏正确，但用于已从明确 action slot 出发且不持久化 primitive/outside action 的 projection 属于层级误用：`FULL_SCOPE_ALIAS_CLASSIFIER_MISAPPLIED_TO_ACTION_INDEXED_PROJECTION`。v3 的 split classifier 与 harness 已覆盖 shoot/UI、edge/outside、cross-gameplay、同 action duplicate、mixed alias、Gamepad outside、manager strict 与 payload scope。

### 2.12 v3 keyboard-held-release canary-05：PASS 1/4

用户新的单独确认后，create-new artifact `artifacts/cs-t0/20260822T124555Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-v3-canary-05` 正常完成；未重试或启动下一场景：

- strict raw analyzer 7/7 PASS，companion V5 18/18 PASS；raw/loop manifest 均 COMPLETE，raw producer closed，companion lifecycle 完整；证据仍永久 `eligibleForActiveGate=false`；
- raw 635,300 条、loop 209,973 条、9,893 个完整 loop；event/native sequence 完全连续，gap、overwrite、wrong-thread、Failure、非零 error、action `12+`、primitive 和主动管线字段均为零；
- 50 槽 before/after 精确一致：schema-3 SHA 均为 `6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`，V5 SHA 均为 `E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`；Native 已卸载，游戏继续响应；
- action `0..7` 每项均有 9,893 条 `RAW_VALUE`、2 rising、2 falling、非零 held 样本与2个完整 closure。non-zero 计数依次为175/164/162/176/143/135/131/144；logical、consumer 与 witness 同 sample/action 对齐；
- v2 中完全缺失的 shoot action `4..7` 在 v3 中持续发布并各闭合两次，同时 aliased UI action 与 primitive 没有落盘。action `8..11` 本场各有9,893条零值projection且无边沿，符合本场未操作 edge actions。

关键 SHA-256：raw manifest `DCDE79F430EE5CE99C7C4E28848EC90EC04D25A3B30FF3A5D2F21C3B35473C4F`、events `DC46B2576D6F24E2F78E76BFF078C12A6DA0091D576CAA2026446620AD1DF123`、analysis `73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`；loop manifest `9B4FABC437E4EB187AAB57C94AAA2B46B237C290FFE5D66F557F91DFE111E3B9`、events `45BF068545D0804E5FF3300F2E2A6C5AF5922A4051F20A3043AD90BFC7C94A77`、analysis `3E10A57902246D2F563F68E3950CA7EC9E9C512C143B71A86AD8A74505737323`。

### 2.13 v3 keyboard-edge canary-06：PASS 2/4

用户新的单独确认后，create-new artifact `artifacts/cs-t0/20260822T133033Z-t0-04-input-raw-passive-keyboard-edge-t0-04-raw-apply-keyboard-edge-v3-canary-06` 正常完成；未重试或进入 controller 场景：

- strict raw analyzer 7/7 PASS；raw/loop manifest COMPLETE、双 producer closed，raw companion-binding PASS；证据永久 `eligibleForActiveGate=false`；
- raw 1,744,176 条、loop 494,136 条、23,098 个完整 loop；event/native sequence 连续，gap、overwrite、wrong-thread、Failure、非零 error、action `12+`、primitive 和主动管线字段均为零；
- 50 槽 before/after 精确一致；schema-3 SHA 均为 `6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`，V5 SHA 均为 `E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`；Native 已卸载，游戏继续响应；
- action `8/9/10/11` 各有23,098条 projection，分别完成11/7/6/7个 rising→falling cycle；non-zero为187/135/126/196。四项均同时出现分离的大 inter-rise gap 与 rapid 的10–13 sample小gap，并闭合到对应 bomb/item/pill-card/drop consumer；bomb/drop批准witness发生变化；
- action `0` 有一次32-sample incidental cycle，action `1..7` 保持零；该事实不属于本场 required set，未隐瞒或外推；
- companion V5 analyzer 17/18，仅旧 `scenario-evidence` 拒绝 rapid-tap 不属于冻结七场景；其它结构、线程、生命周期、恢复和hash checks PASS。raw companion-binding按冻结语义要求COMPLETE/restore/hash/structure而不要求该非V5场景整体PASS，既有 V5 gate保持不变。

关键 SHA-256：raw manifest `8B3A657DB4AA4DD0FCA47DFAD16BE4DE14AC871DB9CA0B4F70C1FA1FA21D419A`、events `C81AF80C2956A0D58FFFCFAFE04D90093CAD86185D95EF030CB59BD60B0C165B`、analysis `73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`；loop manifest `D5351C1EA50EBD64765F234B48DB958964F114942D6F09D408F4F5B8DCE33423`、events `414E171811CFB23E83B609A524A547499261647C8FA546881423E846115CD819`、analysis `38205215CE7B228DA318CC1549976CE05D018989A0AC93AD64E9B8F0A261EB29`。

### 2.14 v3 controller-held-release canary-07：同 family diagonal 下永久 FAIL

用户新的单独确认后，create-new artifact `artifacts/cs-t0/20260822T160527Z-t0-04-input-raw-passive-controller-held-release-t0-04-raw-apply-controller-held-release-v3-canary-07` 安全停止并恢复；不重试、不覆盖、不按 v4 重写：

- action `0..7` 均有 non-zero `RAW_VALUE`、2–4 个完整 rising/falling cycle 和足够的 singleton candidate transaction；singleton 数依次为144/104/106/68/200/190/110/105；
- 另有229个 transaction 同时出现多个具体 logical action：movement `0+3` 45笔，shooting `5+6` 88笔、`5+7` 96笔。它们都落在对应 consumer family/site 内，但 v3 的 `transaction_actions.len()>1` 直接全局 FAIL；
- v4 以 create-new identity 冻结等价且更精确的 fail-closed 规则：同 family 且 site 正确的 multi-candidate transaction 整笔跳过、不给任一 action 或 witness 计分；cross-family、wrong-site、only-diagonal、witness reuse 或任一 required action 缺少 exactly-one candidate transaction 均全局 FAIL。这样不会把同一 witness 归给多个 action；
- canary-07 仍是 v3 永久 FAIL，不能作为 v4 2/4 之外的新 PASS。下一次 v4 controller 场景必须重新取得用户单独确认。

关键 SHA-256：raw manifest `98993BB28FFFDFD73987B5B5F08616814ADFF39CF00EA1B976645AA68646F4D0`、events `DB770338CB6138C7FF401A6C5F8B4A2EC31FC01107498CC8B7B3A36DD439529F`、analysis `9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`；loop manifest `F69EEEF540ACD522E8FA127544FA83F0B5070E0048C21ED9613629668664D163`、events `8A63E3A7076086BF45CC3860C56C7A7895198AA24702CDD8E5DBAFF883D23716`、analysis `6EA2CFE61598E6A4795D7FECB8E554AECF18B3BBCAA8E3D18757282B292DC7D1`。target before/after 均为 `6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`。

### 2.15 v4 controller-held-release canary-08：PASS 3/4

用户新的单独确认且 create-new controller0 RPM 前检取得3/3 coherent unique-owner 后，artifact `artifacts/cs-t0/20260822T165606Z-t0-04-input-raw-passive-controller-held-release-t0-04-raw-apply-controller-held-release-v4-canary-08` 以唯一一次注入和唯一一次停机输入完成：

- strict raw 7/7 与 companion V5 18/18 PASS；raw 470,556条、loop 258,053条、11,162个完整loop，双方序列连续，overwrite/wrong-thread/Failure/nonzero-error均为零；
- action0..7各有2–4个完整RF cycle、singleton candidate依次为197/224/179/116/221/187/201/203，严格proof为2/2/2/1/3/1/2/2；
- v4按冻结规则零计分忽略同family movement `0+3` 37笔与shooting `5+7` 7笔；cross-family/wrong-site为零。下方向action3/action7仍有116/203个singleton和1/2个严格proof，难以完全避免的摇杆斜向没有被冒充为单action证据，也没有使required action缺证；
- 18+32=50槽精确恢复；raw before/after为`6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`，V5为`E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`；PID9688继续响应，118 modules内无T0/forbidden残留；
- artifact仍固定`eligibleForActiveGate=false`，不构成synthetic apply或主动授权。

关键 SHA-256：raw manifest `03E6BD9FF9346CE791F52A5219D70210C07258FA342624854B99D393E4E763FD`、events `29B576F8904B6613B305E151A337791E4D1416AAD65FA57884E4431B4A6B9B95`、analysis `73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`；loop manifest `1AD3B1E6443573F79304638D6E88C1B5260E144B436568583C1BF691A1F2FE4D`、events `1EF4DE57BBF2F19C03897AFD9A3942F729FE44608E6EC0BB62B22E557DF48AD2`、analysis `275683A8A2317836F7E2900BDE8C3A5C466629126A7167588F08B74DBD0882CF`。

### 2.16 v4 controller-edge canary-09：PASS 4/4

用户新的单独确认且controller0 unique-owner前检通过后，artifact `artifacts/cs-t0/20260822T171644Z-t0-04-input-raw-passive-controller-edge-t0-04-raw-apply-controller-edge-v4-canary-09` 以唯一一次注入和唯一一次停机输入完成：

- strict raw 7/7 PASS；raw 534,858条、V5 275,545条、11,806个完整loop，双方序列连续，overwrite/wrong-thread/Failure/nonzero-error均为零；
- action8/9/10/11分别完成7/6/7/7个RF cycle，inter-rise gaps均同时包含明显separated段和9–19 loop rapid簇；singleton/proof为7/7、77/77、7/7、162/162；
- action8 bomb witness变化7次；action11有162个raw-bound proof且drop witness总变化169次。action9/10严格使用冻结consumer-only规则，不伪造universal world witness；multi-action、cross-family和wrong-site均为零；
- 意外action0..6全零；action7仅一次6-sample RF cycle，但logical true/singleton/proof均为零，不影响本场required set；action12+、primitive和主动pipeline字段为零；
- companion V5为预期17/18，仅`scenario-evidence`拒绝controller-edge不属于冻结七场景；其余sequence/thread/lifecycle/restoration/hash binding均PASS，不修改既有V5 gate；
- 18+32=50槽精确恢复；raw before/after为`6954AF27473EC548368FB2226509E704E9555B1F239A1C6C46B10F69C104C220`，V5为`E524318A804A6333978CD82EB93A7EF34770A4173036A8EB1115652816314727`；PID9688继续响应且无T0/forbidden残留。

关键 SHA-256：raw manifest `132C6E2E9AC4FE5E8C92B9F13D3E8C25A73B98492D54CA6E81651F568B64A9D7`、events `CC61049B0CD90F39463F26EA2FF65263753419158E6F99B39F4D85AF5A397446`、analysis `73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`；V5 manifest `490B3E7027C5D95BC78D39ED70F7383D62866292DE0E93AFD89FF99F92712BBB`、events `FED45E85D5EAE0BFE0416F94A3FDC36F1DB4EC2C37F2D1182A01E0D34A130010`、analysis `C0C29A74543100AC4E89976732F542375CBDB7DD5E792CD4496BE12A8AA4864D`。

2026-08-22 新增 `20260821T171600Z-t0-04-rpm-current-p18040-passive-01`：149/149 coherent、0 unstable/read-error drop、8/8 diagnostic checks PASS。该进程位于主菜单，只枚举 Keyboard 且 Player vector 为空，因此 ownership 为 `KEYBOARD:UNKNOWN_NO_PLAYER_MATCH`；它只证明当前只读观察器在 debugger-attached 主菜单状态下仍保持 coherent，不填入任何 gate。

## 3. T0-03R 新掌握的信息

### 3.1 顶层事件循环不是 Authority tick

- `0x531231 → 0x6266A0` 是 top-level loop clock/header 锚。
- `0x5312EC → 0x5AB6D0` 是 update driver；`0x5312F1 → 0x5555C0` 是 render owner。
- case04 观察 35,988 个 top-level iteration，但 `0x555231 → 0x2FADC0` 只有 17,994 次。
- V2 foreground 同样为 10,864 个 loop、5,432 个完整 standard transaction。

因此 top-level/update/render 约 60 Hz，而 broad gameplay/Authority attempt 约隔轮一次。`loopIteration` 不能直接用作 `server_tick`。

### 3.2 Win32 message pump 已归属到 loop-local update

- generic pump owner 为 `0x65E660`。
- `PeekMessageW` 的 register-indirect call 位于 `0x65E68A = FF D7`，正确返回 RVA 是 `0x65E68C`。
- case04 中该返回点与 top-level update 一一对应，并处于同一主线程 update scope。
- Translate/Dispatch/follow-up Peek 的返回点分别为 `0x65E6D6 / 0x65E6E1 / 0x65E6F0`。

这证明窗口消息处理属于 loop envelope，但消息次数仍不能代表 Authority tick；其它 pump candidate 仍需分类。

### 3.3 真实 physical-poll 包络已定位

已冻结：

```text
0x554F7D -> 0x61FC00 InputManager update
0x61FC97 -> 0x61F280 per-device common update
0x61F30A -> vtable +0x78 concrete physical poll
return 0x61F30C
```

- Keyboard vtable `0x7A1678`，poll target `0x66C650`。
- Gamepad vtable `0x782C80`，poll target `0x622940`。
- V3 keyboard foreground 有 7,943 组完整 manager → device → poll 包络。
- keyboard `InputDeviceBase+0x0C` 与 player `+0x1618` 唯一匹配为 key `0`。
- controller ownership capture 以 305/305 coherent、0 unstable/read drop 证明 Gamepad key 与 Player key 持续唯一匹配；同窗口 keyboard 明确为 `UNKNOWN_NO_PLAYER_MATCH`。

同时推翻了两个旧假设：

- `0x554F84 → 0x55B310` 不是 physical sample，降级为 controller-assignment candidate。
- `+0x30/+0x34/+0x38` 是 logical pressed/triggered/scalar consumer，不是每帧本地 physical sample 必经点；用户正常输入时这些 hook 仍可零命中。

`0x5551D8` 目前只确认是条件字段复制，不能命名为 input apply。

### 3.4 Authority 事务必须按显式 owner 分类

稳态 standard transaction 已运行时确认：

```text
0x555231 outer attempt
  -> 0x2FADFA predicate=false
  -> 0x2FAE09 body anchor
  -> 0x2FBA4D Room::Update
  -> 0x403E70 EntityList
  -> 0x2FBBA5 MC_POST_UPDATE tail
```

- V2 foreground：5,432 个完整事务。
- V3 keyboard foreground：3,791 个完整事务。

另有两类不能与 standard 混写的事务：

1. alternate/run transaction：`0x554D5E` owner、`0x559B21` state apply、`0x559B4D` room、`0x559B82` completion，以及五个互斥 activation outcome；当前只有 inactive owner cadence，尚无完整 active transaction。
2. lifecycle FSM：`0x2FB4F0 → 0x52F1C0` owner、`0x52FC77 → 0x402980` nested simulation；当前是静态确认、运行时 armed non-hit。

owner-only attempt 合法但不能计作 Authority mutation。

### 3.5 捕获启动存在 bootstrap 中段竞态

V3 首次失败证明：安装可能发生在已有 loop 的中段，下一次命中的 hook 未必是 LoopClock。修正后的规则是：

- 主线程未冻结时，非 Clock wrapper 仅 original-once passthrough，不发布 phase，不报告 WrongThread；
- 只有下一次 LoopClock 可以冻结 main thread 并发布 iteration 1；
- 冻结后真正的 wrong-thread 仍立即 fail-closed；
- 整个 capture 没有 LoopBegin 时仍必须失败。

### 3.6 V4 假设被实机证伪，V5 普通门路径已纠正

V4 曾把 `0x2FB226 / 0x34584D` 当作普通门 owner/simulation。真实 V4 room capture 中两槽完整安装却均零命中，而同一 loop 出现一对无已知 owner 的 `NewRoom → EntityList`，因此 analyzer 正确 FAIL。这两个 callsite 本身有效；被推翻的只是“它们属于本次普通门路径”的假设，当前标签为 `RUNTIME_ARMED_NONHIT_V4_RETIRED`。

该特殊路径只有在 `root+0x26584 == 0x1F && [edi] == 0x0B` 时才走 `0x345842 NewRoom → 0x34584D Room::Update`；否则走 `0x345856 NewRoom`，函数内部没有紧邻的 Room::Update。两个条件字段语义仍为 `UNKNOWN`，不能命名为 monster count、clear 或 visited。

重新扫描后，普通门静态链为：

```text
0x2FB51C -> 0x4318A0 room-change owner
  -> 0x430950 -> 0x430BBF -> 0x2FE2F0
  -> 0x2FE371 -> 0x33FC80 -> 0x33E0A0
  -> 0x33F5B9 NewRoom
  -> 0x2FE37C -> 0x402980 Room::Update
  -> 0x403E70 -> 0x4186C0 EntityList
```

V5 事件 63–66 因此改为 `ROOM_CHANGE_OWNER_ENTER/EXIT` 与 `ROOM_CHANGE_SIMULATION_ENTER/EXIT`。第三次 canary 已在 loop 27012 运行时确认这四类事件严格包围唯一 `NewRoom → EntityList`；owner 共命中 9 次，其中 1 次形成完整 room-change transaction，其余 8 次是 analyzer 允许的 owner-only attempt。V5 `room` 场景因此 PASS，但不能外推为其余六个场景或完整 Authority mutation envelope 已闭合。

两个 V5 候选锚的冻结契约为：

- owner：`0x2FB51C`，bytes `E87F631300`，target `0x4318A0`，return `0x2FB521`，ECX=`root+0x1B83C`，无栈参数，plain `ret`；
- simulation：`0x2FE37C`，bytes `E8FF451000`，target `0x402980`，return `0x2FE381`，ECX=`root+0x18300`，无栈参数，plain `ret`；
- 事件 63–66 的 phase 均为 `ROOM_LIFECYCLE`。

其控制流与 ABI 为 `STATIC_CONFIRMED`；它们对 V4 trace 的解释为 `RUNTIME_CORRELATED_STATIC_CHAIN`。随后 V5 第三次 canary 已取得真实命中并闭合普通门 transaction，见第 7 节对应 PASS artifact。

### 3.7 有怪与无怪不改变普通门外层路径

在上述普通门路径中，`0x2FE37C → Room::Update` 在 NewRoom 返回后无条件执行，`Room::Update` 的正常路径又在 `0x403E70` 无条件调用 EntityList update。

因此有怪/无怪至少不会改变 `0x2FE37C → 0x402980 → 0x403E70` 这一外层调用包络；空房仍应产生一对 EntityList enter/exit。差异会出现在 entity-list 内容及其内部工作，当前证据不排除怪物状态还影响 Room::Update 的其它内部处理。first-visit、revisit、clear 和门类型可能改变初始化细节，但相关 descriptor/transition bits 的具体语义仍未冻结。

## 4. 逆向交接与后续开放项

原清单中的 post-poll action projection、Player ownership、nested concrete logical query、六个 natural consumer transaction、catalog/plan 版本化修订和实机 proof 已由 schema-3/4 passive 与 active gate 闭合，不再属于“仍需完成的 T0-04 逆向”。完整结论、RVA/ABI、已推翻假设和证据入口已收敛到 [T0-04 逆向交接](./t0-04/isaac-cs-t0-04-reverse-handoff.md)。

后续未知项按任务归属：

- T0-05：local co-op 加入/退出、第二玩家创建、controller assignment、HUD/index、`ClientId -> LocalPlayerOrdinal -> Player*` 和 virtual device 生命周期。
- T0-06 或更晚：`0x33E0A0`/`0x402980` 其余 caller 分类、first-visit/revisit/clear/door-type、完整 alternate/lifecycle、spawn/remove/entity mutation、其它 message-pump candidate、实体身份和状态复制。

这些开放项不反转 T0-04 `PASS`，也不能由 T0-04 的单 Player、OriginalOnline 或历史静态候选外推。

## 5. 测试完成状态

### 5.1 T0-03R V5 loop gate

V5 必需矩阵为 7 个场景，当前为 7/7：

- keyboard：`PASS`
- controller：`PASS`
- device-switch：`PASS`
- pause：`PASS`
- loading：`PASS`
- room：`PASS`
- menu：`PASS`

focus 按用户确认的“失焦自动暂停”行为移出必需矩阵；没有用 focus skip 替代 pause。V3/V4 和历史 V5 FAIL artifact 只作不可覆盖的历史证据，均未进入 V5 gate。create-new matrix 位于 `artifacts/cs-t0/20260821-t0-03r-v5-loop-matrix-7of7.json`，派生 gate 位于 `artifacts/cs-t0/20260821-t0-03r-v5-loop-gate-7of7.json`。

### 5.2 T0-04 input gate

schema-3 passive raw/apply matrix 已为 4/4 `PASS`：`keyboard-held-release`、`keyboard-edge`、`controller-held-release`、`controller-edge`。四份 artifact 仍永久 `eligibleForActiveGate=false`；它们只作为主动门的 prerequisite source。

正式 Schema-4 active matrix 已为 primary `18/18`、extra repeatability source `2/2`，pair comparison 4/4 `PASS`。全量 provenance replay 的 20 份根 analysis 全部 `PASS` 且 `eligibleForActiveGate=true`。

以下关键链已由正式证据闭合：

```text
raw sample
  -> canonical 24B
  -> send/receive
  -> accept/reject
  -> drain-before-resolve
  -> target Authority tick
  -> adapter apply exactly once
  -> world witness
```

### 5.3 自动化残余覆盖

成功安装/卸载、prewrite fail-closed 和中途 Apply 失败后的 rollback 均已有动态 harness。2026-08-22 新增两种确定性 fault injection：两个 patch 共享同一 relative-call site，使第二次 Apply 在首个写入后以 `ERROR_INVALID_DATA` 失败；两个 pointer patch 共享同一 slot，使第二次 Apply 在 relative 与首个 pointer 均写入后失败。schema-2 组合 input runtime 覆盖 43 槽完整恢复、V5 段成功后 input 段失败的整体回滚，以及 input bool/float/void ABI、original-once、bootstrap/wrong-thread 策略；schema-3 又覆盖 50 槽事务、consumer/item ABI、physical-poll return/phase、Keyboard/Gamepad concrete projection、按 action alias/特殊映射/非法轴脱敏、edge history、分阶段 drain/恢复和 schema2/3 fail-closed dispatch。Win32 Debug 与 Release/LTCG harness 均通过。自动门本身不替代游戏内 raw/apply 证据；这些证据现已由正式 18+2 matrix 独立闭合。

历史 Release preflight 曾按设计停在 `derived V5 loop gate is valid, but loopback remains blocked until raw/apply passive evidence and a separate active-experiment authorization are recorded`。后续逐场景授权、18+2 source、comparison、provenance replay 与 final gate 均已完成；该历史停点与旧 build identity 保留，不追写既有 V5 source artifact。

## 6. 下一步

T0-04 的后续清单现为：

1. V5 loop gate 保持 7/7 `PASS`，冻结 ledger 与历史 FAIL 不覆盖。
2. passive raw/apply 保持 4/4 `PASS`，但四份 prerequisite artifact 仍不具 active-gate 资格。
3. 18 primary、2 extra 与 comparison 已正式登记；20/20 provenance replay 通过。
4. verification v2、matrix、repeatability 与 final decision 的 create-new 文件和 SHA 已固定。
5. 最终 gate 为 `PASS`，`t004Unlocked=true`、`activeInputAllowed=true`，T0-04 无剩余实机或登记测试。
6. 按任务依赖，下一阶段可进入 T0-05；这不覆盖 T0-04 历史 artifact，也不自动授权任何新的实机修改。
7. T0-05 的阶段任务、证据门和退出条件见 [T0-05 专属研究计划](t0-05/00-overview/isaac-cs-t0-05-research-plan.md)。

## 7. 关键不可覆盖证据

### V3 keyboard historical PASS

`artifacts/cs-t0/20260820T133127Z-t0-03r-loop-passive-keyboard-t0-03r-v3-keyboard-foreground-02`

- 173,964 连续事件；
- 7,943 个完整 loop/input envelope；
- 3,791 个 steady standard transaction；
- 30 槽完整恢复；
- analysis SHA-256 `111E34B5A8CC0D1FF4462A4E14B6D75FC24BE6CA83BDE78F3518BAD7C1D2B260`。

它证明历史 V3 keyboard envelope，不能进入 V5 gate。

### V3 room missing-owner FAIL

`artifacts/cs-t0/20260820T134130Z-t0-03r-loop-passive-room-t0-03r-v3-room-transition-01`

- 64,921 连续事件；
- 3,044 个完整 loop；
- loop 1838 首次暴露没有已知 simulation owner 的 `NewRoom → EntityList`；
- 30 槽完整恢复。

它证明当时的 transaction owner 集合不完整；后续 V4/V5 必须保持 fail-closed，不能把裸 NewRoom 或 ConditionalAttempt 泛化为 Authority owner。

### V4 room preserved FAIL

`artifacts/cs-t0/20260820T174153Z-t0-03r-loop-passive-room-t0-03r-v4-room-transition-01`

- 57,130 连续事件；
- 2,768 个完整 loop；
- 32 槽完整恢复；
- V4 事件 63–66 全部 armed non-hit；
- analysis SHA-256 `33C2584241C5924B7C0B7B4C2946B03FBB20EFC12D84F651219AF1DE76886DA8`。

它证明 V4 普通门假设错误，不能删除、覆盖或改判。

### V5 pre-install permission FAIL

`artifacts/cs-t0/20260821T054422Z-t0-03r-loop-passive-room-t0-03r-v5-room-ordinary-door-canary-01`

- Helper 在 `OpenProcess`/目标验证阶段返回错误码 4；
- Native mapping 保持 `Empty`，事件数和 hook mask 均为零；
- before/after SHA 同为 `5DEBBE254D2C8A94211C2B55309F40078D335058AAE9F6E51A32484408890965`；
- manifest SHA-256 `58146B8FF27DCAC1873A4943D66A00EF48088438A7C3D0BCAA85A09D1DF1B7C8`。

它只证明受限执行环境阻止 Helper 取得注入所需进程权限；没有 hook 被安装，不能作为 V5 runtime 或 room 证据。

### V5 bounded no-transition FAIL

`artifacts/cs-t0/20260821T054618Z-t0-03r-loop-passive-room-t0-03r-v5-room-ordinary-door-canary-02`

- 664,152 条连续事件、35,993 个完整 loop；
- 35,993 组三层 keyboard physical-poll 包络，52,265 条 message-pump 事件；
- 32 槽完整安装并恢复，mask 为 `0x00000000FFFFFFFF`，无 overwrite/failure；
- `NewRoom`、`RoomChange owner/simulation`、`EntityList`、standard room/tail 均为零；
- 仅 `authority-evidence-nonempty` 与 `scenario-evidence` 两项检查失败；
- manifest SHA-256 `7E55155DD3005704ECDF5FCF3D63434DDFC01497247AEDFE305797621513BFDD`，events SHA-256 `292AF8D1CB87652E3BBB11F9818FC89F7708DC0C2F8EA4116C6A0135A79938F0`，analysis SHA-256 `DAE1FC62AF2A97BCEE6C74E4F5A41BD0B8C74BECEACD19AB944F4F945A11CA9A`。

它证明 V5 runtime 可以在固定目标上连续、安全运行并恢复，但窗口没有执行声明的 `room` 场景；不得把四个 V5 槽的 armed non-hit 解释为普通门静态链被证伪，也不得进入 7-case gate。

### V5 ordinary-door room PASS

`artifacts/cs-t0/20260821T060539Z-t0-03r-loop-passive-room-t0-03r-v5-room-ordinary-door-canary-03`

- 558,237 条连续事件、30,068 个完整 loop；
- loop 27012 的 sequence 499028–499035 严格为 owner enter → NewRoom pair → simulation enter → EntityList pair → simulation exit → owner exit；
- owner enter/exit 各 9 次，nested simulation、NewRoom 各 1 对；其余 8 次是合法 owner-only attempt；
- 90,204 条 input-query envelope 事件、44,732 条 message-pump 事件；
- 18/18 analyzer checks PASS，`eligibleForActiveGate=true`；
- 32 槽完整恢复，无 overwrite/failure；
- manifest SHA-256 `5C9088384C784F571BAA9894DCD2BA0DA7BA7897AC3C9608BD4D9753D38F10AB`，events SHA-256 `3BDCC4B3B3B5996C8DAE1345FD2FE495A6016830076FA4FA8B66F60E979D5CD6`，analysis SHA-256 `59E60B7775155817ACC7305EB7D7BE11CF84FE5EE03E6472E27CFB9A2AC887C1`。

它正式填入 V5 loop gate 的 `room` 槽并确认 V4 缺失 owner 已由 V5 锚闭合；它不覆盖历史 FAIL，也不解锁主动输入。

### V5 keyboard foreground PASS

`artifacts/cs-t0/20260821T064433Z-t0-03r-loop-passive-keyboard-t0-03r-v5-keyboard-foreground-01`

- 105,441 条连续事件、4,837 个完整 loop；
- 4,837 组 InputManager → InputDevice → keyboard physical-poll 包络；
- 2,419 组 standard room → EntityList → MC_POST_UPDATE transaction；
- 14,511 条 input-query envelope 事件、8,692 条 message-pump 事件；
- 18/18 analyzer checks PASS，`eligibleForActiveGate=true`；
- 32 槽完整恢复，无 overwrite/failure；
- manifest SHA-256 `77745F945479B31CFF1C896374A78BDDABC22F8E66F36D6BABBE57499FB71F33`，events SHA-256 `7227798DA5DDF5148B36B453D94432CD9EFFFFFD52B872E6EDCD0D7E183F8DA4`，analysis SHA-256 `465C8D22ED93A087E0F792C865424516CDFFA7B153B5A6879D34087649287891`。

它正式填入 V5 loop gate 的 `keyboard` 槽；其 physical-poll 事件只证明 concrete keyboard device 的每帧包络，不读取具体按键值，也不等于 T0-04 raw/canonical/apply 证据。

### Stable Gamepad ownership RPM diagnostic

`artifacts/cs-t0/20260821-t0-04-rpm-controller-active-p12124-v1`

- 305/305 coherent samples，0 unstable/read-error drop；
- 每个样本均为 `GAMEPAD:UNIQUE_OWNER_MATCH`，同时 keyboard 为 `UNKNOWN_NO_PLAYER_MATCH`；
- analyzer 8/8 diagnostic checks PASS；
- manifest SHA-256 `AB8F1913888FA5E46D9DF6CBD3591A17AEEAEE32624313F8B6896DA06F099723`，events SHA-256 `202FD38AF76D0DE4A6DC342106BFBE84E79F98C796D0E65806BAF4D16707239E`，summary SHA-256 `4C5CFC658F9C7E16D71D72651ADB8473B60A196D433FDA34B6B1C804EF8EAD9C`。

它冻结 stable XInput device → player ownership 诊断，但 evidence class 仍为 `RPM_DIAGNOSTIC_ONLY`，`eligibleForActiveGate=false`。

### V5 controller foreground PASS

`artifacts/cs-t0/20260821T065234Z-t0-03r-loop-passive-controller-t0-03r-v5-controller-foreground-01`

- 117,334 条连续事件、4,606 个完整 loop；
- keyboard 与 Gamepad physical-poll 各 4,606 次，Gamepad device key 为 `1`；
- 2,303 组 standard room → EntityList → MC_POST_UPDATE transaction；
- 18/18 analyzer checks PASS，`eligibleForActiveGate=true`；
- 32 槽完整恢复，无 overwrite/failure；
- manifest SHA-256 `2537F96DD0F44FB0921B18E950C57621609C8BD992C74EB070E2AC57163BBF63`，events SHA-256 `D642E6F66AC4E6484BFFC73B45C0493DE59EC0A508C129DE22B3A0AED43FD110`，analysis SHA-256 `9340A336294926A25909BEE84FECEADAF17862B83655C4D53034FD70267193AF`。

它正式填入 V5 loop gate 的 `controller` 槽；Gamepad ownership 来自独立 RPM 诊断绑定，runtime artifact 本身只记录 concrete poll identity，不读取手柄按钮或轴值。

### V5 device-switch 设置切换：历史 FAIL、analyzer 修正与最终 PASS

`20260821-t0-04-rpm-device-switch-pre-gamepad-p12124-v1` 在切换前取得 123/123 coherent、0 drop，结果为 Gamepad unique owner / Keyboard no player match；切到键盘后的 `20260821-t0-04-rpm-device-switch-post-keyboard-p12124-v1` 同样取得 123/123 coherent、0 drop，ownership 结果严格反转。两者都是 `RPM_DIAGNOSTIC_ONLY`，只证明人工切换窗口两端的 assignment snapshot。

中间 runtime artifact `20260821T070732Z-t0-03r-loop-passive-device-switch-t0-03r-v5-device-switch-settings-controller-to-keyboard-01` 有 208,371 条连续事件和 8,310 个完整 loop，32 槽完整恢复。采集时旧 analyzer 只有 `balanced-depth` 失败：loop 2984、sequence 74981–74992 出现主线程 `Peek(0x65E68C) → Translate(0x65E6D6) → Dispatch(0x65E6E1) → follow-up Peek(0x65E6F0)`，随后不经 InputManager 依次执行完整 Keyboard 与 Gamepad device-update/physical-poll pair。这是设置消息分发后的合法双设备直接刷新，而非未配对深度。

修正后的 analyzer 首先只接受上述 12 事件形状。随后 `20260821T151933Z-...keyboard-to-controller-03` 暴露第二个严格合法变体：四个 message 事件后先出现完整 InputManager → Keyboard → Gamepad sample，manager exit 后再出现 `Peek(0x65E68C)` 和 Keyboard → Gamepad direct refresh，共 23 个连续事件。Analyzer 现只在 `device-switch` 场景、同一 main thread/update/loop 且 manager/direct 两段 device identity 一致时额外接受该精确形状；偏移、缺事件、乱序或其它场景仍失败。该次采集的历史 `analysis.json` 保持 `FAIL`，SHA-256 `F9473AD5D8152AC5D5CF68BCF5B348C2E308D358D695C234DD64D940E616945C`；修正后只读重放 18/18 PASS 且没有改写它。早期 12 事件 artifact 的历史 FAIL 也保持 SHA-256 `EECB3B9830B78368E387D2A0A96E26A872AA9937DD9A489293913CE0E651D039`。两者都不进入 gate。

最终 gate 采集以 `20260821-t0-04-rpm-device-switch-gate-pre-gamepad-active-p12124-v5` 开始：123/123 coherent、0 drop，`GAMEPAD:UNIQUE_OWNER_MATCH`、`KEYBOARD:UNKNOWN_NO_PLAYER_MATCH`。中间 runtime artifact `20260821T152754Z-t0-03r-loop-passive-device-switch-t0-03r-v5-device-switch-gate-gamepad-to-keyboard-04` 直接得到 18/18 PASS：85,048 条连续事件、3,481 个完整 loop、17,409 条 input-query 包络事件、5,315 条 message-pump 事件和 1,104 条完整 standard transaction。精确 12 事件刷新位于 loop 2532、sequence 62922–62933；全部属于同一 main thread/update/loop。Hook mask 为 `0x00000000FFFFFFFF`，无 overwrite/failure，before/after SHA 同为 `5DEBBE254D2C8A94211C2B55309F40078D335058AAE9F6E51A32484408890965`。Manifest/events/analysis SHA-256 分别为 `F4B11B584FA56BC956B30ED014B8FF778095BB3A3161E92CF54C7956331B0AEB`、`37EC4976C2B4956187CFBA773C3995B17812B0F05CAAE9313AA774A7BDADC708`、`3A617EA26508F5942A90C2FE1A8866606F5FBC26F0BA703ED79A4B718243A1C3`。

切到键盘后的 `20260821-t0-04-rpm-device-switch-gate-post-keyboard-p12124-v4` 同样为 123/123 coherent、0 drop，结果严格反转为 `KEYBOARD:UNIQUE_OWNER_MATCH`、`GAMEPAD:UNKNOWN_NO_PLAYER_MATCH`。前置 RPM 的 manifest/events/summary SHA-256 为 `43717279CFAE80BDDBA2E543FF46CD57C0D961FAE0631DB2248266A8B1B545AD`、`D35E9A94EB4A576E393FCEB85A9428F5E7897AA7ADBF02FC7F5589349E84C928`、`2F2D1C4C5533975324926FAB688E08B2DDC927A34E22E7D508C922D72A57177F`；后置分别为 `EF336205EFE216F8736618A9A88B5401F928F734462A6C657B1910AA07692DCC`、`1FD67782D5ED523B83B1A6082910C35AD993007556568DC15373975440458FD2`、`60983AF928087E11B60DECB34E6EE7FB439939B3CF8AF9985E99849C696E8B70`。两份 RPM 仍只属于 `RPM_DIAGNOSTIC_ONLY`；与 runtime artifact 共同固定切换窗口两端 assignment，但不证明 raw/canonical/apply。

### V5 pause 判据修正与 PASS

首次 `20260821T091050Z-t0-03r-loop-passive-pause-t0-03r-v5-pause-bracketed-gameplay-01` 证明 pause 期间 concrete physical poll 每 loop 继续执行，否定旧的“input-query 缺口”假设；采集时仅 `scenario-evidence` 失败，其历史分析保持 `FAIL`，SHA-256 为 `1C9F1B78032903C6CFE2C54D4492C95D489D5877C49BD5028D541E15B6C33650`。Analyzer 随后改为要求：steady standard transaction 在前后均恢复，中间至少连续缺失 30 次 attempt；该窗口的每个 top loop 仍有 update/render/physical poll，且不能混入 NewRoom、Run teardown、room-change、room-lifecycle 或 alternate simulation。正向、缺 poll、混入 lifecycle、未夹闭四组测试均已覆盖。

新 artifact `20260821T092837Z-t0-03r-loop-passive-pause-t0-03r-v5-pause-bracketed-standard-gap-02` 直接得到 18/18 PASS：110,700 条连续事件、5,369 个完整 loop、16,107 条三层 input-envelope 事件、8,297 条 message-pump 事件；2,685 次 standard attempt 中有 1,854 条完整 transaction，并观察到两个前后夹闭缺口（loop 3431–4589，共 580 次 attempt；loop 2305–2805，共 251 次）。Manifest SHA-256 为 `319FDA7285D9F00083E15F9F26F9534306DE802A68AAE869252599AB0F4A8707`，events SHA-256 为 `0E9E5FD7DB5188257FCD1314DA7DE5356436CE3279604F1CBA1789DAFBF2F467`，analysis SHA-256 为 `812D51C963DC565B2605AAF02F8FEEAF4870DB171DA0FAD8C61410EB3C67B1EB`。Hook mask 为 `0x00000000FFFFFFFF`，无 overwrite/failure，before/after SHA 均为 `5DEBBE254D2C8A94211C2B55309F40078D335058AAE9F6E51A32484408890965`。只读重放前后 analysis SHA 不变；该证据填入 V5 `pause` 槽。

### V5 menu 与 loading 串联 PASS

`20260821T151105Z-t0-03r-loop-passive-menu-t0-03r-v5-menu-run-teardown-01` 从 active Run 正常返回主菜单，直接得到 18/18 PASS：54,326 条连续事件、2,795 个完整 loop、8,385 条三层 input-envelope 事件、4,199 条 message-pump 事件、885 条完整 steady standard transaction，以及恰好一对 main-thread `RUN_TEARDOWN_ENTER/EXIT`。Manifest/events/analysis SHA-256 分别为 `B16D303FA0FE9FCCE026E3126FEF79FF4A9FF49E89247C450404764499A91D1C`、`5EEECE7067ACE78C418DC3C92691296AA3C761B1196BBE8611D88C37C79600BC`、`9ED1564007BC2EA91D8920EF506ABEABB2A6DFD17D9549D53463522485F2CD92`。

保持进程存活后，`20260821T151234Z-t0-03r-loop-passive-loading-t0-03r-v5-loading-menu-continue-01` 从主菜单 Continue 进入可操作 Run，直接得到 18/18 PASS：103,189 条连续事件、6,321 个完整 loop、18,963 条三层 input-envelope 事件、9,918 条 message-pump 事件、116 条完整 steady standard transaction，以及恰好一对 main-thread `NEW_ROOM_ENTER/EXIT`。Manifest/events/analysis SHA-256 分别为 `51518025A576F4BB1A9F14E7712819F75CFDB522F83160E22A2D67F9B8177CC3`、`A4E4298A2394A0B503EB4FA80AB3810CEAEEBD373F05D9F339C7DC136492E009`、`F4E789881CDC18C16C7441A8BE22C2264FB473346B66B6E1246D91B837EDD56E`。

两份 artifact 的 hook mask 均为 `0x00000000FFFFFFFF`，无 overwrite/failure，32 槽 before/after SHA 均为 `5DEBBE254D2C8A94211C2B55309F40078D335058AAE9F6E51A32484408890965`；只读重放前后各自 analysis SHA 不变。它们分别填入 V5 `menu` 与 `loading` 槽。

## 8. 不可越过的门

- V5 7/7 已满足，但它只是主动阶段的必要条件，不等于主动授权。
- 在取得 raw sample/apply 的被动闭环证据并再次获得用户对主动实验的独立确认前，主动输入仍然禁止。
- 旧 V1–V4 artifact 不得填入 V5 gate。
- `WITNESS_QUERY_ONLY` 与 `RPM_DIAGNOSTIC_ONLY` 永不解锁 active gate。
- action `12+` 不得观察、canonicalize、抑制、排队或合成。
- OriginalOnline frame builder/apply 只作 oracle，不作 offline sample/apply 证明。
- sequence gap、ring overwrite、线程/深度/phase/return 违约、hash mismatch、非完整 hook mask、before/after 不一致均必须 FAIL。
- loop gate 必须从 source artifact 重放，严格绑定 V5 identity/evidence class、同一 target SHA、ledger SHA 以及 manifest/events/analysis SHA；input gate 还必须绑定 exact loop-gate SHA、target SHA、唯一 case/artifact 和 repeatability。
- artifact 和 gate 一律 create-new，不得覆盖历史 PASS/FAIL 或原始证据。
- 恢复不确定时不得盲重试；不得使用断点采样。
- 所有实机步骤必须在用户确认后开始；主动阶段还需要第二次独立确认。
