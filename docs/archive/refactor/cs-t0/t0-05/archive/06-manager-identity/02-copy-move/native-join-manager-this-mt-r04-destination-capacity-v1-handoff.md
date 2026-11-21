> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R04 destination-capacity REVERSE handoff v1

## Result

MT-R04 was evaluated against the frozen PE for exactly
`{0x0001AAB0, 0x0001AEB8}`.  The REVERSE candidate is fail-closed:

- static-confirmed no-overlap: `0/2`;
- `UNKNOWN`: `2/2`;
- unknown RVAs: `0x0001AAB0`, `0x0001AEB8`;
- no frontier, manager-this, or whole-image promotion is requested.

The first row's exact 177 direct destination-producer callsites and the second
row's exact five direct callsites are serialized in the candidate.  The direct
E8 census is target-bound, and no relative-E9 forwarder was found for either
containing function.  Counting the producers did not establish the required
capacity invariant.

## Frozen target and public bindings

- PE: x86 `PE32_I386`, ImageBase `0x00400000`, SizeOfImage `0x008FE000`;
- PE SHA-256:
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`;
- Wave C public candidate envelope SHA-256:
  `7882E0448E1E721B0A1CA9D5BD5412A1C931AD55ABE145994B664FEC62B14335`;
- the candidate also binds v31 plus its audit, the Wave B audit, v33, caller-heap
  v29 plus its audit, and copy-range v7 at their envelope-declared hashes.

The builder read only the disk PE and public repository evidence.  The game was
not started, no process was opened or read, no debugger was attached, and the
IDB was not modified.

## Current-PE findings

Both rows have confirmed instruction boundaries, cdecl `memcpy` call setup,
element stride four, and helper target `0x00AF05DF`:

| Callsite RVA | Function RVA | Bytes | Producers |
|---|---|---|---:|
| `0x0001AAB0` | `0x0001A790` | `E8 2A 5B 6D 00` | 177 |
| `0x0001AEB8` | `0x0001AB50` | `E8 22 57 6D 00` | 5 |

The smallest common open boundary is the sublist constructor/helper at
`0x00417540`:

1. It derives the destination pointer from `parent.backing + 4*parent.count`
   and records the requested `sourceCount` as the destination capacity.
2. At `0x0041756D` it tests
   `parent.count + sourceCount <= parent.capacity`.
3. If the test fails it only logs `EL: invalid sublist`; at `0x00417590` it
   falls back to the parent backing start, and at `0x0041759B` it still writes
   the advanced parent count.
4. The caller-side checks at `0x0041AA8F` and `0x0041AE97` likewise only log
   `EL: not enough space for copy` and leave the following `memcpy` reachable.

Consequently the destination descriptor's nominal `capacity=sourceCount` does
not prove that its pointer denotes that many elements of live parent backing.
The unresolved proof obligation is the parent sublist backing-capacity and
owner/lifetime invariant on every producer path.  The five direct callers of
the second row were enumerated at `0x00434420`, `0x004DF964`, `0x005E7802`,
`0x006FD9CF`, and `0x00799A10`; each passes a stack result descriptor, but none
of those local result descriptors closes the queried parent backing invariant.
For the first row, the exact 177-entry direct-call census is preserved without
claiming that all producer semantics, indirect/address-taken paths, source
count bounds, or backing writers are closed.

The retained countermodel is therefore:
`parent.count + sourceCount > parent.capacity`; logging occurs, the helper
falls back to the parent backing start and advances the count, and the query
path can still reach a `4*sourceCount` copy.  With the parent backing extent,
source-count upper bound, owner lifetime, and all-writer fixed point open,
`sourceCount <= usable destination capacity`, multiplication no-wrap, and
GameRoot-slot non-alias cannot be promoted.

## Artifacts and replay

- builder:
  `out/cs-t0/build-manager-this-mt-r04-destination-capacity-v1.py`
  - SHA-256:
    `D108950DD1794C2055BD8187AC66D03367872C22E0192FD1E00BF7A3B86515C1`;
- candidate:
  `out/cs-t0/native-join-manager-this-mt-r04-destination-capacity-v1.json`
  - schema:
    `cs-t0.manager-this.mt-r04-producer-capacity.positive-candidate.v1`;
  - SHA-256:
    `8D74E4FB7384AE66273C216E9145243FBE9F139F569178BAC2E68850275BA8A6`.

Create-new and the single compressed verify-existing replay both emitted the
same candidate SHA-256:

```powershell
python out\cs-t0\build-manager-this-mt-r04-destination-capacity-v1.py
python out\cs-t0\build-manager-this-mt-r04-destination-capacity-v1.py --verify-existing
```

Both commands reported `verified=0 unknown=2 mutations=6
independentAudit=false`.  The six author-side mutations cover target PE, exact
rows, a fabricated capacity proof, removed countermodel, false promotion, and
permission drift.  No separate self-test artifact, Cargo test, or full T0 build
was run.

## Role boundary

This is a REVERSE-authored candidate and handoff only.  No independent TEST
audit was performed or imported, no TEST auditor was read, and no audit or
frontier artifact was authored.  The independent TEST role must recompute the
RVA set, bytes, producer/writer unions, capacity countermodel, and verdict from
the frozen PE and public inputs.
