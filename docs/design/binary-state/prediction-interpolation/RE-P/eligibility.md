# RE-P3：预测资格字段

J273 静态结果。“同步状态”一列指当前 Player／Entity 布局是否已有该偏移的条目（是否在 demand 中被实际发送，实施时再核）。

## 1. 移动被跳过或清零的条件

| 条件 | 字段 | 原生行为 | 布局条目 | 预测处理建议 |
|---|---|---|---|---|
| 控制禁用 | ControlsEnabled byte `+0x410` | 输入为零，物理（摩擦、积分）照常 | 有（`unknown_410_41b`） | 输入当零继续预测物理 |
| 控制冷却 | ControlsCooldown int `+0x414` | >0 时输入为零 | 在 `unknown_410_41b` 范围内 | 同上 |
| 控制块整体跳过 | `sub_425160(...)` 且（临时效果 PAUSE 478 且 word `+0x2E82` 为零，或 freezeCountdown `+0x218`） | Player::Update 跳过移动、炸弹、主动道具、卡牌／药丸、射击、丢弃整块，速度清零 | `+0x1500` 临时效果有；`+0x2E82` 未查 | 不预测 |
| 空中／跳跃 | float `+0x1E54` < 0 或 posOffset.y `+0x350` < 0 | ControlMovement 直接返回 | `+0x1E54` 有 | 不预测 |
| 跟随目标 | target `+0x3C4` 非空 | velocity = target.pos − pos | Entity 引用有 | 不预测 |
| TF 本体 | 类型 35 且 isCoopGhost `+0x207D` 为零 | 输入为零；灵魂（类型 40）正常移动 | 有 | 本体不预测；灵魂若为本机控制体则按普通规则 |
| 抱摔 | suplexState `+0x1F08` ≠ 0 | 输入为零；状态>1 改摩擦 | 有 | 不预测 |
| 未知锁 | byte `+0x1FE0` | 输入为零 | 无 | 未知，实施时补核 |
| 横版房 | FLAG_APPLY_GRAVITY（1<<15）且不能飞 `+0x1564` | 输入 y=0，y 方向加重力 0.225 | 标志有 | 首版不预测 |
| 联机幽灵 | isCoopGhost `+0x207D` | 参与类型分支 | 有 | 按普通规则，碰撞放行坑 |

## 2. 多身体角色

- **Jacob & Esau**：两个身体读同一控制器（controllerIndex 相同）。按住 DROP 时 Esau（类型 20）或 parent 指向玩家的身体输入为零。预测时两个身体共用同一输入，DROP 状态从同一 InputState 取；首版按设计 5.5 不预测，待实测再开。
- **Tainted Lazarus**（29/38）：双子经 twinPlayer `+0x1E4C` 关联，序号较低的一方在 DROP 时输入为零。
- **Tainted Eve**（26）：只与双子比较，并重置 actionHoldDrop `+0x17B0`；不影响移动。
- **Tainted Forgotten**：见上表。

## 3. 由已同步会话状态判定的条件

- Host 暂停：已有 paused 位（`c.progress.authority_paused` 与 World 的终结分支）。
- 房间转场：RoomTransition 字段随 World 安装（显示委托 Q11）。
- 输入代次变化、会话恢复：客机输入链已有 generation 与 Neutral 处理。

## 4. 参数字段（预测每步从权威对象读）

MoveSpeed `+0x155C`、CanFly `+0x1564`、ponyCharge `+0x1874`、friction `+0x36C`／基础摩擦 `+0x368`、时间缩放 `+0x39C`、size `+0x370`、尺寸倍率 `+0x374/+0x378`、gridCollisionClass `+0x184`、采样点向量 `+0x178/+0x17C`、实体标志 `+0x168`。布局中均有条目。

以下 ControlMovement 内部状态没有布局条目，客机无法从权威值读到，对账重放时须从零或从本地预测状态起算，误差交给纠偏：滑行计数 `+0x1650`、上一步速度 `+0x1654`、逆战车计数 `+0x1EB4`、`+0x1638/+0x1648` 输入历史。冰面房间里滑行计数决定 s，重放起点若取 0 会有系统误差；实施时决定是否加同步（并入 P3 的 Body 改动）。
