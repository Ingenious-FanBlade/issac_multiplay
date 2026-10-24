> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 virtual-device ownership/lifecycle joint static reverse v2

Date: 2026-09-08  
Role: independent REVERSE  
Evidence class: read-only disk-PE `STATIC_CONFIRMED` observations and explicit `UNKNOWN` verdicts

## 1. Scope and safety boundary

This continuation is limited to the remaining native lifecycle questions: Manager delegate registration/removal, the keyed-reader caller universe, shared-pair copy/reset/release mechanics, shutdown caller provenance, and final strong/weak release caller provenance. It does not promote an offline design into native evidence.

Only IDA static reads and the disk PE were used. No game was started; no process was enumerated, opened, or read; no x32dbg, IPC, hook, injection, canary, IDB edit/save, implementation, or test was used. OriginalOnline `NetInputDevice` remains a negative/reference shape only and is not a local C/S component.

## 2. Target and sealed inputs

- IDB: `F:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth\\isaac-ng.exe.i64`
- disk PE: `F:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth\\isaac-ng.exe`
- machine / image base / image size: Win32 x86 `0x014C` / `0x00400000` / `0x008FE000`
- PE SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- address rule: `RVA = VA - 0x00400000`; runtime address is `moduleBase + RVA`
- IDA health: auto-analysis, Hex-Rays, and strings cache ready
- TEST preseal v1: SHA-256 `4B503EC14184A75BAE8F92BD78BEA173557E3652347744EC6BCAD99D41551B39`
- decision v3: SHA-256 `6E069DD4A66D90A4122D4D1D189ED99C2CFA101357515CC6D2AEBC2A9F01E219`
- coordinator contract v1 (`DESIGN_ONLY`): SHA-256 `7999DA674CC04905D857A7F542174E564FDA33082D492A0B44D31849C40A9665`
- joint REVERSE v1: SHA-256 `5C3BEB037C03C51F42EC85E7E513FAD3D23A1F0FCCC0A1059B137A3A581B7C38`

All 36 decision-v3 anchors are inherited unchanged. The following anchors are additive; image-pointer targets are recorded after subtracting the preferred image base.

## 3. Additive static anchors

| ID | RVA | Expected bytes / image-pointer target | Meaning |
|---|---:|---|---|
| `JRV2_INSERT_DELEGATE_REGISTRATION` | `0x00552C36` | `6A006890B29500B9447BC500E8C9A90C00` | register target RVA `0x0055B290`, context `0`, into Manager `+0x2C` via RVA `0x0061D610` |
| `JRV2_REMOVE_DELEGATE_REGISTRATION` | `0x00506E70` | `566860379100B94C7BC500E890671100` | register target RVA `0x00513760`, caller context, into Manager `+0x34` |
| `JRV2_DELEGATE_BULK_CLEAR` | `0x00621C1A` | `E8E188FFFF8B472C89008B472C894004C74730000000008B5734E8C788FFFF8B473489008B4734894004C74738000000008B573CE8AD88FFFF8B473C89008B473C894004C74740000000008B5744E89388FFFF8B47448D4F1489008B4744894004C7474800000000` | stop bulk-frees and resets lists `+0x2C/+0x34/+0x3C/+0x44` |
| `JRV2_LOOKUP_BASE_VTABLE_ENTRY` | `0x0078281C` | image pointer `0x00620390` (`9003A200`) | base Manager keyed-lookup indirect entry, table RVA `0x007827F4 + 0x28` |
| `JRV2_LOOKUP_DERIVED_VTABLE_ENTRY` | `0x00782978` | image pointer `0x00620390` (`9003A200`) | derived Manager keyed-lookup indirect entry, table RVA `0x00782950 + 0x28` |
| `JRV2_PAIR_COPY_ACQUIRE_RELEASE_CORE` | `0x00502D78` | `8B78048B1885FF74338B078BCF8B4004FFD084C074268B4E0485C9741A8B018B400CFFD084C0740FA114E8C700` | pair copy uses control slot `+0x04`; replacement cleanup uses slot `+0x0C` |
| `JRV2_PAIR_SWAP_ASSIGN_CORE` | `0x00502EAF` | `E8ACFEFFFF8B4F048BF08B5604894E048B078957048B0E8906890FC745FC000000008B4DEC85C974` | copy-to-temp, swap, then old-pair cleanup |
| `JRV2_PAIR_DTOR_RELEASE_CORE` | `0x000242D5` | `8B4E0485C9741A8B018B400CFFD084C0740FA114E8C70085C074` | pair destructor dispatches control slot `+0x0C` inline |
| `JRV2_PAIR_RESET_CONTROL_CORE` | `0x00502F3C` | `E87FC510008BF883C404897DF08B5D08C745FC0000000085FF74128BCFE8B29CB0FFC7071480B700895F14EB0233FF` | reset allocates/binds a new `0x18` control; both direct users pass raw pointer zero |
| `JRV2_STOP_DIRECT_MAIN_CALL` | `0x0053140E` | `E85D031400` | `_main` calls shutdown chain RVA `0x00671770`, which directly calls Manager stop |
| `JRV2_ATEXIT_TARGET_BINDING` | `0x0000232C` | `68106CB100` | static initialization supplies cleanup RVA `0x00716C10` to `atexit` |
| `JRV2_ATEXIT_REGISTER_CALL` | `0x00002374` | `E836D26E00` | actual `atexit` registration call |
| `JRV2_ATEXIT_STOP_ENTRY` | `0x00716C3E` | `C705187BC5005029B800E823AFF0FF` | atexit cleanup restores Manager vtable and directly invokes stop |

## 4. Exhaustion results

### 4.1 Delegate lists `+0x78/+0x7C/+0x80`

The notification slots remain `+0x78 -> RVA 0x006211D0`, `+0x7C -> 0x006212C0`, and `+0x80 -> 0x006213B0`; their lists are Manager `+0x2C`, `+0x34`, and `+0x44` respectively.

- Generic registrar RVA `0x0061D610` has exactly 14 direct code xrefs. Every call was inspected. Exactly two target the three Manager lists: RVA `0x00552C42` registers `0x0055B290` into `+0x2C`; RVA `0x00506E7B` registers `0x00513760` into `+0x34`. No registrar xref targets `+0x44`.
- Target RVA `0x0055B290` has one static function-pointer xref, the registration above. It consumes the by-value pair only to pass `device+0x0C` to RVA `0x00435C90`, then releases the callback pair before return; it does not store the pair.
- Target RVA `0x00513760` likewise has one function-pointer xref. It performs disconnect bookkeeping and may reset other stored device pairs with raw pointer zero, then releases the callback pair before return; it does not store the supplied pair.
- Generic whole-list clear RVA `0x0061A500` has 28 direct code xrefs. For these Manager lists, the observed deregistration is only the four-list bulk clear in Manager stop. The registrar return values are discarded, and no Manager per-node unregister handle/path was found.
- Thus the currently registered `+0x78/+0x7C` callback pair itself is synchronous and non-retained, while `+0x80` has no statically registered target. This closes only the concrete delegate-target subchain. It does not close reader aliases, stop races, or future independent-virtual lifecycle authority.

### 4.2 Keyed lookup caller universe

RVA `0x00620390` has exactly 30 static xrefs: 28 code calls across 25 functions plus the two vtable cells frozen above. The 28 direct callsite RVAs are:

```text
0023D12 002AF89 029200C 02F6A24 038323F 03B9F9C 03BDBA6
0435BD7 0435CCF 0435E07 04D21C8 04D8B6A 04D9154 04D9395
04DA06A 04DA79E 0501AF2 0506054 050AAB3 0534B0B 0535046
055661E 055AF6A 055AFEE 055B1BA 0589B26 058DD51 05B279D
```

Every direct call receives a caller-owned `{object, control}` temporary after the lookup's Manager mutex is released. The direct functions include gameplay/UI readers, OriginalOnline-only readers, removal helpers at RVAs `0x00506020/0x0050A8A0`, and the registered insert callback's downstream reader at RVA `0x00435C90`. No admission token, reader registry, or drain joins those temporaries to Manager removal.

The first unclosed edge is the two vtable cells: indirect `call [Manager-vtable+0x28]` sites cannot be reduced to the 28 direct xrefs by code-xref enumeration. Therefore the complete dynamic reader/alias universe and the thread that drops its last pair are not statically closed.

### 4.3 Copy, reset, erase, stop, and final release

- Pair copy RVA `0x00502D60` has 20 direct code xrefs and conditionally invokes control slot `+0x04` (`TryAddStrong`). Pair swap/assignment RVA `0x00502E80` has two direct callers; it copies to a temporary, swaps `{object,control}`, then releases the replaced temporary. Pair destructor RVA `0x000242B0` invokes control slot `+0x0C` inline.
- Reset RVA `0x00502F10` has exactly two direct callers, at RVAs `0x00513817` and `0x00621416`; both pass raw pointer zero. It allocates a fresh control block, releases the old pair, then stores `{0,new-or-null-control}`. This does not create an observed second control for a live non-null object, but it also supplies no publication/drain protocol.
- Vector shift RVA `0x00621570`, grow/copy RVA `0x00621630`, range copy RVA `0x00621880`, and range release RVA `0x00621800` implement acquired copies and inline releases. Their direct xref counts are `2/2/2/8`; range release is used by erase/grow/stop. These mechanics do not bound aliases outside the vector.
- Manager stop RVA `0x00621B70` has three direct code callers (deleting-dtor chain RVA `0x00621910`, global shutdown RVA `0x00671770`, and atexit RVA `0x00716C10`) plus derived-vtable cell RVA `0x00782958`. The deleting destructor itself has only the vtable xref. Global shutdown has one direct caller, `_main`; atexit cleanup is registered statically. Neither the indirect destructor caller nor the thread that invokes CRT exit is fixed by these static edges.
- InputDevice control `ReleaseStrong` RVA `0x000358B0` has 11 xrefs and `ReleaseWeak` RVA `0x0000C5C0` has 20; all are vtable data xrefs, with zero direct code callers. Actual releases are indirect. Last strong deletes the object and tail-calls weak release on that same releasing thread; a separately held weak can make a later indirect weak releaser the control-block-free thread.

## 5. Three native-evidence verdicts

| Proof ID | `nativeEvidenceStatus` | Newly closed native subchain | First exact missing evidence |
|---|---|---|---|
| `VIRTUAL_DEVICE_OBJECT_OWNER` | `UNKNOWN` | the fixed delegate registry has two known non-retaining targets and no `+0x44` target; Manager vector ownership mechanics remain confirmed | vtable cells at RVAs `0x0078281C/0x00782978` expose keyed lookup beyond the 28 direct calls. No closed indirect-call/alias universe ties all outstanding reader pairs to removal or identifies who is authorized to perform the final release. |
| `VIRTUAL_DEVICE_CREATION_DESTRUCTION` | `UNKNOWN` | concrete pair copy/reset/delete mechanics and shutdown entry shapes are identified | no independent non-OriginalOnline virtual factory/coordinator exists in the PE; v1's `{object,null-control}` publication edge and constructor/control/insert failure rollback remain. Consequently exactly-once destruction for the future virtual object is not a native fact. |
| `VIRTUAL_DEVICE_REFERENCE_COUNTING` | `UNKNOWN` | copy, swap/assignment, zero-reset, vector copy/shift/range release, six-slot mechanics, and inline last-strong/last-weak behavior are identified | all actual strong/weak releases dispatch indirectly; static xrefs to the two release implementations are data-only. Reader alias cardinality, rollback balance, acquire-after-last-release exclusion, `uint16` bounds, and final releaser/thread remain unclosed. |

`promotionCount` supported by this handoff is `0`.

## 6. Related retained UNKNOWN items

- `VIRTUAL_DEVICE_CALLING_THREAD`: `UNKNOWN`. Native worker insertion/removal remains the concrete counterexample from v1; additionally stop is reachable through `_main`, atexit, and an indirect destructor, while final ref release runs on an alias's caller thread. No single authority-main token/phase covers all paths.
- `VIRTUAL_DEVICE_VECTOR_INSERT_REMOVE_OWNERSHIP`: `UNKNOWN`. Concrete callbacks no longer supply the first alias gap, but keyed readers live beyond the Manager lock, indirect lookup callers are unclosed, and stop/remove has no publication-close plus protocol drain.
- `VIRTUAL_DEVICE_TEARDOWN_DRAIN_RESTORE`: `UNKNOWN` and outside the three-axis v4 native batch. Bulk delegate clear is not a reader/callback drain proof.
- `VIRTUAL_DEVICE_KEY_ALLOCATION`, `VIRTUAL_DEVICE_VTABLE_COL_SLOT_ABI`, `VIRTUAL_DEVICE_ASSIGNMENT_INTEGRATION`, and `VIRTUAL_DEVICE_GENERATION_LIFECYCLE`: unchanged `UNKNOWN`.

## 7. Disposition

The fixed necessity result remains `REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`. All three v4 `nativeEvidenceStatus` values remain `UNKNOWN`; offline implementation evidence cannot change them. Therefore `formalDecision=null`, `workPackageStatus=BLOCKED_BY_EVIDENCE`, `implementationEligibility=false`, all runtime/active/mutation permissions remain false, and CS-T0-05-06 remains locked.
