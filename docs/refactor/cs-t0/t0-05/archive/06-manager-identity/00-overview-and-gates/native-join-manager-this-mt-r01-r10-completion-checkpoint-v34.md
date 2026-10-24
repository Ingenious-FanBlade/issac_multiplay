> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this MT-R01–R10 completion checkpoint v34

本检查点记录按 v33 最新分解完成 `MT-R01`–`MT-R10` 静态正向逆向后的真实状态。任务完成表示 20 个精确 RVA 均已产生 REVERSE 候选与独立 TEST 逐行裁决；不表示十项全部正向闭合。

## 结论

- 独立审计通过并进入 positive frontier：`0x001CBEB5`、`0x00689F63`。
- 保持 `UNKNOWN`：其余 18 行。
- v32 账本：copy/move `321/405`，memset `225/242`，总计 `546/647`，remaining `101`。
- manager-this、whole-image raw v2 与 formal v7 均未推进，所有运行时与发布权限继续关闭。

## 十项逐任务裁决

| 任务 | 行数 | 正向 / UNKNOWN | 独立裁决与最小首缺口 |
| --- | ---: | ---: | --- |
| `MT-R01` | 1 | `1 / 0` | `0x001CBEB5` 的 save-record `[EBP-0x298]` liveout 与 167/167 writer/caller seed 闭合。REPENTOGON 仅用于外部候选搜索，正向结论只来自当前 PE。 |
| `MT-R02` | 2 | `0 / 2` | 两个 incoming pair 均未证明 `oldCount <= newCount`；保留 `new=1, old=2` 反例。 |
| `MT-R03` | 2 | `0 / 2` | 5000-element 限制仅记录日志；`emittedCount=5001` 超过两个固定 backing。最终候选为 v2。 |
| `MT-R04` | 2 | `0 / 2` | 177+5 个 destination producer 仍未给出硬 capacity 不变量，失败路径只日志后继续复制。 |
| `MT-R05` | 1 | `0 / 1` | 真实 filtered-vector 可为空；无 dominating nonempty guard，erase count 反例仍成立。 |
| `MT-R06` | 1 | `0 / 1` | double-buffer 基类 ctor/allocator/cleanup 已定位，但跨设备动态类型的 offset-collision/间接 writer fixed point 与 source tuple lifetime 未闭。 |
| `MT-R07` | 5 | `0 / 5` | RTTI-bound BufferedStream 三张 vtable 与构造链已定位；实际间接 callsite 到 `a2` producer/output span，以及 replacement writer universe 未闭。 |
| `MT-R08` | 3 | `1 / 2` | `0x00689F63` 由两个 caller 的 fresh allocation 与同一 Size 传递闭合；`0x00661577` 保留 `Size=0x10001`，`0x0068E3DB` 的 nested `+176/+180/+184` writer fixed point 未闭。 |
| `MT-R09` | 2 | `0 / 2` | 当前 PE 证明 `EntityList+0x70` 四字段 scratch tuple，但 source count `<=0x8000` 未证明；保留 `0x8001` 反例。 |
| `MT-R10` | 1 | `0 / 1` | 22 direct writer/28 LEA alias 已登记但未完成语义 fixed point；`n4=10` 产生 36 字节写入 32 字节目标。 |

逐行 machine verdict 与 18 行精确差集位于最终 completion JSON；所有 `UNKNOWN` RVA 仍存在于 v32 的 copy/move `remainingRvas`，两条正向 RVA 均已精确移除。

## 机器证据与独立验收

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/build-manager-this-mt-r01-r10-completion-v1.py` | `8D6D0E826238CFB1528E28BE0F56C478A0640040030A9279B0240545C246EF99` |
| `out/cs-t0/native-join-manager-this-mt-r01-r10-completion-v1.json` | `BF75125FD5B6D07FA5468EB29982B16CC8A0247EEF62B45F90E35BD6E5BBE0D1` |
| `out/cs-t0/audit-manager-this-mt-r01-r10-completion-independent-v1.py` | `5F458D208C901259A9E0CD618CC4F43FBA40DD3E76494F64186CA484DF47B992` |
| `out/cs-t0/native-join-manager-this-mt-r01-r10-completion-independent-audit-v1.json` | `22372544AFF2807E8CAB38594A886E5D5BD8C1D3E2112DC7225513C918FACAB0` |
| `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v32.json` | `A336FF06F05A11CFF97CE937BF0E8CEE030E498B3E7C401A811021EFB665EFF3` |
| `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v32-independent-audit-v1.json` | `CE5849CC3A37498D09AB00388E2BEFFB2AE5D967728C4F82E6938CCBEE8D04ED` |

最终组合独立 TEST 没有读取或 import 主线 completion builder。它从五个 Wave audit 与 v32/v32 audit 独立重构 10 个任务、20 行、两个 accepted RVA 和 18 行 `UNKNOWN` 差集；执行 `create-new + 1 verify-existing`、8/8 核心 mutation、wrong-hash fail-before-write。为响应测试规模压缩要求，没有运行重复 self-test、Cargo 或全量 T0 构建。

## 边界

本轮仅使用冻结 PE/IDA 静态读取；未启动游戏、未注入 DLL、未运行 REPENTOGON scanner、未修改 IDB 或生产代码。`D:/projects/REPENTOGON` 保持只读，且只有 R01/R09 的 `EXTERNAL_CANDIDATE_ONLY` 搜索使用了其固定提交与文件哈希。R28/R29、whole-image raw v2、formal v7 及 manager-this 最终状态不在本轮范围。
