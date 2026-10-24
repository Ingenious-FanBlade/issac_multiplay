> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager receiver identity independent test results v1

Date: 2026-09-01

Status: `FAILED_MANAGER_THIS_IDENTITY_PREMISE` (expected conservative result, exit 1)

This report records independent static acceptance of the v6 manager receiver work. It does not perform reverse engineering on behalf of the producer, read or enumerate a process, run the game, rebuild production code, or authorize an active join. The bounded ordinary-spawn receiver formula is independently confirmed. The claim that the receiver denotes the same live manager object remains `UNKNOWN`.

## Sealed inputs

- Admission contract v6: `cs-t0/coop-native-join-admission-contract-v6.json`, SHA-256 `1022B812A6F98CB1D409E5B7E5FA9E8077BDA97C58746A98BE5DB0BEE343F7F4`.
- Init closure v8: `cs-t0/coop-native-join-init-closure-v8.json`, SHA-256 `32EC5EC7FFEA7D749FE07403CFAD24CC0D610F30FC0404D564863B641F2D71F5`.
- Manager receiver sidecar v1: `cs-t0/coop-native-join-manager-receiver-v1.json`, SHA-256 `E4CFFC6C973244D112C28E690EAC46A83E4277A3F10CBDC216389E2C8A752CAE`.
- Public blocked inspection: `out/cs-t0/native-join-manager-receiver-v6-inspection-v1.json`, SHA-256 `5D2FFBDA91BC1EBA5014DECB4DB17B71D86C19408ECF0B6E94BF29A51ED7096E`.
- Independent formal verification: `cs-t0/coop-native-join-admission-verification-v6.json`, SHA-256 `E6E59215B4775931DE3DD8E04C765698AE872A412FF1107463FC726F9EA7B5DE`.
- Formal verifier log: `out/cs-t0/native-join-manager-receiver-v6-verifier-formal-attempt-v1.log`, SHA-256 `36F5512D263BB119E03F582626885EF57681ACA7375CB8FB5907ECC1E95449BB`.

The formal verifier froze 523 input bindings. Its historical baseline retained the exact v5 473-input verification, v4 426-input verification, 174 source files, 164 snapshots, 149 original baseline files, three production files, and three mutable index files. The current plan and progress remain `cs-t0/coop-plan-v10.json` and `cs-t0/coop-native-join-progress-v4.json`.

## Independent checks

- Contract mutation cases: 34/34 passed. They reject reused v5 identity/phase/schema, receiver proof drift or promotion, fake or incomplete 14-key contract permissions, parent proof promotion, active claims, and reader obligation promotion.
- Synthetic receiver model cases: 32/32 passed. They cover wrong slot and embedded offset, ECX overwrite, callee clobber or slot replacement, same-value write, write then restore, generation advance and ABA, duplicate/missing operations, non-boolean call flags, non-JSON receiver values, and events after spawn.
- Submitted receiver evidence cases: 44/44 passed. They cover omitted caller, caller-target drift, wrong formula bytes, hidden or wide slot writes, address escape claims, alias/generation promotion, exception resume omission, malformed bindings, sidecar inventory/count/permission drift, and ordinary/possessor confusion.
- Public inspector blackbox: 6 groups and 89 assertions passed. Legal input returns the entire bounded `BLOCKED_BY_EVIDENCE` report with exit 1. Raw JSON, option duplication, repository/output aliases, trailing-dot and trailing-space forms, junction traversal, create-new collisions, and bound-input mutations are rejected without a report.
- Explicit dependency audit: six interpreted nodes, 133 edges, 107 bound leaves, zero self-bindings, and zero cycles. Historical opaque reports were not recursively reinterpreted.
- Frozen v5 public behavior remained byte-for-byte equal to its sealed inspection report. The old production preflight remained blocked with its seven original missing-evidence items.

All seven structural checks passed. The single semantic check completed with unresolved obligations. All seven integrity checks passed. `localChecksPassed=true` and `allRequiredChecksExecuted=true`; these fields do not imply semantic closure.

## Disk reconstruction and semantic boundary

GNU objdump 2.44 and the independent PE reader rebuilt 89 Init-v8 function rows plus five receiver-sidecar rows: 94 submitted rows, 93 unique entries, 20,558 instructions, 1,905 calls, 8,011 writes, and 201 exits. Every declared byte/instruction/call/write/exit set matched the approved target SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

The receiver sidecar's five functions independently rebuilt to 2,337 instructions, 225 calls, 842 writes, and seven exits. Ten selected chain instructions were checked directly from disk. The whole executable contains one relative direct call to manager entry `0x005BAC40`, from `0x002FADA7`. The caller contains 39 preceding calls, seven indirect, with the first unresolved at `0x002FA55E`.

The raw disk contains 11,303 byte candidates for the GameRoot slot address. Independent instruction decoding classifies 11,301 inside instructions and two as unaligned or crossing an instruction boundary. These counts are not proof. Operand-level classification found exactly three direct four-byte overlapping writes (`0x00552847`, `0x005597D0`, `0x005AB8A9`), no direct wide overlap, and no exact slot address formation. Calculated aliases, lifecycle replacement, exceptional resume, and generation/ABA remain outside that direct-operand result.

The independently supported local relation is limited to ordinary spawn: load the GameRoot slot at `0x005BB408`, preserve the argument at `0x005BB40E`, derive `ECX + 0x1BAA8` at `0x005BB40F`, then call `0x005B9CD0` at `0x005BB415`. The possessor path uses its separately saved receiver at `0x005BAFF8` and was not conflated with this formula.

## Required unresolved result

The following frontiers remain material:

- `CALLER_PRECEDING_CALLEE_PRESERVATION`
- `GAMEROOT_SLOT_ALIAS_WRITER_CLOSURE`
- `SAME_LIVE_GAMEROOT_GENERATION_ABA`
- `LIFECYCLE_AND_EXCEPTION_PRESERVATION`

Therefore `managerReceiverFormulaStatus=STATIC_CONFIRMED`, while `managerThisIdentityStatus=UNKNOWN`, `managerContextStatus=UNKNOWN`, `publicationIdentityStatus=UNKNOWN`, `freshBStatus=UNKNOWN`, and the whole synchronous proof remains `UNKNOWN`. The formal verifier correctly reports `FAILED_MANAGER_THIS_IDENTITY_PREMISE`, `overallAccepted=false`, and exit 1. It publishes no new plan or progress and grants no runtime, injection, mapping, arm, claim, unlock, or active-join authority.

Development failures and retries were retained create-new, including the initial model/evidence false accepts, baseline creation errors, independent scan syntax error, reviewer expected-byte correction, public report-field correction, contract schema failure, two development verifier reports/logs, and the inspector report.
