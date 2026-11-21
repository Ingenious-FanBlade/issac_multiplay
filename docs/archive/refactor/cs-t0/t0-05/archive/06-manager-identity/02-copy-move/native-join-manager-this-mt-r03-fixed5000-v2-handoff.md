> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R03 fixed-5000 v2 handoff

本记录替代同目录 v1 handoff 作为 `MT-R03` 的最终 REVERSE 候选入口；v1 JSON 与 v1 handoff 仅保留为被拒绝的序列化历史，不参与晋升。

## 逐行结论

| RVA | 结论 | 最小首缺口 |
| --- | --- | --- |
| `0x0001513A` | `UNKNOWN` | 全部 parser/interpolation 输入域未证明 `emittedCount <= 5000`；`emittedCount=5001` 时需求 `420084` 字节，超过固定 `420000` 字节 backing。 |
| `0x000155A4` | `UNKNOWN` | 同一数量边界未闭；`emittedCount=5001` 时需求 `540108` 字节，超过固定 `540000` 字节 backing。 |

相关检查只记录日志并继续执行，不能作为硬边界。两行均为 `fullyAdjudicatedNoOverlap=false`，没有进入 positive frontier。

## 冻结产物

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/build-manager-this-mt-r03-fixed5000-v2.py` | `5962EE57A6BB26D9DA6D68C5AAB40749F2D3B7066C38213735D5D131551B2E6F` |
| `out/cs-t0/native-join-manager-this-mt-r03-fixed5000-v2.json` | `A8DA2B59E5D9D26D22DAC734A6E791A6D4E745D76857DF68562D563A04202B4F` |
| `out/cs-t0/native-join-manager-this-mt-wave-b-independent-audit-v1.json` | `8DB1A94C61B9798012CD73E596A07EEA897103D93A5FEC6204FCE989B55A44FB` |

候选执行 `create-new + 1 verify-existing`，字节一致，6/6 作者 mutation 被拒绝。Wave B TEST 独立重算后仍给出两行 `UNKNOWN`；未创建空 successor。没有启动游戏、读取目标进程、注入、修改 IDB、生产代码或历史证据。
