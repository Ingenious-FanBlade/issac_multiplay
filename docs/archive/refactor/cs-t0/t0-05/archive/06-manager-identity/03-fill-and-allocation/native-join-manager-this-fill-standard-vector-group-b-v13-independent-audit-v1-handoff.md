> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 standard-vector group B v13 independent audit v1

## Verdict

The independent disk audit accepts the candidate only as a negative partition:
`0 VERIFIED / 5 UNKNOWN`.  It accepts no positive row and grants no promotion.

The audit independently replayed:

- all five exact call bytes from the frozen PE;
- the complete raw direct-call universe for each of the five helper owners;
- the address-taken `sub_755470` occurrence and its still-open indirect-owner
  boundary;
- the five distinct incoming tuple-writer first gaps;
- the candidate analyzer's `0/5` replay and `28/28` mutation result.

All five incoming tuple writer fixed points remain open, so their retained
slot-covering countermodels are not discharged.

## Protocol finding

The candidate handoff records `TIMEOUT_NO_PASS` and explicitly claims no IDA
PASS, but the candidate machine JSON has no IDA-disposition field.  This does
not convert any negative row into a positive result.  A later positive
superseder must record its IDA result or timeout in machine-readable data.

## Reproduction

```powershell
python out\cs-t0\audit-manager-this-fill-standard-vector-group-b-v13-independent-v1.py --verify-existing
```

Two consecutive audit replays passed and all `9/9` audit mutations were
rejected.

## Artifact hashes

- Audit analyzer:
  `41EFF787DE6DA27B0BA8D8E7402768D88B7E043E0FA7DCC793D64A40EAD31E01`
- Audit machine JSON:
  `8F19E646B24A57F3F8959D8A53BF94643B3E6EFCA6691D17E0D047C286955A28`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
