> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 Wave B MT-R02/R03 independent audit v1 handoff

## Result

The compact independent TEST verdict is
`PASS_WAVE_B_PARTIAL_STATIC_CONFIRMED_FAIL_CLOSED`, with zero positive rows and
four retained UNKNOWN rows:

| Task | RVA | Independent verdict | Frontier-eligible |
|---|---:|---|---:|
| MT-R02 | `0x00008A62` | `UNKNOWN` | no |
| MT-R02 | `0x00008B2A` | `UNKNOWN` | no |
| MT-R03 | `0x0001513A` | `UNKNOWN` | no |
| MT-R03 | `0x000155A4` | `UNKNOWN` | no |

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
- Prior callerHeap exact17 independent audit SHA-256:
  `6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD`.
- v33 work breakdown SHA-256:
  `D1004AE824677B37C006EE873EFC80A48566CBACE1B1EE44F81FD1A952FA3308`.
- Public envelope SHA-256:
  `BDA205AB7A2A6A4DAA61E9B5B1C61EBB9F1AD269CE01AC22805466E62F182C9F`.
- R02 candidate SHA-256:
  `A484882A285B1CB41907F4F02C34FB5B0A540D539C5290172ACFCD0D62241C3A`.
- Final R03 candidate v2 SHA-256:
  `A8DA2B59E5D9D26D22DAC734A6E791A6D4E745D76857DF68562D563A04202B4F`.

R03 v1 was retained as a rejection record: its row writer list omitted two
ALIAS entries present in its own public universe. V2 corrected only that
serialization inconsistency and retained the same 0V/2U conclusion.

## Independent row findings

All four frozen call instructions, rel32 targets to `memcpy` at `0x00AF05DF`,
containing-function identities, proof-atom bytes/hashes, and public
writer/caller-universe unions were independently rebound to the target PE.
Exact row order and membership, predecessor bindings, countermodel partitions,
aggregate classifications, and permissions were recomputed rather than
accepted from candidate summaries.

For MT-R02, both rows preserve the concrete countermodel `newCount=1,
oldCount=2`: allocation may cover four bytes while the copy requests eight.
The remaining gaps include the missing `oldCount<=newCount` or stronger span
invariant, multiplication/span closure, incoming descriptor and address-taken
caller producer closure, and GameRoot-slot non-alias proof. The first row binds
the descriptor `+0x1C` pair and the second binds `+0x24`; neither is promoted.

For MT-R03, the fixed backing extents are correctly identified as 5000
elements, but the observed limit is logging-only and does not stop execution.
The retained minimal counterexamples are:

- `0x0001513A`: `emittedCount=5001` requests `420084` bytes against a
  `420000`-byte (`5000*84`) backing;
- `0x000155A4`: `emittedCount=5001` requests `540108` bytes against a
  `540000`-byte (`5000*108`) backing.

Parser and interpolation paths do not establish a hard `<=5000` bound, so
count multiplication, fixed-backing coverage, and GameRoot-slot exclusion
remain open. A log event was not accepted as a boundary.

## Compact negative and replay gates

Per the user's reduced-test instruction, no preliminary self-test JSON was
created. The formal auditor retains 14 high-value mutations only. They cover
exact-row omission/duplication, frozen PE identity and call bytes, required
binding loss, proof-atom byte/hash drift, writer/caller-union inconsistency,
open universe closure, the R02 old/new-count bound, the R03 logging-only and
hard-bound countermodels, reopened countermodels, aggregate fake promotion,
and enabled permissions. All 14 were rejected.

The formal audit was created once and reproduced byte-for-byte by one
`--verify-existing` run. Its SHA-256 is
`8DB1A94C61B9798012CD73E596A07EEA897103D93A5FEC6204FCE989B55A44FB`.
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
  `out/cs-t0/native-join-manager-this-mt-wave-b-candidate-envelope-v1.md`.
- Auditor: `out/cs-t0/audit-manager-this-mt-wave-b-independent-v1.py`.
- Formal audit:
  `out/cs-t0/native-join-manager-this-mt-wave-b-independent-audit-v1.json`.
- This handoff:
  `docs/refactor/cs-t0/t0-05/06-manager-identity/02-copy-move/native-join-manager-this-mt-wave-b-independent-audit-v1-handoff.md`.
