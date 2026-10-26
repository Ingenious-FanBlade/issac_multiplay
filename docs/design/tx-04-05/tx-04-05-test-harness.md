# tx-04-05 Lua 夹具与 Rust harness

日期：2026-10-24。对应[实现方案](tx-04-05-implementation-plan.md)第 11、12 节。本次完成剩余夹具、S0–S11 单启动编排与 V1–V10 证据归约；Rust i686 Release 与 Win32 DLL 均纯编译链接成功。没有部署、启动游戏、运行 harness 或执行测试；唯一整体 E2E 尚未执行。纯编译记录见 [artifact](../../artifacts/repentogon/tx-04-05-harness/README.md)。

## 代码与行为

| 文件 | 职责 |
| --- | --- |
| `tests/e2e-mods/isaac-cs-test/main.lua` | 保留旧命令桥观测；tx-04-05 配置才启用 MC_PRE_UPDATE 跳帧与游戏侧观测 |
| `tests/e2e-mods/isaac-cs-test/harness_config.lua` | 默认 tx-04-04；部署时仅在临时夹具目录写入 tx 与本次 token |
| `tools/harness/src/tx05.rs` | 环境检查、S0–S11 编排、实时等待、失败现场与单启动收尾 |
| `tools/harness/src/tx05/evidence.rs` | 原始 trace/Lua 解析、V1–V10 判定、D18 历史基线的严格字节比较 |
| `tools/harness/scenarios/tx-04-05.json` | 唯一整体 E2E 的菜单序列、控制口与历史基线来源 |
| `tools/harness/src/win32.rs` | 新增 D/W/A/S/E/F8/F9 和多键 held；RAII 在错误或 panic 时释放本次按下的键 |

`harness e2e tx-04-04` 保留原两次启动入口；新入口为 `harness e2e tx-04-05`，只启动一次。两者默认使用各自场景 JSON。不存在独立的 core 自测、负向批次、离线回放测试入口或单元测试。

D20（第一次 E2E 之后）：启动等待期间的 focus 只尽力激活并记录，失败不致命；发键与截图前仍必须成功。D21（第二次 E2E 之后）：S0 结束前须观测到两个不同的正 `game_frame`；切房改由第二次 F8 触发，F9 是原版手动截图热键，不再使用。D22（第三次 E2E 之后）：S10 沿 S9.move 的反方向移动。

S0 只读检查 SkipIntro、Steam、目标 PE 时间戳、已运行游戏、启用 mod 与两个 inputconfigs 文件。默认键位不满足、F8/F9 被映射为动作或存在第三方启用 mod 时，在部署/启动前结束。记录原始键位、环境与两个存档来源；不写用户选项。选档必须观察到第二档，New Run 菜单必须观察到选择 0。进入 run 后确认 Isaac、keyboard controller 0 和至少一枚起始炸弹，不修改玩家类型或资源。

S1 在未激活时按 D 并反向回到房间内；S2 发出 solo start；S3 验证右移/上移及松键；S4 按右、加按左、释放左、释放右；S5 只轻按一次 E，然后移动离开炸弹。S6 打开 REPENTOGON 控制台浮层，期间按 D/E，不发送 ENTER；S7 暂停并跨恢复保持 D；S8 按 F8；S9 按 F9 并向新房间内部移动；S10 stop 后观察原生移动；S11 重新启动 solo，退出菜单并继续观察静止统计。

等待 Authority 进度使用 completed tick，并检查 generation 不变、子进程存活与 runtime 错误。1 秒/0.5 秒的按键保持来自方案规定；截止时间用于识别卡死，不把墙钟当作 tick。控制请求、变更和物理输入失败均不重发；未到达场景保留 NOT_RUN。清理只处理本次部署的测试 mod、自有子进程与自有 held 键，关闭配置，不恢复存档。

## Lua 夹具

- F8 从 render 收到请求后，让 MC_PRE_UPDATE 恰好 30 次返回 true；第 31 次允许更新前记录结束。起止记录可与 Manager 快照轮次对齐。OddPlayer 数量按实际 trace 记录。
- 第二次 F8（D21，原为 F9）按 DoorSlot 0–7 选第一扇已开启、未锁定、目标为普通相邻房间的实际门。调用全参数 `StartRoomTransition(target, direction, 0, nil, -1)`；日志包含请求参数、来源身份和实际到达身份。无候选、原生改写目标、超时或 Lua 异常都不能通过 S9；不修改门、房间或 Player。API 签名参考 [Game 文档](https://wofsauge.github.io/IsaacDocs/rep/Game.html#startroomtransition)，门字段见 [GridEntityDoor](https://wofsauge.github.io/IsaacDocs/rep/GridEntityDoor.html)。
- 在 MC_POST_RENDER 读取位置、炸弹数、泪弹/炸弹实体数、controller、角色与房间身份；变化时记录，静止时每 30 render 留一份心跳。每条结构化日志带部署 token、递增 serial 和命令桥返回的 Manager 快照轮次。不会把到房日志当作 T6 最终位置时序的证明。
- 实时读取忽略尚未写完的最后一行；进程退出后的证据归约严格解析全部记录。缺 token、缺轮次、序号缺失、JSON 损坏和 fixture-error 均不能通过。

## 为验收补充的只读诊断

原 game 适配缺少“正常提交”与炸弹重复查询的直接证据，不能仅凭 published 事件推断。因而补充以下诊断，不改变 core 判定、Hook 清单、输入队列或原生调用策略：

- `tick_phase` 分别在成功 Authority-pre 与 NativeReturned 后记录；tick 汇总增加 authority_pre/native_commit、实际 owner 和 Resolve 的 input_sequence。
- submit 增加 loop、session_generation 和 canonical 状态，便于将 sample → accepted sequence/target → Resolve → consumer → Lua 位置串联。
- 普通 Odd shim 的 enter/leave token 与外层 Player 入口次数配对；特殊 Odd Player 单独记录，无普通标记时不能出现新 tick。非普通 Room 记录实际 owner/token 和未具备入口资格的事实。
- Shooting 的每次分派记录是否调用 original 及当时 tick/blocks，不使用 wrapper completed 数冒充原生调用数。
- Bomb 的 action 8 / Triggered 记录本 tick 首值、读次数与不一致次数；允许同 tick 多次幂等查询，要求不一致次数为零。

V1–V10 缺必要证据时不通过。未实际经过的特殊 Odd 路径数量按零记录，不声称已运行覆盖该原生分支。V9 可以观察持续 Loading 区间或 animation=0 的非普通 Room 调用；没有任一种实际观测时不通过加载判定。

## 存档与产物

单次启动不会产生新的关闭配置对照。D18 的历史样本默认取 `artifacts/repentogon/tx-04-04-e2e-08`，要求记录的目标版本、存档来源与关闭配置观察相符。原规则要求本次前后字节分别完全等于那次前后字节；第一次 E2E 证实每次启动都会改写末尾计数与校验，这条规则不可能匹配。D20 改为：只有第一/三档某文件在历史关闭启动中确实变化，本次与历史的长度都不变，且本次变化字节全部落在末尾 Counter/Checksum 两个 u32 字段内、属于历史改动过的字段，才按基线放行；路径相同或历史 PASS 本身不构成放行依据。历史快照不在时保持严格判定，新增或无法匹配的变化判失败，并报告来源 UNKNOWN。不计算哈希，不为取得新对照额外启动游戏。

一次执行生成 `scenario.json`、环境/部署清单、原始键位、源码快照、`steps.jsonl`、`control.jsonl`、`checkpoints.json`、`lua-observations.json`、`tick-evidence.json`、原始 trace 与游戏日志、关键 BMP、存档前/中/后快照和变化账本，以及 `report.json/md`。失败后先记录现场，再释放子进程；最终报告仅列出实际生成的产物。不自动重跑。

未来执行入口如下；此文档中的命令没有在本次运行：

```powershell
out/repentogon/harness.exe e2e tx-04-05 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-05-e2e-01
```

实机前仍须告知用户：harness 将接管键盘数分钟，SkipIntro 已开启，无需手动跳动画，从启动前开始停止手动操作。若实际需要手动配合，先确认准备好再启动。当前交付是代码与纯构建，不构成 V1–V10 通过或 tx-04-05 实机完成。
