> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 ownership/lifecycle successor independent TEST preseal v1

Date: 2026-09-08 (Asia/Shanghai)

Author role: independent TEST. This document is sealed before the successor
REVERSE handoff exists. TEST does not perform reverse engineering and does not
assume that any of the three candidate promotions will pass.

## Fixed successor identity

- candidate: `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v2.json`
- decision identity:
  `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecision-v2`
- verification identity:
  `IsaacCsT0-CoopVirtualDeviceOwnershipLifecycleDecisionVerification-v2`
- public entry:
  `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v2.ps1`
- TEST implementation:
  `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v2/`
- target: Win32/x86 approved-unpacked PE SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- fixed goal:
  `ENDPOINT_B_MUST_REMAIN_OPERATIONAL_WITHOUT_A_SECOND_PHYSICAL_CONTROLLER_ON_AUTHORITY`

The successor is create-new only. It preserves
`necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`,
`formalDecision=null`, and `workPackageStatus=BLOCKED_BY_EVIDENCE`. It keeps
all permissions false, claims no runtime/process/game/IPC/implementation work,
and keeps `csT00506Unlocked`, `t005Unlocked`, and `t006Unlocked` false.

## Exact evidence transition

The six v1 dependency findings remain `STATIC_CONFIRMED`. Only these three v1
unknowns may be assessed for promotion:

1. `VIRTUAL_DEVICE_OBJECT_OWNER`
2. `VIRTUAL_DEVICE_CREATION_DESTRUCTION`
3. `VIRTUAL_DEVICE_REFERENCE_COUNTING`

The other seven IDs remain exactly `UNKNOWN`, with null findings and precise
non-empty `requiredEvidence`:

- `VIRTUAL_DEVICE_CALLING_THREAD`
- `VIRTUAL_DEVICE_KEY_ALLOCATION`
- `VIRTUAL_DEVICE_VECTOR_INSERT_REMOVE_OWNERSHIP`
- `VIRTUAL_DEVICE_VTABLE_COL_SLOT_ABI`
- `VIRTUAL_DEVICE_ASSIGNMENT_INTEGRATION`
- `VIRTUAL_DEVICE_GENERATION_LIFECYCLE`
- `VIRTUAL_DEVICE_TEARDOWN_DRAIN_RESTORE`

Those seven IDs must always remain in `missingEvidence`. No promotion is
presumed: each of the three assessments may remain `UNKNOWN`, in which case it
also remains in `missingEvidence`. `promotedIds`, `retainedUnknownIds`, and
`promotionCount` must be derived exactly from the assessment statuses. Even if
all three promotions are accepted, they do not imply implementation eligibility
or unlock 05-06.

## Promotion assessment schema

The candidate contains exactly three ordered `promotionAssessments`. Every
assessment has exactly `id`, `status`, `claim`, `evidenceRefs`, `anchorIds`,
`pathClasses`, `coverage`, and `requiredEvidence`. `status` is either
`STATIC_CONFIRMED` or `UNKNOWN`.

For `STATIC_CONFIRMED`, `claim`, references, and anchors must be non-empty,
`requiredEvidence` must be null, and the exact path/coverage conditions below
must all hold. Every reference must resolve to the hash-bound successor REVERSE
handoff. Every anchor ID must be an exact fixed-target anchor sealed into the
v2 verifier after that handoff is final. No candidate-selected RVA or bytes are
trusted.

For `UNKNOWN`, `claim` is null, `anchorIds` and `pathClasses` are empty,
`requiredEvidence` is precise and non-empty, the exact coverage keys are still
present with boolean values, and at least one coverage value is false. An
`UNKNOWN` row cannot be silently omitted or presented as a proof.

### `VIRTUAL_DEVICE_OBJECT_OWNER`

`pathClasses` is exactly:

```text
ALLOCATE_TO_CONSTRUCT
CONSTRUCT_TO_PUBLISH
PUBLISH_TO_VECTOR_OWNERSHIP
VECTOR_TO_REMOVE
REMOVE_TO_FINAL_RELEASE
```

All exact coverage flags must be true:

```text
ownerIdentityClosed
responsibilityBoundaryClosed
allocationAuthorityClosed
publicationAuthorityClosed
removalAuthorityClosed
finalReleaseAuthorityClosed
transferAliasUniverseClosed
```

This rejects a constructor/destructor-only claim: the owner identity,
responsibility boundary, transfer/alias universe, publication/removal
authority, and final-release authority must all be closed.

### `VIRTUAL_DEVICE_CREATION_DESTRUCTION`

`pathClasses` is exactly:

```text
SUCCESS
ALLOCATION_FAILURE
CONSTRUCTOR_FAILURE
PRE_PUBLICATION_EARLY_RETURN
POST_CONSTRUCTION_PRE_PUBLICATION_FAILURE
POST_PUBLICATION_ROLLBACK
PARTIAL_INITIALIZATION_CLEANUP
NORMAL_TEARDOWN
```

All exact coverage flags must be true:

```text
allocationPreconditionsClosed
constructorSuccessClosed
constructorFailureClosed
destructorEntryClosed
deallocationClosed
failurePathsClosed
earlyReturnPathsClosed
rollbackPathsClosed
reentrancyOrUnwindDispositionClosed
exactlyOnceCleanupClosed
```

Local ctor/dtor shapes are insufficient. Missing allocation preconditions,
failure branches, early returns, partial initialization cleanup, post-create
rollback, unwind/reentrancy disposition, or exactly-once cleanup rejects the
promotion.

### `VIRTUAL_DEVICE_REFERENCE_COUNTING`

`pathClasses` is exactly:

```text
SHARED_COPY
SHARED_MOVE
SHARED_RESET
VECTOR_INSERT
VECTOR_ERASE
STRONG_NONLAST_RELEASE
STRONG_LAST_RELEASE_OBJECT_DESTROY
WEAK_NONLAST_RELEASE
WEAK_LAST_RELEASE_CONTROL_BLOCK_FREE
ROLLBACK_RELEASE
```

All exact coverage flags must be true:

```text
controlBlockLayoutClosed
sharedAcquireClosed
sharedReleaseClosed
strongLastReleaseClosed
objectDestructionDispatchClosed
weakAcquireClosed
weakReleaseClosed
weakLastReleaseClosed
controlBlockDeallocationClosed
copyMoveResetEraseClosed
rollbackReleaseClosed
destructionThreadContractClosed
underflowAndDoubleReleaseExcluded
```

Closing only a shared decrement or destructor call is insufficient. The
strong last release, object destruction dispatch, weak last release, control
block free, copy/move/reset/vector paths, rollback release, thread contract,
and underflow/double-release exclusions are all mandatory.

## Hash and anchor seal

The formal v2 profile binds v1 decision, v1 verification, v1 REVERSE handoff,
v1 TEST report, the immutable v1 verifier core used for PE parsing, this
preseal, and the final successor REVERSE handoff by exact repository-relative
path and SHA-256. The successor handoff and its proof anchors are currently
`PENDING_REVERSE_SEAL`. TEST must replace that slot with exact path/hash/RVA/
kind/bytes-or-target-RVA constants and rerun the four preparation groups before
the mainline candidate is created. A pending, extra, reordered, missing, or
candidate-selected binding/anchor is rejected.

## Compressed acceptance set

Only four groups are permitted:

1. one fixed PE/hash/three-proof-anchor batch;
2. one create-new successor verification;
3. one deterministic read-only verify-existing check;
4. one parameterized negative batch covering local ctor/dtor-only promotion,
   missing owner boundary, incomplete strong/weak last release, omitted
   failure/early-return/rollback coverage, promotion of any remaining unknown,
   permission/unlock escalation, wrong hash/anchor, and output overwrite.

Before the successor REVERSE seal and candidate exist, the public entry must
return a preparation status and write nothing. After sealing, a successful
verification still reports `workPackageStatus=BLOCKED_BY_EVIDENCE`; it is not a
05-05 PASS.

TEST will not run Cargo full tests, the full T0 build, Native Debug/Release
matrices, historical broad regression, a live canary, or the v1 verifier/test.
No game process may be enumerated, opened, or started; no IPC, hook, input,
virtual device, or runtime mutation is authorized.
