> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 Wave C MT-R04/R05 independent audit v1 handoff

## Result

The compact independent TEST verdict is
`PASS_WAVE_C_PARTIAL_STATIC_CONFIRMED_FAIL_CLOSED`, with zero positive rows and
three retained UNKNOWN rows:

| Task | RVA | Independent verdict | Frontier-eligible |
|---|---:|---|---:|
| MT-R04 | `0x0001AAB0` | `UNKNOWN` | no |
| MT-R04 | `0x0001AEB8` | `UNKNOWN` | no |
| MT-R05 | `0x0033570B` | `UNKNOWN` | no |

Because the accepted delta is zero, the main integration role must not create
an empty positive-frontier successor for this wave. The live frontier remains
v31.

## Frozen inputs and isolation

The TEST role read only the frozen PE, sealed predecessor/public evidence, and
the two candidate JSON documents. Neither candidate builder was read,
imported, or executed; no candidate was modified.

- Target SHA-256:
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- v31 frontier SHA-256:
  `37C23D80BF45F4162305EA174A053E0D294BD9D32B0494D4639B516C2D7551EB`.
- v31 independent audit SHA-256:
  `244459B2B5C47A74E1D806E237895A9C7DB896CC748E0710885CFBFAC46F314D`.
- Wave B independent audit SHA-256:
  `8DB1A94C61B9798012CD73E596A07EEA897103D93A5FEC6204FCE989B55A44FB`.
- v33 work breakdown SHA-256:
  `D1004AE824677B37C006EE873EFC80A48566CBACE1B1EE44F81FD1A952FA3308`.
- Public envelope SHA-256:
  `7882E0448E1E721B0A1CA9D5BD5412A1C931AD55ABE145994B664FEC62B14335`.
- R04 candidate SHA-256:
  `8D74E4FB7384AE66273C216E9145243FBE9F139F569178BAC2E68850275BA8A6`.
- R05 candidate SHA-256:
  `1FB31C6A04355C0FA03C3F8D69043DFD58EC16B85705E76E0EEFDB66D463F26C`.

## Independent row findings

All three frozen call instructions, rel32 targets, containing-function
identities, proof-atom bytes/hashes, and public writer/caller-universe unions
were independently rebound to the target PE. Exact row membership,
predecessor bindings, producer counts, countermodel partitions, aggregate
classifications, and permissions were recomputed rather than accepted from
candidate summaries.

For MT-R04, the two rows identify the complete advertised producer censuses:
177 producers at `0x0001AAB0` and 5 producers at `0x0001AEB8`. Those counts do
not establish safety. The destination parent-backing capacity invariant is
open for both producer sets; the invalid-sublist path logs, falls back to the
parent backing, and advances the parent count; and the copy-capacity check logs
and continues to `memcpy`. Source-count bounds, multiplication no-wrap,
owner/lifetime closure, and GameRoot-slot non-alias also remain open. A
logging-only check was not accepted as a hard boundary.

For MT-R05, the real filtered-vector owner, backing extent, begin/end order,
and selected-iterator fixed point are identified. The empty range is
nevertheless reachable. It yields `selected=begin=end` and the transfer-length
expression evaluates to `0xFFFFFFFC`. No target-backed dominating nonempty
guard exists, and none was synthesized by TEST. The owner/backing facts do not
resolve this countermodel, so the row remains UNKNOWN.

## Compact negative and replay gates

Per the reduced-test instruction, no preliminary self-test JSON was created.
The formal auditor retains exactly 12 high-value mutations. They cover exact
row loss, frozen call-byte drift, required-binding loss, writer/caller-union
drift, open universe closure, the R04 177-producer census, open capacity
invariants, a synthesized R05 nonempty guard, the unresolved `0xFFFFFFFC`
countermodel, a reopened countermodel, aggregate fake promotion, and enabled
permissions. All 12 were rejected.

The formal audit was created once and reproduced byte-for-byte by exactly one
`--verify-existing` run. Its SHA-256 is
`9B61B0E76C59E059D8208C2805B90E39BB2FFFFAB0B2E747326A9E846F5B816D`.
An explicit wrong-candidate-hash run failed before creating an output.

## Ledger and permissions

With zero accepted rows, v31 remains unchanged:

- copy/move: `320/405`, remaining `85`;
- memset: `225/242`, remaining `17`;
- total accepted: `545/647`;
- total remaining: `102`.

No production code, runtime interface, frontier, status/index file, IDB, or
prior evidence was modified. Manager-this, whole-image, and overall promotion
remain disabled.

## Artifacts

- Public envelope:
  `out/cs-t0/native-join-manager-this-mt-wave-c-candidate-envelope-v1.md`.
- Auditor: `out/cs-t0/audit-manager-this-mt-wave-c-independent-v1.py`.
- Formal audit:
  `out/cs-t0/native-join-manager-this-mt-wave-c-independent-audit-v1.json`.
- This handoff:
  `docs/archive/refactor/cs-t0/t0-05/06-manager-identity/02-copy-move/native-join-manager-this-mt-wave-c-independent-audit-v1-handoff.md`.
