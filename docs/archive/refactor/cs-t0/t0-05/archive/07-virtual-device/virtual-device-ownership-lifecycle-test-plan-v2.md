> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual device ownership/lifecycle joint proof v3 — TEST preseal

Date: 2026-09-08  
Role: independent TEST  
Phase: candidate-absent / rules sealed before REVERSE evidence and v3 decision  
Scope: read-only static verification only

## 1. Purpose and non-authority

This document seals the independent TEST acceptance profile for the v3 ownership/lifecycle joint proof. It does not assert that any new proof has passed, does not create evidence, and does not authorize implementation or runtime work.

The fixed objective remains: Endpoint B must continue to work when Authority has no second physical controller. The inherited necessity assessment remains `REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`; necessity is not implementation eligibility.

This profile must not be interpreted as any of the following:

- a virtual-device ABI or lifecycle proof;
- permission to create a virtual device;
- permission to start, enumerate, open, read, hook, inject into, or mutate a game process;
- permission to create IPC, send input, run a passive or active canary, or unlock CS-T0-05-06.

The v3 decision remains a blocking/read-only decision. Even if one or more of the five proofs is promoted, `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, and CS-T0-05-06 remain locked because teardown/drain/restore, ABI, key, assignment, and generation evidence remains incomplete.

## 2. Fixed target and immutable predecessor bindings

### 2.1 Fixed disk PE

The only target accepted by the future v3 verifier is the already approved unpacked Win32/x86 disk PE:

- expected SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- expected PE machine: `0x014C`
- address convention: `moduleBase + RVA`

The caller supplies the path. A path, filename, timestamp, or version string is not identity; the bytes, SHA-256, PE machine, and fixed anchors are identity. The verifier may read the disk file only. It must not enumerate, open, inspect, or otherwise access a running process.

### 2.2 Existing exact predecessor bindings

The v3 candidate and verifier profile must bind these exact predecessors:

| Artifact | SHA-256 | Evidence role |
|---|---|---|
| `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v2.json` | `3ACAFAEB608EB1794B8E16477F6FFFE64EC95442E1C6C0EBE7FDC4061E40A916` | inherited blocking decision |
| `out/cs-t0/virtual-device-ownership-lifecycle-verification-20260908-v2.json` | `03E275D747CBFED0908F67F1400AFEA38B1AD40A07692F4710D8B3DDCAAC2682` | inherited independent verification |
| `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-reverse-v1.md` | `9CB6CC4D08DC0F75B0921D0C7B3A5A2753EB5A64930DFCDC58E62F70613A307C` | v2 static reverse handoff |
| `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-test-plan-v1.md` | `7481F05B81FE5728D104115A2CEFEFA93DC64CFB3729F732431AA05F7BD5DD32` | v2 presealed TEST profile |
| `docs/archive/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-ownership-lifecycle-test-results-v1.md` | `5E56396729317B01BCA58693AE7D07C6304598039C693475FA9DFA956C24DEEA` | v2 TEST result |
| `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v2.ps1` | `417401D1AF5DA605820005B3CA6ACAA038E8622F10E3007B5F1BFBC5A41EF329` | v2 entry point |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v2/verify_virtual_device_ownership_lifecycle_decision_v2.py` | `A06C2B164F6C100955363B5B9191732E407A17A90D0C58A052233BB092E91D43` | v2 verifier core |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v2/test_virtual_device_ownership_lifecycle_decision_v2.py` | `8ED0757583003B342027C67E283E1883C808DBB652A5F6DFD093B7E0DA5A7F4C` | v2 sealed fixture suite |

The final v3 profile must additionally bind, before the v3 decision is created:

1. this preseal file by its exact post-write SHA-256; and
2. the independent v3 REVERSE handoff by its exact path and SHA-256.

If a design-contract artifact is used, it may also be hash-bound, but its evidence class must remain `DESIGN_ONLY`. A predecessor binding proves file identity only; it does not promote the file's statements to observed facts.

No candidate-selected predecessor path or digest is accepted. Missing, duplicated, additional, reordered where order is normative, or mismatched bindings are a hard failure.

### 2.3 Fixed production anchors

The following exact 17 anchors are inherited verbatim from `BASE_ANCHORS` and `PROOF_ANCHORS` in the v2 production verifier. A dash in `proof_id` means the production `Anchor` has no proof ID argument.

| Anchor ID | kind | RVA | expected bytes / target RVA | proof_id |
|---|---|---:|---|---|
| `input-manager-sample` | `bytes` | `0x00554F7D` | `E87EAC0C00` | — |
| `input-device-update` | `bytes` | `0x0061FC97` | `E8E4F5FFFF` | — |
| `gamepad-physical-poll` | `image-pointer` | `0x00782CF8` | `0x00622940` | — |
| `gamepad-logical-pressed` | `image-pointer` | `0x00782CB0` | `0x0061E8D0` | — |
| `gamepad-logical-triggered` | `image-pointer` | `0x00782CB4` | `0x0061E970` | — |
| `gamepad-logical-scalar` | `image-pointer` | `0x00782CB8` | `0x0061EA20` | — |
| `player-update-owner` | `image-pointer` | `0x0076BDDC` | `0x00382AF0` | — |
| `player-movement-consumer` | `bytes` | `0x00385BC2` | `E8793FFFFF` | — |
| `device-prompt-unassign` | `bytes` | `0x0055B87F` | `E8CCABE4FF` | — |
| `device-prompt-reassign` | `bytes` | `0x0055BCA0` | `E8ABA7E4FF` | — |
| `owner-control-block-wrapper` | `bytes` | `0x00505F8B` | `6A18C70700000000C7470400000000E821951000` | `VIRTUAL_DEVICE_OBJECT_OWNER` |
| `owner-manager-insert-transfer` | `bytes` | `0x00505FF1` | `B9187BC500897704891FE8F0AD1100` | `VIRTUAL_DEVICE_OBJECT_OWNER` |
| `creation-local-allocation-ctor` | `bytes` | `0x002F6631` | `68F8030000E8858E3100` | `VIRTUAL_DEVICE_CREATION_DESTRUCTION` |
| `creation-remote-allocation-ctor` | `bytes` | `0x002F66E7` | `68F8030000E8CF8D3100` | `VIRTUAL_DEVICE_CREATION_DESTRUCTION` |
| `creation-net-deleting-dtor` | `bytes` | `0x005018C3` | `8BCFC78770030000202EB800C7075827B800` | `VIRTUAL_DEVICE_CREATION_DESTRUCTION` |
| `refcount-six-slot-vtable` | `bytes` | `0x00778014` | `50CB4000B0C6400090C64000B058430010C64000C0C54000` | `VIRTUAL_DEVICE_REFERENCE_COUNTING` |
| `refcount-strong-acquire` | `bytes` | `0x0000C690` | `568BF1576AFF8B46088D4E08FF500C66FF4604` | `VIRTUAL_DEVICE_REFERENCE_COUNTING` |

V3 will append new anchors frozen by the new independent REVERSE handoff. Those additions must be copied into a fixed verifier profile before candidate creation. The decision may reference only the inherited 17-anchor set plus that sealed v3 addition set. A wrong kind, RVA, expected byte window/target RVA, proof ID, candidate-only anchor, or anchor bound to a different PE is a hard failure.

## 3. Exact five-proof assessment set

The v3 candidate's joint proof ID set is exact and closed:

1. `VIRTUAL_DEVICE_OBJECT_OWNER`
2. `VIRTUAL_DEVICE_CREATION_DESTRUCTION`
3. `VIRTUAL_DEVICE_REFERENCE_COUNTING`
4. `VIRTUAL_DEVICE_CALLING_THREAD`
5. `VIRTUAL_DEVICE_VECTOR_INSERT_REMOVE_OWNERSHIP`

Each proof is promoted independently and only when every claim in that proof's own coverage set is closed by fact evidence. `promotionCount` may be any integer from 0 through 5 and must equal the exact number of `STATIC_CONFIRMED` records; `promotedIds` must be the corresponding ordered subset of the five-proof set.

Lifecycle facts can be shared across proof records, and one missing fact can legitimately keep several affected records `UNKNOWN`. That coupling does not impose an all-or-none rule: a gap may block only the proof records whose required claim sets include it. A proof with complete coverage must not borrow a promotion from another proof, and an incomplete proof must not be promoted merely because another proof passed.

The inherited six static findings remain static. For a derived promotion count `p` in the inclusive range 0..5, the only valid totals are:

- `STATIC_CONFIRMED=6+p`
- `UNKNOWN=10-p`

The remaining five IDs stay `UNKNOWN` in v3 regardless of the five assessed proofs' outcomes:

- `VIRTUAL_DEVICE_TEARDOWN_DRAIN_RESTORE`
- `VIRTUAL_DEVICE_KEY_ALLOCATION`
- `VIRTUAL_DEVICE_VTABLE_COL_SLOT_ABI`
- `VIRTUAL_DEVICE_ASSIGNMENT_INTEGRATION`
- `VIRTUAL_DEVICE_GENERATION_LIFECYCLE`

The thread contract in this profile is the coverage source for `VIRTUAL_DEVICE_CALLING_THREAD`. It may promote that proof only when its complete required claim set is closed. It cannot promote `VIRTUAL_DEVICE_TEARDOWN_DRAIN_RESTORE`, which remains outside this five-proof set.

## 4. Design contracts are not fact evidence

The candidate must keep these namespaces separate:

- `designContractRefs`: desired behavior, allowed states, required transitions, invariants, and acceptance criteria; evidence class must be `DESIGN_ONLY`.
- `factEvidenceRefs`: observations from the exact hash-bound REVERSE handoff over the fixed PE; every reference must resolve to `STATIC_CONFIRMED` evidence with fixed anchors.

A design contract may define what must be proved, but it cannot prove that the binary implements it. It contributes zero observed-fact coverage. Promotion is rejected if any required fact is supported only by:

- architecture text, a task plan, README, test plan, proposed pseudocode, schema, or candidate decision prose;
- a `DESIGN_ONLY` record relabeled or copied into a fact field;
- an unanchored narrative claim;
- the presence of a constructor/destructor-shaped function without the full owner and lifecycle paths;
- the candidate's own booleans, counts, status, or summaries.

Mixed citations do not cure this: if a required fact relies on a design reference for the missing observation, that fact remains unproved.

## 5. Required v3 decision schema

The candidate identity and path are sealed as:

- path: `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v3.json`
- identity: `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecision-v3`

The JSON must use explicit fields, arrays, and booleans; free-form prose cannot substitute for a required field.

### 5.1 Top-level conservative state

The following values are invariant:

- `necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`
- `formalDecision=null`
- `workPackageStatus=BLOCKED_BY_EVIDENCE`
- `csT00506Unlocked=false`
- `t005Unlocked=false`
- `t006Unlocked=false`
- `runtimeObserved=false`
- `processAccessed=false`
- `gameStarted=false`
- `ipcCreated=false`
- `virtualDeviceImplemented=false`
- `implementationEligibility=false`
- `designContractStatus=DEFINED_NOT_IMPLEMENTED`

All permission values must exist and be `false`:

`activeExperimentAllowed`, `activeInputAllowed`, `applyAllowed`, `automaticLiveRetryAllowed`, `inputMutationAllowed`, `liveAuthorizationCreated`, `modeLatchAllowed`, `originalOnlinePositiveAllowed`, `overrideAllowed`, `passiveHookTransactionAllowed`, `passiveObservationAllowed`, `processEnumerationAllowed`, `processReadAllowed`, `runtimeMutationAllowed`, `schedulerAllowed`, `transportAllowed`, `virtualDeviceCreationAllowed`, and `worldMutationAllowed`.

Omitting a permission is not equivalent to `false`. An unknown or additional permission field is rejected unless the verifier profile was revised and resealed before candidate creation.

### 5.2 Joint assessment object

The candidate must contain one `lifecycleProofBatch` object with:

- `proofIds`: exact ordered five-proof set;
- `assessments`: exact five proof records, each independently `UNKNOWN` or `STATIC_CONFIRMED`;
- `promotionCount` and `promotedIds`: independently derived count and ordered subset;
- `alwaysUnknownIds`: exact ordered five-ID set listed in Section 3;
- `retainedUnknownIds`: every non-promoted item, derived to contain exactly `10-p` IDs;
- `coverage`: exact proof-by-proof coverage records;
- `stateMachine`, `rollbackMatrix`, `referenceClasses`, and `threadContract` objects conforming to Sections 6–9;
- `factEvidenceRefs`, `designContractRefs`, and `anchorIds` in separated namespaces;
- per-assessment `missingEvidence`: non-empty, precise gaps when that assessment is `UNKNOWN`, and empty only when that proof's promotion conditions are independently satisfied.

An underclaim is not silently promoted: each `UNKNOWN` assessment must name at least one failed gate for that proof. An overclaim is rejected: a proof is `STATIC_CONFIRMED` only if every gate assigned to that proof is mechanically true and supported by the sealed fact evidence. `promotionCount` and aggregate totals derive from the five records rather than determining them.

### 5.3 Proof coverage records

Each of the five exact records must contain:

- `proofId`
- `status`
- `requiredClaimIds`
- `coveredClaimIds`
- `factEvidenceRefs`
- `anchorIds`
- `designContractRefs`
- `uncoveredClaimIds`
- `missingEvidence`

For promotion, required and covered claim sets must be identical and non-empty, uncovered/missing sets must be empty, all fact references must resolve, and all anchors must belong to the sealed set. Local ctor/dtor or refcount shapes cannot satisfy owner, rollback, drain, or thread claims outside their observed paths.

## 6. Lifecycle state-machine schema and gate

The state-machine vocabulary is exact:

`EMPTY`, `OBJECT_ALLOCATED`, `CONTROL_READY`, `REGISTERED`, `RETIRING`, `UNREGISTERED`, and `DESTROYED`.

Constructor completion, vector insertion, publication, final strong release/object destruction, and final weak release/control-block free are transition/action properties; they must not be introduced as additional state names.

Every transition record must include:

- `transitionId`, `fromState`, `toState`, and `trigger`;
- `ownerBefore` and `ownerAfter`;
- `acquiredRefClasses` and `releasedRefClasses`;
- `threadClass`;
- `successorTransitionIds` and, where the operation can fail or return early, `rollbackId`;
- `factEvidenceRefs` and `anchorIds`;
- `closed`.

The successful path must cover, in order, `EMPTY -> OBJECT_ALLOCATED`, constructor completion and control-block binding into `CONTROL_READY`, vector insertion plus publication into `REGISTERED`, publication close plus reader/callback drain into `RETIRING`, vector removal into `UNREGISTERED`, and final strong/weak release actions into `DESTROYED`. The exact binary may combine operations, but the transition/action records must still account for every effect without adding state vocabulary.

For every proof whose claim set references these transitions, promotion requires:

- every reachable success, failure, exception-equivalent, callback-reentry, and early-return edge is represented;
- every mutating edge has a proved owner/ref delta and thread class;
- publication cannot precede a safely owned object and control block;
- removal cannot precede publication close and reader drain;
- destruction cannot precede removal and release of all strong-reference classes;
- all failure edges converge to `EMPTY` or `DESTROYED` with no live publication, vector entry, reader, callback, strong ref, or leaked control block;
- no unproved state, edge, or owner handoff is treated as unreachable.

An incomplete state edge keeps every proof whose claim set depends on that edge `UNKNOWN`; it does not mechanically force unrelated, fully covered proof records to `UNKNOWN`.

## 7. Rollback-matrix schema and gate

The rollback matrix must contain exact records for at least these failure classes:

1. `OBJECT_ALLOCATION_FAILURE`
2. `OBJECT_CONSTRUCTION_FAILURE`
3. `CONTROL_BLOCK_ALLOCATION_NULL`
4. `CONTROL_BLOCK_CONSTRUCTION_FAILURE`
5. `VECTOR_INSERT_FAILURE`
6. `VECTOR_GROW_OR_COPY_FAILURE`
7. `INSERT_NOTIFICATION_FAILURE`
8. `CALLBACK_REENTRY_OR_ESCAPE`
9. `PUBLICATION_FAILURE`
10. `REMOVE_CALLBACK_FAILURE`
11. `READER_DRAIN_FAILURE`
12. `STOP_OR_SHUTDOWN_EARLY_RETURN`
13. `FINAL_RELEASE_FAILURE_OR_WRONG_THREAD`

Each rollback record must contain:

- `failureId`, `failurePointAnchorIds`, and `preState`;
- `ownedRefClassesBefore`;
- ordered `rollbackActions`;
- `postState` and `ownedRefClassesAfter`;
- `publicationClosed`, `readersDrained`, `vectorEntryRemoved`, and `callbacksQuiesced`;
- `threadClass`;
- `factEvidenceRefs` and `anchorIds`;
- `affectedProofIds` as the ordered subset whose claim sets depend on this row;
- `closed`.

Every listed row must be present to make the schema complete, and statically reachable/unreachable status must itself be evidenced. For a particular proof to be promoted, every reachable row naming that proof in `affectedProofIds` must be `closed=true`; an unclosed row does not mechanically block a proof whose claim set does not depend on it. A `CONTROL_BLOCK_ALLOCATION_NULL` branch that still inserts, or an unclosed constructor/insert/grow/callback/early-return rollback, rejects each affected promotion. Merely asserting that an exception is impossible is not evidence.

## 8. Reference-class and counter-safety schema

The exact reference classes are:

- `FACTORY_LOCAL_STRONG`
- `MANAGER_VECTOR_STRONG`
- `INSERT_CALLBACK_TRANSIENT_STRONG`
- `REMOVE_CALLBACK_TRANSIENT_STRONG`
- `READER_TRANSIENT_STRONG`
- `ROLLBACK_TRANSIENT_STRONG`
- `CONTROL_IMPLICIT_WEAK`
- `EXTERNAL_WEAK`

Each class record must include:

- acquire/release sites as `anchorIds` plus `factEvidenceRefs`;
- owner and lifetime interval;
- minimum and statically proved maximum cardinality;
- allowed acquire/release thread classes;
- `escapeClosed`, `balancedOnSuccess`, and `balancedOnEveryFailure`;
- zero-to-nonzero resurrection status;
- relationship to publication, drain, removal, final strong release, and final weak release.

The counter contract must explicitly state and prove:

- strong counter width and weak counter width;
- a static upper bound below the representable maximum for every acquisition path, including callbacks and reentry;
- no decrement from zero and no release without a matching live reference;
- no double release through aliases, rollback, callback retention, reader drain, stop, or destruction;
- no acquire after the last strong release begins;
- exactly one object deletion and exactly one control-block free on every completed lifecycle.

Observed six-slot mechanics, strong/weak primitives, and last-release shapes close only the mechanical refcount subchain. They do not prove bounded callback retention, final-release authority, counter overflow/underflow exclusion, or double-release exclusion. If any callback can retain a shared reference without a statically closed upper bound and release path, or any count-safety field lacks anchored fact evidence, every proof whose required claims depend on that fact remains `UNKNOWN`.

## 9. Thread-contract and drain schema

The thread-class vocabulary is exact:

- `AUTHORITY_MAIN_LIFECYCLE`
- `MANAGER_CALLBACK`
- `READER`

`UNKNOWN`, `ANY`, or an inferred caller thread is not an acceptable class for a promoted operation.

The thread contract must map every lifecycle operation to one proved thread class and must include anchored dispatch/affinity evidence for:

- allocation, construction, control-block binding, insertion, and publication;
- insert/remove callbacks and callback-retained-ref release;
- reader admission, reader release, publication close, and drain completion;
- vector removal, all teardown releases, final strong release/object destruction, and final weak release/control-block free;
- every rollback action and early-return cleanup.

Promotion requires these exact contracts:

- publication close prevents every new reader and callback ref acquisition;
- all admitted readers and callbacks are proved to leave and release before vector removal;
- removal cannot return success until reader/callback drain is complete;
- final strong release and object destruction occur on `AUTHORITY_MAIN_LIFECYCLE`;
- final weak release/control-block free has a proved permitted thread and cannot race object destruction;
- failure rollback preserves the same affinity and drain rules as normal teardown.

An unknown final-release thread rejects `VIRTUAL_DEVICE_CALLING_THREAD` and any other proof whose release claim depends on it. An incomplete remove callback, missing reader-admission barrier, merely sampled zero-reader observation, or incomplete reader/callback drain rejects every affected proof. The inline nature of a release primitive does not prove the caller's thread.

## 10. Derived promotion gate

An individual assessment is accepted as `STATIC_CONFIRMED` if and only if every general gate and every proof-specific gate assigned to it are true:

1. Fixed PE hash, machine, byte windows, and all sealed anchors match.
2. Every predecessor path and SHA-256 binding matches exactly.
3. The independent v3 REVERSE handoff and this preseal are bound before candidate creation.
4. Every required fact reference resolves only to hash-bound `STATIC_CONFIRMED` reverse evidence; design material contributes no fact coverage.
5. That proof's complete required coverage record is closed; incompleteness in another record is not inherited unless the missing claim is shared.
6. Every reachable state-machine transition referenced by that proof is anchored and closed.
7. Every reachable rollback row referenced by that proof is anchored and closed.
8. Every reference class referenced by that proof has closed success, failure, escape, and release paths.
9. For `VIRTUAL_DEVICE_OBJECT_OWNER`, all owner handoffs and callback/reentry escape paths have finite proved bounds and complete release paths.
10. For `VIRTUAL_DEVICE_CREATION_DESTRUCTION`, allocation, construction, control binding, success destruction, and applicable failure rollback prove exactly-one object deletion and control-block free.
11. For `VIRTUAL_DEVICE_REFERENCE_COUNTING`, counter overflow, underflow, resurrection, unmatched release, and double release are excluded by static facts for every reference class.
12. For `VIRTUAL_DEVICE_CALLING_THREAD`, every required operation, including final strong/weak release and rollback actions, has anchored authority/affinity evidence and no required thread is unknown.
13. For `VIRTUAL_DEVICE_VECTOR_INSERT_REMOVE_OWNERSHIP`, insert/grow/copy/remove ownership, publication close, callback/reader drain, and applicable rollback are anchored and closed.
14. Promotion fields and aggregate evidence counts are precisely derived from the five independent assessment statuses.
15. Formal decision, work-package status, locks, runtime facts, and all permissions retain the conservative fixed values.

Failure of a condition forces each assessment whose claim set depends on that condition to `UNKNOWN` with a precise non-empty `missingEvidence` list. Other proofs are evaluated independently. It is invalid to promote any proof on the strength of a constructor/destructor local shape, a mechanical refcount subchain, or a design description when that proof's required owner/lifecycle boundary is not closed.

## 11. Future verifier interface and create-new policy

This preseal does not create a verifier. The future independent v3 verifier identity and paths are reserved as:

- PowerShell entry: `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v3.ps1`
- Python core/tests: `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v3/`
- verification identity: `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecisionVerification-v3`

The future interface must be:

```powershell
pwsh -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v3.ps1 `
  -GameExe <PE> `
  -RepositoryRoot . `
  [-Output <new-json>]
```

It may read only the disk PE, v3 decision, fixed predecessor files, and sealed static evidence. It must not start or enumerate processes, open a process handle, create IPC, inject, hook, send input, or mutate runtime state.

`-Output` is create-new only. It may target only a nonexistent JSON file whose resolved parent is under `out/cs-t0` or `artifacts/cs-t0`. Existing file, symlink/reparse escape, wrong extension, path traversal, alternate data stream, or target outside those roots is rejected before any write. Verification success means the decision accurately represents its conservative static evidence; it does not mean CS-T0-05-05 passes.

## 12. Sealed compressed acceptance run

The future fixture suite is run once after the verifier profile and v3 REVERSE binding are finalized but before candidate creation. After the candidate is published, the fixture suite is not rerun. Candidate acceptance then performs only the one real create-new invocation and one verify-existing invocation.

Exactly four groups are permitted:

1. **Fixed target/hash/anchor plus contract batch (one run).** Verify the fixed PE identity, machine, predecessor digests, baseline and new sealed anchors, exact five-proof set, separated design/fact evidence, state-machine schema, rollback schema, reference classes, thread contract, aggregate rules, conservative state, and all-false permissions.
2. **Create-new successor (one run).** Invoke the future PowerShell entry once against the fixed disk PE and one nonexistent allowed output JSON. Require exit code 0, `verificationStatus=PASS`, and `workPackageStatus=BLOCKED_BY_EVIDENCE`.
3. **Verify-existing (one run).** Invoke once without `-Output` against the same PE and unchanged v3 decision. Require exit code 0 and the same status pair.
4. **Parameterized negative batch (one run).** Mutate isolated fixtures in memory or a temporary test directory; require nonzero exit and no requested output for every case below.

The single negative batch must cover at least:

- promote while callback escape/reentry ownership or retained shared refs are not closed;
- promote with incomplete remove callback, reader admission closure, or reader/callback drain;
- promote with unknown final strong release, object-destruction, or final weak-release thread;
- promote an affected proof with any missing constructor/control-block/insert/grow/copy/callback/publication/remove/drain/stop/early-return rollback;
- promote without excluding counter overflow, underflow, acquire-after-zero, unmatched release, or double release;
- use design/architecture/task/README/test/candidate prose as observed fact evidence;
- use ctor/dtor/refcount local shapes as a substitute for the complete owner boundary;
- a promoted proof with incomplete own coverage, or any `promotionCount`/`promotedIds`/aggregate total inconsistent with the independently derived assessment statuses;
- wrong PE hash/machine, predecessor digest, anchor RVA/bytes/role, candidate-only anchor, or missing v3 REVERSE/preseal binding;
- any permission, runtime flag, implementation flag, or unlock changed to true;
- non-null `formalDecision`, non-blocked package status, or changed necessity assessment;
- missing/extra/duplicate/reordered proof, claim, state, rollback, reference-class, or thread-contract members where exactness is required;
- output outside the two allowed roots, non-JSON output, path escape, or attempted overwrite of an existing output.

This is one parameterized batch, not one standalone run per mutation.

## 13. Explicitly omitted tests

Do not run Cargo full tests, a complete T0 build, Debug/Release Native matrices, game startup, runtime canaries, process probes, injection, historical broad regression, the v1/v2 fixture suites, or repeated self-tests. Existing historical artifacts are not deleted or rewritten.

## 14. Candidate-preseal disposition

At this phase no v3 REVERSE handoff, v3 candidate, v3 verifier, v3 verification output, or v3 test result is created by TEST. The five joint proofs have not been evaluated and are not presumed to pass. This document only fixes the future independent rejection boundary before those artifacts exist.
