# RE-P5：附着在玩家上的对象

J273 静态结果。设计 5.6 的 Δ 传递按下表实施。

## 1. 跟班

- 所属玩家：EntityFamiliar `_player +0x410`。
- 环绕：OrbitLayer `+0xD70`（默认 −1，`AddToOrbit` 后为层号）、OrbitDistance `+0xD64`（Vector，椭圆半径）、OrbitSpeed `+0xD6C`、OrbitAngleOffset `+0xD60`；State `+0xD88`。偏移来自 exe 中 Lua 绑定的压栈指令。
- 跟随队列：`_isFollower +0xD74`。
- Render 只读 pos＋posOffset（见 [render-camera.md](render-camera.md)），覆盖层给 OrbitLayer ≥ 0 且 `_player` 为本机身体的跟班加 Δ 即可。跟随队列中的跟班沿路径追随，首版不加 Δ。

## 2. 激光

- 跟随父体：Host 更新 `0x94CA0E—0x94CA77`：parent `+0x3BC` 非空且 DisableFollowParent `+0x4E4` 为零时，pos = parent.pos + ParentOffset（`+0x410/+0x414`）；父体为玩家时再加玩家 `+0x1640/+0x1644`（疑似发射口偏移）。父体不是玩家（父体 `+0x28` type ≠ 1）且激光 `+0x34`（含义未核）为 1 时不加 ParentOffset。
- 绘制：Laser::Render `0x9514B0` 把“优化后点表”（`+0x480` 起，begin/end 在 `+0x480/+0x484`）当作绝对世界坐标使用，另用 pos 与 posOffset。
- 房间光照 Type7 读“未优化点表” `+0x48C`；EndPoint 在 `+0x4D8`。

所以附着激光的覆盖必须平移 pos、优化点表全部点、未优化点表全部点和 EndPoint，并在 Render 返回后逐项写回。点表是原生 vector 的堆内存，写回前要核对 begin/end 指针未变。

> **待核（2026-11-19 阶段 1 实施时发现）**：SDK `HomingLaser.zhl` 把 `HomingLaser` 放在 `Entity_Laser+0x464`，其中 `_origin +0x4`、`_endPoint +0xC`（即 Laser `+0x468`／`+0x470`）、`_samples +0x1C`（`+0x480`）、`_nonOptimizedSamples +0x28`（`+0x48C`）。点表偏移与本节一致，但 EndPoint 与本节的 `+0x4D8` 不一致，`_origin` 本节未列。阶段 1 因此不覆盖任何激光；阶段 2 实施 Δ 传递前先逆向确认 Laser::Render 实际读的端点字段。

> **阶段 2（2026-11-19）**：激光端点仍未逆向确认，阶段 2 照旧不覆盖任何激光（含附着在本机身体上的激光）。

## 3. 其他

| 对象 | 引用 | 处理 |
|---|---|---|
| 举起的实体 | Player heldEntity `+0x140C` | 加 Δ |
| 刀（Mom's Knife 等） | parent `+0x3BC` | 加 Δ |
| 激光命中特效 | isaac-lan 把带 parent 的 LASER_IMPACT 也视为附着 | 首版跟随激光一并加 Δ，待实测 |
| spawner `+0x3C8` | 发射者 | 不视为附着（与 isaac-lan 一致） |
