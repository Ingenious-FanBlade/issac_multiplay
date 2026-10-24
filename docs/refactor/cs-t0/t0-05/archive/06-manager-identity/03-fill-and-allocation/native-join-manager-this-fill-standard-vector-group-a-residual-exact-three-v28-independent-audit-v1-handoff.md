> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 standard-vector group A residual exact3 v28 independent audit v1

## Verdict

The independent disk-only PE audit accepts the candidate only as a strict
negative partition: `0 VERIFIED / 3 UNKNOWN`.  No row is promoted, so the
accepted memset ledger remains `225/242` with `17` rows remaining.

The retained first gaps are:

- `0x00308B5A`: unowned split/EH parent successors for the
  `GameRoot+0x6774C` and object `+0x1E790` member families;
- `0x003271E2`: selected-record vector pre-offset and element-lifetime
  successors for `(n298+0x2A404)+0x0C`;
- `0x003272FC`: the `n298+0x2A404` root pre-offset split tail and successor
  classification of all 188 formation seeds.

All three local resize inequalities, fresh allocation extents, and local
no-wrap bounds are closed only conditionally on a well-formed incoming tuple.
The open successor fixed points preserve the explicit manager-slot
countermodels.

## Independent replay

The audit independently checked all three exact call bytes and rel32 targets,
seven split/constructor/manual-clear signatures, 12 function byte hashes,
nine direct-owner call universes, two tail-transfer universes, the 82-E8 / 15-E9
destructor census, and exact address-formation censuses of 5, 8, and 188 seeds.
It also replayed the candidate's `0/3` and `28/28` mutation result.

The candidate's external-analysis disposition remains `TIMEOUT_NO_PASS`; it
was not counted as a PASS or as positive evidence.

## Reproduction

```powershell
python out\cs-t0\audit-manager-this-fill-standard-vector-group-a-residual-exact-three-v28-independent-v1.py --verify-existing
```

Two consecutive audit replays passed and all `12/12` independent audit
mutations were rejected.

## Artifact hashes

Candidate bindings:

- Analyzer: `647047858C22A25E61BEF144E988537738B42DC6548230B0DC307200A72DDD87`
- Machine JSON: `438A03053113FD0102263E8FFF350A512F4978819BBE71FBC85C2F8C3128ACDB`
- Handoff: `2ABC63B41E1B8300AEC8C9637604B487134ACA3E1FFCD3229BC4F7F8A9841384`

Independent audit:

- Analyzer: `14777CC9D3C80DB20AA7BDEAE7124C109719E18C2B6EADE78F608E3E99265246`
- Machine JSON: `A36840AB4F0C72AB7DE1AF561A1993787DA13176FA6D2D8AEAEC23E64812E5CC`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
