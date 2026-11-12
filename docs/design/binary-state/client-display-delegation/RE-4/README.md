# RE-4：显示帧判断、跳帧与停顿风险

日期：2026-11-12。状态：**静态研究完成；未实施、未测试**。目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273。按[主方案第 6 节](../../client-display-delegation.md#6-新增逆向任务j273-静态反编译)执行，沿用帧号方案 B、Manager 边界两档调度和 Q8；第 15 步仍是唯一统一整体验收。

> **2026-11-12 审阅后标注**：用户决定同步当前房间描述的 SpawnSeed（Q9）。[帧号消费者 2.2](frame-consumers.md#22-roomage30历史-surface-捕获) 中“本次没有新增 seed writer”仍是本次研究的事实；Client 的 seed 来源改由[修改方案第 5 节](../../client-display-delegation.md#5-仍需同步的显示相关状态候选)表后说明规定。[处理方式第 5 节](handling.md#5-第13步修改与第15步覆盖)的第 15 步覆盖已写入验收 E40。

本次先查本地 Wiki、SDK 与官方源码，复用 RE-3 的位移候选并逐项复读采用分支。当前 IDB 仍为 `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，imagebase `0x400000`。没有启动游戏、调用原生函数、附着调试器、设断点、修改 IDB／补丁／目标内存、构建或运行测试／校验脚本。

## 成果

- [帧号消费者与静态证据](frame-consumers.md)：相位求值、周期请求、严格年龄判断、按 Manager 帧缓存、动画事件与本地调用计数，附 VA／原始指令。
- [处理方式与第13步交接](handling.md)：显示批次去重、周期跨越、缓存刷新、冷绑定、停顿／跳帧／回退处理及第15步覆盖。

## 结论

1. **F 不能作为所有显示工作的去重键。** F 是最近已应用的 Host GameFrame；M 是 Client 本地 ManagerFrame。本地30Hz批次在 M 偶数运行，相机有独立阶段；F 没变时 HUD／地图／FX／本地 Sprite 仍可按已选分支推进。单次批次不因多包、重入或多次 Render 重复调用。
2. **周期 HUD 刷新需要改成跨周期识别。** 原生 `0x6F78BA` 只在新 F 恰好是30的倍数时请求刷新。直接照搬会在29→31漏掉，在 F=30 停包时反复把 countdown 置2。采用同代次内向前跨越30帧边界的合并请求，尊重 type=2；同一 F 的显式库存／地图变脏仍应刷新。
3. **不是所有“相等帧”都是一次性事件。** Game::Render `0x6F7CA6` 的 RoomAge==30 控制本地历史 surface 捕获，不是奖励／清房。分支本身可在相同年龄反复进入；不得改成 >=30 或借用假 F 补触发。历史资源归本地，冷构建不能依赖已经错过的第30帧。
4. **平均 Player 位置是明确的按帧缓存。** `0x995B30` 的 useCache=true 分支以 M 对比全局戳；同 M 换 roster／world 后可能取旧值。一个本地周期先完成安装再查询；冷绑定／缓存失效时用已有 false 路径刷新，不通过推进 F 或多调一次 FX 更新来补救。
5. **多数闪烁／shader 是当前相位求值。** PlayerHUD `%45`、Minimap `%8`、HUD overlay `%20`、材质 `%256`、水面 F/M 组合相位等，在跳帧时直接显示目标相位，在停顿时按所读时钟保持或继续，不补演遗漏相位。Camera `%3` 不是随机调用去重；同一 F 的不同本地相机周期仍可能消耗本地外观 RNG。
6. **实体年龄阈值与本地初始化不能混为一谈。** StatHUD 用 F−Entity基准>1 决定属性变化提示；晚加入时实体虽已老，本地 HUD 缓存仍可能是新的。首次本地绑定需初始化显示历史，不得篡改 Host 锚点冒充年龄0。NPC／光照的年龄阈值直接求目标表现。Player 伤害闪烁另读 `+0x13B8` 计数，不读 F/M，不能靠推进 F 修复它。
7. **没有可代替调度去重的通用原生保护。** ANM2／HUD／Minimap 等按调用推进；动画当前事件位读取不消费事件。SFX 的 FrameDelay 是以 M/2 计的重播间隔，取0时也不会替代 Q7 的去重。Render 不额外调用这些 Update；不按 ΔF 补跑玩法或完整显示链。
8. **Q8 标量保持 Host 当前值。** 水色、波纹、颜色、震屏 countdown、fade 等不在 Client 递减／趋近；在成功安装点补做已核阈值动作。暂停／网络停包时不能用本地时间补它们的结束。

## 资料顺序与范围

优先资料：原版 [Game](F:/projects_isaac/IsaacDocs/docs/Game.md:250)、[Isaac](F:/projects_isaac/IsaacDocs/docs/Isaac.md:255)、[Sprite](F:/projects_isaac/IsaacDocs/docs/Sprite.md:207)、[SFXManager](F:/projects_isaac/IsaacDocs/docs/SFXManager.md:53)、[EntityPlayer](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md:1024)；REPENTOGON [Sprite Wiki](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)，以及 libzhl/functions 下的 [AnimationState](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/AnimationState.zhl)、[ANM2](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ANM2.zhl)、[HUD](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/HUD.zhl)、[Camera](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Camera.zhl)、[Minimap](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Minimap.zhl)、[StatHUD](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/StatHUD.zhl)、[SFXManager](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/SFXManager.zhl)。官方 Anm2Extras／InterpolV2／ExtraRenderSteps 与原版 IDB 分开记录。

`SOURCE` 表示本地资料，`J273_STATIC` 表示本次静态指令／反编译，`REUSED` 表示项目既有事实，`DESIGN` 表示后续处理建议。本次覆盖已核显示根与具体新增消费者，不宣称所有实体变体或整个 Render 尾部都已静态闭合；其他写集仍归 RE-5／E28。

RE-1 的提前返回分支补遗仍待完成；本任务只约束所选有效分支内的节奏，不把暂停／演出一律当普通分支。转场阶段／等待计数归 RE-9，音效完整来源／载荷归 RE-8。收尾仅静态文档审阅，没有运行时观察或 E2E PASS。
