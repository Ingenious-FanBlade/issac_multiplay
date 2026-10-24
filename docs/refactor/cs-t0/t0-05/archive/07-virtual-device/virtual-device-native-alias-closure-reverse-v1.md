> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias universe closure — independent REVERSE handoff v1

Date: 2026-09-09  
Role: independent REVERSE  
Scope: `NATIVE-ALIAS-CLOSURE` only  
Machine evidence: `cs-t0/native-alias-universe-v1.json`  
Machine evidence SHA-256: `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`  
Verdict: `UNKNOWN`

## 1. Scope and safety disposition

This pass performed only the two sealed alias fixed-point investigations. It did not investigate native factory/rollback or final-release/thread closure, run TEST, modify the v4 decision or verifier, edit production code, start/enumerate/open the game process, create IPC, use x32dbg, or modify/save the IDA database.

The IDA database was used read-only for function navigation, CFG inspection, and pseudocode assistance. All claimed instruction bytes, image pointers, whole-image callsite sets, section geometry, and hashes were independently reproduced from the disk PE with GNU `objdump` and direct disk-byte hashing. IDA names and pseudocode are not treated as the independent oracle.

## 2. Fixed identities and bindings

- PE SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- PE machine: `0x014C` (Win32/x86)
- preferred image base: `0x00400000`
- image size: `0x008FE000`
- read-only IDB: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`
- disk PE: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`
- `objdump`: `D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe`
- `objdump` SHA-256: `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`
- banner: `GNU objdump (GNU Binutils) 2.44`
- TEST preseal SHA-256: `1273DC21E3EF88DA65304859C19EC7881703C635154389D780BFB6C5FC825342`
- inherited REVERSE v2 SHA-256: `AD94ECE74EE79D866E275E3A87672FFD6A0B1CCB20FA48EA9378C0B47620AF42`
- parent v4 decision SHA-256: `73161422E04832566EB86B9D702C0FE6ABEBAA336FEDC86EF49F0B05DC8F04FD`
- parent v4 final verification SHA-256: `048634971F408F30E2A06552C96A5AC3ACF5850BBAE5D1FF2D0321656FE0FCA4`

## 3. Seed and whole-image census

The independent direct-call census reproduced the exact ordered 28-RVA presealed set. Missing, unexpected, and duplicate direct seed sets are all empty.

Both vtable cells reproduce the same keyed lookup pointer:

| Owner shape | cell RVA | raw bytes | decoded VA | target RVA |
|---|---:|---|---:|---:|
| base | `0x0078281C` | `90 03 A2 00` | `0x00A20390` | `0x00620390` |
| derived | `0x00782978` | `90 03 A2 00` | `0x00A20390` | `0x00620390` |

A whole-`.text` decoded base-only operand census used the rule `call *0x28(%reg)` over EAX, ECX, EDX, EBX, ESI, EDI, EBP, ESP, and any equivalent 32-bit base register form emitted by `objdump`. It produced 489 canonical candidates: EAX 469, ECX 1, EDX 11, ESP 8, and zero for EBX/ESI/EDI/EBP. Seven are proved Manager keyed lookup dispatches:

`0x00023E5B`, `0x0028B338`, `0x0051F3D5`, `0x0055642A`, `0x00556483`, `0x0055B6B3`, `0x0055B8EB`.

At each proved site, the containing block materializes the Manager singleton VA `0x00C57B18` as the receiver and dispatches the anchored slot `+0x28`; the two possible Manager vtable shapes agree on target RVA `0x00620390`.

The full-register pass adds these eight candidates beyond the earlier EAX/ECX/EDX subset:

`0x0004C6C3`, `0x001862D7`, `0x0018641A`, `0x0024CA1D`, `0x0038E7E4`, `0x0038E8F7`, `0x0038EA04`, `0x0038ECD5`.

All eight decode as `FF 54 24 28 / call *0x28(%esp)`. Their anchored local contexts perform normal argument pushes plus reads/writes through `[ESP+offset]`; none loads ESP from Manager. The ModRM/SIB effective address is therefore the active stack slot `[ESP+0x28]`, not `receiver-vtable+0x28`. They are classified `EXCLUDED_WITH_INSTRUCTION_PROOF`. The remaining 474 decoded candidates stay canonical `UNCLASSIFIED_RECEIVER_PROVENANCE` records rather than prose-only exclusions.

The base-only operand set is explicitly not used as the alias-universe definition. A second whole-`.text` oracle enumerated all 35,189 decoded indirect calls, then propagated two local states, `SLOT_TARGET` and `SLOT_ADDRESS`, from positive `+0x28` loads/address computations through register copies, stable EBP spills and exact reloads, conservative ESP spills, and address dereferences. It recognizes direct register calls, precomputed `*(%reg)` calls, and indexed/SIB `+0x28` operands.

The deterministic window is the ascending decoded instruction stream since the last conditional/unconditional branch, return, loop, or `int3`; it has no arbitrary instruction-count cutoff. The corrected oracle kills caller-saved EAX/ECX/EDX after every direct or indirect call and preserves only ABI callee-saved taint. A write to a tracked stable spill from an untracked source deletes the stale spill state, and `pop %reg` explicitly kills the destination. Any call that consequently leaves the bounded split projection remains in the residual frontier. The exact generation, transfer, regex, destination-kill, stack-kill, and call-recognition rules are machine fields under `managerReceiverDispatch.equivalentDispatchOracle`.

This reproducible projection adds 270 split-register equivalent dispatches. Re-running it with the corrected caller-saved/spill-overwrite/`pop` kills produced the same ordered RVA set; its compact RVA-array SHA-256 is `44F2208D124A5B83AC9EB970D5CEDB49558AF975E08BE40592EA16BC90BB7946`. Every observed hit has the form `mov 0x28(%vptr),%tmp; ...; call *%tmp`; register-copy, spill/reload, precomputed-address, and indexed/SIB rules are active but each produced zero additional hits on this PE. All 270 remain `UNCLASSIFIED_EQUIVALENT_SLOT_DISPATCH` because the receiver/vptr origin and concrete target set are not exhausted. Two lie in IDA-undefined code regions and retain explicit unresolved function ranges rather than being dropped.

The first non-base-only edge is:

- containing function RVA/range: `0x0002B560..0x0002B763`
- producer: RVA `0x0002B63A`, raw `8B 70 28`, `mov 0x28(%eax),%esi`
- call: RVA `0x0002B645`, raw `FF D6`, `call *%esi`
- disposition: slot transfer is reconstructed, but the EAX vptr/receiver provenance and target set remain unresolved.

For independent population checking, the canonical compact array of all indirect-call rows `{rva,rawBytes,operand}` is recorded under `equivalentDispatchOracle`. The machine artifact now partitions the full 35,189-row census one-to-one into 489 base-only rows, 270 corrected split-register rows, and 34,430 individually identified residual rows. Each residual row carries RVA, raw bytes, verbatim `objdump` decode/operand, edge kind, source, prospective destination, callee disposition, reason, and affected proof. No residual is represented only by a count or digest.

A separate whole-`.text` census found 3,810 decoded indirect tail transfers whose mnemonic starts with `jmp` and whose AT&T operand starts with `*`. Every tail transfer is retained as its own canonical `indirectTailTransferFrontier` row. This avoids treating an unresolved tail continuation as a terminal. Together with the separately represented Manager-argument service escape described below, the receiver frontier total is 38,985: 474 base, 270 split, 34,430 residual calls, 3,810 tail transfers, and one Manager-argument escape.

All 38,240 residual/tail rows now also carry the presealed containing-function and provenance contract. A read-only IDA `get_func` lookup identified and disk-hash-anchored the containing range for 37,702 rows; 538 rows in IDA-undefined code regions remain present with `containingFunctionRange.status=UNRESOLVED_IDA_CODE_REGION` and `functionHashRef=null` (residual 33,981 identified / 449 unresolved; tail 3,721 identified / 89 unresolved). The identified projection adds 8,867 unique disk-hashed census function ranges, yielding 8,903 anchors before the base-frontier provenance expansion. Each row ID is its inline instruction anchor, so `anchorRefs=[row.id]` without duplicating 38,240 top-level instruction anchors; identified rows additionally resolve `functionHashRef` to `anchors.functionRanges`.

The same contract is now materialized on every one of the 474 base-only unresolved receiver-frontier rows: each has verbatim independent decode, containing range or explicit unresolved status, successor/callee disposition, inline anchor reference, and function-hash reference. Read-only IDA navigation identifies 473 rows and leaves one row explicitly `UNRESOLVED_IDA_CODE_REGION`; 48 newly required unique ranges are disk-hashed. The service destructor reuses the already-present canonical census range `indirect-census-function-0065a4c0`; no duplicate range is added. The final function-range array therefore remains 8,951 entries (36 core + 8,867 census + 48 receiver-frontier-only ranges).

The seven proved Manager dispatches now each have a one-to-one proof-context anchor binding the complete `A1 18 7B C5 00` Manager vptr load, `B9 18 7B C5 00` Manager/sret receiver materialization, and `FF 50 28` dispatch. The two sites in function RVA `0x0055B310` additionally bind the `jne` at RVA `0x0055B6A1`: the false successor reaches `0x0055B6B3`, while the true successor reaches block RVA `0x0055B8DF` and call RVA `0x0055B8EB`, with no EAX write on either recorded CFG path.

All 20 instruction-backed receiver graph edges (0005 through 0024) now carry exact raw bytes, verbatim `decoded` and `independentDecodedInstruction`, an `instructionSpan`, a disk-hash-anchored containing function range, successor/callee disposition, semantic interpretation, anchor references, and the containing function-hash reference. Edges 0005 through 0011 are grounded at their seven distinct `B9 18 7B C5 00` Manager receiver-materialization instructions; edges 0012 through 0018 are separately grounded at the paired `FF 50 28` dispatches. This avoids using the dispatch instruction as false evidence for the receiver-materialization edge while retaining the one-to-one complete context anchor.

Edges 0019 through 0024 use explicitly declared multi-instruction spans for the service receiver/vptr load plus Manager/callback pushes and virtual call, wrapper argument forwarding, callback/Manager stores into the first context, second-context/`beginthreadex` transfer, trampoline incoming-argument load plus Manager/callback call, and the complete callback no-argument-read proof. Semantic interpretation remains in a separate field and is not substituted for machine decode.

The whole-image immediate-reference census for Manager VA `0x00C57B18` produced 762 decoded references: 759 `mov`, two `movl`, and one `push`. The exceptional `push` at RVA `0x0052486D` was followed through a statically reconstructed but conditional thread-start path:

`service slot +0x08 -> 0x0065A530 -> service slot +0x04 -> 0x0065A570 -> first context +8 -> second context +4 -> 0x0067F130 -> 0x0065A760 -> 0x006220C0`.

The service-global whole-`.text` oracle searched the complete independent `objdump -d` output for decoded operand VA `0x00C57B64`. It finds exactly four direct references:

| RVA | raw bytes | decoded access | disposition |
|---:|---|---|---|
| `0x0000234C` | `C7 05 64 7B C5 00 5C EE B9 00` | direct write | vptr `0x00B9EE5C` |
| `0x00524863` | `A1 64 7B C5 00` | direct read | loads current vptr into EAX |
| `0x00524868` | `B9 64 7B C5 00` | address materialization | service global becomes ECX `this` for the virtual call |
| `0x00716C4D` | `B9 64 7B C5 00` | address materialization | service global becomes ECX `this` for direct destructor call RVA `0x0065A4C0` |

The destructor function range `0x0065A4C0..0x0065A52B` is independently disk-hashed as `0ADBB1787EA4D8A7BDF67B087666BB3CF4559DC0F573850648FF80EA09A00451`. Its complete decode preserves incoming ECX as ESI, writes vptr `0x00B9EE5C` through `[esi]` at RVA `0x0065A4C7`, then writes base vptr `0x00B9FB88` through the same alias at RVA `0x0065A523`. The base-vtable `+0x08` cell at RVA `0x0079FB90`, raw `30 28 A6 00`, resolves RVA `0x00662830`, not the wrapper. Thus the four-row direct census is closed, but the call-time vptr set is not: no static temporal/concurrency proof excludes the destructor state or another mutation through the service `this` escape before the RVA `0x00524863` load.

Edge 0019 therefore has disposition `UNRESOLVED_FRONTIER`, not an exact-wrapper resolution. Its canonical `RECEIVER-ESCAPE-FRONTIER-0001` records the Manager argument passed through `call *0x8(%eax)` at RVA `0x0052487E`, with observed targets including wrapper RVA `0x0065A530` and base-vtable target RVA `0x00662830`, while explicitly leaving the call-time target set unclosed.

Under the explicit path condition that service vptr `0x00B9EE5C` selects wrapper RVA `0x0065A530`, edge 0020 now anchors the complete wrapper function from entry to epilogue. It includes ECX `this` → callee-saved ESI at RVA `0x0065A535`, the preliminary virtual check, all argument pushes, ESI → ECX restoration, slot `+0x04` load, and call. This proves callback `[ebp+8]` and Manager `[ebp+0x14]` are forwarded as implementation `[ebp+8]` and `[ebp+0x18]`; it does not turn the unresolved edge 0019 into an unconditional path.

At RVA `0x0065A5E3`, a non-null first-context allocation is saved in EBX. On the selected success path the `je` at `0x0065A5ED` falls through, callback/Manager are stored at `[ebx+4]`/`[ebx+8]`, and the `jmp` at `0x0065A5FD` skips the null-path `xor ebx,ebx`. The complete selected continuation `0x0065A601..0x0065A65D` contains no EBX destination write before `89 58 04` stores the same EBX into second-context `[eax+4]` at RVA `0x0065A657`; intervening callees preserve EBX under the Win32/x86 ABI. The call at RVA `0x0065A663` is independently bound through IAT RVA `0x00718854`, disk raw `E0 72 7F 00`, to `api-ms-win-crt-runtime-l1-1-0.dll!_beginthreadex` by `objdump -p`.

The start routine remains anchored at RVA `0x0067F130..0x0067F1CA` (size 154, raw file offset `0x0067E530`, SHA-256 `01BF950B0857B7DBFA26CDDBCAA293D04043370CA93C70A61F056A3E0FD0153C`). Its critical span at RVA `0x0067F159..0x0067F16A`, raw `8B 7D 08 C7 45 FC 00 00 00 00 FF 77 04 8B 07 FF D0`, loads the second context from `[ebp+8]`, pushes `[edi+4]` (the first context), loads `[edi]` (trampoline RVA `0x0065A760`), and calls it. Edge 0023 starts at RVA `0x0065A763` with `8B 45 08`, explicitly loading the incoming first-context argument into EAX before the callback `+4` and Manager `+8` loads, Manager push, and callback call.

The complete callback function at RVA `0x006220C0` does not read its incoming callback argument. That is a valid conditional exclusion only for the fully reconstructed `0x00B9EE5C` wrapper/non-null-allocation path. Because edge 0019 cannot be confined to that path, it does not close the Manager-pointer escape; the escape remains explicit alongside the 474 slot candidates.

The first receiver-census frontier is therefore:

- containing function RVA/size: `0x00017E50 / 0x000003D3`
- candidate instruction RVA/raw: `0x00017FA6 / FF 50 28`
- context bytes from RVA `0x00017F9E`: `8B 47 24 8B 0C B0 8B 01 FF 50 28`
- edge: field-backed vector element -> receiver register -> `receiver.vtable[0x28]`
- unresolved reason: the element's producer/type/vtable provenance was not exhausted, so the target cannot be proved as keyed lookup or excluded from it.

Accordingly `receiverDispatchFixedPointClosed=false`.

## 4. Returned pair trace and new seed-reachable frontier

The 28 direct plus seven proved indirect lookup sites create 35 canonical `{object,control}` return seeds. This pass expanded the first direct and first indirect returns in function RVA `0x00023CC0`:

- direct call RVA `0x00023D12` returns to stack pair `[ebp-0x30],[ebp-0x2c]`;
- indirect call RVA `0x00023E5B` returns to stack pair `[ebp-0x40],[ebp-0x3c]`;
- indirect-pair control release is at RVA `0x00024024`;
- direct-pair control release is at RVA `0x00024082`;
- when either release returns true, the code loads a callback pointer from RVA `0x0087E814`, passes the address of the two-word pair, and calls it indirectly.

The earliest confirmed-seed-reachable unclosed edge is:

- containing function RVA: `0x00023CC0`
- release instruction: `0x00024024 / FF D0`
- callback pointer load: `0x0002402A / A1 14 E8 C7 00`
- unclosed callback edge: `0x00024037 / FF D0`
- edge kind: `POST_RELEASE_INDIRECT_CALLBACK_WITH_PAIR_ADDRESS`
- source: global callback slot RVA `0x0087E814`
- prospective destination: unknown callee which receives the address of `[ebp-0x40]`
- affected proofs: pair fixed point, dynamic alias completeness, drainability, and owner-through-drain.

The independent whole-image disassembly contains 101 references to VA `0x00C7E814`; every form is `mov 0xc7e814,%eax`, and no decoded write was found. PE section geometry places the slot after the raw-backed end of `.data` but within its virtual extent, so the disk initializes it as zero fill. Static bytes therefore supply neither a writer nor a callable target set. The same unresolved callback appears at RVA `0x00024095` for the other local pair and at RVA `0x000242F1` in the anchored generic pair destructor at RVA `0x000242B0`.

Because an unknown callback receives the pair address, it cannot be classified as a terminal: it may copy, retain, reset, store, return, or otherwise escape the pair. The remaining 33 return seeds are explicitly recorded as unexpanded liveout frontiers, and the first SEH/nonlocal-successor set is also left explicit. No count or absence of direct xrefs is used as a closure proof.

Every instruction-backed pair seed, graph node/edge, unresolved call, liveout, escape, the unique exception edge, and the generic destructor now carries its containing function range, exact raw bytes, verbatim independent `objdump` decode, successor/callee disposition, anchor references, and function-hash reference. Semantic interpretation is a separate field; it is never appended to `independentDecodedInstruction`. The two release-branch graph edges separately anchor the `test` instruction and its conditional successor instead of joining two instructions into one decode. The generic destructor also carries three independent instruction-evidence rows for release, callback-slot load, and callback call. The exception frontier additionally binds:

- function entry RVA `0x00023CC0`, first 16 raw bytes `53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04`;
- SEH setup RVA `0x00023CD6`, raw bytes `6A FF 68 00 19 AF 00 64 A1 00 00 00 00 50 53 81 EC 80 00 00 00 56 57 A1 B4 93 BF 00 33 C5 50 8D 45 F4 64 A3 00 00 00 00`;
- containing function hash `587A0789C121716CE8CF3F128D839FDD9FF522F490EC164D3B4C82E0EB1B4CA7`.

Accordingly `pairAliasFixedPointClosed=false`.

## 5. Added function-range anchors

| ID | RVA | size | raw file offset | SHA-256 |
|---|---:|---:|---:|---|
| manager-static-init | `0x00002280` | 267 | `0x00001680` | `2FA14DEC88E61EE8B9E3AA24991C72090E68161ED0AE9AD22C688EFC5EE110B7` |
| manager-atexit | `0x00716C10` | 96 | `0x00716010` | `6B2FFB2FFF68C03D62858513D5A25436F96000E28407E36EA1FE31D17EEAB95C` |
| keyed-lookup | `0x00620390` | 173 | `0x0061F790` | `90607D3828B66E01FA83A688CB27F8BC4C5AB07910D44808BBDACAEA0B90902E` |
| caller-423cc0 | `0x00023CC0` | 1006 | `0x000230C0` | `587A0789C121716CE8CF3F128D839FDD9FF522F490EC164D3B4C82E0EB1B4CA7` |
| caller-68b260 | `0x0028B260` | 2360 | `0x0028A660` | `4FCA997A6A1A36A6B6777FE495E71A63268CFD8014355337FDBF5AD2CFE83538` |
| caller-91ee90 | `0x0051EE90` | 1847 | `0x0051E290` | `74E608A1E6D4C02A60CC541481BEC431167C5E64E93BBA5A3B513E64D7CCB6BB` |
| caller-9563b0 | `0x005563B0` | 915 | `0x005557B0` | `CC4D0D3B176DEE4F74D71FC99EFE7980D51ADE9E3EA2FD61EFF33434FA419DA4` |
| caller-95b310 | `0x0055B310` | 2740 | `0x0055A710` | `39199AE09D8585FBD02AD84E484427398275268FFB129B15787E6DC092B1E126` |
| pair-dtor | `0x000242B0` | 86 | `0x000236B0` | `DCF09DE1BE0322A06431B818D31686D8623A68AEF17FD678CEEF74644797F9C9` |
| `indirect-census-function-0065a4c0` (thread-service dtor) | `0x0065A4C0` | 107 | `0x006598C0` | `0ADBB1787EA4D8A7BDF67B087666BB3CF4559DC0F573850648FF80EA09A00451` |
| thread-start-wrapper | `0x0065A530` | 62 | `0x00659930` | `79D1041D21CAA4EAB39C4874D236D17DCB871221512E2E71EA31024A6B8BC31A` |
| thread-start | `0x0065A570` | 373 | `0x00659970` | `43F9BEFDD3194CDF67036054E7A91303C2E4F5293642FBBF73741D7F4F69214A` |
| thread-trampoline | `0x0065A760` | 51 | `0x00659B60` | `21CFA62D4624C04DCB2BD8058736162F5CF81C383F98747BA984168B5C0FCDD4` |
| manager-callback | `0x006220C0` | 46 | `0x006214C0` | `520BB72C096CEF52FFDB8F1466B1A6CF684DBF653742306777E4F2E67FBBF8A9` |

To make every unexpanded pair-liveout row independently hash-addressable, this pass adds the following caller ranges (the already-present five caller anchors are reused rather than duplicated):

| ID | RVA | size | raw file offset | SHA-256 |
|---|---:|---:|---:|---|
| `pair-seed-caller-0002af00` | `0x0002AF00` | 280 | `0x0002A300` | `69E67E8211732C1B9DCC5C3B3A2552C507264392BE81A587D005B8C3AB451529` |
| `pair-seed-caller-0028cdc0` | `0x0028CDC0` | 22279 | `0x0028C1C0` | `71C82EB76406E1D8A8EAF76860F003C85455D0CF5DAB5F734250B7ED61430A94` |
| `pair-seed-caller-002f6500` | `0x002F6500` | 2242 | `0x002F5900` | `9E431BE23D068A213A699ABB2F9D8199010C58F53FFF0AC604A5AD2D8EA0EA7E` |
| `pair-seed-caller-00382af0` | `0x00382AF0` | 34868 | `0x00381EF0` | `527410FCD0B425929965F70675FA85011B5C8948D0C0068107CDC1F5E511A83C` |
| `pair-seed-caller-003b9f60` | `0x003B9F60` | 833 | `0x003B9360` | `9DB8CE9AECC9D3921B8225F9ADB285315797F151E973FBDB4E23AE3AF6F535F5` |
| `pair-seed-caller-003bc740` | `0x003BC740` | 9046 | `0x003BBB40` | `569536FD965878D65F8D9CDD059EEF74372B368EF7A8534FA67C15603EBF2721` |
| `pair-seed-caller-00435ba0` | `0x00435BA0` | 227 | `0x00434FA0` | `DECCCCD0F2A8C5228AF8ECD7DE0F22CCF71FCC574AD732760DF5D1BAA368AF69` |
| `pair-seed-caller-00435c90` | `0x00435C90` | 727 | `0x00435090` | `1DD546536E0EED955A6390C1D28C7BEA6489F30BAF5545CDC8C1CC8392F85885` |
| `pair-seed-caller-004d1f20` | `0x004D1F20` | 1327 | `0x004D1320` | `09FFEB4AD1649D8FA4BF48CA8A3D536B1009010DFD8D398F6C2BF566CEB2E9CE` |
| `pair-seed-caller-004d8b00` | `0x004D8B00` | 2098 | `0x004D7F00` | `5BBF820750D9AD708E7D2372AED6F077C8BCA9D6ED406E362808E12B596EDEC4` |
| `pair-seed-caller-004d9340` | `0x004D9340` | 3309 | `0x004D8740` | `23B05988F374258B2ADE28E0CE9B3E3DBA255F5CAA6F6A08A08C4C1A9E9E8D4C` |
| `pair-seed-caller-004da030` | `0x004DA030` | 1207 | `0x004D9430` | `1F009B47C228DE3A082B4BB5F5F385E990A787E47D41839C42A6242B645F2213` |
| `pair-seed-caller-004da510` | `0x004DA510` | 2912 | `0x004D9910` | `C7BCD41BAA78D8D63DEB52D268E7CFFE12AD7AC697E4046FF41B92483A7E247E` |
| `pair-seed-caller-00501930` | `0x00501930` | 572 | `0x00500D30` | `E8FFD6B2E7B1F8AE15EB26A45D26050876ABCD9096C1AEE460ACDC257743A705` |
| `pair-seed-caller-00506020` | `0x00506020` | 148 | `0x00505420` | `77DB035044AC62C0E83A846A8D14CC18333F51FA684FA1E812B67AFE01A28049` |
| `pair-seed-caller-0050a8a0` | `0x0050A8A0` | 835 | `0x00509CA0` | `767E2561CAD52B7539879594BC9A0F7E77E7953E14A7325377E18D18CC879AEC` |
| `pair-seed-caller-00534770` | `0x00534770` | 2196 | `0x00533B70` | `AFA6CB194C261ED958E150B07B4738A63512C6A3E934F14DF5C8EE83C16E9F3A` |
| `pair-seed-caller-00535010` | `0x00535010` | 599 | `0x00534410` | `1C19892C67A5BE20CBE402D10374A5E81622FFA48FC9974A0716E0260B0C672F` |
| `pair-seed-caller-0055af30` | `0x0055AF30` | 582 | `0x0055A330` | `86FE148C5FDE943556598D302B19208022DC20B69B935F2750CBDB96B6AC6CEE` |
| `pair-seed-caller-0055b180` | `0x0055B180` | 263 | `0x0055A580` | `AEEE08B21D66E0F61803E053DA74A9958AC04F5273094F045CA4B18DA7AB3F30` |
| `pair-seed-caller-005897d0` | `0x005897D0` | 2055 | `0x00588BD0` | `95C193DABDCD9322C58D969B195FFCEA6A1C26FFC25358AFCF18C0D25265FA5D` |
| `pair-seed-caller-0058dba0` | `0x0058DBA0` | 1482 | `0x0058CFA0` | `73B7A3B4395020E219F2902D1FCEB7490A16A6FB2514260E9ED454AEEFDC3A16` |
| `pair-seed-caller-005b23a0` | `0x005B23A0` | 3502 | `0x005B17A0` | `273FB4F92C28222153B6A557830F04DE4BD75BEF316C563821C7A38083CCCB64` |

The machine artifact carries 8,951 function-range hashes (36 core + 8,867 census ranges + 48 receiver-frontier-only ranges), 56 instruction anchors, four thread-service image-pointer anchors, and one import anchor. The destructor evidence binds the existing canonical census range rather than adding an alias ID. In addition to the original evidence, these bind the service-global direct census, destructor alias writes, both observed service vtables, the complete wrapper, the selected-path first-context EBX identity, `_beginthreadex`, and the conditional callback path without promoting it to an unconditional exclusion.

## 6. Actual graph and frontier counts

| Set | Count |
|---|---:|
| expected/discovered direct seeds | 28 / 28 |
| vtable seeds | 2 |
| whole-`.text` indirect calls | 35,189 |
| slot-`+0x28` base-only syntax candidates | 489 |
| reconstructed split-register equivalent candidates | 270 |
| core alias-dispatch projection | 759 |
| canonical residual indirect-call rows | 34,430 |
| canonical indirect-tail-transfer rows | 3,810 |
| proved keyed Manager dispatches | 7 |
| instruction-proved exclusions among syntax candidates | 8 |
| unclassified receiver candidates, base / split / total | 474 / 270 / 744 |
| receiver graph nodes / edges | 18 / 24 |
| Manager-argument service escape frontier | 1 |
| receiver frontier records, base / split / residual / tail / Manager escape / total | 474 / 270 / 34,430 / 3,810 / 1 / 38,985 |
| pair return seeds / expanded / unexpanded | 35 / 2 / 33 |
| pair graph nodes / edges / frontiers | 10 / 8 / 37 |
| unresolved calls | 3 |
| unresolved liveouts | 33 |
| unresolved escapes | 3 |
| unresolved exception edges | 1 |
| callback-slot decoded reads / writes | 101 / 0 |

All numbers above are derived from canonical arrays in the machine evidence. The 37 pair frontier records are three unresolved calls, 33 unexpanded return liveouts, and one exception edge; the three escape records cross-reference the unresolved calls rather than being counted twice as frontier records.

The final linkage correction makes `zeroSetInputs.unresolvedCalls`, `unresolvedLiveouts`, `unresolvedEscapes`, and `unresolvedExceptionEdges` literal deep copies of the corresponding canonical `pairAliasTransfer` arrays. Its omitted/duplicate/unclassified seed inputs are also deep-equal to both the receiver seed zero sets and the pair node zero sets. The pair arrays themselves did not change, so their compact profiles below remain unchanged. The final `actualCounts` schema intentionally omits the obsolete `longFlowResidualIndirectCallCount` and `longFlowReceiverFrontierCount` fields; `receiverFrontierCount=38985` is the deliberate sum of 474 base, 270 split, 34,430 residual-call, 3,810 indirect-tail, and one Manager-argument service-escape row.

The fixed profile may bind the following array hashes. Serialization is each parsed array alone using Python `json.dumps(value, ensure_ascii=False, separators=(",", ":"))`, preserving parsed manifest property order, encoded as UTF-8 without BOM and without a trailing newline.

| Canonical array | Count | Compact bytes | SHA-256 |
|---|---:|---:|---|
| instruction anchors | 56 | 45417 | `079975A6E1F9169F444AE78DBF464602152E523A269BF0D4A60C90B146701BA6` |
| function ranges | 8,951 | 1723664 | `00C9E21BFF989AF5D7FF9A1D10CEB9FFDDD8B0479FE671695B55FE8007411F3F` |
| thread-service image-pointer anchors | 4 | 1198 | `FB191EE235DB1DA43FBF3C527E75DC36338301E90D42BEBB253E59AA3E340BA3` |
| import anchors | 1 | 369 | `CD42C8C8D5F5A1B264E45ECA3C56F7539A7B307ED1C856A3A7DC0E518A91B56C` |
| receiver graph nodes | 18 | 3217 | `48231F81D67B2FBB0A6BBE43CD3CBEF05FD6002AC1CB61DB119A09B33DD64586` |
| receiver graph edges | 24 | 39633 | `E2E4F0171993CD3468652F3407733C5C142F29C32D0E3C758F4679D8420AA2BA` |
| base-only dispatch candidates | 489 | 391060 | `6909DF38303AAF81A1A13E3BC7A3D755A8FB7BBF174F9623C5B6EDB6D2891A14` |
| base-only receiver frontier | 474 | 468823 | `E21261D31B16D9E21034E5F2C398E03F8BD981930791F89816EE52AB6C76CB58` |
| split-register dispatch candidates | 270 | 297614 | `DDAB96FAF2383E4B65C0886D55391DF3AB2167C3F6B219F7143CCA621E003B1A` |
| split-register unresolved frontier | 270 | 266008 | `CC0D5BC177915BA21B284F69A3A6D2CED1A0308DE6073D092C7BC8452D09CE42` |
| residual indirect-call frontier | 34,430 | 36785974 | `434ABE7E8689E952178DD4845146C869B2DE2795EC6ABAAD96BDB05486A9E043` |
| indirect-tail-transfer frontier | 3,810 | 3780622 | `57F3A3C66B239B40459347EFE70A2209D7520AB4A1E7FD1992F8A5ECF093AFE6` |
| service-global decoded references | 4 | 2547 | `538E0595548DFD01C50D1CDD51F081D8C9CDFBD1ABE7A91A534581885A7696FE` |
| service-global alias writes | 2 | 1385 | `AAE4D56512E73B16D744781D51E9AA61BFEE74AA2BF47D92F6D81D768138B210` |
| Manager-argument escape frontier | 1 | 1548 | `1AD3B8D51D5D123056284C7BEB3376BADDDF488763906893E00D820FA49ABFEE` |
| pair seed returns | 35 | 28410 | `811C2CBE38862C76138F12B7918443B413CFCFF0B24BD3BE8204575D90D9BD17` |
| pair graph nodes | 10 | 7164 | `BF49C2C17C0255E3FAA7F959AE2FA1E6B4529901925B54B18A83BFF14D0E33C5` |
| pair graph edges | 8 | 5876 | `C54A4DD071027D733743C29E640393B97DD9D0174104C69FB01C130F00EF5F7E` |
| pair unresolved calls | 3 | 3000 | `F5EEE5ABFB1D4EAEA96C6EBFE7D8B40D4214D59B6F74807C9637EC94829319F3` |
| pair unresolved liveouts | 33 | 34122 | `5CA6C537F86D29EA7B2AC409A876AB6145011E7AA25010BAEB9786B0E5ACFB77` |
| pair unresolved escapes | 3 | 2409 | `516D6EB1D332C96829FDCADA1E1C20572B3144264387B8B532C487C48CDFDCB3` |
| pair unresolved exception edges | 1 | 1044 | `9EE37D852CC0913611246C3194FADE33359991AEB42C3FF84C3594B9B24FEA7A` |
| pair unresolved frontier | 37 | 38164 | `DD62918DB8D992857F684AE835112652270BB0D14470B8E0881A23D017B8AABD` |

Schema additions are explicit in the manifest:

- domain-specific canonical order: decoded census/frontier arrays that explicitly declare RVA ordering are checked by ascending instruction RVA; function/instruction/image-pointer anchor arrays retain their sealed role/append-batch sequence and are exact-profile-bound; receiver graphs use canonical numeric IDs; pair arrays use their declared seed/row-ID order. No global RVA ordering is claimed and no 44-MB array was reordered;
- receiver fields: `dispatchUniverseDefinition`, corrected `equivalentDispatchOracle`, `splitRegisterDispatchCandidates`, `splitRegisterUnresolvedFrontier`, `residualIndirectCallFrontier`, `indirectTailTransferOracle`, and `indirectTailTransferFrontier`;
- service escape fields: `serviceGlobalAliasCensus` and `managerArgumentEscapeFrontier`, with separate canonical arrays for direct decoded references, resolved alias writes, and the unresolved Manager-argument edge;
- each residual/tail row: canonical ID, RVA, raw bytes, verbatim decode, decoded operand, edge kind, source, prospective destination, successor/callee disposition, reason, affected proof, `containingFunctionRange`, `anchorRefs`, and `functionHashRef`;
- census-row provenance rule: row ID is the inline instruction anchor; an IDA-identified containing range is independently disk-hashed and referenced through `functionHashRef`, while an IDA-undefined row remains explicit with unresolved range status and a null hash reference;
- base-only receiver-frontier rows: `independentDecodedInstruction`, `containingFunctionRange`, `successorOrCalleeDisposition`, `anchorRefs`, and `functionHashRef`, with the row ID serving as the inline instruction anchor;
- receiver instruction-backed graph edges: `instructionRva`, `rawBytes`, `decoded`, `independentDecodedInstruction`, `instructionSpan`, `containingFunctionRange`, `successorOrCalleeDisposition`, `semanticInterpretation`, `anchorRefs`, and `functionHashRef`;
- receiver multi-instruction spans: verbatim `objdump` instructions are joined in ascending RVA order while each instruction remains separately reconstructible in `instructionSpan`; semantic interpretation is kept separate;
- thread-service image-pointer anchors: both observed vtables and their `+0x04`/`+0x08` cells are disk-bound; they demonstrate why edge 0019 cannot assume the wrapper target;
- thread import anchor: the disk IAT cell plus independent `objdump -p` identity binds RVA `0x00718854` to `_beginthreadex`;
- receiver cross-function evidence: edge 0022 carries separate construction and start-routine forwarding spans, each with its own instruction anchor, containing range, and function hash;
- wrapper/first-context proof: the wrapper anchor starts at function entry and carries ECX→ESI, exact push order, slot `+0x04`, and epilogue; two selected-path anchors prove first-context EBX allocation, stores, branch, ABI preservation, and second-context `+4` identity;
- thread callback dataflow proof: edge 0019 remains unresolved unless the call-time service-vptr set is closed; edges 0020–0024 are explicitly conditional on the observed wrapper/non-null path and cannot be used as an unconditional escape exclusion;
- each proved Manager context: separately decoded Manager-vptr load, receiver materialization, dispatch instruction, one or more raw context segments, and CFG proof; the context and candidate IDs are one-to-one;
- pair instruction-backed rows: `rawBytes`, verbatim `independentDecodedInstruction`, separate `semanticInterpretation`, hash-anchored `containingFunctionRange`, `successorOrCalleeDisposition`, `anchorRefs`, and `functionHashRef`;
- release-branch edges additionally split out the conditional successor instruction; `genericDestructor` carries its release/load/call `instructionEvidence` array.

## 7. Independent verdicts

- `universeCompleteness.status=UNKNOWN`
- `receiverDispatchFixedPointClosed=false`
- `pairAliasFixedPointClosed=false`
- `dynamicAliasUniverseClosed=false`
- `drainability.status=UNKNOWN`
- `allAliasesBoundedAndReleasedBeforeManagerRemove=false`
- `nativeOwnerThroughDrain.status=UNKNOWN`
- `nativeOwnerHeldThroughDrain=false`
- owner promotion authorized: `false`
- `formalDecision=null`

The universe verdict is not inferred from counts. It remains `UNKNOWN` because both fixed points retain explicit nonempty frontiers. The earliest overall seed-reachable gap remains the pair callback at RVA `0x00024037`; the receiver side now also explicitly retains the Manager-argument service escape at RVA `0x0052487E` because its call-time vptr/target set is unclosed. These edges prevent proving that every alias is bounded and released before Manager remove, and therefore prevent proving a native owner is held through drain.

This handoff does not change any factory/rollback, reference-counting/final-thread, implementation, runtime, permission, or unlock conclusion inherited from v4.
