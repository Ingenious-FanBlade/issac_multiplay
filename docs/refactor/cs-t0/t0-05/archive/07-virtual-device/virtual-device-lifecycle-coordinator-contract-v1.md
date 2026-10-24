> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 VirtualDeviceLifecycleCoordinator contract v1

Date: 2026-09-08 (Asia/Shanghai)  
Evidence class: `DESIGN_ONLY`  
Implementation status: `DEFINED_NOT_IMPLEMENTED`

## Purpose and evidence boundary

This contract defines the lifecycle that a future local C/S virtual input
device would have to implement. It is not evidence that the fixed game binary
already implements the contract, and it contributes no `STATIC_CONFIRMED`
coverage. Every factual promotion must come from the independently produced,
hash-bound REVERSE handoff over the approved Win32/x86 disk PE.

The coordinator is the sole semantic lifecycle authority. It decides whether
creation, publication, retirement, removal, and final release may proceed. It
is not assumed to be the sole strong-reference holder: the InputManager vector
and bounded synchronous operation copies may temporarily own strong
references. Player and assignment consumers hold only a device key and never
own the object.

This contract does not implement a virtual device, select a device key, define
the full InputDevice vtable, integrate assignment, or authorize game/runtime
access. OriginalOnline `NetInputDevice` is not a permissible local C/S
implementation component.

## Exact state machine

Only these states exist:

```text
EMPTY
  -> OBJECT_ALLOCATED
  -> CONTROL_READY
  -> REGISTERED
  -> RETIRING
  -> UNREGISTERED
  -> DESTROYED
```

| Transition | From -> To | Required action and owner/ref delta | Required thread | Failure disposition |
| --- | --- | --- | --- | --- |
| `LC_ALLOCATE` | `EMPTY -> OBJECT_ALLOCATED` | Allocate an unpublished object. Coordinator becomes its unique raw allocation owner; no refcount object exists yet. | `AUTHORITY_MAIN_LIFECYCLE` | Allocation failure leaves `EMPTY` with no cleanup action. |
| `LC_CONSTRUCT_CONTROL` | `OBJECT_ALLOCATED -> CONTROL_READY` | Finish object construction, allocate and initialize the control block, bind the managed pointer, acquire exactly one `FACTORY_LOCAL_STRONG` and one `CONTROL_IMPLICIT_WEAK`. The factory-local strong becomes the coordinator root without another increment. | `AUTHORITY_MAIN_LIFECYCLE` | Destroy/free the raw object if binding never completed; otherwise release the one root through the frozen ABI. End at `EMPTY` or `DESTROYED`. |
| `LC_REGISTER` | `CONTROL_READY -> REGISTERED` | Insert one shared pair into InputManager. Acquire exactly one `MANAGER_VECTOR_STRONG`; any insert callback copy is synchronous and bounded. Publication is considered complete only after vector membership and callback return are both confirmed. | `AUTHORITY_MAIN_LIFECYCLE`; callback executes as `MANAGER_CALLBACK` | Before vector membership, release operation copies then root. After membership, use the exact manager removal path; never drop the root first. |
| `LC_BEGIN_RETIRE` | `REGISTERED -> RETIRING` | Close producer and reader admission, publish neutral input, prevent new callback/ref acquisition, then drain all admitted readers and callbacks. No ownership is released yet. | `AUTHORITY_MAIN_LIFECYCLE`; admitted users are `READER`/`MANAGER_CALLBACK` | On incomplete drain, stay fail-closed in `RETIRING` with root and vector ownership intact. Destruction is forbidden. |
| `LC_REMOVE` | `RETIRING -> UNREGISTERED` | Invoke the exact InputManager remove operation after drain, wait for remove callbacks to return, confirm vector absence, and account for release of `MANAGER_VECTOR_STRONG` and every remove transient. | `AUTHORITY_MAIN_LIFECYCLE`; callback executes as `MANAGER_CALLBACK` | If absence cannot be confirmed, remain `RETIRING`; retain the coordinator root and do not attempt final release. |
| `LC_FINAL_RELEASE` | `UNREGISTERED -> DESTROYED` | Release the coordinator root. It must be the final strong release, synchronously dispatch exactly one object destruction, clear the managed pointer, release the implicit weak, and free the control block exactly once when no permitted weak remains. | `AUTHORITY_MAIN_LIFECYCLE` | A wrong-thread or unmatched-release condition is a hard stop before decrement; never compensate with a second release. |

There is no direct `REGISTERED -> DESTROYED`, `RETIRING -> DESTROYED`, or
`CONTROL_READY -> REGISTERED` publication shortcut that omits confirmed vector
ownership. State changes are coordinator-only and monotonic. A teardown
operation that cannot finish is quarantined in `RETIRING`; it is not reported
as destroyed and cannot grant implementation eligibility.

## Reference classes and bounds

All strong and weak acquire/release operations must use the hash-bound engine
ABI. Manual arithmetic on the 16-bit counters is forbidden.

| Reference class | Contract owner and lifetime | Maximum cardinality | Escape rule |
| --- | --- | ---: | --- |
| `FACTORY_LOCAL_STRONG` | Coordinator root from `CONTROL_READY` through confirmed `UNREGISTERED` | 1 | Never copied into Player, assignment, IPC, scheduler, or helper-thread state. |
| `MANAGER_VECTOR_STRONG` | InputManager vector from successful insert through confirmed removal | 1 | Only the exact vector slot may retain it. |
| `INSERT_CALLBACK_TRANSIENT_STRONG` | Synchronous insert notification | 1 | Must be released before `LC_REGISTER` returns; retention or reentry fails the contract. |
| `REMOVE_CALLBACK_TRANSIENT_STRONG` | Synchronous remove notification | 1 | Must be released before `LC_REMOVE` returns; retention or reentry fails the contract. |
| `READER_TRANSIENT_STRONG` | One admitted poll/query reader inside the proved reader phase | 1 | Admission closes before drain; no reader survives confirmed drain. |
| `ROLLBACK_TRANSIENT_STRONG` | One coordinator operation-local rollback copy | 1 | Cannot be published or stored; released on every return edge. |
| `CONTROL_IMPLICIT_WEAK` | Control block from binding through object destruction | 1 | Released exactly once after managed-pointer clear. |
| `EXTERNAL_WEAK` | Not supported by contract v1 | 0 | Any observed acquire keeps the proof and implementation eligibility blocked. |

The conservative design ceiling is six simultaneous strong references and one
weak reference, both far below `uint16` maximum. This ceiling is a requirement,
not a fact: static evidence must prove the actual native copy, grow, callback,
reader, rollback, and reentry paths stay within it. Any extra reference class,
unbounded callback retention, acquire-after-final-release, zero decrement,
unmatched release, or possible double release fails the contract.

## Rollback matrix

| Failure ID | Pre-state | Required ordered recovery | Required safe result |
| --- | --- | --- | --- |
| `OBJECT_ALLOCATION_FAILURE` | `EMPTY` | Record failure only. | `EMPTY`; no object/control/ref. |
| `OBJECT_CONSTRUCTION_FAILURE` | `OBJECT_ALLOCATED` | Destroy initialized members in reverse order; free raw object. | `EMPTY`; exactly-once partial cleanup. |
| `CONTROL_BLOCK_ALLOCATION_NULL` | `OBJECT_ALLOCATED` | Do not form or publish `{object, null-control}`; destroy/free object. | `EMPTY`. |
| `CONTROL_BLOCK_CONSTRUCTION_FAILURE` | `OBJECT_ALLOCATED` | Destroy any control members, free control allocation, then destroy/free object. | `EMPTY`. |
| `VECTOR_INSERT_FAILURE` | `CONTROL_READY` | Release insertion temporaries; confirm no vector membership; release root through ABI. | `DESTROYED`. |
| `VECTOR_GROW_OR_COPY_FAILURE` | `CONTROL_READY` or provisional registration | Undo every completed destination acquire, preserve or restore the original vector, confirm no virtual entry, release root. | `DESTROYED`; existing devices unchanged. |
| `INSERT_NOTIFICATION_FAILURE` | provisional registration | Close publication; drain callback; remove confirmed entry; release callback/local refs, then root. | `DESTROYED`. |
| `CALLBACK_REENTRY_OR_ESCAPE` | any callback edge | Reject publication/removal completion and retain safe owners; never guess alias cardinality. | `CONTROL_READY` before publication, otherwise fail-closed `RETIRING`; proof stays `UNKNOWN`. |
| `PUBLICATION_FAILURE` | provisional registration | Close admission; drain; remove confirmed entry; release temporaries and root in that order. | `DESTROYED`. |
| `REMOVE_CALLBACK_FAILURE` | `RETIRING` | Keep admission closed and both coordinator/vector ownership intact until callback quiescence and absence can be proved. | `RETIRING`; no final release. |
| `READER_DRAIN_FAILURE` | `RETIRING` | Keep neutral/closed state and all long-lived owners; do not remove or decrement. | `RETIRING`; no destruction. |
| `STOP_OR_SHUTDOWN_EARLY_RETURN` | any nonterminal state | Route through the same retire/drain/remove sequence; never bypass it because shutdown is in progress. | `DESTROYED` only after all confirmations; otherwise fail-closed current state. |
| `FINAL_RELEASE_FAILURE_OR_WRONG_THREAD` | `UNREGISTERED` | Do not decrement; enqueue no helper-thread fallback; retain root for an allowed main-lifecycle point. | `UNREGISTERED`; no duplicate release. |

Rows that intentionally retain a safe nonterminal state describe a fail-closed
quarantine, not a completed rollback. They remain factual proof gaps until an
implementation supplies and independently verifies a bounded completion path.

## Thread and drain contract

- Allocation, construction, control binding, registration, publication close,
  drain completion, removal, rollback, final strong release, object
  destruction, and final weak release must originate from
  `AUTHORITY_MAIN_LIFECYCLE` at a statically identified main-loop phase.
- `MANAGER_CALLBACK` is synchronous under the manager operation that invoked
  it. It may neither retain a reference nor schedule a later release. A native
  callback implementation that cannot be proved non-escaping blocks the
  corresponding factual coverage.
- `READER` admission is permitted only while state is `REGISTERED`. The state
  change to `RETIRING` must close admission before drain begins. The drain
  predicate is zero admitted readers and zero callback transients, established
  by protocol rather than by a sampled zero.
- No IPC, transport, scheduler, debugger, or helper thread may execute an
  operation that can become the last strong or last weak release.
- Inline refcount deletion proves dispatch mechanics only. It does not prove
  the caller thread; the caller chain and main-loop phase require independent
  static evidence.

## Factory and ABI rules

- A future factory returns an explicit status and an unpublished coordinator
  handle. No exception may cross an engine ABI boundary.
- The object and control block must both be valid before manager insertion.
  Null control allocation cannot produce a shared pair or reach a callback.
- The factory may use only the independently frozen InputDevice and control
  block ABI. It may not instantiate or delegate lifecycle authority to
  OriginalOnline `NetInputDevice` or Online Manager state.
- No assignment or selected-device-key mutation is part of this contract.
  Those operations remain separate `UNKNOWN` decision items.

## Formal disposition

This document fixes the desired behavior but proves none of it. Until a
hash-bound implementation and independent evidence close every applicable
path, the relevant decision rows remain `UNKNOWN`,
`formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`,
`implementationEligibility=false`, and every runtime/active permission and
unlock remains `false`.
