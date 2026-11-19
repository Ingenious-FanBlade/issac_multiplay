# RE-P4：Render 与相机读取的位置字段

J273 静态结果。

## 1. 实体 Render

- 基类 Render `0x6AC4E0`：屏幕坐标 = floor 取整的 WorldToScreen(pos + posOffset) + offset。WorldToScreen 为 ×0.65 并加 −60／−140 偏移。结果写入屏幕缓存 `+0x1A4/+0x1A8`（RE-5 write-set 已记录）。offset 参数 = 震屏偏移 + Room `+0x1204` 渲染滚动（REPENTOGON `LuaIsaac.cpp:227` 同样计算）。
- Player::Render `0x784A60`（vtable slot 5）：用 pos、posOffset × 0.65 和 offset。
- 两者都不读历史位置 `+0x344`。

## 2. 原生插值

- 非玩家的奇帧 Interpolate 直接改 pos，并在下一个偶帧更新前从 `+0x344` 恢复（见 [movement.md 第 5 节](movement.md#5-速度单位与原生插值)）。所以 Render 只认 pos；覆盖层写 pos 与原生插值做法一致。
- REPENTOGON `Patches/InterpolV2.cpp` 只在 `AnimationLayer::RenderFrame` 中按 Manager 帧奇偶对 ANM2 关键帧（位置、缩放、旋转、颜色）插值，默认关闭，不涉及实体世界坐标。与覆盖层无关。

## 3. 相机

- UpdateUltrasmooth `0x91D840`（`thiscall(Camera*, bool halfFrame)`）：读玩家 pos `+0x33C` 和 velocity `+0x360`，单人时按 velocity × 4.5 前视。
- UpdateDrag2 `0x91E5E0`：经 PlayerManager 中心 `0x995B30(Game+0x1BAA8, &out, 1)` 取焦点，使用 Camera `+0x40` 起的环形缓冲，并调用 Random。

结论：

1. 覆盖层首版写 pos 即可让 Render 正确；posOffset 不需要覆盖（它是跳跃等竖直偏移，随权威值安装）。
2. 单人前视读 velocity：本机身体预测时，覆盖层宜同时写预测速度，否则相机前视按权威速度，转向时相机与身体不同步。多人时走哪条相机分支、是否读 velocity，未逐分支核实（README 未知项）。
3. UpdateDrag2 调用 Random：客机相机阶段已按显示委托运行，覆盖层不改变调用与否。
