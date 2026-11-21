> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias universe closure — independent TEST preseal v1

Date: 2026-09-09  
Role: independent TEST  
Phase: pre-REVERSE / candidate absent  
Scope: `NATIVE-ALIAS-CLOSURE` only

## 1. Purpose and non-authority

This document seals the acceptance boundary before the new REVERSE pass begins. It does not perform reverse engineering, assert that the alias universe is closed, create the alias manifest or v5 decision, or authorize implementation/runtime work.

The fixed objective remains `ENDPOINT_B_MUST_REMAIN_OPERATIONAL_WITHOUT_A_SECOND_PHYSICAL_CONTROLLER_ON_AUTHORITY`, and the inherited necessity result remains `REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`. This batch may close only the keyed-reader alias universe and, if independently proved, the owner consequence that follows from it. It must not investigate or promote the native factory/rollback or final-release/thread work packages.

Only two alias outcomes are valid:

1. both required fixed points are complete under the set and edge rules below; or
2. the result remains `UNKNOWN` and records the first exact edge that cannot be excluded or resolved.

A count, a candidate-authored boolean, an offline lifecycle test, or the absence of a direct xref is never a closure proof.

## 2. Fixed target, parent, and seed identities

### 2.1 Disk PE

The sole accepted target is the approved unpacked disk image:

- SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- PE machine: Win32/x86 `0x014C`
- preferred image base: `0x00400000`
- image size: `0x008FE000`
- address convention: `RVA = VA - 0x00400000`; runtime form is `moduleBase + RVA`

The path supplied by the caller is not identity. The future verifier must recompute the file SHA-256 and machine from the disk bytes and must not enumerate or open a process.

### 2.2 Immutable v4 parent

| Role | Path / identity | Required SHA-256 |
|---|---|---|
| parent decision | `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json` / `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecision-v4` | `73161422E04832566EB86B9D702C0FE6ABEBAA336FEDC86EF49F0B05DC8F04FD` |
| parent final verification | `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v4-retry-v1.json` / `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecisionVerification-v4` | `048634971F408F30E2A06552C96A5AC3ACF5850BBAE5D1FF2D0321656FE0FCA4` |
| parent independent TEST result | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-v4-test-results-v1.md` | `CD65E379DA498333465FF8367BC83A570A7D916566340B104CF44A81B44701F2` |
| inherited REVERSE baseline | `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-joint-reverse-v2.md` | `AD94ECE74EE79D866E275E3A87672FFD6A0B1CCB20FA48EA9378C0B47620AF42` |

The future v5 decision must additionally bind, by exact path and post-publication SHA-256:

- this preseal;
- create-new `cs-t0/native-alias-universe-v1.json`;
- create-new `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-reverse-v1.md`;
- the fixed verifier profile containing every new byte, image-pointer, and function-range/hash anchor.

No missing, duplicate, candidate-selected, cyclic, or hash-mismatched binding is accepted. Binding a document proves its identity only; it does not make its prose a PE fact.

### 2.3 Exact initial seed set

The alias analysis starts from keyed lookup RVA `0x00620390`, the two image-pointer vtable cells `0x0078281C` and `0x00782978` (both targeting RVA `0x00620390`), and this exact ordered set of 28 known direct callsite RVAs:

```text
0x00023D12 0x0002AF89 0x0029200C 0x002F6A24 0x0038323F 0x003B9F9C 0x003BDBA6
0x00435BD7 0x00435CCF 0x00435E07 0x004D21C8 0x004D8B6A 0x004D9154 0x004D9395
0x004DA06A 0x004DA79E 0x00501AF2 0x00506054 0x0050AAB3 0x00534B0B 0x00535046
0x0055661E 0x0055AF6A 0x0055AFEE 0x0055B1BA 0x00589B26 0x0058DD51 0x005B279D
```

These are seeds, not the answer. The independent whole-image census must rediscover the direct set from decoded instructions and must also discover every indirect `slot +0x28` dispatch reachable through a Manager receiver. A mismatch, missing seed, duplicate seed, additional direct caller, wrong direct target, or changed vtable target is a hard failure, not a count adjustment.

## 3. Create-new `native-alias-universe-v1` evidence contract

The machine artifact identity is `IsaacCsT0-NativeAliasUniverse-v1`. It must be create-new and must contain canonical, uniquely identified node and edge arrays; summary numbers are derived from those arrays and cannot stand in for them. Every instruction-backed record contains the containing function range, instruction RVA, exact raw bytes, independent decoded instruction, successor/callee disposition, and anchor ID or function-range SHA-256.

The verifier must independently decode the disk PE with the caller-supplied `objdump`, bind the resolved tool path, executable SHA-256, and version banner, and compare reconstructed sets with the manifest. Candidate-produced disassembly text, IDA names, IDA pseudocode, or manifest counts cannot be the independent oracle.

### 3.1 Manager receiver-dispatch fixed point

The first graph starts from the keyed-lookup function and both vtable-owner shapes and follows Manager receiver provenance through all of these transfer classes:

- register copy, phi/join, spill and reload;
- stack argument/local and interprocedural parameter transfer;
- return-value transfer;
- field store/load and computed-address alias;
- direct call, resolved indirect call, tail transfer, callback/reentry, and exceptional/nonlocal successor.

Every decoded candidate call whose target operand can be `receiver-vtable + 0x28` must appear exactly once in `dispatchCandidates`. Each candidate is classified as either:

- `KEYED_LOOKUP_DISPATCH`, with a proved Manager receiver and target set exactly `{0x00620390}`; or
- `EXCLUDED_WITH_INSTRUCTION_PROOF`, with the raw instruction and a concrete, independently checkable reason showing that the receiver or target cannot be the keyed Manager lookup.

No `UNKNOWN`, unresolved target, unclassified candidate, missing successor, or prose-only exclusion is allowed for a closed first fixed point. The artifact must expose the exact discovered and expected seed sets, duplicate and omission sets, graph nodes/edges, candidate classifications, unresolved frontier, and a derived `receiverDispatchFixedPointClosed` value.

### 3.2 Returned `{object, control}` pair-alias fixed point

The second graph begins at every direct and indirect keyed-lookup return and tracks the two-word `{object, control}` value after the Manager mutex is released. Object and control provenance must remain paired even when the words travel through different registers, stack slots, fields, or basic blocks.

The graph must classify every reachable operation and outgoing edge involving the pair:

- copy/acquire, move, swap/assignment, reset and replacement;
- stack/field/container store and load;
- parameter transfer, return, callback/reentry, and retained escape;
- vector/container copy, grow, shift, erase, clear, destruction, and shutdown use;
- direct, resolved indirect, tail, split-CFG, early-return, exception/unwind, and block-liveout edges;
- strong release, final local destruction, ownership transfer to another tracked node, or an explicit unresolved frontier.

Every alias node must have a unique provenance and exactly one disposition for each outgoing control-flow edge. A transfer is closed only when its destination rejoins the graph; a terminal is closed only when its release/destruction behavior is anchored. Unknown callees, unresolved indirect targets, unanalyzed exception successors, escaped stores/returns/callbacks, and block liveouts are not terminals and must remain in the frontier.

For a closed second fixed point, omitted/duplicate/unclassified alias nodes and edges, unresolved calls, unresolved liveouts, unresolved exceptions, and unresolved escapes must all be empty sets. The verifier must derive `pairAliasFixedPointClosed` from set equality and edge disposition, rather than trusting the field in the artifact.

### 3.3 Completeness is separate from drainability

The artifact and v5 decision must contain distinct records:

- `universeCompleteness`: whether receiver dispatch and pair transfer are both complete;
- `drainability`: whether publication close prevents new reader/callback aliases and every admitted alias is bounded and released before Manager remove;
- `nativeOwnerThroughDrain`: whether an independently evidenced native owner retains a strong reference until that drain completes.

The exact derived rule is:

```text
dynamicAliasUniverseClosed =
    receiverDispatchFixedPointClosed
    AND pairAliasFixedPointClosed
    AND omittedSeedCount == 0
    AND duplicateSeedCount == 0
    AND unclassifiedSeedCount == 0
    AND unresolvedCallCount == 0
    AND unresolvedLiveoutCount == 0
    AND unresolvedEscapeCount == 0
    AND unresolvedExceptionEdgeCount == 0
```

`dynamicAliasUniverseClosed=true` does not imply that aliases are drainable. `allAliasesBoundedAndReleasedBeforeManagerRemove` and `nativeOwnerHeldThroughDrain` are separately derived from anchored paths. A complete census with an unproved close/admission barrier, drain join, bound, or owner lifetime remains complete as a universe but cannot promote owner safety.

If either fixed point is incomplete, `universeCompleteness.status` must be `UNKNOWN`, `dynamicAliasUniverseClosed` must be false, and both the manifest and REVERSE handoff must record `firstUnclosedEdge` with function/instruction RVA, raw bytes, edge kind, source and destination/prospective target, why it is unresolved, and affected proof. If the universe is complete but drainability or owner-through-drain is not, the corresponding record must be `UNKNOWN` and carry its own first exact missing edge. Empty or generic text such as “more analysis required” is rejected.

## 4. REVERSE handoff acceptance

The independent REVERSE handoff must record the fixed PE identity, `objdump` identity, all added anchors and function-range hashes, actual node/edge/frontier counts, exact set comparisons, and one of the two legal universe verdicts:

- `STATIC_CONFIRMED`: both fixed points and all zero-frontier conditions above are independently reproducible; or
- `UNKNOWN`: at least one fixed point is incomplete and the first exact unclosed edge is preserved.

The handoff must separately state the drainability and owner-through-drain verdicts. It must not turn a complete alias census into a drain proof, use the offline coordinator as a native close/admission barrier, infer a thread from an inline release primitive, or begin factory/rollback or final-release/thread closure.

New anchors must be sufficient for the verifier to read the raw bytes or image pointer and recompute every claimed function-range hash from the PE. A function name, RVA count, xref count, or IDA-only database fact without the corresponding disk evidence is insufficient.

## 5. v5 decision state coupling

The create-new successor is fixed as:

- path: `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v5.json`
- identity: `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecision-v5`
- schema: `schemaVersion=5`
- scope: `NATIVE_ALIAS_CLOSURE_ONLY`

All non-alias v4 conclusions and their ordered arrays/records must be copied without semantic or textual change. In particular, `offlineHarnessSummary`, `lifecycleContract`, `VIRTUAL_DEVICE_CREATION_DESTRUCTION`, `VIRTUAL_DEVICE_REFERENCE_COUNTING`, factory/rollback gaps, final-release/thread gaps, and all retained non-alias `UNKNOWN` records remain deep-equal to v4. No factory, counter-bound, final releaser, calling-thread, ABI, key, assignment, generation, or teardown conclusion may be improved by this batch.

`VIRTUAL_DEVICE_OBJECT_OWNER` may be promoted to `nativeEvidenceStatus=STATIC_CONFIRMED` and `formalStatus=STATIC_CONFIRMED` if and only if all three independently derived values are true:

1. `dynamicAliasUniverseClosed`;
2. `allAliasesBoundedAndReleasedBeforeManagerRemove`;
3. `nativeOwnerHeldThroughDrain`.

Otherwise that proof stays `UNKNOWN` and names the first failing exact edge. Consequently this batch allows only `promotionCount=0` with no promoted IDs, or `promotionCount=1` with the sole promoted ID `VIRTUAL_DEVICE_OBJECT_OWNER`. Aggregate static/unknown totals and `missingEvidence` must be derived consistently; candidate-authored counts never determine promotion.

Regardless of alias or owner outcome, these values are invariant:

- `formalDecision=null`;
- `workPackageStatus=BLOCKED_BY_EVIDENCE`;
- `implementationEligibility=false`;
- `virtualDeviceImplemented=false`;
- `offlineLifecyclePrototypeImplemented=true` and `implementationScope=OFFLINE_NOT_LINKED_NOT_LOADED`;
- `nativeStaticClosure.finalReleaseCallerUniverseClosed=false`;
- `nativeStaticClosure.stopAndDestructorThreadProvenanceClosed=false`;
- `runtimeObserved=false`, `processAccessed=false`, `gameStarted=false`, `ipcCreated=false`, `hookInstalled=false`, `inputMutationPerformed=false`, and `worldMutationPerformed=false`;
- `csT00506Unlocked=false`, `t005Unlocked=false`, and `t006Unlocked=false`;
- every one of the 18 v4 permission fields exists and remains `false`, with no added permissive alias.

Thus even a fully closed alias fixed point does not complete CS-T0-05-05 and does not authorize factory work, release/thread work, process access, a passive canary, or implementation.

## 6. Future verifier and write boundary

The only public v5 entry is:

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v5.ps1 `
  -GameExe <fixed-disk-PE> `
  -Objdump <objdump-executable> `
  -RepositoryRoot . `
  [-Output <new-json>]
```

No other public parameter is accepted. The verifier may read only the supplied disk PE, supplied `objdump`, repository files bound by v5, and temporary files used for independent decoding. It must not start/enumerate/open a game process, invoke IDA/x32dbg, create IPC, inject, hook, or modify runtime state.

The verifier must independently check PE identity, all raw-byte/image-pointer/function-range anchors, whole-image direct and indirect seed census, both graph set equalities, every edge disposition, frontier derivation, v4 inheritance, promotion coupling, permissions, and locks. It must fail if `objdump` is absent, fails, has the wrong bound identity, produces incomplete output, or cannot decode a required range.

Without `-Output`, verification is read-only and must create, remove, or change no repository file. With `-Output`, the destination must be an absent `.json` leaf under resolved `out/cs-t0` or `artifacts/cs-t0`; existing files, repository/output path aliases, symlink/reparse/junction escapes, traversal, alternate data streams, device names, and other extensions are rejected before any write. All failures occur before publication and leave the requested path absent. Success writes one complete file using create-new semantics and never overwrites prior v1–v4 evidence.

A verifier exit code `0` and `verificationStatus=PASS` mean that the v5 decision faithfully represents the sealed static evidence. They do not mean that the work package or virtual-device implementation is complete.

## 7. Exactly four compressed acceptance groups

Each group is executed exactly once and in this order after REVERSE and implementation artifacts are frozen. A failure stops later groups and consumes that group; any repair requires a new TEST seal and distinct create-new output.

1. **Fixed PE / anchors / independent alias oracle — one run.** Independently hash the PE and bound inputs; bind `objdump`; verify every new raw byte, image pointer, and function-range hash; reconstruct the whole-image 28-direct-call seed set and both vtable entries; rebuild receiver-dispatch and pair-alias graph sets; check exact set equality, zero/recorded frontiers, the completeness/drainability separation, and the legal `STATIC_CONFIRMED` or first-counterexample `UNKNOWN` disposition. No repository output is created.
2. **Public CLI create-new — one run.** Invoke the v5 PowerShell entry once with the fixed PE, bound `objdump`, repository root, and one absent allowed output. Require exit `0`, `verificationStatus=PASS`, the evidence-derived alias/owner statuses, `formalDecision=null`, and `workPackageStatus=BLOCKED_BY_EVIDENCE`.
3. **Public CLI verify-existing — one run.** Invoke the same entry once without `-Output`. Require exit `0`, `outputWritten=false`, and no repository or prior-output change, measured by a path/length/UTC-write-time manifest plus SHA-256 of all bound and generated evidence before and after.
4. **One parameterized negative batch — one run, at most two test methods.** The future test module may expose only `test_semantic_mutations` and `test_write_boundaries` (or two equivalently scoped methods). All individual cases run as table-driven subtests against one temporary fixture; the batch is not rerun per mutation.

The semantic-mutation method must cover at least:

- omitted or duplicate indirect caller/dispatch candidate;
- wrong Manager receiver or target set not exactly `{0x00620390}`;
- an untracked pair copy/move/store/load/return/callback/container/release transfer;
- escaped alias or unresolved block liveout;
- unknown direct/indirect callee, tail edge, or exception/unwind successor;
- missing/duplicate/unclassified seed or graph node/edge;
- self-reported fixed-point/zero-frontier boolean or count inconsistent with reconstructed sets;
- `dynamicAliasUniverseClosed=true` while either fixed point or any required zero set is false/non-empty;
- drainability inferred from universe completeness, or owner promotion without all three required predicates;
- promotion of creation/destruction, reference counting, calling thread, final-release/thread, or another non-alias proof;
- v4 parent identity/SHA, final verification SHA, TEST preseal, manifest, REVERSE handoff, anchor, function hash, or `objdump` identity drift;
- any change to `formalDecision`, blocked status, implementation flags, runtime facts, any of the 18 permissions, or any 05-06/T0-05/T0-06 unlock.

The write-boundary method must cover at least an existing output, repository/output path alias, traversal, symlink/reparse/junction escape, ADS/device name, non-JSON leaf, target outside the two roots, and a semantic/tool/hash failure before write. Every case must exit nonzero, leave a requested absent output absent, preserve an existing output byte-for-byte, and make no change to bound v1–v4 files.

## 8. Explicitly omitted work and tests

Do not run or delete the v4 offline lifecycle harness, any v1–v4 test suite/report, Cargo full tests, the complete T0 build, Debug/Release production Native matrix, historical broad regression, duplicate self-test, game startup, process probe, passive/active canary, x32dbg, IPC, hook, injection, input mutation, or world mutation.

Existing v1–v4 artifacts and all failed attempts remain immutable. Test compression applies only to the new v5 acceptance execution; it is not authorization to remove hash-bound historical tests or evidence.

## 9. Pre-REVERSE disposition

At this seal point the alias manifest, new REVERSE handoff, v5 decision, verifier, tests, and verification output do not exist and no test has run. `NATIVE-ALIAS-CLOSURE` is not presumed complete. REVERSE must now work against this fixed acceptance boundary, and TEST must not reinterpret the rules to make a later candidate pass.
