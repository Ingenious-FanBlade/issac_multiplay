# 已知 Render 写集：本地范围与漂移校正

日期：2026-11-12。范围：方案 4.4 已知项定稿，原 RE-6 并入；其余交第 15 步 E28，不新增 Render 全图静态闭合任务。VA 基址 `0x400000`；字段偏移均为十六进制。结果为静态证据与实现交接，尚无运行时写集或漂移校正实现。

## 1. 已知写入与归属

| 写入方／J273 VA | 实际字段与范围 | 当前源及处理 |
| --- | --- | --- |
| Entity 基础绘制 `0x6AC4E0`，`0x6AC646／0x6AC64A` | Entity `+0x1A4/+0x1A8`，8B 屏幕坐标；按世界位置、PositionOffset、视口和相机参数计算，再交 `0x4059E0` 绘制 | `entity.json / Entity.screen_position_cache` 已为**本地**，无 wire；不采集、不写回 Host 屏幕坐标、不作漂移比较 |
| Tear::Render `0x675AC0`，`0x675C0B／0x675C0F` | 同一屏幕坐标 `+0x1A4/+0x1A8`；`0x675BE0 lea ebx,[esi+1A4h]`，随后写 `[ebx]／[ebx+4]` | 同上；不是 Tear Scale `+0x420/+0x424` 的写入证据 |
| Room::PreRender `0x803E20`，`0x804371／0x804387` | Room `+0x6EDC/+0x6EE0`，属于 LightGradient ANM2 `Room+0x6E40` 内的 `+0x9C` scale，8B | `room.json / Room.light_gradient_sprite` 为**派生**，没有整 Sprite wire；作为 Client 本地显示 scratch，不在同步块中另加两个浮点；必要资源覆盖和正常 Sprite owner 继续保留 |
| Room::PreRender，`0x8044CF` 检查、`0x8044DA` 清零，`0x8044E1→0x81BB30` | Room `+0x11F6`，1B 坑纹理 dirty；只在 dirty==1 时清零并重建 | `room.json / textured_pit_dirty` 为**派生**、无 wire；Client 在相关 Grid／纹理输入提交后置本地 dirty，Render 消费，不每帧恢复为 Host dirty。真实纹理／surface 生命周期不变 |
| Game::Render，`0x6F7AC6／0x6F7ACF／0x6F7AD4` | 本机 `Game+0x25AAC` 非零时调用 `0x703E40`，然后清请求字节 | `run.json / window_resize_requested` 与 `presentation.json / GameScreenEffects.resize_request` 已为**本地**、无 wire；不从 Host 安装，不阻止本机请求消费。`0x703E40` 的其他写集归 RE-9 |
| Game::Render，`0x6F7D38` | `Game+0x25AB8=1`，1B Render 反馈 | `presentation.json / GameScreenEffects.render_feedback` 已为**本地**、无 wire；保留原生写入，不纠正为 Host 值。`run.json` 的 `+0x25AB0..+0x25AB8` 是旧跨域未细分视图，不据此新增整段复制；计时两 i32 按 Q8，反馈字节仍本地 |
| Projectile::Render `0x659460`，`0x659484` 减 Height、`0x6597D1` 加回 | Entity `+0x350`，4B，公共 `position_offset` 的 y | **继续同步**，不能改成本地字段。原生以浮点减加恢复，可能出现逐位残差；进入绘制前保存原始值，正常返回后按相同对象身份精确恢复，后续合法读回仍覆盖该已同步块 |
| Tear::Render，`0x675CA7—0x675CD6` 保存，`0x675D4C→0x4029B0` 条件改色，`0x675D6F—0x675D89` 恢复 | Entity `+0xF0..+0x11B`，44B，`Entity._sprite(+0x48)._color(+0xA8)` | **继续同步主 Sprite 颜色**。正常路径完整保存／恢复 16+16+8+4B；保留为已知暂存及 E28 检查项，正常精确恢复的路径无需额外每帧重装整个 ANM2。它不是公共 Entity.Color `+0x1B8` |

上述“本地／派生”是现有布局类别：派生范围在本表中的职责是由 Client 原生显示生成。没有把所有同类型 ANM2 的 scale／color 一并改成本地，也没有把已同步 PositionOffset 裁掉。

**旧报告纠正与补充**：[tx-04-17 ST4](../../../tx-04-17/tx-04-17-render-presentation-boundary.md) 的 Tear 行写作 `+420/+424`，容易被理解成十六进制子类字段。当前伪代码明确是十进制 420／424，即 `0x1A4／0x1A8`；已用指令再次确认。其“暂时更改颜色”原先没有具体定位，本次核定在主 Sprite ColorMod `+0xF0`，不能外推为 `Entity.Color+0x1B8` 被写。旧原文保留并追加纠正与补充。

## 2. 官方补丁与本地缓存

- [InterpolV2.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/InterpolV2.cpp) 的 AnimationLayer::RenderFrame 保存共享 `AnimationFrame` 的 scale、pos、rotation、4 个 tint 和 3 个 offset，插值后调用 super，正常返回再恢复。`_interpol_state／_interpol_isgamerender` 也是本机状态。共享资源不能套用逐实体网络块 writer 来“恢复”；异常／递归不由正常返回源码保证，仍按资源 owner／在途寿命约束处理。
- [Anm2Extras.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/Anm2Extras.cpp) 的 RenderFrameEx 在局部 ColorMod 中改 shader progression，提交本地图形顶点，并恢复 graphics manager blendMode。不能把局部 `color._colorize[3]` 当成写回实体 Sprite 的证据。
- HUD::Render `0x97C343` 起重置每份 PlayerHUD crafting bounds；八份 stride `0x6E0`，首项 HUD `+0x6C0`。`presentation.json / PlayerHUD.crafting_bounds` 已为派生、无 wire。这是本地绘制包围框，不用 Host 每帧覆盖。
- 其他 HUD／Minimap／FX 的派生布局、图像、容器与随机值按显示委托退役／本地生成；这里只复用已知源，不继续逐个尾部 helper 闭合。Host 已选的规则／布局结果、Q8 当前标量与资源身份继续按需求清单保留。

## 3. 与版本 72 收缩的交接

本次源文件仅补证据／说明，不改变 wire 或版本。后续按需收缩统一处理以下边界：

| 范围 | 交接规则 |
| --- | --- |
| 本表无 wire 的本地／派生缓存 | 继续排除在采集、安装、影子相等与漂移纠正之外；保持本地构造、清理、缓存失效与资源 owner |
| Camera、Room scroll、Game 震屏偏移 | RE-1／RE-2 已交本地相机；旧版本 71 仍有 `Room.render_scroll`／`GameRun.screen_shake` wire。统一迁移时撤销旧发送／安装及相关读回，不允许相机写一次又被旧 Host 偏移纠正一次；震屏倒计时归 Q8，归零时清本地偏移 |
| HUD／Minimap／FX／光照／水面最终结果 | 按方案第 8—9 节退役结果布局与旧 writer；不将原生本地生产的结果列作漂移。输入状态、必要资源、Host 已选结果和 Q8 标量分开保留 |
| Projectile PositionOffset、主 Sprite ColorMod 等保留的真实输入 | 对应应用块继续有影子目标；已知暂存先在绘制作用域精确恢复，剩余差异才进入合法校正。不用复制整对象／调用 Update 纠偏 |
| Game／Room Q8 当前标量、Game 帧号与 Host 锚点 | 同目标安装，不本地二次推进；放行显示若改到保留的标量，才纳入 E28 和漂移校正。不能用 Render 后写回帧号掩盖玩法或生命周期已运行 |

## 4. 漂移校正的具体范围

1. **比较对象是实际仍应用的同步块**，而不是原生每个字节。版本／scope／generation／对象身份与资源贡献必须和成功提交的影子目标一致。本地、派生、已裁字段和未解释字节不作 Host 原值相等要求。
2. **已知临时写入优先作用域恢复**。Projectile 保存 4B 原始 y，不靠 `y−h+h` 作为精确恢复；Tear 正常路径已恢复 44B，但仍可在 E28 检查返回后的主 Sprite 颜色。Hook 若需要新增，只经统一清单常驻、按会话逻辑启停，不能临时 patch／卸载。
3. **恢复发生在原生树正常返回且身份仍稳定时**。不得在回调内部恢复、在半套 world 上绘制，或向已退休／复用／换房后的对象写入旧暂存。递归绘制需按真实嵌套保存；异常、身份变化或 owner 不确定时停用相关消费，按实际生命周期处理，不宣称任意故障可回滚。
4. **其余同步块轮转读回**，沿用 13.4 的合法读回／规范投影。Host 复制计划不能机械地用作 Client 根偏移读取；资源、引用、Synthetic 与条件投影走各自有证据的来源，不为检测运行 Getter／Update／生产器。
5. **发现差异按块和依赖恢复**。只在目标及 writer 仍有效时重写对应块；可能部分失败时沿用 13.3 的停止消费、Resync 和完整恢复，不继续发布混合目标。Render 后置恢复不推进成功安装版本或 ACK。

## 5. 第 15 步 E28 交接

- 对“显示推进前／后”和“原生 Game::Render 前／正常返回后”分别观察同一影子目标的合法读回，区分写入来源；记录实际变动字段及应用块，按差异补查。
- 已知本地项不报漂移；确认原生显示可连续消费 dirty／反馈／本地缓存，不因每帧纠正反复重建。
- 覆盖 Projectile Height 偏移、Tear 条件颜色分支、普通／过渡房间、HUD／地图／全屏演出，以及 Host 状态不变时多次本地显示推进和 Render。帧跳变／停顿的比较按 RE-3／RE-4，同目标安装失败／身份变化按既有生命周期规则。
- 新出现的已同步字段差异才增加校正范围；纯外观随机按 Q4，不把所有 RNG 当玩法 RNG。玩法、保存、PGD、成就等副作用仍由 E36／已有隔离检查，不能靠字节复原判定无副作用。

以上仅为整体验收交接，尚未执行。未闭合的 Render 尾部、资源 helper、派生类条件路径、异常／重入与官方运行时 Hook 组合继续保留未知；它们不构成本轮静态穷尽任务。
