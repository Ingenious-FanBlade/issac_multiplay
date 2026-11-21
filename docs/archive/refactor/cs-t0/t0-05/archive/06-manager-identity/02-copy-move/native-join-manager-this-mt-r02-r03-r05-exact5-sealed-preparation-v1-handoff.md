> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02/R03/R05 exact5 sealed TEST preparation v1 handoff

## Outcome

Independent TEST admission rules were sealed before any new REVERSE successor was supplied or read. The validator is ready for a later public candidate covering exactly:

- MT-R02: `0x00008A62`, `0x00008B2A`;
- MT-R03: `0x0001513A`, `0x000155A4`;
- MT-R05: `0x0033570B`.

The preparation binds v36, frontier v33, the prior exact16 audit, the R02/R03/R05 historical handoffs, and the frozen PE. All five rows are present in v33 remaining and retain the prior exact16 `UNKNOWN` baseline.

No current-round REVERSE candidate or builder was known, read, imported, or authored by TEST. No frontier, index, checkpoint, production code, historical evidence, or IDB was modified.

## Sealed artifacts

| File | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-mt-r02-r03-r05-exact5-sealed-acceptance-spec-v1.json` | `B8EA5449E90F398D2B2514D9176810E4B91986783AE3E0482ECF4B07F79D6A93` |
| `out/cs-t0/audit-manager-this-mt-r02-r03-r05-exact5-sealed-v1.py` | `E70C507AFCA086435F1DC939BDD86DDC3DFA18E882877F7FF46C034E60B97027` |
| `out/cs-t0/native-join-manager-this-mt-r02-r03-r05-exact5-sealed-preparation-v1.json` | `36231A55B099685A06F00D4F7FE2F315EFCDB11CFCF5266FD0EFE65558593F4C` |

A later candidate must bind the sealed spec path and hash exactly. The validator supports candidate mode without requiring any change to the sealed rules.

## Decision contract

Every row is evaluated against this ordered universal obligation list:

1. `backingExtent`;
2. `countAndNoWrap`;
3. `sourceLifetimeAndIdentity`;
4. `targetIntervals`.

A `STATIC_CONFIRMED_NO_OVERLAP` row must close all four obligations for `ALL_REACHABLE_EXECUTIONS`, cite frozen-PE proof-atom IDs, and provide structured details rather than a prose-only assertion:

- source and destination extent expressions, with both proven to cover the transfer;
- the exact transfer-length expression, nonnegativity, and no-wrap;
- source owner, identity, lifetime coverage, and stability during transfer;
- exact source/destination start and end-exclusive expressions, the manager-this slot `[0x00C71678,0x00C7167C)`, and both disjointness results;
- `memcpy` additionally requires source/destination `DISJOINT`; `memmove` must explicitly state either disjointness or overlap allowed by memmove semantics.

An `UNKNOWN` row must name the first obligation whose state is `OPEN`, include a specific uppercase gap ID, a concrete statement, a retained countermodel, and a nonempty unresolved list. If the first open obligation is unchanged from the baseline, its ID and countermodel must match the sealed baseline exactly; a generic “universe open” statement cannot replace it.

## Sealed baseline first gaps

| RVA | First obligation | Exact retained countermodel |
|---|---|---|
| `0x00008A62` | `backingExtent` | `oldCount=2,newCount=1` allocates 4 destination bytes and copies 8 bytes. |
| `0x00008B2A` | `backingExtent` | Same independent second-pair countermodel. |
| `0x0001513A` | `backingExtent` | `emittedCount=5001` requests 420084 bytes from a 420000-byte backing. |
| `0x000155A4` | `backingExtent` | `emittedCount=5001` requests 540108 bytes from a 540000-byte backing. |
| `0x0033570B` | `countAndNoWrap` | `auxiliaryRangeCount>0 && filteredCount==0` yields raw length `0xFFFFFFFC`. |

## Compressed preparation verification

- One preparation create-new run: PASS; candidate read was false.
- Exactly one preparation verify-existing run: PASS and byte-identical.
- Six decision-rule mutations were rejected: exact-row omission, obligation field omission, weakened quantifier, weakened first-open rule, incomplete positive target interval, and weakened permission policy.
- One wrong-spec-hash probe failed before write; the probe file was absent before and after.

No Cargo test, full T0 build, broad suite, repeated self-test, game/process access, debugger attachment, runtime sampling, or IDB mutation was performed.

## Final audit invocation boundary

When MAIN supplies the public candidate path and physical SHA-256, TEST may run the already sealed validator in candidate mode. A final audit output will be create-new, followed by exactly one verify-existing and the candidate-mode decision mutations. No validator or spec edit is permitted after candidate handoff without invalidating the seal.
