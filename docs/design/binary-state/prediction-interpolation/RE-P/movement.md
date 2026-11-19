# RE-P1：玩家移动积分

J273 静态结果。地址为 VA（RVA = VA − 0x400000）。

## 1. 调用位置

| 函数 | VA | 说明 |
|---|---|---|
| Player::Update | `0x77BD40`（RVA `0x37BD40`） | Player vtable slot 3 |
| Entity::Update | `0x6A9CF0` | Player::Update 在 `0x77D05D` 调用；两个半步都调用 |
| ControlMovement | `0x772540`，`__fastcall(Player*, a2)` | 只有 Player::Update `0x77DDFF` 一个调用点 |
| GetMovementInput | `0x771D70` | ControlMovement 内读取输入 |
| PlayerCollideWithGrid | `0x6A6A40(Player*, bool quiet)` | 见 [collision.md](collision.md) |

每个玩家半步的顺序：

1. **Entity::Update**：处理击退 FLAG_KNOCKED_BACK（1<<47）；取摩擦 f=`+0x36C`（Lua `Friction`），时间缩放 ts=`+0x39C` 不为 1 时 f = f/(f + ts − f·ts)；`velocity *= f`；`pos += velocity × ts`；再把摩擦从基础值 `+0x368` 复位。半步 B（带 FLAG_INTERPOLATION_UPDATE 0x4000）在此之后跳过其余逻辑。
2. **ControlMovement**：由输入算出新速度，不改 pos。
3. **格子碰撞**：半步 A 在 EntityList::Update 之后的碰撞阶段（`sub_415880`，`0x41501C` 调用）做实体间碰撞和格子碰撞；半步 B 在 Odd helper 中直接调 `PlayerCollideWithGrid(p, 0)`。

所以位置推进用的是上一步结束时的速度；新输入要到下一步才影响位置。

## 2. ControlMovement 流程

1. **提前返回**：`float +0x1E54 < 0` 或 posOffset.y（`+0x350`）< 0（空中／跳跃）时直接返回。`+0x3C4`（target）非空时 velocity = target.pos − pos 后返回。
2. 记录上一步速度到 `+0x1654/+0x1658`。
3. **取输入**：仅当 ControlsEnabled（byte `+0x410`）为真、ControlsCooldown（int `+0x414`）≤0 时调用 GetMovementInput；输入向量长度钳到 1。FLAG_APPLY_GRAVITY（1<<15）且不能飞时输入 y=0。
4. **输入清零**：类型 35（Tainted Forgotten 本体）且非联机幽灵（`+0x207D`）；suplexState `+0x1F08` ≠ 0；byte `+0x1FE0` 非零。
5. **逆战车**（NullItem 63，`sub_90A520(+0x1500, 63)` 判定）：每 5 次更新给一次冲量，`+0x1EB4` 为计数。
6. **小马冲刺**：ponyCharge `+0x1874` > 0 时输入以 0.6 向 `+0x1638` 插值。当前输入存 `+0x1638`，最近的非零输入存 `+0x1648`。
7. **缩放输入**：in = 输入 × (MoveSpeed × 0.25 + 0.75)，MoveSpeed 即 `+0x155C`；由它设置 moveDirection `+0x1618`。suplex 状态 1 另走 `sub_6A7E80`。
8. **滑行系数** s = 滑行计数 `+0x1650` / 60。FLAG_SLIPPERY_PHYSICS（1<<27）时计数置 60，否则每次更新减 1；SEED_ICE_PHYSICS（52）会设这个标志。FLAG_SLOW（1<<7）强制 s=0。
9. **垂直阻尼**：dot = dot(vel, in̂)（in̂ 为 in 的方向）；vel = (vel − in̂·dot) × sqrt(0.2s + 0.8) + in̂·dot。
10. **加速**：a = (max(0, (0.7s − 0.2)·dot(vel, in)) / MoveSpeed + 3) × 0.4；vel += in × 0.5 × ((a×0.25 − a)×s + a)。s>0 且加速后速度超过 4.41 × MoveSpeed 时，按比例缩回到**加速前（阻尼后）的速度大小**，不是缩到上限（2026-11-19 阶段 2 补核）。第 9 步的 in̂ 实为缩放后的输入 in 本身（未归一化），dot 也用 in。
11. **摩擦**（2026-11-19 阶段 2 补核勘误：ControlMovement 不写 `+0x36C`，而是在本步末尾直接把速度乘以下列系数；`+0x36C` 只在 Entity::Update 中使用并复位为基础值）：
    - 默认 sqrt(0.185s + 0.775)；
    - 逆战车、suplex>1 或 TF 本体：sqrt(0.51s + 0.45)；
    - FLAG_THROWN（1<<58，高双字 0x4000000）：1.0；
    - 类型 35 且非幽灵：半步 B（0x4000）为 1.0，THROWN 为 0.97，否则 sqrt(0.51s + 0.45)；
    - APPLY_GRAVITY 且不能飞：y 方向不用摩擦，改为 vel.y += 0.225。
12. **SPIN_TO_WIN**（NullItem 50）：改速度并按碰撞法线（`+0x190`、`+0x194..+0x1A0`）反弹。

上面的常数是 IDA 中的浮点立即数；与 isaac-lan 的拟合常数（如 √0.775）一致。

## 3. GetMovementInput `0x771D70`

- 用 controllerIndex `+0x160C` 调 `Input::GetActionValue 0x92F7B0` 读动作 0—3。
- 返回零向量：（Esau 类型 20，或 parent `+0x3BC` 是玩家，或 Tainted Lazarus 29/38 中序号较低的一方）且控制开启且 DROP（动作 11，`IsActionPressed 0x92F610`）按住。
- `Game+0x25B8C ≥ 2` 且 FLAG_CONFUSION（1<<9）时取反。
- SEED_CONTROLS_REVERSED（12）取反；SEED_AXIS_ALIGNED_CONTROLS（76）吸附到轴（种子在 `Game+0x1BBD8`，`sub_4240B0` 判定）。
- 镜像世界：`Game+0x1830C == 1`、楼层类型 4/5、stage 2（或 stage 1 带诅咒位）时 x 取反。

客机预测取输入时可直接复用 Host 已在用的输入语义（同一份 InputState），再在本地按这些规则变换；或者调用原生 GetMovementInput（它只读 Input 与游戏状态，需在客机已安装的 Game 状态上核对种子与镜像字段已同步）。

## 4. 副作用（排除 M-b 调 ControlMovement）

- 头部和装扮动画（`sub_754030`）：Gamekid 93、Unicorn 77、Unicorn Stump 298、Taurus 299、Book of Shadows 58、Holy Mantle 313；半步 B 跳过。
- 木星（594）：`Isaac::Spawn 0x424340` 生成，用 RNG `sub_6EB4B0`。
- 火星（593）：冲刺。

## 5. 速度单位与原生插值

- **玩家**：每个游戏帧真实执行两个半步（A 在 Game::Update 内，B 在 Odd helper 或插值关闭时的 EntityList::Update 补调），velocity 单位是“每半步”（1/60 秒）。
- **非玩家**：velocity 单位是“每游戏帧”（1/30 秒）。奇帧 Odd helper 对非玩家调 Interpolate（基类 `0x6AC2A0`，vtable slot 4）：把 pos 存到 `+0x344`，pos += vel × friction × ts × 0.5，置 byte `+0x175`。Manager::Update 在偶帧 Game::Update（`0x6F6D30`）之前（`0x92E8B1—0x92E8FA`）把带 `+0x175` 的实体 pos 从 `+0x344` 恢复。

所以原生非玩家插值只影响显示，且直接写 pos。远端外推按此换算：非玩家每半帧位移为 vel × friction × ts × 0.5。

## 6. Odd helper 与插值开关

- Odd helper `0x6F9340`：对类型 1（玩家）置 0x4000、调 Update（vtable+0xC）、`PlayerCollideWithGrid(p, 0)`、清 0x4000，返回地址 `0x6F955C`；对其他实体调 Interpolate（vtable+0x10）。调用点 `0x92E921`，仅在 Manager `+0x2A1FB`（OptionsConfig `_enableInterpolation`）为真且为奇帧时执行。
- 开关来源：游戏原生隐藏设置。Manager 初始化 `sub_92BA20`（`0x92BD33`）调 `sub_8FE1C0`，从 `config.ini` 读 `EnableInterpolation`（缺省 1）写入 OptionsConfig `+0x83`（即 Manager `+0x2A1FB`）；同函数还读 EnableColorCorrection、Caustics、Shockwave、Lighting、Filter、Pixelation、Bloom、WaterSurface、ColorModifier。该字符串在 exe 中只有这一处引用，没有写回，也不在 `options.ini` 与选项菜单中。`config.ini` 的查找目录未核实；2026-11-19 本机游戏目录与两个存档目录都没有该文件，因此按缺省为开。
- EntityList::Update `0x413F00`：插值关闭时，主循环之后对每个玩家在同一帧内再调一次带 0x4000 的 Update 和 `PlayerCollideWithGrid(p, 0)`。
- 上述两种情况下，玩家半步 B 都执行 Entity::Update 的位置积分和 ControlMovement 的速度计算；已知 Entity::Update 在 0x4000 下跳过积分之后的逻辑，ControlMovement 跳过动画副作用。Player::Update 其余玩法分支在半步 B 中是否全部跳过，未逐项核实（不影响移动预测）。
