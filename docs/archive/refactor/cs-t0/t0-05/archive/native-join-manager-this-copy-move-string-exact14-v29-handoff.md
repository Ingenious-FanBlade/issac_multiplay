> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this copy/move string exact14 v29

## Outcome

This reverse-author package replays the exact `copyMove.groups.string` frontier
from bulk-v26 against the frozen PE.  String-extent v9 is used only as an
immutable numeric/layout input; its exact14 UNKNOWN identity is independently
cross-checked with v26 and all call bytes/targets are rebound to the PE.

All 14 rows have a locally reconstructed string transfer, including SSO/heap
selection, length/capacity fields, destination offset, helper count, total
written bytes, and conditional no-wrap/extent inequality.  None has a finite
fixed point for all incoming string identities, borrowed/external pointers,
SSO-union state, capacity allocation provenance, state transitions, and
return/liveout/indirect consumers.  Therefore the result is fail-closed:

| Result | Count |
|---|---:|
| Local conditional transfer reconstructed | 14 |
| Fully backed `VERIFIED` | 0 |
| `UNKNOWN` | 14 |

Copy/move remains `319/405`; no accepted row, whole-image, or manager-this
promotion is made.

## Exact set

```text
0x000310F4  0x00031154  0x000995C0  0x000995F6  0x00099608
0x00099684  0x00099690  0x000996A7  0x00099828  0x00251B8E
0x00251B9D  0x00251BB5  0x00284EB5  0x00284F19
```

Every exact call matches its frozen five bytes and rel32 target.  The helper
census records 272 raw whole-`.text` E8 candidates to `memcpy` and 306 to
`memmove`; the exact14 is proven to be the appropriate subset.  These are
conservative raw-byte censuses, not decoded-instruction claims.

## String layout and transition evidence

The local PE anchors consistently use a 24-byte narrow-string object:

- union/inline bytes at `+0x00`, inline extent 16 bytes;
- length at `+0x10`;
- capacity at `+0x14`;
- `capacity < 16` selects inline storage;
- `capacity >= 16` loads the heap pointer from the union, whose required
  character extent is conditionally `capacity + 1`.

Whole owner-function byte hashes and exact anchors bind reset, move adoption,
replace/assignment, append, insert, erase, allocate-grow, replaced-heap free,
and object/interior-pointer return forms.  The exact allocator calls target
`0x0040CF00`; replacement frees target `0x00AEF15C` where present.

The six owner roots and their raw direct caller counts are:

| Owner RVA | Raw E8 callers | Return/liveout form |
|---|---:|---|
| `0x00031070` | 21 | destination string object after move adoption |
| `0x00099580` | 14 | `this` after replace/assignment |
| `0x000997F0` | 166 | `this` after append |
| `0x00251B10` | 22 | `this` after insert |
| `0x00284E70` | 1 | interior character pointer through out parameter |
| `0x00284EE0` | 6 | `this` after erase |

The 230-caller union exactly reproduces the per-owner counts and caller-set
hashes frozen in v9.  No raw E9 or non-`.reloc` exact-VA owner occurrence was
found.  This does not close image-base+RVA, split-tail, indirect, return, stack,
or out-parameter liveout propagation, so the transfer fixed point remains
explicitly false.

## First gaps

| First gap class | Count |
|---|---:|
| Move/adoption plus incoming identity/capacity provenance open | 2 |
| Borrowed/external pointer plus SSO-union/capacity provenance open | 10 |
| Incoming object identity plus SSO-union/capacity provenance open | 2 |

The first two rows can adopt an incoming heap buffer before copying.  The ten
replace/append/insert rows accept a borrowed pointer or an overlap-adjusted
alias.  The final two erase rows return either an interior pointer or the
mutated object.  In each case a legal local path can be paired with an
unclassified incoming heap pointer covering manager slot
`[0x00C71678,0x00C7167C)`.  Thus local arithmetic does not establish manager-slot
non-overlap.

## Frozen inputs

- string-extent v9 analyzer:
  `601349A3B79E90C1E2CDF09D23F7BFA26B44A213CB9301239EA0A3D7A14EB583`
- string-extent v9 JSON:
  `DA2986BD521AA6967B62DDF99B27725C9AF9C9277BB902C45D2BEF44B4F228D5`
- string-extent v9 handoff:
  `59C15C48889025DA723279288E8B629DEAA406F2361942D5A2DE4953973C9177`
- bulk-v26 builder / JSON / handoff:
  `7E74CE0C92DBB253059CF4560FEDF29EF2C25975F38E81CF96B0236E62EBFE07`,
  `514399E365C79126E9256993549B957BF93800BDF8C1A788CEE588E3FE0328EF`,
  `DF537CDFFE9889BE11556D13AB24493A52B80376A19240945F62763604CCD4C5`

Legacy rejected evidence was not opened by this reproducer.

## Reproduction

Create-new was followed by two consecutive byte-identical verify-existing
replays:

```powershell
python out\cs-t0\analyze-manager-this-copy-move-string-exact14-v29.py
python out\cs-t0\analyze-manager-this-copy-move-string-exact14-v29.py --verify-existing
python out\cs-t0\analyze-manager-this-copy-move-string-exact14-v29.py --verify-existing
```

All 100 semantic mutations were rejected: 84 per-row status/call/no-wrap/gap/
provenance/non-overlap mutations, six owner-universe mutations, and ten global
status/ledger/permission mutations.  External analysis is recorded only as
`TIMEOUT_NO_PASS` and was not used for the verdict.

## Artifact hashes

- Analyzer:
  `B99894F74CD9D88CEC110D32BCAC4D3E79D1B2B5EFBCFA0ED0924A1CEF5E73F1`
- Machine JSON:
  `F9A85F3022E659250A7A386AD367B0CD400599C1ECBC14DB93FCC4DBF8CE0790`
- Frozen PE:
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

No game, process, debugger, runtime mutation, IDB write, production/test edit,
or status/index edit was used.  All permissions and promotions remain false.
Final acceptance must be performed by a separately isolated tester.
