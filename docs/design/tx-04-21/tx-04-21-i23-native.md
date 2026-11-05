# tx-04-21 I23：ANM复用与身体渲染组静态主审

日期：2026-11-05。目标：官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273。按用户指示，由一个 GPT-6 Luna max 子智能体定点研究，主智能体审阅后实现。先查本地 Wiki、官方 wrapper／ZHL 和既有源码，最后只读 IDA；没有游戏、测试、动态探针或 IDA 修改。

## 查阅顺序与来源

本地原版 `F:/projects_isaac/IsaacDocs/docs/Sprite.md` 与 REP `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md` 提供 SetAnimation／SetFrame 的公开行为。REP 的 `EntityPlayer.md:337–339,405–423,931–944`、`CostumeSpriteDesc.md:43–45` 分别说明 body Sprite、层映射、服装和五个 Weapon 槽。Wiki 对 GetBodySprite 的“临时copy”描述不能代替实例所有权：官方 `repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp:3407–3412` 实际返回 `&player->_bodySprite`；`LuaCostumeSpriteDesc.cpp:5–25` 返回当前 vector 元素和 desc 内嵌 Sprite。不能由概念名虚构第二份 Head／Hand owner。

源码继续复读本仓库 `client_player_equipment_model.cpp`、`client_player_equipment_native.cpp`、`client_player_draw_native.cpp`、`client_entity_draw_native.cpp` 和 [tx20专用绘制合同](../tx-04-20/tx-04-20-special-draw-control.md)，确认现有 main／13个物理内嵌 ANM／costume 的来源与唯一 owner。

## 同资源跨帧／跨动画

Luna 定点读取 VA `0x405F50`（RVA `0x5F50`）、`0x406430`（`0x6430`）、`0x406010`（`0x6010`）、`0x406530`（`0x6530`）、`0x403DE0` 和 `0x403EC0`。主智能体直接复读 `0x405F50`、`0x406010`、`0x403EC0` 的伪代码及 `0x403EC0` 的指令：

- SetAnimation 的 thiscall 参数为名字与 reset；overlay 对应独立 AnimationState。SetFrame／SetOverlayFrame 会进入 reset=true 路径，不能用于每次热姿态安装。
- reset=true 释放／重建 layer 与 null selector 存储，清 phase、playing 和事件 mask；不是“同动画自动无分配”。
- reset=false 只有两类 selector 数量都相同、旧 buffer 有效时可采用。`0x404021` 的 null resize 分支把新指针写到 `+8` 而非 `+C`；layer 缩小分支也不能被当作安全重分配接口。实现先核数量，绝不进入这些尺寸变化分支。
- 所有目标 AnimationData、layer／null 数量、phase／frame 和 selector 先预检，再写入。精确 selector 是采样结果，不随后调用 SetFrame／SetLayerFrame 重新推导；事件 mask 清零，不调用 Update 或玩法事件。

现有文件、layer 图像 pair、shader 与资源实例必须仍匹配。资源变化交完整结构路径，快写使用 `ClientResourcePoseReusable`，不走 Load／LoadGraphics／ANM候选构造。相位数量变化按已核 reset=true 处理，图片与 ANM owner 不因此重建。

## Body／Head 的同源组

Luna 最初概括“Body／Head读取13个内嵌ANM”，随后逐调用点纠正：该最小读集并不包含全部13项。主智能体直接复读 Head `0x786460`（RVA `0x386460`），确认 `0x7864A4` 读 map[1..3]，`0x786565` 用 desc Sprite 和 layer，负 index 回退 main Sprite；因此 main-only 更新不能代表完整身体渲染。

研究结论：普通 Body 合成链 `0x783680`／Head `0x786460`／Glow `0x786760`／`0x786900`／`0x7868A0` 消费 map[1..11,14] 及选中服装姿态。29/38 分支还临时 copy／Swap 对侧 main 与 `Player+0x41C` body Sprite，并遍历全部15项映射；`Player+0x1E50` 的跨身体来源不能由本身体 PostUpdate 见证授予。未见这些函数直接读取 Weapon 五槽，故本轮不把武器私有状态升级为独立身体快源。

实现选用现成的更完整同身体组：main、13个已核物理槽、全部已登记 costume pose／公开动态值和 head_animation，在同一来源窗口采样并在同一 Manager 窗口安装／回读。13项是已选资源组，不声称每项都是 Body／Head 的最小直接读集。costume顺序、config身份、15项层映射和资源generation必须匹配精确 ordinary 依赖；不创建服装vector或移植地址。普通值与快值按真实 tick 选择；公开直接backing、服装及 HUD 查询视图同时推进，较旧 ordinary 不能回写选中的较新字段。

29/38 的单侧 PlayerFast 明确报告 `player_fast_cross_body_requires_ordinary`，消费端也在写前暂缓。完整 ordinary 组合仍须通过现有各身体来源完整性和实际关系预检；缺少合法对侧来源时不伪造新鲜 pair、不扩读 backup、不拼接两个时点继续绘制。**没有承诺这两类身体达到60 Hz独立快写**；未来如加入合格跨身体组合，须先闭合双方来源再原子提交。

上述为静态来源、直接复读和实现决定；实际资源复用次数、画面、频率与开销统一留 I39 的 P2全套→P4全套验收，不将本文件写成运行通过记录。
