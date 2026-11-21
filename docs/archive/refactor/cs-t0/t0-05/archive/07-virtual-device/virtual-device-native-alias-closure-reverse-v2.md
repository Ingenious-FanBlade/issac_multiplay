> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias universe provenance repair — independent REVERSE handoff v2

Date: 2026-09-09  
Role: independent REVERSE  
Scope: `NATIVE-ALIAS-CLOSURE` exact 21-row provenance repair only  
Machine evidence: `cs-t0/native-alias-universe-v2.json`  
Machine evidence SHA-256: `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`  
Verdict: `UNKNOWN`

## 1. Scope and safety disposition

This retry implements only the exact 21-row adjudication set sealed by TEST preseal v7. It used the current IDA database read-only for function-owner and tail/chunk navigation, and the fixed disk PE plus the bound GNU `objdump` for independent instruction decoding and raw-byte confirmation.

It did not run TEST, start/enumerate/open the game process, use x32dbg, modify or save the IDA database, investigate factory/rollback or final-release/thread closure, modify production code, or change either immutable v1 artifact.

## 2. Fixed identities and lineage

- TEST preseal v7 SHA-256: `C2EB81220638EC37E51A2BB11C15B8BBA9F173D41E310FD17BE9D61F69743B3F`
- immutable manifest v1 SHA-256: `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`
- immutable REVERSE v1 SHA-256: `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`
- manifest identity: `IsaacCsT0-NativeAliasUniverse-v2`
- manifest schema version: `2`
- PE SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- PE machine/image base: Win32/x86 `0x014C`, `0x00400000`
- read-only IDB: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`
- disk PE: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`
- `objdump` SHA-256: `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`
- `objdump` banner: `GNU objdump (GNU Binutils) 2.44`

The v2 manifest binds the preseal and both immutable v1 artifacts by path and SHA-256. Both v1 files were rehashed before publication and remained exact.

## 3. Adjudication method

For each sealed instruction RVA, IDA navigation returned an owning function name, but disassembly exposed the instruction in a noncontiguous tail/chunk remote from that owner's main range. The former v1 provenance copied the owner's main `[startRva,endRvaExclusive)` range even though it did not contain the instruction.

The manifest schema has one linear containing-function range and does not encode IDA chunk intervals or a discontiguous range set. A remote owner main range is therefore not a truthful containing range. The repair does not widen the owner range, substitute a basic block for a function, or invent a new function boundary. Every affected row uses exactly:

```json
{"status":"UNRESOLVED_IDA_CODE_REGION","queriedInstructionRva":"<row instruction RVA>"}
```

and has `functionHashRef=null`. All other row fields and residual-array positions remain unchanged.

Independent disk confirmation used `objdump -d --start-address=<VA> --stop-address=<VA> <fixed PE>` around every cluster. All 21 raw byte strings and AT&T decodes reproduced the immutable v1 instruction evidence.

## 4. Exact 21-row dispositions

The `IDA owner main range` column records the noncontiguous owner returned by the current database. It is diagnostic evidence only and is deliberately not published as the row's containing range.

| Residual ID | instruction RVA | raw bytes / independent decode | IDA owner main range and name | v2 disposition |
|---|---:|---|---|---|
| `RESIDUAL-INDIRECT-CALL-00037` | `0x0000C2C6` | `FF 15 94 88 B1 00` / `call *0xb18894` | `0x00715170..0x0071517A`, `sub_B15170` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-00316` | `0x0001F7E0` | `FF 15 94 88 B1 00` / `call *0xb18894` | `0x00715310..0x0071531A`, `sub_B15310` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-00317` | `0x0001F851` | `FF 15 94 88 B1 00` / `call *0xb18894` | `0x00715320..0x0071532A`, `sub_B15320` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-00318` | `0x0001F8C5` | `FF 15 94 88 B1 00` / `call *0xb18894` | `0x00715300..0x0071530A`, `sub_B15300` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-03856` | `0x00308E0F` | `FF 15 94 88 B1 00` / `call *0xb18894` | `0x002F3050..0x002F3058`, `sub_6F3050` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-04923` | `0x003E26AE` | `FF 15 94 88 B1 00` / `call *0xb18894` | `0x00717020..0x0071702A`, `sub_B17020` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-28609` | `0x006156D1` | `FF 15 74 82 B1 00` / `call *0xb18274` | `0x00712DCF..0x00712DDA`, `sub_B12DCF` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-28611` | `0x00615742` | `FF 15 74 82 B1 00` / `call *0xb18274` | `0x0070BD24..0x0070BD2F`, `sub_B0BD24` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-30704` | `0x0066278B` | `FF 15 00 89 B1 00` / `call *0xb18900` | `0x0062CA10..0x00630347`, `sub_A2CA10` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-30705` | `0x0066279D` | `FF 15 80 88 B1 00` / `call *0xb18880` | `0x0062CA10..0x00630347`, `sub_A2CA10` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-30706` | `0x006627A8` | `FF 15 B4 82 B1 00` / `call *0xb182b4` | `0x0062CA10..0x00630347`, `sub_A2CA10` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-30707` | `0x006627B9` | `FF 15 B0 82 B1 00` / `call *0xb182b0` | `0x0062CA10..0x00630347`, `sub_A2CA10` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-30708` | `0x006627C4` | `FF 15 10 34 C1 00` / `call *0xc13410` | `0x0062CA10..0x00630347`, `sub_A2CA10` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-31081` | `0x0066C350` | `FF 15 8C 36 C7 00` / `call *0xc7368c` | `0x006CC7C0..0x006CC7D2`, `sub_ACC7C0` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-31082` | `0x0066C367` | `FF 15 8C 36 C7 00` / `call *0xc7368c` | `0x006CC7C0..0x006CC7D2`, `sub_ACC7C0` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-31083` | `0x0066C37E` | `FF 15 8C 36 C7 00` / `call *0xc7368c` | `0x006CC7C0..0x006CC7D2`, `sub_ACC7C0` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-32100` | `0x0067F23B` | `FF 15 68 82 B1 00` / `call *0xb18268` | `0x00712DDA..0x00712DE2`, `sub_B12DDA` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-34312` | `0x006EF301` | `FF 15 28 8A B1 00` / `call *0xb18a28` | `0x006EF253..0x006EF29D`, `__Init_thread_footer` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-34313` | `0x006EF307` | `FF D6` / `call *%esi` | `0x006EF253..0x006EF29D`, `__Init_thread_footer` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-34314` | `0x006EF311` | `FF 15 18 82 B1 00` / `call *0xb18218` | `0x006EF253..0x006EF29D`, `__Init_thread_footer` | `UNRESOLVED_IDA_CODE_REGION` |
| `RESIDUAL-INDIRECT-CALL-34315` | `0x006EF31D` | `FF 15 1C 82 B1 00` / `call *0xb1821c` | `0x006EF253..0x006EF29D`, `__Init_thread_footer` | `UNRESOLVED_IDA_CODE_REGION` |

IDA's decoded tail clusters agree with disk control flow. Examples include the invalid-parameter tails at `0x0000C2C6`, `0x0001F7E0`, `0x0001F851`, `0x0001F8C5`, `0x00308E0F`, and `0x003E26AE`; the remote GL loader at `0x00662770..0x006627CB`; the three-resource release tail at `0x0066C340..0x0066C38E`; and the CRT init-thread tail at `0x006EF2EF..0x006EF324`.

## 5. Mechanically affected function ranges and profiles

Ten census function-range rows were referenced only by the now-unresolved target rows and were removed without reordering the retained array:

`indirect-census-function-00715170`, `indirect-census-function-00715310`, `indirect-census-function-00715320`, `indirect-census-function-00715300`, `indirect-census-function-002f3050`, `indirect-census-function-00717020`, `indirect-census-function-00712dcf`, `indirect-census-function-0070bd24`, `indirect-census-function-006cc7c0`, and `indirect-census-function-00712dda`.

The owner ranges `indirect-census-function-0062ca10` and `indirect-census-function-006ef253` remain because unchanged evidence still references them. Their retention does not make them containing ranges for the repaired tail rows.

| Canonical array | v2 count | compact bytes | SHA-256 |
|---|---:|---:|---|
| `anchors.functionRanges` | 8,941 | 1,721,746 | `F74FFF7F9BCA4166CCF670274AFE400807B3A3C2FEA48F8490DAF0A207FDC80F` |
| `managerReceiverDispatch.residualIndirectCallFrontier` | 34,430 | 36,781,259 | `F0BA9E436C1A4DB7ED872EFA05E55E9FF262C9BCCA74468F399134BF6FB22984` |

Serialization is the presealed Python compact form: `json.dumps(value, ensure_ascii=False, separators=(",", ":"))`, parsed property order, UTF-8 without BOM or trailing newline. Every other canonical array profile remains identical to v1.

The final function-range composition is 8,941 rows: 36 core, 8,857 retained census ranges, and 48 receiver-frontier-only ranges. Residual provenance is now 33,960 identified plus 470 unresolved. Tail provenance remains 3,721 identified plus 89 unresolved, so the combined residual/tail totals are 37,681 identified and 559 unresolved.

New mechanically derived `actualCounts` fields are:

- `functionRangeCount=8941`
- `residualIdentifiedFunctionRangeCount=33960`
- `residualUnresolvedIdaCodeRegionCount=470`
- `residualProvenanceAdjudicationCount=21`
- `residualProvenanceCorrectedIdentifiedCount=0`
- `residualProvenanceDowngradedToUnresolvedCount=21`
- `removedUnreferencedFunctionRangeCount=10`

`managerReceiverDispatch.residualFunctionProvenanceSummary` records the exact 21-ID sequence, the conservative tail/chunk rule, the two retained still-referenced owner ranges, and the ten removals. `setSemantics.canonicalOrder.functionRanges` now truthfully states that retained v1 evidence order is preserved after the exact ten-row removal.

## 6. Preserved evidence meaning and verdict

The residual cardinality remains 34,430 and the full indirect-call partition remains 489 base + 270 split + 34,430 residual = 35,189. Tail rows remain 3,810. Receiver nodes/edges remain 18/24 and receiver frontier remains 38,985. Pair seeds/nodes/edges/frontier remain 35/10/8/37.

All non-target residual rows, all tail rows, direct/vtable/pair seeds, receiver and pair graphs/frontiers, service/thread/import/Manager-escape evidence, zero-set inputs, drainability, owner-through-drain, and permission fields preserve v1 meaning.

- `verdict=UNKNOWN`
- `dynamicAliasUniverseClosed=false`
- first unresolved edge remains RVA `0x00024037`, raw `FF D0`
- drainability remains `UNKNOWN`
- native owner through drain remains `UNKNOWN`
- `formalDecision=null`
- promotion and every permission/unlock remain false

This retry repairs provenance validity only. It does not close any alias frontier and does not authorize subsequent work.

## 7. REVERSE-only consistency checks

The frozen artifact was checked read-only for exact v1/preseal bindings, v2 identity/schema, the 21-ID target set and array order, exact unresolved shapes/null refs, unchanged target row fields outside the allowed provenance pair, 34,430 residual cardinality, 8,941 unique function-range IDs/RVAs, disk SHA-256 for every retained identified function range, fixed-PE raw bytes for all 21 instructions, canonical compact hashes, derived counts, deep-equal zero-set/pair arrays, unchanged verdict/first gap, and false runtime/scope flags.

No TEST command or verifier was run.
