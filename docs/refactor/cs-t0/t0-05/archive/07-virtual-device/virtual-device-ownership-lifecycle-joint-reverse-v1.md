> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device ownership/lifecycle joint static reverse v1

Date: 2026-09-08  
Role: independent REVERSE  
Work package: `CS-T0-05-05`  
Evidence class: disk-PE `STATIC_CONFIRMED` observations plus explicit `UNKNOWN` coverage verdicts

## 1. Scope and safety boundary

This handoff evaluates exactly:

1. `VIRTUAL_DEVICE_OBJECT_OWNER`;
2. `VIRTUAL_DEVICE_CREATION_DESTRUCTION`;
3. `VIRTUAL_DEVICE_REFERENCE_COUNTING`;
4. `VIRTUAL_DEVICE_CALLING_THREAD`;
5. `VIRTUAL_DEVICE_VECTOR_INSERT_REMOVE_OWNERSHIP`.

`VIRTUAL_DEVICE_TEARDOWN_DRAIN_RESTORE` remains outside this five-proof batch and remains `UNKNOWN`.
The fixed objective is still `ENDPOINT_B_MUST_REMAIN_OPERATIONAL_WITHOUT_A_SECOND_PHYSICAL_CONTROLLER_ON_AUTHORITY`.
The necessity result remains `REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`; necessity is not implementation eligibility.

Only the already-open IDA database and the disk PE were read. No game was started; no process was enumerated, opened, or read; no x32dbg, injection, hook, IPC, canary, implementation, test, IDB save, patch, comment, rename, or type change was used.
OriginalOnline `NetInputDevice` is used only as a native lifetime counterexample/reference. It is not a positive local C/S component.

## 2. Target and sealed inputs

- IDB: `F:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth\\isaac-ng.exe.i64`
- input PE: `F:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth\\isaac-ng.exe`
- architecture / machine: Win32 x86 / `0x014C`
- image base: `0x00400000`
- image size: `0x008FE000`
- PE SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- address rule: `RVA = VA - 0x00400000`; runtime address is `moduleBase + RVA`
- IDA health: auto-analysis, Hex-Rays, and strings cache ready
- sealed TEST preseal: `virtual-device-ownership-lifecycle-test-plan-v2.md`, SHA-256 `2043AF3F13F834925E3814FA6A1F929C6524709B571F7C9BC55ED37FFD28A623`
- v2 decision: `coop-virtual-device-ownership-lifecycle-decision-v2.json`, SHA-256 `3ACAFAEB608EB1794B8E16477F6FFFE64EC95442E1C6C0EBE7FDC4061E40A916`
- v2 reverse: `virtual-device-ownership-lifecycle-reverse-v1.md`, SHA-256 `9CB6CC4D08DC0F75B0921D0C7B3A5A2753EB5A64930DFCDC58E62F70613A307C`
- v2 TEST result: `virtual-device-ownership-lifecycle-test-results-v1.md`, SHA-256 `5E56396729317B01BCA58693AE7D07C6304598039C693475FA9DFA956C24DEEA`

The inherited 17 production anchors are exactly TEST-preseal Section 2.3: `input-manager-sample`, `input-device-update`, `gamepad-physical-poll`, the three gamepad logical-query image pointers, `player-update-owner`, `player-movement-consumer`, prompt unassign/reassign, the two owner anchors, the three creation anchors, and the two refcount anchors. No inherited anchor was reinterpreted or replaced.

## 3. New fixed anchors

Hex windows below contain no spaces. Image-pointer values are target RVAs after subtracting the preferred image base.

| ID | Kind | RVA | Expected bytes / target | Use |
|---|---|---:|---|---|
| `JRV1_MANAGER_LIFECYCLE_VTABLE_TAIL` | bytes | `0x007829BC` | `F00DA200B00EA200B00FA200D011A200C012A200B013A200` | Manager slots `+0x6C/+0x70/+0x74/+0x78/+0x7C/+0x80` |
| `JRV1_INSERT_UNLOCK_THEN_NOTIFY` | bytes | `0x00620E4C` | `8B47148D4F14FF50108B0783EC088BCC8B70788D450850E8F81EEEFF8BCFFFD6` | unlock, shared-pair copy, `+0x78` callback |
| `JRV1_REMOVE_NOTIFY_BEFORE_ERASE` | bytes | `0x00620EEA` | `8B0783EC088BCC8B707C8D450850E8631EEEFF8BCFFFD68B0783EC088BCC8BB0800000008D450850E8491EEEFF8BCFFFD6` | `+0x7C/+0x80` callbacks precede vector search/erase |
| `JRV1_READER_PAIR_COPY` | bytes | `0x0062042B` | `8B4D0850E82C29EEFFC745FC01000000EBD1` | keyed lookup returns an acquired pair |
| `JRV1_READER_UNLOCK` | bytes | `0x0062040E` | `8B168BCEFF5210` | keyed lookup unlock reached after the acquired pair is formed |
| `JRV1_UPDATE_LOCK_AND_DEVICE_UPDATE` | bytes | `0x0061FC7D` | `FF500C8B45E8C645FC018B76083B700C0F84270200008BF88B0EE8E4F5FFFF` | manager mutex spans entry to per-device update |
| `JRV1_UPDATE_UNLOCK` | bytes | `0x0061FEBA` | `C645FC048BCF8B07FF5010C645FC00` | manager update unlock before later listener notifications |
| `JRV1_DEVICE_WORKER_CREATE` | bytes | `0x00621B30` | `75208B474C8D4F4C5768001000006A0268C020A200FF50088B47040FBAE801894704` | creates worker entry RVA `0x006220C0` |
| `JRV1_DEVICE_WORKER_LOOP` | bytes | `0x006220C0` | `558BEC83E4F851F6051C7BC500025674188B35D882B100E8D4B904006A64FFD6F6051C7BC5000275EE5E8BE55DC3` | calls enumeration RVA `0x0066DAB0`, then sleeps 100 ms |
| `JRV1_WORKER_CONNECT_CALLBACK` | bytes | `0x0066DCBE` | `A1A85DC700893CB5E0E2C70085C07444FF358C8DC70057FFD083C408` | indirect connect callback through global set to RVA `0x00621D40` |
| `JRV1_WORKER_DISCONNECT_CALLBACK` | bytes | `0x0066DA10` | `558BEC518B157C8DC70053568BF15785D27413A1EC9BC700FF35888DC700FF34B0FFD283C408` | indirect disconnect callback through global set to RVA `0x00621F40` |
| `JRV1_WORKER_CONNECT_INSERT` | bytes | `0x00621F02` | `E8E9EEFFFF` | direct call to Manager insert RVA `0x00620DF0` |
| `JRV1_WORKER_DISCONNECT_REMOVE` | bytes | `0x0062206D` | `E8EE0CEEFFB9187BC500E834EEFFFF` | pair copy then direct Manager remove RVA `0x00620EB0` |
| `JRV1_MANAGER_STOP_JOIN_RELEASE` | bytes | `0x00621B97` | `836704FD8D4F4C8B01FF500CF64704010F84DF0000008B47148D4F146AFFFF500C8B570C518B4F08E83CFCFFFF` | stop flag, worker join, lock, vector release |
| `JRV1_CONTROL_NULL_BRANCH` | bytes | `0x00505FB1` | `85F674128BCEE8546CB0FFC7061480B700895E14EB0233F6` | control allocation null branch yields `ESI=0`; inherited insert anchor follows |
| `JRV1_TRY_ADD_STRONG_TWO_PHASE` | bytes | `0x0000C6B0` | `56578BF96AFF8B47088D4F08FF500C8B47088D4F0883C01066837F04007507FF105F32C05EC3FF108B078BCFFF50085FB001` | check under mutex, unlock, then separate `AddStrong` call |
| `JRV1_RELEASE_STRONG_LAST_PATH` | bytes | `0x000358B0` | `568BF1576AFF8B46088D4E08FF500C0FB746046685C0743248668946046685C075288B46088D4E08FF50108B4E1485C9740D8B016A01FF10C74614000000008B068BCE5F5E8B4014FFE0` | last strong deletes object inline, clears `+0x14`, tail-calls weak release |
| `JRV1_RELEASE_WEAK_LAST_PATH` | bytes | `0x0000C5C0` | `56578BF96AFF8B47088D4F08FF500C0FB747066685C0742048668947066685C075168B47088D4F08FF50108B078BCF6A01FF10` | last weak frees control block inline |
| `JRV1_MANAGER_DTOR_CHAIN` | bytes | `0x0062193C` | `C7065029B800E8290200008D4E4CE8718B03008BCEE82AE0FFFF` | derived stop, worker member destruction, base destruction |

## 4. STATIC_CONFIRMED native mechanics

### 4.1 Manager vector, slots, and callbacks

- Global Manager object is at RVA `0x00857B18`; derived vtable RVA `0x00782950`.
- The active vector is `{object, control}` pairs of stride eight at Manager `+0x08/+0x0C/+0x10`; Manager mutex is `+0x14`.
- Vtable slots resolve as: insert `+0x6C -> RVA 0x00620DF0`, remove `+0x70 -> 0x00620EB0`, query `+0x74 -> 0x00620FB0`, insert notification `+0x78 -> 0x006211D0`, and remove notifications `+0x7C -> 0x006212C0`, `+0x80 -> 0x006213B0`.
- ABI for insert/remove/notification routines is x86 `__thiscall(Manager*, object, control)`, callee `ret 8`. Pair copy RVA `0x00502D60` calls control slot `+0x04` (`TryAddStrong`).
- Insert locks the Manager mutex, copies/grows the pair vector, unlocks, then copies the pair again and invokes `+0x78`. The notification routine walks a mutable delegate list at Manager `+0x2C`; each indirect delegate receives a by-value shared pair. Nothing in that routine prevents the delegate from copying and retaining it.
- Remove itself contains no Manager-mutex acquire. It first copies the pair into `+0x7C`, then separately into `+0x80`; only afterward does it search by raw object pointer, shift pairs through RVA `0x00621570`, release the tail control, and decrement end by eight.
- `+0x7C` and `+0x80` walk delegate lists at Manager `+0x34` and `+0x44`. Every delegate call is indirect. Direct xrefs to the three notification implementations are only the two Manager vtables; their dynamic delegate callees are not statically closed.
- Keyed lookup RVA `0x00620390` locks the Manager, finds an entry by `device+0x0C`, copies the pair with `TryAddStrong`, then unlocks and returns the owned pair to its caller. It has 28 direct code callers plus two vtable data xrefs. The returned reader strong reference is outside the Manager lock, and no reader registry or drain counter accompanies it.
- Manager update RVA `0x0061FC00` holds `+0x14` while traversing and polling the current vector. Its invalid-device path invokes `+0x7C/+0x80`, shifts the vector, and releases the tail while still locked; it later unlocks and invokes another delegate list for a temporary vector of removed pairs. This is not a publication-close/readers-drained protocol.
- Derived stop RVA `0x00621B70` clears the device-worker run bit, joins the worker, locks the Manager, releases the vector range, empties it, unlocks, and clears delegate lists. It does not enumerate or drain already escaped reader/callback references.

### 4.2 Concrete worker-thread counterexample

- Manager initialization RVA `0x00621980` creates a worker whose entry is RVA `0x006220C0`.
- The worker calls enumeration RVA `0x0066DAB0` and sleeps 100 ms while the run bit is set.
- Enumeration invokes a function-pointer global initialized to gamepad-connect RVA `0x00621D40`; disconnect RVA `0x0066DA10` invokes the global initialized to gamepad-disconnect RVA `0x00621F40`.
- Connect constructs a Gamepad and control block, then directly calls Manager insert. Disconnect acquires a pair by index, then directly calls Manager remove.
- Consequently, native physical insertion and removal are demonstrably reachable from a distinct worker entry. There is no dispatch from those calls to an authority-main lifecycle phase. This native path is a counterexample to inferring main-thread affinity from Manager method names or mutex use.

### 4.3 Creation/destruction and control block

- Keyboard factory path RVA `0x00621980` allocates `0x370`, constructs `InputDeviceBase`, installs the Keyboard vtable, allocates a `0x18` control block, then publishes through Manager slot `+0x6C`.
- Gamepad-connect RVA `0x00621D40` allocates `0x4F0`, constructs the base and Gamepad state, deletes the object if its device initializer fails, otherwise allocates the `0x18` control block and inserts it.
- OriginalOnline Net objects are allocated/constructed only from RVA `0x002F6500` through ctor RVAs `0x00501930/0x005015B0`, wrapper RVA `0x00505F60`, and deleting destructor RVA `0x005017C0`. This proves a native reference shape only, not an allowed local C/S factory.
- In wrapper RVA `0x00505F60`, if the `0x18` control-block allocation returns null, execution sets the control pointer to zero and still reaches the inherited Manager-insert anchor with `{object, null-control}`. The same null-control publication shape exists in Keyboard/Gamepad construction. No refcount owner can later delete that raw object through this pair.
- No independent virtual `InputDeviceBase` allocation factory, non-OriginalOnline lifecycle coordinator, rollback table, or failure-complete publication transaction exists in the inspected binary.

### 4.4 Reference-count mechanics

- InputDevice control block layout is: vptr `+0x00`; `uint16 strong +0x04`; `uint16 weak +0x06`; embedded mutex `+0x08/+0x0C/+0x10`; managed object `+0x14`.
- Constructor RVA `0x0000CC10` initializes strong/weak to `1/1`. Vtable RVA `0x00778014` has six slots: deleting dtor `0x0000CB50`, `TryAddStrong 0x0000C6B0`, `AddStrong 0x0000C690`, `ReleaseStrong 0x000358B0`, `AddWeak 0x0000C610`, `ReleaseWeak 0x0000C5C0`.
- Direct xrefs to ref primitives are vtable data refs: 20 each for deleting dtor, `TryAddStrong`, `AddStrong`, `AddWeak`, and `ReleaseWeak`; 11 for this `ReleaseStrong` specialization. Control-block ctor has 37 direct code callers. Actual acquire/release calls are indirect through those vtables.
- `AddStrong` and `AddWeak` increment a 16-bit counter under the embedded mutex with no maximum/overflow check.
- `ReleaseStrong` and `ReleaseWeak` test zero before decrement, so the primitive does not itself decrement from zero. That does not prove matching live-reference discipline or prevent an unmatched second release at the alias level.
- On last strong, `ReleaseStrong` unlocks, calls managed object vslot zero with deleting flag one, clears `+0x14`, then invokes weak release. On last weak, `ReleaseWeak` unlocks and invokes control-block deleting dtor. Both happen inline on the releasing caller thread.
- `TryAddStrong` checks nonzero strong under the mutex, unlocks, and only then invokes the separate `AddStrong` slot. Without an external lifetime/thread exclusion, another releaser can perform last-strong destruction between the check and increment; the binary contains no atomic combined check-and-increment in this helper.

## 5. Exact five verdicts

| Proof | Verdict | Closed subchain | First unclosed fact / minimal counterexample |
|---|---|---|---|
| `VIRTUAL_DEVICE_OBJECT_OWNER` | `UNKNOWN` | Manager vector holds one strong per non-null stored pair; callback/read copies use `TryAddStrong` | A `+0x78/+0x7C/+0x80` delegate or keyed-lookup caller can retain a copied pair after remove returns. Manager has no registry for that alias, so final-release authority/cardinality is not closed. No non-OriginalOnline virtual owner exists. |
| `VIRTUAL_DEVICE_CREATION_DESTRUCTION` | `UNKNOWN` | Concrete Keyboard/Gamepad/Net success ctor and deleting-dtor shapes are identified | At `JRV1_CONTROL_NULL_BRANCH`, control allocation null still flows to insertion; later vector removal cannot ref-delete the object. Constructor/control/insert/grow/callback exception-equivalent rollback and an independent virtual factory are also absent. |
| `VIRTUAL_DEVICE_REFERENCE_COUNTING` | `UNKNOWN` | Layout, six slots, zero guard, last-strong object delete, and last-weak control free are closed mechanically | `uint16` acquire has no proved bound/overflow guard; `TryAddStrong` separates check from increment; indirect alias acquires/releases, rollback balance, acquire-after-zero exclusion, and final-release authority are not closed. |
| `VIRTUAL_DEVICE_CALLING_THREAD` | `UNKNOWN` | A concrete worker entry and inline release behavior are identified | Worker RVA `0x006220C0` reaches native insert/remove directly. Final strong/weak release runs on whichever caller drops the last alias. No anchored authority-main dispatch/affinity covers creation, callbacks, readers, rollback, removal, or final release. |
| `VIRTUAL_DEVICE_VECTOR_INSERT_REMOVE_OWNERSHIP` | `UNKNOWN` | Insert/grow/copy/shift/tail-release/stop/dtor mechanics are identified | Remove has no own Manager lock, notifies before erase, and has no publication-close or reader/callback drain; keyed lookup deliberately returns a strong reference beyond the lock. Vector/callback failure and reentry ownership are unclosed. |

Promotion count supported by this handoff is therefore `0`.

## 6. State, rollback, reference, and thread coverage

### 6.1 State-machine coverage

| Required transition/action | Fact status |
|---|---|
| `EMPTY -> OBJECT_ALLOCATED` | native physical/OriginalOnline examples only; independent virtual factory `UNKNOWN` |
| constructor + control binding -> `CONTROL_READY` | normal shape confirmed; null-control and constructor/member rollback `UNKNOWN` |
| vector insert + publication -> `REGISTERED` | insert mechanics confirmed; callback failure/reentry and null-control publication `UNKNOWN` |
| publication close + reader/callback drain -> `RETIRING` | no such barrier/counter/drain observed; `UNKNOWN` |
| vector removal -> `UNREGISTERED` | erase mechanics confirmed; locking, notification failure/reentry, and drain precondition `UNKNOWN` |
| final strong/weak actions -> `DESTROYED` | primitive mechanics confirmed; alias cardinality, thread, exactly-once full-path proof `UNKNOWN` |

### 6.2 Rollback first gaps

`OBJECT_ALLOCATION_FAILURE`, `OBJECT_CONSTRUCTION_FAILURE`, `CONTROL_BLOCK_CONSTRUCTION_FAILURE`, `VECTOR_INSERT_FAILURE`, `VECTOR_GROW_OR_COPY_FAILURE`, `INSERT_NOTIFICATION_FAILURE`, `CALLBACK_REENTRY_OR_ESCAPE`, `PUBLICATION_FAILURE`, `REMOVE_CALLBACK_FAILURE`, `READER_DRAIN_FAILURE`, `STOP_OR_SHUTDOWN_EARLY_RETURN`, and `FINAL_RELEASE_FAILURE_OR_WRONG_THREAD` have no complete anchored rollback/affinity proof. `CONTROL_BLOCK_ALLOCATION_NULL` has a stronger negative fact: it still publishes `{object, null}`. None may be marked closed for an affected proof.

### 6.3 Reference classes

- `MANAGER_VECTOR_STRONG`: acquire/release mechanics confirmed, but duplicate-object insertion and total per-object cardinality are not bounded.
- `INSERT_CALLBACK_TRANSIENT_STRONG`, `REMOVE_CALLBACK_TRANSIENT_STRONG`, `READER_TRANSIENT_STRONG`: acquisition is confirmed; escape, maximum retained cardinality, permitted release thread, and drain are `UNKNOWN`.
- `FACTORY_LOCAL_STRONG`: native temporary pairs are visible; an independent virtual factory and all failure releases are `UNKNOWN`.
- `ROLLBACK_TRANSIENT_STRONG`: no complete rollback owner was found.
- `CONTROL_IMPLICIT_WEAK`: initial one and last-strong release are confirmed mechanically.
- `EXTERNAL_WEAK`: AddWeak/ReleaseWeak primitives exist; all external weak owners, bounds, and release sites are not closed.

### 6.4 Thread contract

- manager tick/poll has a main-loop-shaped static caller, but the PE does not enforce the required `AUTHORITY_MAIN_LIFECYCLE` affinity for every caller;
- physical connect/insert and disconnect/remove are reachable on the explicit device worker;
- Manager notifications run inline on the insert/remove caller;
- keyed-reader release runs on its external caller;
- last strong/object deletion and last weak/control free run inline on the final releasing caller;
- stop/destructor and every failure rollback lack a complete caller-thread/main-loop-phase proof.

Therefore none of the exact TEST thread classes can be assigned to every required lifecycle operation using static fact evidence.

## 7. Decision boundary

- `necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`
- `formalDecision=null`
- `workPackageStatus=BLOCKED_BY_EVIDENCE`
- the five assessed proofs remain `UNKNOWN`
- `VIRTUAL_DEVICE_TEARDOWN_DRAIN_RESTORE` remains outside this batch and `UNKNOWN`
- virtual-device implementation, runtime observation, process access, active/passive permissions, input/world mutation, and CS-T0-05-06 unlock remain forbidden/false

The native binary demonstrates useful local mechanics, but it does not prove a safe future independent virtual-device owner/lifecycle. In particular, the null-control publication, worker-thread insert/remove, escaped reader/callback references, absent drain barrier, and unbounded/two-phase refcount acquisition are sufficient static blockers. No further whole-program scan is necessary to state this conservative result.
