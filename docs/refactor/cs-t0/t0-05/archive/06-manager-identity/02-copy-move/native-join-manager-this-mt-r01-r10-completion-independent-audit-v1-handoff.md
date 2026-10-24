> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 MT-R01–R10 completion independent audit v1 handoff

## Result

The compact independent completion audit passed. It reconstructed all ten
tasks and all twenty rows from the five hash-bound Wave audits, then checked
the v32 frontier and its independent audit. The TEST role did not read or
import `build-manager-this-mt-r01-r10-completion-v1.py`.

- Accepted: `0x001CBEB5`, `0x00689F63`.
- Fail-closed `UNKNOWN`: the other 18 rows.
- Ledger: copy/move `321/405`, memset `225/242`, total `546/647`, remaining
  `101`.
- Manager-this, whole-image, formal-v7, runtime-mutation, process, debugger,
  IDB, production and prior-evidence gates remain false.

The completion JSON exactly equals the independently reconstructed exact10 /
exact20 object, including task-to-candidate bindings derived from the public
Wave audits, per-row verdict order, classification, v32 binding and ledger.

## Bound public inputs

| Input | SHA-256 |
|---|---|
| Wave A audit | `3B85E54D25888F5DA260971BB3BED0194C2032DAA313A775BEA9430BD71E73AA` |
| Wave B audit | `8DB1A94C61B9798012CD73E596A07EEA897103D93A5FEC6204FCE989B55A44FB` |
| Wave C audit | `9B61B0E76C59E059D8208C2805B90E39BB2FFFFAB0B2E747326A9E846F5B816D` |
| Wave D audit | `EBDB4E41B97E7D6D5ED265882EBE3A768A8B136937A22F58DE9EFBF9BBFC0E23` |
| Wave E audit | `C351089C8CCAF56D3E26CF866D9FCCC692166759489F4E60B009EBC57376EE45` |
| v32 frontier | `A336FF06F05A11CFF97CE937BF0E8CEE030E498B3E7C401A811021EFB665EFF3` |
| v32 independent audit | `CE5849CC3A37498D09AB00388E2BEFFB2AE5D967728C4F82E6938CCBEE8D04ED` |
| completion JSON | `BF75125FD5B6D07FA5468EB29982B16CC8A0247EEF62B45F90E35BD6E5BBE0D1` |

All physical hashes were recomputed before adjudication.

## Compact rejection and replay checks

All eight planned mutations were rejected: Wave row omission, Wave fake
promotion, retained accepted row in v32, v32-audit ledger drift, completion
binding hash drift, task-row partition drift, classification/ledger drift, and
enabled terminal gate/runtime permission.

The audit used create-new followed by exactly one byte-identical
verify-existing. A separate wrong completion hash failed with exit code 1
before output creation; the probe was absent both before and after. No
self-test JSON, Cargo run, full T0 build, game process, debugger or IDB was
used.

