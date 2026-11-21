> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02/R03/R05/R09/R10 static-successor handoff v1

## Scope and authority

- Work package: `CS-T0-05-04` / manager-this.
- Frozen target: `isaac-ng.exe`, PE32/I386, image base `0x00400000`, image size `0x008FE000`, SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- REVERSE-only static analysis used IDA operations `survey_binary`, `analyze_batch`, `lookup_funcs`, `insn_query`, `get_bytes`, `disasm`, and `server_health`.
- No game start, process open/read, debugger attach, runtime probe, or IDB change occurred.
- MT-R04 was intentionally skipped under the current user constraint. No R04 successor or test was created.
- No status/index/frontier/v34 file was edited by this work.

## Append-only successors

| Task | Builder | Candidate | SHA-256 | Verdict |
|---|---|---|---|---|
| MT-R02 | `out/cs-t0/build-manager-this-mt-r02-old-new-count-v2.py` | `out/cs-t0/native-join-manager-this-mt-r02-old-new-count-v2.json` | `3A6872DB6AB1E7CCC0A61ED752A68DCCF23A85C2FEEE96D7EAA67ED20EE3E0AB` | both rows `UNKNOWN` |
| MT-R03 | `out/cs-t0/build-manager-this-mt-r03-fixed5000-v3.py` | `out/cs-t0/native-join-manager-this-mt-r03-fixed5000-v3.json` | `961B2C8B444778BFBDEF91167DAC5A30870FC31C4E9319C30EF8CA8F4808E355` | both rows `UNKNOWN` |
| MT-R05 | `out/cs-t0/build-manager-this-mt-r05-filtered-vector-v2.py` | `out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v2.json` | `509D89D51F3983FB98FB2FE0A50A7B5157658972AAC035F94BC288C78D6898EB` | row `UNKNOWN` |
| MT-R09 | `out/cs-t0/build-manager-this-mt-r09-entity-list-v4.py` | `out/cs-t0/native-join-manager-this-mt-r09-entity-list-v4.json` | `720990B7534EA623803ABEE95DDE06CB2C80697C6569779723D6A58BE8FE1F98` | both rows `UNKNOWN` |
| MT-R10 | `out/cs-t0/build-manager-this-mt-r10-local-tuple-v2.py` | `out/cs-t0/native-join-manager-this-mt-r10-local-tuple-v2.json` | `DC2129494CC781DD26FD0F8D730BD43423591593B3C79DA3B3F756FDCBC5B260` | row `UNKNOWN` |

## New evidence and minimum open obligations

- MT-R02 `0x00008A62`, `0x00008B2A`: concrete `push 0` callers at `0x0003ABD6` and `0x000E54C2` select the direct `sub_408970` path. Allocation request multiplication uses `mul` plus `seto`, but the subsequent copy is still `4*oldCount` with no `oldCount<=newCount` comparison or clamp. Minimum open countermodel remains reachable `oldCount=2,newCount=1`; only the incoming pair/backing-span producer fixed point can close it.
- MT-R03 `0x0001513A`, `0x000155A4`: both `5000` branches log and rejoin. All three direct consumers converge inside `sub_40ED50` (`0x00010DDC`, `0x0001300C`, `0x00011DE6`). The open universe is therefore narrowed to the parser stack-object count producers; `5001` elements is the minimum open case.
- MT-R05 `0x0033570B`: the filtered tuple is locally initialized empty and can remain empty; the target block independently requires a nonempty auxiliary range. The precise unresolved conjunction is `auxiliaryRangeCount>0 && filteredCount==0`. No filtered-nonempty guard dominates the memmove. Static predecessor/path feasibility remains open.
- MT-R09 `0x0001B072`: the row-0 source root is `(this+0x94,this+0x9C)`. `0x0001B3E3`: the row-1 source root is `(this+0x44,this+0x4C)`. On both paths the `0x8000` overflow diagnostic continues to write/increment scratch count and then copy it. Each minimum open source count is `0x8001`; the two source-root producer/lifetime closures remain distinct and open.
- MT-R10 `0x002EE04F`: 22 direct `+0x5C0` writers are partitioned without overlap into 15 constant-safe writes, 4 `count<8` guarded appends, and only 3 value-propagating writes (`0x002E19E2`, `0x002EE0E8`, `0x003B1400`). Thus 19/22 direct writers are locally bounded `<=8`; the three value writers plus 28 LEA-alias entry paths remain the minimum universal gap. The copy routine itself only rejects zero and has no `n4<=9` guard.

These are v35-style stop points: further positive claims require producer/lifecycle fixed points rather than more bounded local-callee expansion. The UNKNOWN verdicts are retained; no row is eligible for frontier composition.

## Compressed replay

Each successor was run once in create-new mode and exactly once with `--verify-existing`. Each wrapper ran only three core mutations: current-PE atom tamper, illegal successor promotion/partition tamper, and inherited-row promotion. Every wrapper reported `3/3` rejected. No Cargo build, full build, full audit, or repeated self-test was run.

Commands:

```text
python out/cs-t0/build-manager-this-mt-r02-old-new-count-v2.py
python out/cs-t0/build-manager-this-mt-r03-fixed5000-v3.py
python out/cs-t0/build-manager-this-mt-r05-filtered-vector-v2.py
python out/cs-t0/build-manager-this-mt-r09-entity-list-v4.py
python out/cs-t0/build-manager-this-mt-r10-local-tuple-v2.py
python out/cs-t0/build-manager-this-mt-r02-old-new-count-v2.py --verify-existing
python out/cs-t0/build-manager-this-mt-r03-fixed5000-v3.py --verify-existing
python out/cs-t0/build-manager-this-mt-r05-filtered-vector-v2.py --verify-existing
python out/cs-t0/build-manager-this-mt-r09-entity-list-v4.py --verify-existing
python out/cs-t0/build-manager-this-mt-r10-local-tuple-v2.py --verify-existing
```

No dynamic scene is requested for this handoff. If a future existence-only R05 probe is authorized, it must produce a `sub_734F30` invocation with a nonempty auxiliary range while every candidate is rejected by the filtered-vector predicates; a sampled scene still cannot prove the universal positive bound.
