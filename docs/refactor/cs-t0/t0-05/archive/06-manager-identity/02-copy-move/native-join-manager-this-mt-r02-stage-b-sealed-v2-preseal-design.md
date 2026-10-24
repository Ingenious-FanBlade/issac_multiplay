> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02 Stage B sealed acceptance v2 pre-seal design

日期：2026-09-06  
角色：`TEST`  
状态：`PREPARED / WAITING_FOR_FROZEN_VANILLA_ASSET_MANIFEST`

## 边界

本设计只准备 candidate 前的 v2 验收契约。当前尚未收到 frozen vanilla asset manifest
的公开路径和物理 SHA-256，因此没有创建或封印最终 spec/preparation，也没有读取、定位、
import 或猜测未来 `asset-census-state4-graph-v1` candidate/builder。

固定范围只有 MT-R02 exact rows `0x00008A62`、`0x00008B2A`，目标 PE SHA-256 为
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`，量词为
`ALL_REACHABLE_EXECUTIONS`。资产域必须是 manifest 冻结的原版资产全集，Mods 明确排除。

## v2 最小契约

最终 spec 将物理绑定 manifest，并要求 candidate 同时提供：

- parser proof atoms：`LayerAnimations -> descriptor+0x1C`、
  `NullAnimations -> descriptor+0x24`；
- selector universe：primary `1048`、secondary `91`，只允许以 `flag != 0` 剪枝，
  candidate 必须分别报告原始、剪除和保留计数；
- `state+4` writer fixed point：`sub_408830`、`sub_408970`、`sub_4086E0` 以及
  `copyMove`、`reload`、`addressTaken` producer class 全覆盖；
- 两个 descriptor field 的 asset maximum 与 reachable old/new graph edge 完整 join；
- `0x00008B6A: 89 5E 08` adoption 纳入第二 backing 的 lifetime/identity 与 graph；
- 两行逐行按 `backingExtent -> countAndNoWrap -> sourceLifetimeAndIdentity ->
  targetIntervals` 裁决。

positive 只有在 deterministic census 与候选逐字一致、exact row/target/manifest/graph
coverage 完整、四项义务全闭合、旧 countermodel 全消解、reachable universe、state+4
fixed point、asset-max/edge join 与 `0x8B6A` adoption 均闭合且 `unresolved=[]` 时成立。
否则必须为 `UNKNOWN`，并保留按固定顺序遇到的第一项开放义务；若仍是 v1 的
`backingExtent` 首缺口，其 gap ID 与 `oldCount=2,newCount=1` countermodel 必须逐字保留。

## 压缩验收

candidate 交付后的独立 audit 只允许：

1. 一次 deterministic census 复算；
2. exact row、target/spec/manifest hash 与 required risk-edge graph coverage；
3. 三类决策性负向 mutation：target/binding hash drift、缺失 required risk edge、
   selector/census count 篡改。

不运行 Cargo、全量 T0/build、历史回归、游戏、进程或 x32dbg。audit 必须 create-new；
不修改 candidate、sealed spec/preparation、frontier、README/status、既有证据或生产代码。

## 已准备文件与等待点

已准备但尚未执行：

- `out/cs-t0/prepare-manager-this-mt-r02-stage-b-sealed-v2.py`。

manifest 到达后，该工具只接受 manifest 路径/SHA，先重验 manifest、冻结 PE、exact2 v1
spec/preparation/audit 与 v41 checkpoint 的物理 hash，再 create-new 生成：

- `out/cs-t0/native-join-manager-this-mt-r02-stage-b-sealed-acceptance-spec-v2.json`；
- `out/cs-t0/native-join-manager-this-mt-r02-stage-b-sealed-preparation-v2.json`。

当前等待唯一输入：新 frozen vanilla asset manifest 的公开 workspace 路径和 uppercase
SHA-256。在此之前不得运行生成器，也不得创建最终 spec/preparation 或 candidate audit。
