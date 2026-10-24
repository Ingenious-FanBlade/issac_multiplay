> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 non-vector memset exact8 v27 independent audit v1

## Verdict

The independent disk-only PE audit accepts the candidate partition:
`4 VERIFIED / 4 UNKNOWN`.

The four accepted positive rows are:

- `0x0001D1D9`
- `0x0024CF7A`
- `0x0029FA85`
- `0x003DE449`

The four retained negative rows are:

- `0x000148AA`
- `0x005CF2B3`
- `0x0062C316`
- `0x006EC167`

This advances the accepted memset ledger from `221/242` to `225/242`, leaving
`17` exact memset rows.  It does not grant manager-this, whole-image, or T0
promotion.

## Independent replay

The audit independently checked all eight exact call byte sequences, 18
branch/allocation/frame anchors, and complete raw PE `E8`/`E9`/absolute-target
universes for the five positive or conditional helper functions.  It also
replayed the candidate analyzer and required its `4/4` classification and
`30/30` mutation rejection result.

The positive rows retain explicit start/count/extent inequalities and no-wrap
certificates.  The four negative rows remain fail-closed at their first open
receiver/producer/backing-extent premises.  In particular, the
`0x0062C316` format call at `0x009A382B` still lacks a proved `0x200`-byte
backing extent from `EBP-0x118`.

The recorded external-analysis disposition is `TIMEOUT_NO_PASS`; it was not
counted as positive evidence.

## Reproduction

```powershell
python out\cs-t0\audit-manager-this-fill-nonvector-exact-eight-v27-independent-v1.py --verify-existing
```

Two consecutive audit replays passed and all `10/10` independent audit
mutations were rejected.

## Artifact hashes

Candidate bindings:

- Analyzer: `70AF71C3B039C654050E2F17E2622A4A453B8100580DD1B9FD80224A805FD9A2`
- Machine JSON: `0EE93EF99415707B0EA0A43515BB0A9B31B558D7AE85BFA80F1AF2BD77841E9A`
- Handoff: `2D1854D685E62D35B593CD0F54AEC88F56510FF7BAB7571CD6446B40E2B7AB2C`

Independent audit:

- Analyzer: `10A34E3C9F32543D9389BCADFDC665FAA0ECBA9E5EF9B6926D20947D24DA7CD1`
- Machine JSON: `F01AD1505160C661E147CBF6D8A6AA77531DD2398913C07948C435DC2F7B51EB`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
