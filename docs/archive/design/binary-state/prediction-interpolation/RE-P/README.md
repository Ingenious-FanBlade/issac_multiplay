# RE-P1—RE-P6：预测插值阶段 0 逆向与计时

日期：2026-11-19。状态：RE-P1—RE-P6 静态研究完成；端到端延迟计时已实现，产品 DLL 编译通过。未改运行行为和线格式，没有启动游戏或运行测试；运行读数并入第 15 步统一整体验收。

任务来源：[预测插值设计第 8、11、12 节](../../prediction-interpolation.md#8-逆向与核对任务)。地址取自当前 J273（v1.9.7.12）IDB，VA = RVA + 0x400000。字段名优先用 REPENTOGON SDK（`libzhl/functions/*.zhl`）和本地 Wiki；SDK 未命名的写偏移。没有使用 J460 偏移，也没有复制 isaac-lan 代码。

## 结果入口

| 任务 | 文件 | 一句话结论 |
|---|---|---|
| RE-P1 | [移动积分](movement.md) | ControlMovement `0x772540` 只写速度；位置在 Entity::Update `0x6A9CF0` 中按“先摩擦、再按旧速度积分”推进。玩家速度按半帧（1/60 秒）计，其他实体按整帧计 |
| RE-P2 | [可调用纯函数](collision.md) | 只有地形查询（`0x6B1C20`、`0x7E6AF0`、`0x8087F0` 等）是纯读；ControlMovement 和完整格子碰撞都有副作用，速度模型须 M-a 复刻 |
| RE-P3 | [预测资格字段](eligibility.md) | 控制开关、冷却、空中、跟随目标、暂停效果、多身体角色等字段及其行为 |
| RE-P4 | [Render 与相机读的字段](render-camera.md) | Render 只读 pos＋posOffset，不读历史位置 `+0x344`；单人相机还读 velocity。原生非玩家插值本身就是“奇帧写 pos、偶帧恢复” |
| RE-P5 | [附着对象](attachments.md) | 跟班平移 pos 即可；附着激光要同时平移点表和 EndPoint |
| RE-P6 | [时序对应与计时](timing.md) | Host 开插值时 Authority tick 约 60Hz，每个 tick 一个玩家半步，提前量 2 tick ≈ 33ms；Body 帧头只有 game_frame，两个半步同号，5.7 的每身体序号确有必要。计时字段见该文 |

## 关键结论

1. **速度模型须复刻（M-a）**。ControlMovement 内含头部／装扮动画、木星生成（消耗随机数）、火星冲刺；完整 PlayerCollideWithGrid（quiet=0）会破坏格子、开门、推石头、调用格子碰撞回调。M-b 只能用于地形查询。
2. **半步顺序**：Entity::Update（摩擦 → `pos += 旧速度 × 时间缩放`）→ ControlMovement（由输入算新速度）→ 格子碰撞。客机重放必须保持这个顺序，否则每步差一个速度。
3. **节拍**：Host 开插值时偶帧 StandardRoom、奇帧 OddPlayer 交替，每个输入命令只驱动一个玩家半步；Host 关插值时一个 StandardRoom tick 内玩家用同一输入走两个半步。客机预测必须知道 Host 处于哪种模式。
4. **覆盖层写 pos 与原生一致**：原生非玩家插值在奇帧把 `prevPos(+0x344)` 存下、直接改 pos，偶帧更新前再恢复。覆盖层“写 pos、Render 后写回”与原生机制相同。
5. **地形数据缺口**：`Room::GetGridCollision` 在格子没有 GridEntity 时改读 `Room+0x76C` 的 448 格寻路 cost（≥1000 视为阻挡、3000—3999 视为坑）。该表在布局中“留 Host”，客机没有，预测碰撞要么补同步这张表，要么另行判定。
6. **5.7 线格式确有必要**：Body 帧头只有 game_frame，同一游戏帧的两个半步同号；只有 source_revision 的先后能区分。Body 改动属 P3，不在阶段 0。

## 未知项（保留）

- Player `+0x1E54`（float，<0 时 ControlMovement 直接返回，疑似跳跃／空中计时）的含义。
- Player `+0x1FE0`（byte，非零时输入清零）的含义。
- Player `+0x2E82`（word，与 PAUSE 临时效果一起决定是否跳过控制块）的含义；与它同用的 `sub_425160` 的判定内容。
- `0x7E6AF0` 开头“`this[1]+16` 指向 type 35、variant 16 的实体时一律返回 0（无碰撞）”的场景未确认。
- “同一游戏帧两个半步的 Body 帧头 game_frame 相同”是读码推断，未实测。
- 多人时相机走 UpdateDrag2（PlayerManager 中心）还是 UpdateUltrasmooth、是否读本机身体的 velocity，未逐分支核实。
- Render 期间是否会删除或重建实体（覆盖层写回安全性），未核实。
