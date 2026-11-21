> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 split rule-hit accounting — independent REVERSE diagnostic v1

Date: 2026-09-09  
Role: independent REVERSE  
Scope: read-only diagnosis of `REBUILT_EQUIVALENT_DISPATCH_ORACLE` only  
Alias verdict impact: none; `UNKNOWN` and `dynamicAliasUniverseClosed=false` remain unchanged

## 1. Fixed identities and safety boundary

- fixed PE: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`
- PE SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- fixed `objdump`: `D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe`
- `objdump` SHA-256: `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`
- banner: `GNU objdump (GNU Binutils) 2.44`
- manifest inspected read-only: `cs-t0/native-alias-universe-v2.json`
- manifest SHA-256: `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`

The diagnostic did not modify the manifest, verifier, decision, production code, PE, IDB, or any prior evidence. It did not enumerate, open, start, attach to, or otherwise access the game process, and did not use the game, x32dbg, IPC, injection, hook, input, or world mutation. This create-new handoff is the only file publication resulting from the diagnosis.

## 2. Transparent execution history

There were three incomplete diagnostic surfaces before one effective whole-image reconstruction:

1. The inherited v11 Group 1 TEST failure reported `REBUILT_EQUIVALENT_DISPATCH_ORACLE`, but the combined comparison did not identify which member differed and emitted no five-field rebuilt count. Independent REVERSE did not invoke or rerun that verifier.
2. The first REVERSE PowerShell streaming reproduction used a top-level pipeline control-flow form that returned before a census/result was produced. It emitted no usable count and is not evidence.
3. The second REVERSE PowerShell streaming reproduction was restructured as a helper function but reached the approximately 30-second host command boundary without producing a completed census/result. It likewise emitted no usable count and is not evidence.

The only effective diagnostic reconstruction used one completed fixed-tool whole-`.text` disassembly with `objdump -d -w`, parsed independently with the verifier's regexes and state transitions. It did not import, compile, or execute the verifier. The reconstruction consumed 2,094,565 decoded instructions. Observed instruction addresses had zero non-monotonic transitions, so processing the `objdump` stream was equivalent to the verifier's ascending-address sort for this fixed PE.

The two incomplete REVERSE drivers and the inherited TEST error label are disclosed to avoid representing an empty or timed-out result as a successful independent observation. Only the completed reconstruction below is used for the diagnosis.

## 3. Exact reconstructed counts

The effective verifier-equivalent state machine produced:

```json
{
  "directPositiveSlotLoadThenRegisterCall": 270,
  "localRegisterCopy": 53,
  "spillReload": 14,
  "slotAddressLeaOrAddThenMemoryCall": 0,
  "indexedSibCallOperand": 0
}
```

The related population counts were:
\n+- reconstructed split-register candidates: `270`
- base-only candidates: `489`
- core projection: `759` (`489 + 270`)

Final recognition classes were:

- `SLOT_TARGET_REGISTER`: `270`
- `SLOT_ADDRESS_MEMORY`: `0`
- `INDEXED_SIB_SLOT_OPERAND`: `0`

All 270 final split candidates have `localTransferCount=0`: each is the direct positive-slot-load-then-register-call form. No final candidate depends on a local register copy, spill reload, precomputed slot address, or indexed/SIB operand.

## 4. Exact manifest mismatch

The manifest stores:

```json
{
  "directPositiveSlotLoadThenRegisterCall": 270,
  "localRegisterCopy": 0,
  "spillReload": 0,
  "slotAddressLeaOrAddThenMemoryCall": 0,
  "indexedSibCallOperand": 0
}
```

Only two members differ from the verifier's current `observed_rule_hits` accumulator:

| Field | verifier reconstruction | manifest | disposition |
|---|---:|---:|---|
| `localRegisterCopy` | 53 | 0 | mismatch |
| `spillReload` | 14 | 0 | mismatch |

The other three rule-hit members match. `baseOnlyCandidateCount=489`, `splitRegisterCandidateCount=270`, and `coreProjectionCandidateCount=759` also match. Therefore the combined check raises `REBUILT_EQUIVALENT_DISPATCH_ORACLE` solely because `oracle.get("observedRuleHits") != observed_rule_hits`; no population-count comparison is responsible.

## 5. Accounting-semantics diagnosis

The verifier increments `localRegisterCopy` immediately whenever any tracked state is copied between registers. It increments `spillReload` immediately whenever any tracked stable spill is reloaded. These increments occur at intermediate state transitions without requiring that the propagated state later reach an indirect call or enter `reconstructed_split`.

On this PE, 53 tracked register-copy transitions and 14 tracked spill-reload transitions are subsequently killed, abandoned, or otherwise fail to reach a recognized final candidate. They do not change the 270-member split candidate set.

The manifest and REVERSE handoff use `observedRuleHits` as final-candidate recognition contribution: a rule is counted when it contributes to a candidate admitted to the 270-row projection. Under that definition, direct positive slot load contributes all 270 candidates, while local register copy and spill reload contribute zero. The discrepancy is therefore an accounting-domain mismatch:

```text
verifier: all intermediate tracked state-transition events
manifest: rules contributing to final reconstructed candidates
```

It is not an alias census difference and does not reveal a missing split candidate.

## 6. Minimal repair recommendation

Keep the manifest's final-candidate contribution semantics. Preserve the raw 53/14 transition totals only as optional, separately named diagnostic counters that are not compared with `equivalentDispatchOracle.observedRuleHits`.

After `reconstructed_split` is complete, derive contractual `observed_rule_hits` exclusively from each final candidate's independently reconstructed recognition class, producer chain, and local-transfer count. For this fixed PE that derivation is exactly:

```json
{
  "directPositiveSlotLoadThenRegisterCall": 270,
  "localRegisterCopy": 0,
  "spillReload": 0,
  "slotAddressLeaOrAddThenMemoryCall": 0,
  "indexedSibCallOperand": 0
}
```

Simply changing the manifest to 53/14 would make the current comparison pass, but it would relabel intermediate transitions as final-candidate rule hits, contradict the recorded 270 candidate chains and requiring a new evidence/hash seal. It is therefore not the recommended minimal correction.

No alias evidence, frontier, first unresolved edge, drainability, owner-through-drain conclusion, permission, or unlock changes as a result of this diagnostic.
