> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 Wave D MT-R06/R10 independent audit v1 handoff

## Result

The compact independent TEST verdict is
`PASS_WAVE_D_PARTIAL_STATIC_CONFIRMED_FAIL_CLOSED`, with zero positive rows and
two retained UNKNOWN rows:

| Task | RVA | Independent verdict | Frontier-eligible |
|---|---:|---|---:|
| MT-R06 | `0x0061F2C2` | `UNKNOWN` | no |
| MT-R10 | `0x002EE04F` | `UNKNOWN` | no |

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
- Wave C independent audit SHA-256:
  `9B61B0E76C59E059D8208C2805B90E39BB2FFFFAB0B2E747326A9E846F5B816D`.
- v33 work breakdown SHA-256:
  `D1004AE824677B37C006EE873EFC80A48566CBACE1B1EE44F81FD1A952FA3308`.
- R06 prior callerHeap-v29 independent audit SHA-256:
  `6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD`.
- R10 prior localHard-v28 independent audit SHA-256:
  `95148047034CA738BB426BCD46F68D19B052EA3A67360551E22729E1815A5102`.
- Public envelope SHA-256:
  `89DB52BC84CF91C322CA5F78B74AC843442DC95D8FFE395783FFE32E5E54CF10`.
- R06 candidate SHA-256:
  `A3A167CD597CD6BE0D3B6496E48ADADA2F43275E731AE9897FEBB94A0A6B08E5`.
- R10 candidate SHA-256:
  `C47B3CCEB50BFC0DFA61F8CA1F8A447ACDD6C09016240720F6300782BE1A2B43`.

## Independent row findings

Both frozen call instructions, rel32 targets to `memcpy` at `0x00AF05DF`,
containing-function identities, proof-atom bytes/hashes, and public
writer/caller-universe unions were independently rebound to the target PE.
Exact row membership, current and legacy audit bindings, universe censuses,
countermodel partitions, aggregate classifications, and permissions were
recomputed rather than accepted from candidate summaries.

For MT-R06, the call at `0x0061F2C2` independently resolves source
`[this+0xC0]` (`this[48]`), destination `[this+0xC4]` (`this[49]`), and length
`4*((this[5]-this[4])>>4)`. The InputDeviceBase-compatible constructor and the
known zero/allocation/cleanup sites are bound, but the derived
Net/Gamepad/Keyboard constructor fixed point and class-specific direct,
indirect, address-taken, and already-offset writer aliases remain open. No
persisted capacity or replacement-writer invariant ties either buffer extent
to the recomputed length. The global source-template tuple writer lifetime and
post-init immutability are also open. An omitted writer can therefore replace
`this[49]` with a short backing or grow the tuple after initialization without
reallocation; source/destination disjointness and GameRoot-slot non-alias are
not established.

For MT-R10, TEST independently confirms the exact 22 direct `this+0x5C0`
writer sites and 28 LEA aliases, the source `+0x2094 -> this+0x5C0` transitive
copy, and the 32-byte destination stack object. Enumeration alone is not a
value-bound proof: helper/address-taken/return effects for the LEA aliases and
the source-object `+0x2094` writer fixed point remain open. The local path
excludes `n4=0`, but no closed global invariant establishes `n4<=9`.
`n4=10` remains a concrete countermodel because `4*n4-4` requests 36 bytes
from a 32-byte destination. The row therefore remains UNKNOWN.

## Compact negative and replay gates

No preliminary self-test JSON was created. The formal auditor retains exactly
12 high-value mutations. They cover exact-row loss, frozen call-byte drift,
loss of the task-specific legacy audit binding, writer/caller-union drift,
open R06 constructor and pointer/capacity writers, open R06 source/overlap
relations, R10 22-direct and 28-LEA census drift, the open `n4` range
countermodel, row-disposition fake promotion, and enabled permissions. All 12
were rejected.

The formal audit was created once and reproduced byte-for-byte by exactly one
`--verify-existing` run. Its SHA-256 is
`EBDB4E41B97E7D6D5ED265882EBE3A768A8B136937A22F58DE9EFBF9BBFC0E23`.
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
  `out/cs-t0/native-join-manager-this-mt-wave-d-candidate-envelope-v1.md`.
- Auditor: `out/cs-t0/audit-manager-this-mt-wave-d-independent-v1.py`.
- Formal audit:
  `out/cs-t0/native-join-manager-this-mt-wave-d-independent-audit-v1.json`.
- This handoff:
  `docs/refactor/cs-t0/t0-05/06-manager-identity/02-copy-move/native-join-manager-this-mt-wave-d-independent-audit-v1-handoff.md`.
