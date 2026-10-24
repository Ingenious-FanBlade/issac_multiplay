> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 Wave A MT-R01/MT-R09 independent audit v1 handoff

## Result

Independent TEST verdict is `PASS_WAVE_A_PARTIAL_STATIC_CONFIRMED_FAIL_CLOSED`.
Exactly one of the three audited copy/move RVAs is eligible for composition into
the next positive frontier:

| Task | RVA | Independent verdict | Frontier-eligible |
|---|---:|---|---:|
| MT-R01 | `0x001CBEB5` | `STATIC_CONFIRMED_NO_OVERLAP` | yes |
| MT-R09 | `0x0001B072` | `UNKNOWN` | no |
| MT-R09 | `0x0001B3E3` | `UNKNOWN` | no |

This is a partial Wave A result, not an assertion that all three rows closed.
It does not promote manager-this, whole-image raw v2, formal v7, or T0-05.

## Frozen inputs and role isolation

The independent auditor used only the frozen PE, the pinned read-only
REPENTOGON checkout, the two public candidate JSON documents, and their public
evidence bindings. It did not import, execute, or inspect either candidate
builder/analyzer and did not modify either candidate.

- Target: PE32 I386, ImageBase `0x00400000`, SizeOfImage `0x008FE000`,
  SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Baseline frontier v30 SHA-256:
  `12EC336104A9C049EE15E83E634977F2C8CAE3E11AD4AD3E9711DEBCC3D4F814`.
- Updated work breakdown v33 SHA-256:
  `D1004AE824677B37C006EE873EFC80A48566CBACE1B1EE44F81FD1A952FA3308`.
- REPENTOGON checkout: commit
  `421c91d082db9be0ae02ac49801b60e3c5a0ad0c`, version `1.1.2`, documented
  target `v1.9.7.12.J273`. Git identity and all five pinned source hashes were
  recomputed independently. The checkout remained
  `EXTERNAL_CANDIDATE_ONLY`; no scanner, injection, offset import, type-name
  import, or external-address promotion was permitted.
- R01 candidate v3 SHA-256:
  `EB7A61A2F8DF7370055C7444D348794A841582B4456C240F479F1BF3DF2C7932`.
- R09 candidate v3 SHA-256:
  `10045BD2C00B7DE845807DE7DF6FFE566265DA05ED1AC443C9E325D60251F445`.

## Independent recomputation

The TEST auditor independently rebound all three call instructions to the
frozen PE, decoded the rel32 helpers, checked the containing-function ranges,
rehash-verified current-PE proof atoms and writer/caller public-universe
entries, reconstructed the exact writer/caller unions, recomputed 32-bit
interval/no-wrap and backing-extent relations, and repartitioned every required
countermodel. Duplicate JSON keys, extra claims, missing/reordered rows, and
inconsistent aggregate verdicts are rejected.

For MT-R01, `GameStatePlayer::Init` was the sole relocated external seed.
Store and Restore had multiple matches and PostLevelInit had zero matches, so
those three were independently retained as `NO_USABLE_SEED`; all four remained
`usedForPromotion=false`. The positive result is instead bound to current-PE
proof for the `0x5DC` record formula and `+0x500/+0x504/+0x508` tuple. The
independent checks closed all required producer classes and the exact public
universe union (13 writer/lifecycle/alias RVAs and 41 caller RVAs), including:

- manual construction/insertion;
- online-specific callers;
- the `[EBP-0x298]` spill/reload liveout;
- constructor, clear, save, restore, copy and move paths;
- rejection of REPENTOGON `0x5BC` or unnamed-field layout import.

The resulting destination/source spans are confined to the same live
allocator-owned vector backing. Their mutual overlap is expected and handled
by `memmove`; neither backing can alias the mapped GameRoot slot
`[0x00871678,0x0087167C)`. Thus `0x001CBEB5` satisfies
`fullyAdjudicatedNoOverlap=true`, has no unresolved item, and is independently
eligible for positive-frontier composition.

For MT-R09, the public candidate and TEST agree that current-PE four-field
object identity, shared sublist backing, and rejection of a direct-parent-store
only proof are established. They do not establish the remaining premises.
Both rows therefore remain fail-closed `UNKNOWN` with this exact unresolved
set:

- `UNTIE_DETACH_TO_INDEPENDENT_BACKING`;
- `ALREADY_OFFSET_ENTITYLIST_EL_ALIAS`;
- `SOURCE_COUNT_GT_0X8000`;
- `ARENA_EXTENT_LT_COPY`;
- `SLOT_ALIAS_DESTINATION`;
- `SLOT_ALIAS_SOURCE`.

In particular, the `0x8000` source-count label and `0x20000` arena-extent label
are not treated as proofs while their closure flags remain false. Neither R09
row is eligible for composition.

## Negative and replay gates

The frozen validator rejected all 74 negative mutations. The set covers wrong
PE identity, wrong or elevated REP provenance, wrong commit/version/file hash,
signature-only promotion, missing/duplicate/reordered rows, PE byte/helper/ABI
drift, proof-atom drift, open writer/caller universes, inconsistent row and
aggregate verdicts, runtime permissions, and all required R01/R09 task-specific
countermodels.

Candidate formatting was also fail-closed before final acceptance:

- R01 v1 was rejected for public-envelope drift; v2 was rejected because its
  row writer set did not equal the independently reconstructed universe union;
  only create-new/byte-stable v3 was audited as the final input.
- R09 v1 was rejected for the same writer-union inconsistency; v2 was rejected
  for noncanonical bound/extent serialization; only create-new/byte-stable v3
  was audited as the final input.

The formal audit was created once and reproduced byte-for-byte by two
consecutive `--verify-existing` runs. Its SHA-256 remained
`3B85E54D25888F5DA260971BB3BED0194C2032DAA313A775BEA9430BD71E73AA`.
An ordinary create-new rerun correctly rejected overwrite, and a run with an
incorrect R01 candidate hash failed before producing a negative-test output.

The validator self-test was also created once and reproduced by two consecutive
verify-existing runs. It rejected all 74 mutations and has SHA-256
`9A31061FD2D4B69C5117DB89F776DD3C550815B391367E143504E1733091B0A4`.

## Ledger consequence

Relative to v30, only the independently accepted R01 row may be added:

- copy/move: `319/405 -> 320/405`;
- memset: unchanged at `225/242`;
- total: `544/647 -> 545/647`;
- remaining: `103 -> 102`.

The two R09 RVAs remain in the precise UNKNOWN difference set. A successor may
be created by the main integration role for the one accepted R01 row; this
TEST artifact does not itself mutate the frontier, status/index documents,
production code, runtime, or IDB.

## Artifacts

- Validator: `out/cs-t0/audit-manager-this-mt-wave-a-independent-v1.py`.
- Public candidate envelope:
  `out/cs-t0/native-join-manager-this-mt-wave-a-candidate-envelope-v1.md`.
- Validator self-test:
  `out/cs-t0/native-join-manager-this-mt-wave-a-independent-validator-v1-self-test.json`.
- Formal machine audit:
  `out/cs-t0/native-join-manager-this-mt-wave-a-independent-audit-v1.json`.
- This handoff:
  `docs/refactor/cs-t0/t0-05/06-manager-identity/02-copy-move/native-join-manager-this-mt-wave-a-independent-audit-v1-handoff.md`.
