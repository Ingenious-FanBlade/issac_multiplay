# tx-01-62：全部既定 E2E 的记录核对

日期：2026-10-17。用户要求检查全部E2E是否完成。本次仅阅读现行规范、预先定义的E2E场景、控制器完整清单和既存实机报告；不运行游戏、构建、测试或原分析器，不校验哈希。

## 结论

**当前Virtual Device原定42项，加上新增B选角1项，共43项，均已有历史实机PASS记录；没有从未覆盖的条目。**此前七项缺观察中，Loading/LoadingExit/重复查询由55覆盖，死亡/复活由50观察，最后Pill消费/held10由61覆盖。

这是多轮累计的用例覆盖。**最新实现尚无同一实机窗口内43项全PASS的完整报告**：61的完整报告仍为33 PASS、10 NOT_OBSERVED；同次捕获的角色/Pill/held专项为32/32 PASS。专项含额外身份、资源和恢复核对，32与43不是同一检查集合。保留两个原报告，不把历史记录拼接成新的当前版本全PASS结论，也不因单窗口缺覆盖而要求新增测试或设置推进条件。

## 完整清单逐项映射

逐条名称、原始报告和事件序号见[coverage.json](../../../../../artifacts/cs-t0/tx-01-62-e2e-review/coverage.json)。它只登记既有观察，不是新测试结果。

| 取证来源 | 映射项数 | 覆盖内容 |
| --- | ---: | --- |
| [tx-01-61完整报告](../../../../../artifacts/cs-t0/tx-01-61-live-01/full-feature-result.json) | 33 | 单次注册/Join/选角、A/B身份和移动射击、输入拒绝/幂等/中性、Item/Pill、Drop consumer、Item/Pill held、pause/resume、menu、B退休/A继续、纯读、51Hook恢复 |
| [tx-01-48报告](../../../../../artifacts/cs-t0/tx-01-48-live-01/result.json) | 5 | 首个current无previous的显式edge、Bomb edge、Drop edge、Bomb consumer变化、Drop held |
| [tx-01-55完整报告](../../../../../artifacts/cs-t0/tx-01-55-live-01/full-feature-result.json) | 3 | Room、Loading、LoadingExit标签及来源；专项另验证真实Continue、恢复前中性、重绑后新输入 |
| [tx-01-50完整报告](../../../../../artifacts/cs-t0/tx-01-50-live-01/full-feature-result.json) | 2 | B死亡、复活；专项另观察死亡期A继续、复活唯一重绑和新输入恢复 |

最新61未在本轮观察的10项正是后三行5+3+2；它们在既有记录中都有实际PASS，因此不能把“该轮没执行”写成“从未执行”。同样不能把跨版本旧观察称为最新版本刚刚全套回归通过。

## 额外修复专项与历史失败

- 50总体仍为FAIL（19 PASS、1 FAIL），失败项为held下派生aim中性；死亡/复活项实际PASS保留，不把总体改判。其后用户报告的蓄力/换向问题分别经52方向专项和54跨房射击专项覆盖。
- [52](../../../../../artifacts/cs-t0/tx-01-52-live-01/result.json)方向专项36/36 PASS；[operator-result.json](../../../../../artifacts/cs-t0/tx-01-52-live-01/operator-result.json)仍为FAIL，因新raw脚本重复移动死区。该具体缺陷由[53](../../../../../artifacts/cs-t0/tx-01-53-live-01/result.json)修复并8/8 PASS，不覆盖52原失败。
- [54](../../../../../artifacts/cs-t0/tx-01-54-live-01/result.json)跨房射击与暂停恢复18/18 PASS；[55](../../../../../artifacts/cs-t0/tx-01-55-live-01/result.json)真实加载与三种查询重复读取20/20 PASS；[56](../../../../../artifacts/cs-t0/tx-01-56-live-01/result.json)异常Baby模式定向普通Join、A健康和51Hook恢复21/21 PASS；[61](../../../../../artifacts/cs-t0/tx-01-61-live-01/result.json)选角与最后Pill/held两项32/32 PASS。
- 61的held10为真实Pill consumer/action/exact binding上下文的三次controlled companion读取，值相同且完整Participant状态不变；不能表述为引擎主动发起Pressed10。
- 58构建成功但报告序列化FAIL、59报告修复PASS；60受限终端不可见而在注入前FAIL、61修复执行环境与PID/创建时间绑定后实机PASS。CLI/build结果不充当游戏E2E，失败原件保留。

## 范围与唯一静态审阅

本结论仅回答当前Virtual Device既定E2E清单的累计覆盖，不宣布整个T0-05架构任务、真实双端网络链路或T0-06至T0-10完成。所有角色/武器/加载变体、OOM/EH/worker并发闭包不在这份清单的已证范围，不能把它们悄然扩成当前必做项。

本次唯一最终验收为静态文档审阅：43个条目均附既存报告和事件引用；最新单轮与累计观察分开；失败与companion来源准确保留；没有游戏操作、测试重跑或新增推进门。
