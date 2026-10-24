> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CST0-05 Wave E MT-R07/R08 independent audit v1 handoff

## Result

The independent TEST audit accepted exactly one of the eight Wave E rows:
`MT-R08 0x00689F63`. The other seven rows remain fail-closed `UNKNOWN`.
This is a partial static result, not manager-this or whole-image promotion.

| Task | RVA | Independent verdict |
|---|---|---|
| MT-R07 | `0x00657E0A` | `UNKNOWN` |
| MT-R07 | `0x00657E94` | `UNKNOWN` |
| MT-R07 | `0x00658338` | `UNKNOWN` |
| MT-R07 | `0x006583C2` | `UNKNOWN` |
| MT-R07 | `0x00658487` | `UNKNOWN` |
| MT-R08 | `0x00661577` | `UNKNOWN` |
| MT-R08 | `0x00689F63` | `STATIC_CONFIRMED_NO_OVERLAP` |
| MT-R08 | `0x0068E3DB` | `UNKNOWN` |

The accepted delta is therefore one copy/move row. Relative to v31, the
recomputed ledger is copy/move `321/405`, memset `225/242`, total `546/647`,
remaining `101`. `frontierCompositionEligible=true`, but TEST created no
frontier successor and did not modify status/index files.

## Independent basis

TEST read only the sealed public envelope, the two candidate JSON files, the
frozen PE, and the public bound evidence. It did not read or import either
candidate builder/analyzer and did not use candidate classifications as truth.
The audit independently rehashed the PE/bindings/proof atoms, checked the exact
eight callsites and rel32 helper targets, reconstructed each row-local
writer/caller union, and re-evaluated the row-specific bounds and countermodels.

The accepted `0x00689F63` row has exactly two direct callers. Both callers bind
the output pointer and capacity to the same `Sizea` allocation/read span; the
non-wrapping size, owner lifetimes, backing extents, source/destination
disjointness, and GameRoot-slot non-alias conditions are closed. Every required
countermodel is resolved.

The retained first gaps are:

- R07 first four rows: type-erased BufferedStream vtable dispatch and indirect
  `a2` allocation/output-span fixed points remain open; requested-byte no-wrap,
  caller extent, disjointness, and GameRoot non-alias are therefore unproven.
- R07 `0x00658487`: constructor/replacement and begin/used/capacity writer
  fixed points remain open. The threshold virtual call does not establish the
  required used bound for every dynamic type.
- R08 `0x00661577`: the second static ring extent is `0x10000`, but no hard
  `Size <= 0x10000` bound closes all three callers. A logging-only power-of-two
  check is not a boundary; `Size=0x10001` remains a countermodel.
- R08 `0x0068E3DB`: the nested `+0xB0/+0xB4/+0xB8/+0xBC` constructor and
  external/address-taken writer fixed point remains open, as do
  `consumed <= used <= capacity` and shifted-length no-wrap.

## Rejection and replay checks

The compact gate rejected all 16 planned high-value mutations: exact-row
omission; PE call/helper drift; missing required binding; target drift; proof
atom byte/hash drift; row-local writer/caller-union drift; open selected
universe; open vtable dispatch; open indirect-a2 countermodel; open Buffered
begin/capacity writer; open second-ring hard bound; promotion from a logging-only
ring check; two-caller output-capacity drift; open nested writer; fake row
promotion; and enabled permission.

The audit JSON was produced with create-new and reproduced byte-identically by
one verify-existing run. A separately supplied wrong R07 candidate hash failed
before output creation (`exitCode=1`, probe absent before and after).

## Bindings and permissions

- R07 candidate: `out/cs-t0/native-join-manager-this-mt-r07-buffered-stream-v1.json`,
  SHA-256 `B32470D7F09B08B1F6A31B845905BF983F227AC55299A7E07A543E58C8FA1BD1`.
- R08 candidate: `out/cs-t0/native-join-manager-this-mt-r08-stream-output-realloc-v1.json`,
  SHA-256 `90932901F2723C64CE5348D3C964BC54A2CB494111CDA66379FBCA34802C31ED`.
- Public envelope: `out/cs-t0/native-join-manager-this-mt-wave-e-candidate-envelope-v1.md`,
  SHA-256 `D472A5ADB33E8C85152C5FCE40E03F8693F510675BCCBB3B5298A0692B876135`.
- Frozen PE SHA-256:
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.

No game process was started/opened/read, no debugger was attached, no IDB was
modified, and no candidate, production code, prior evidence, status, or index
was modified.

