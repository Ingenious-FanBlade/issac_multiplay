> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 vectorGrow exact23 v27 independent audit v1

## Verdict

**PASS_CANDIDATE_FAIL_CLOSED_0V23U**.  The immutable v27 candidate correctly
retains all 23 rows as UNKNOWN.  It does not change the copy/move ledger:
`319/405` remains closed, `86` remains, and the accepted delta is zero.

This audit was performed by a tester that did not author v27.  The candidate
analyzer was not imported.  All positive observations were rebuilt from the
canonical local PE; no game process, debugger, IDB mutation, or runtime source
was used.

## Candidate bindings

| Artifact | SHA-256 |
|---|---|
| `out/cs-t0/analyze-manager-this-copy-move-vector-grow-exact23-v27.py` | `F3A0E38567A8FD87B83264CF90B4DB8CE73DFC1B0C97F8FE689D5A4CD0CB3CAB` |
| `out/cs-t0/native-join-manager-this-copy-move-vector-grow-exact23-v27.json` | `D8F618EFC26866EF7B70D1831E9D49FEB5F7AC731D96E41FD1556A232692E8B0` |
| `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-copy-move-vector-grow-exact23-v27-handoff.md` | `9BC95AF2BD554A61232E2B974BF2ACA0C914EAC20A1CC14B8CFAEFA008CEA595` |
| bound bulk-v26 JSON | `514399E365C79126E9256993549B957BF93800BDF8C1A788CEE588E3FE0328EF` |
| target PE | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

The hashes were independently recomputed before and after the audit.  No
candidate or prior artifact changed.

## Independent PE checks

- The bulk-v26 `copyMove.groups.vectorGrow` set equals the independently frozen
  23 RVA set exactly.
- All 23 bulk calls have the candidate bytes and resolve to `memmove` or the
  single expected `memcpy` target.  Each compare-through-call span contains the
  expected grow-helper edge; the 22 ordinary rows also reproduce the local
  capacity compare / `jbe` branch.
- Nineteen unique containing functions were scanned across the entire `.text`
  section for raw E8/E9 candidates, plus every non-`.reloc` exact-VA immediate.
  All candidate universes match.  Their decoded/indirect/split fixed points
  remain explicitly false.
- Seven grow roots independently reproduce requested-count limits,
  element-count multiplication without 32-bit wrap, the allocation call to
  `sub_40CF00`, and begin/end/capacity install anchors.  The byte-vector special
  chain also reproduces its nested grow and tuple-install helper.
- The two first-gap classes are exact:
  `DEST_AND_SOURCE_TUPLE_WRITER_FIXED_POINT_OPEN` = 22 rows;
  `CALLER_DEST_TUPLE_WRITER_FIXED_POINT_OPEN` = 1 row (`RVA 0x0062C746`).
  Consequently the local grow facts cannot promote any row.
- `rejected v4` is neither opened nor positively consumed.  Selection comes
  only from bulk-v26.
- IDA remains `TIMEOUT_NO_PASS` and was not used for this verdict.

## Replay and mutations

The immutable candidate was replayed twice consecutively:

```powershell
python out\cs-t0\analyze-manager-this-copy-move-vector-grow-exact23-v27.py --verify-existing
python out\cs-t0\analyze-manager-this-copy-move-vector-grow-exact23-v27.py --verify-existing
```

Both exited zero with:

```text
PASS PE vectorGrow exact23 verified=0 unknown=23 helpers=7 owners=19 mutations=54
```

The independent audit was then created and verified byte-identically twice:

```powershell
python out\cs-t0\audit-manager-this-copy-move-vector-grow-exact23-v27-v1.py
python out\cs-t0\audit-manager-this-copy-move-vector-grow-exact23-v27-v1.py --verify-existing
python out\cs-t0\audit-manager-this-copy-move-vector-grow-exact23-v27-v1.py --verify-existing
```

All 110 tester-authored mutation vectors were rejected: 69 per-row
status/gap/call-byte mutations, 19 owner-census mutations, 14 helper
no-wrap/install mutations, and 8 global rejected-v4/ledger/IDA/permission/set
mutations.

## Audit artifacts

- Analyzer:
  `out/cs-t0/audit-manager-this-copy-move-vector-grow-exact23-v27-v1.py`
- Machine JSON:
  `out/cs-t0/native-join-manager-this-copy-move-vector-grow-exact23-v27-audit-v1.json`

Analyzer SHA-256:
`FC8F66089B38748B76584322F8B7285241E8D5AFDA5D2B331D89403E77A95D1A`

JSON SHA-256:
`C16054194FE4BC2E27E0EF856CB761EE7B64CF910EAAB650B15F9A70DD7AAA2F`

All audit permissions and promotions are false.  The audit does not authorize
manager-this, whole-image, active input, passive probe, status/index changes,
or T0-05 unlock.
