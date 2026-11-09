二进制布局版本43（2026-11-09）：[NPC受控UI来源闭合](../design/binary-state/npc-control-ui-evidence.md)以Manager.zhl／NetplayManager.zhl／官方Player.IsLocalPlayer确认8E7550身份，纠正本地设备解释。源仅取NPC F1C哨兵并输出0/2；记录两处UI及Spawner淡出整段绕行要求，没有安装Hook或触及原版Online运行状态。

二进制布局版本42（2026-11-09）：[NPC职责复核](../design/binary-state/npc-synchronization-review.md)复读49F3D0原type恢复、6CFFD1/6CFFDA缩放覆写、6D027C/6D0337→8E7550本地设备谓词。取消Boss选择8B与shape、raw设备键及OverlayEffect原Scale；没有新增Hook或执行游戏／测试，正式表现adapter待后续接线。

二进制布局版本41（2026-11-09）：[NPC静态证据](../design/binary-state/npc-evidence.md)复读J273 ctor／Init／Update／Render／Shadow、大小／RTTI和三deque。六项真实绘制参数、双ANM2、复合引用与Delirium结构shape进入生成链；AI／Siren actor留Host，223B本类未知及Pathfinder声明保留。无新增Hook或实机操作，普通Release编译／链接成功，没有运行测试。

二进制布局版本38（2026-11-09）：[Effect各臂与payload复核](../design/binary-state/effect-synchronization-review.md)区分Render／Light与Host更新；撤销六项参数、TearFlags／136描述的传输，保留三种表现payload。补核MrMe四项私有状态、Trail606190的64B分配／复制及Ghost有效前缀和绘制派生Offset／Scale。没有安装新Hook、构建或测试。

二进制布局版本39（2026-11-09）：[Player/HUD装备及Plus复核](../design/binary-state/player-equipment-plus-synchronization-review.md)核835B40主动槽、758850进度条、60C1F0鞭主实体、415350水面分派与官方Plus hook；新增Whip135／TemporaryEffects136及Familiar D76来源。裁剪八项非展示范围，保留效果cooldown及两nullable；没有安装Hook、构建或测试。

二进制布局版本37（2026-11-09）：[Laser／Homing采样、运动与计时复核](../design/binary-state/laser-homing-synchronization-review.md)确认optimized供Laser绘制、unoptimized供Type7 Room Light；indices为8B双f32中间点缓存，旧i32类型错误，Bezier与采样标量及十九项Laser更新输入留Host。直接展示字段保留，两个Laser锚点撤销；原生Interpolate仍属模拟。没有安装新Hook、构建或测试。

二进制布局版本36（2026-11-09）：[Projectile运动与展示参数复核](../design/binary-state/projectile-synchronization-review.md)确认659460在+440开关下借Height临时改写+350，公共EntityFlags与私有+460／+464用于特效绘制；八项运动／规则输入归Host，原生Interpolate仍属模拟且不在Client放行。私有展示来源未闭合与375B未知保持；没有安装新Hook、构建或测试。

二进制布局版本35（2026-11-09）：[Pickup／Slot选择、交互与奖励复核](../design/binary-state/pickup-slot-synchronization-review.md)确认Pickup Price／DropDelay的WaterClip／Render消费者，排除Host选择／触碰／定价输入、未来循环列表及Slot八项参数；Slot奖品ANM2+34／+44分别为当前动画指针／is_playing，继续保留最终Sprite。原生宽度、未知和历史记录不变，没有安装新Hook、构建或测试。

二进制布局版本34（2026-11-09）：[Knife／Bomb／Tear参数展示复核](../design/binary-state/knife-bomb-tear-synchronization-review.md)核对Render／Shadow／PostRender、Room Light、排序／WaterClip及原生Interpolate，42个Host参数不上线。保留Knife拖影输入、Bomb最终costume／开关及pending清除、Tear效果位／DeadEye；Bomb懒资源重建的间接消费者依既有Client隔离合同处理。原未知和历史证据不变，没有安装新Hook、构建或测试。

tx-04-20 I39补充（2026-11-05）：[非活动Player来源、资源借用与绘制主审](../design/tx-04-20/player-i39-source.md)登记J273具名子体owner RVA3C1F40、ChangeType3C4620、完整Init34F6A0、Esau Jr owner3A3A40及Lazarus/Replace收口；读取资格4／5与当前exact-pair见证3分开，真正生命周期撤销。另核WaterClip库存依赖、ANM copy/Swap遗漏标量、普通Room主层mode1、Knife插值Update及伤害闪隐余6条件。新增Hook仍只经统一清单常驻安装，Source只观察正常owner返回，未增加原版Online/Isaac Sync操作；实际有限验收见[20记录](../design/tx-04-20/tx-04-20-e2e.md)。

tx-04-18 I29补充：[Room/Grid统一验收与主审](../design/tx-04-18/tx-04-18-acceptance.md)登记Crawlspace FX适用性、Room+7220 byte、Minimap连续深度，以及Backdrop图形构建入口917910/墙面911310/装饰915890和两处直接实体分支。同时补充704FB0基Grid物理构造观察，覆盖工厂之外的直接Wall构造；Luna/max结论已经主审独立复读。实际Hook/条件shim仍只由`mod/binding/hooks.json`生成并常驻，不改下面旧机器索引的历史快照。

# Hook 与逆向点位总入口

tx-04-17设计修订（2026-11-01）：[原生客户端世界与R1—R6补充研究](../design/tx-04-17/tx-04-17-workflow.md)保留create／remove／change，改在原生世界应用。研究顺序为原生初始化→操作生命周期→维护绘制→资源借用→失败退休／持久整链→模型角色接线；R1—R5已形成当轮静态合同，本轮[R6模型／角色／应用器](../design/tx-04-17/tx-04-17-model-role-applier-integration.md)对照15／16源码归并目标与A、原生计划、正式Scope／输入换代、提交与ACK及后续域职责；R1—R6指定文档结果均已形成，具体原生前置／取消／菜单及新增接线仍未实现。本轮没有新增native地址／ABI或逆向。没有新增运行结论或实现。ST1—ST8指定静态研究已于2026-10-31完成，下方更早的“ST8待完成”等保持原时点；原静态事实可复用，冲突的独立场景设计按新方案替代。

tx-04-17 ST5—ST7补充（2026-10-31）：[资源／姿态](../design/tx-04-17/tx-04-17-resource-pose-runtime.md)、[线程／context／在途绘制](../design/tx-04-17/tx-04-17-thread-context-lifetime.md)、[Lua与借用寿命](../design/tx-04-17/tx-04-17-lua-borrow-lifetime.md)已按每项一个GPT-6 Luna／max子智能体、三方来回互核及主审完成J273静态研究。补齐Load／Reload、帧setter、启动加载render旁路、WGL/TLS、raw image queue reset／remove、最后strong→Image析构→GL删除，以及Lua GC／原生借用和分段关闭顺序。正常静态链不代表实机context、全部在途引用或GPU完成；ST8、可控释放、世界壳与完整实现／统一E2E仍待完成。本轮未构建、运行、安装新Hook或改机器点位索引，具体接受范围见[工作流程内的主审记录](../design/tx-04-17/tx-04-17-workflow.md)。

tx-04-05 game 代码（2026-10-24）：[主线程适配与 Hook 接线](../design/tx-04-05-game-adapter.md)已编写并纯构建成功，清单为 28 个入口、8 个设备槽和 Odd 普通点 shim。没有在游戏进程中安装或测试，不改写下方历史运行观察与静态证据等级。

日期：2026-10-22；tx-03-09。用户确认覆盖全部已知点位，并按功能模块一模块一文档。此目录是整理后的阅读入口；原始研究、失败和既有源码保留。

后续补充：tx-03-10[Replica更新与展示分派研究](../research/tx-03-10-replica-update-presentation-boundary.md)。下列机器索引与计数仍为tx-03-09整理快照；新研究的函数、指令与结论由该报告及其原始证据单独登记。

tx-03-11进一步补充[实体绘制与动画事件分离](../research/tx-03-11-logic-presentation-separation.md)：Entity `+0x14/+0x18` 的不同绘制 pass、动画推进／目标帧／事件位、`Shoot` 到 Type=9 Spawn 的静态路径；Luna 研究已经主智能体独立复核。机器索引仍保留原快照，不把新增候选计为已安装 Hook。

tx-03-12补充[派生绘制与 Sprite 资源生命周期](../research/tx-03-12-derived-render-sprite-lifecycle.md)：NPC／Tear／Projectile 各自的条件 Lua 路线、附层／几何依赖，以及 Sprite 的构造、共享缓存与图像引用、Reset／析构区别。上述新证据单独保存在研究报告和原始 JSON，未改机器索引快照或安装 Hook。

tx-03-13补充[Sprite 图像资源与图形后端](../research/tx-03-13-sprite-image-resources.md)：独立取证复核 ImageBase control 六槽、缓存退休回调与 PNG 上传／删除，区分 Sprite／layer／Image 的 loaded 状态。加载、取图像时的 sampler 更新及最后释放可直接进入 GL；实际 renderer 线程／context 未确认，未安装资源 Hook。

tx-03-14补充[renderer／context 生命周期](../research/tx-03-14-renderer-context-lifecycle.md)：正常 `_main` 初始化及帧循环、WGL make-current／TLS、缓存释放到 context／窗口删除的顺序。启动加载循环另有直接 render 调用，不在现有主循环 callsite Hook 覆盖内；新证据未计入已安装 Hook 或机器索引。

tx-03-15补充[Sprite 换图／Reload／ANM 引用](../research/tx-03-15-sprite-replacement-reload.md)：实际 Lua closure 与 native ABI、layer 路径及 pair 更新、ANM 默认层重建和条件失败后态；Reload 额外计数与 Load 失败借用风险为限定静态推导。未安装新 Hook，未改机器索引或实现 adapter。

tx-03-16补充[独立 Sprite 展示字段](../research/tx-03-16-independent-sprite-display-fields.md)：两套动画与 track 关键帧、逐层姿态覆盖、属性布局、颜色栈／镜像／特殊 flags 的外部依赖；setter 的诊断不总能阻止数组访问。固定姿态支持描述仍是设计候选，不新增 Hook 或机器索引记录。

tx-03-17补充[独立 Sprite owner 生命周期](../research/tx-03-17-independent-sprite-owner-lifecycle.md)：原生／userdata 构造、copy／swap 的字段保留、候选 staging 与正常失败退休；GC wrapper slot 与 native dtor 分开，control 减引用与 optional callback 分开。未安装新 Hook 或更新机器索引，尚无 adapter。

tx-03-18补充[最小单 Sprite 适配合同](../research/tx-03-18-single-sprite-adapter-contract.md)：最终 pair 与预赋值 loaded 标志分开，PNG loaded 仅逻辑位；descriptor 的 layer 映射明确为 64 项。提出完整姿态验证、候选发布／退休及失败前缀接口，尚未实现；下一项研究实际绘制接入窗口，不新增 Hook 或机器索引。

tx-03-19补充[实际绘制接入窗口](../research/tx-03-19-sprite-render-entry-window.md)：RVA5560D0 的最终 flush/present pre-call 是受限屏幕 Sprite 候选；外层 RenderHook 不覆盖加载旁路，phase／TLS 均不能独自证明主线程／OS current。batch 正常持有自己的 image/control，绘制并非即时像素结果。仅补静态研究，不安装槽或更新机器索引。

tx-03-20补充[具体资源与姿态闭环](../research/tx-03-20-sprite-resource-pose-closure.md)：单层 Poof／RGBA8 PNG 的现成文件资料、ANM 相对路径与 parser 调整、同 Authority Capture 的姿态扩展位置、同 descriptor 连续写入和变更候选路线。Entity+48 已按实际 caller 复核，shadow pass 不保证恢复任意 visible；尚无采集／adapter 实现，不更新机器索引。

tx-04-02补充（2026-10-24）：用户决定改用官方 REPENTOGON，游戏降级到 v1.9.7.12.J273。本目录的全部 RVA、字节、对象偏移和槽位仍是 v1.9.7.17.J460 原型的事实，只作索引；在新版本上重新核对的工作见[REPENTOGON 适配重构设计](../design/repentogon-refactor.md)第6节。

tx-04-03补充（2026-10-24）：v1.9.7.12 上的核对结果见[REPENTOGON 目标版本点位核对](repentogon-binding.md)和[binding 清单](repentogon-binding.json)。本目录其余文件仍是原型版本的记录，不改写。

tx-04-05补充（2026-10-24）：[循环、输入与生命周期静态确认](../research/tx-04-05-static-confirmation.md)覆盖 T1–T8；明确 Odd 普通点 `0x2F9559`、按设备 id 绑定、公共更新返回后采样、Console/PauseScreen/失焦分支和 Player 字段宽度。GPT-6 Luna / max 的结论已由主智能体独立取证审阅；新增候选未安装，不改现有机器清单为“已实现”。

## 范围与证据口径

tx-04-08 清单与生成器补充（2026-10-27）：唯一清单 `mod/binding/hooks.json` 已升级为 schema 2，新增 19 项 world 观察入口（逐参数 ABI、完成含义、SDK `flags/argdata` 交叉校验），并新增 `mod/binding/world_layout.json` 登记 78 项字段偏移/证据、15 个来源、10 个 RTTI 类与目标版本常量；`mod/tools/gen_binding.py` 生成 wrapper/容量常量/布局表/清单，生成期显式报告 ABI、容量（60/64）与几何关系问题。world 组当前不安装（`WorldObservationArmed()` 固定 false）。本目录原有的原型点位与 ST1–ST8 静态结论不变；本次只做一次临时目录生成与纯编译，未运行游戏或测试。见[布局与 Hook 清单实现说明](../design/tx-04-08/tx-04-08-layout-hook-manifest.md)。

tx-04-08 ST6–ST8 补充（2026-10-26）：[owner、生命周期与工程边界核验](../research/tx-04-08-st6-st8-static-confirmation.md)已按每项一个 GPT-6 Luna / max 子智能体逆向、主智能体独立取证审阅完成。确认公开 Spawn/Morph/Grid 桥、恢复/替换完成边界及当前 Player witness 接线不足；主审纠正 Replace 分支倒置、补齐 Morph Lua 路由。41 行/40 RVA、pending64 与同址 GridFactory 定义已核对，注册不等于安装；Shutdown 在 Manager 循环后，完整退出协调仍待实现。具体 UNKNOWN 与 B 部分撤销分支缺口保留。ST1–ST8 静态工作完成；未改生产绑定或 IDB，未构建/运行游戏/测试，实现和整体 E2E 待完成。下方旧说明保留当时范围。

tx-04-08 ST3–ST5 补充（2026-10-26）：[字段、来源与分类身份静态核验](../research/tx-04-08-st3-st5-static-confirmation.md)已按每项一个 GPT-6 Luna / max 子智能体逆向、主智能体独立回读审阅完成。[78 项字段表](../research/tx-04-08-st3-agent-notes.md)逐项登记 J273 偏移/宽度/初始化；确认 448 Grid/8 门及其别名、SubPlayer 等保留关系、原生格点公式、七类 RTTI/COL 和非析构池复用。主审补正代理漏项与过强的坐标限制；具体读取资格/owner/Hook UNKNOWN 保留。未修改绑定/安装 Hook，未构建或测试；ST6–ST8、实现及整体 E2E 待完成。下方 ST1/ST2 说明保留当时范围。

tx-04-08 ST2 补充（2026-10-26）：[完整采样阶段、Hook 次序与短路静态核验](../research/tx-04-08-st2-static-confirmation.md)已由 GPT-6 Luna / max 子智能体逆向、主智能体独立取证审阅。确认普通 Game→Room→标准尾部→Manager 收束顺序，区分 1026 的整轮短路、1160 的派生 Player 短路及 tail 内部错误处理；完整复制位置在 Manager 下游整链返回后、EndManager 发布/退休前。未改绑定/安装 Hook，ST3–ST8、实现及整体 E2E 待完成。下方 ST1 说明保留当时范围。

tx-04-08 ST1 补充（2026-10-26）：[world_record 入口/ABI 静态核验与主审](../research/tx-04-08-st1-static-confirmation.md)登记 v1.9.7.12 的 19 项观察语义。GPT-6 Luna / max 子智能体逆向，主智能体独立读取 IDB/源码并审阅；已区分 Publish/Replace 的缺定义、GridFactory 的寄存器 ABI/同址定义差异及析构/删除包装。没有修改现有绑定或安装新 Hook，ST2–ST8 和实现/整体 E2E 尚未完成。

收录全部当前Native安装家族、直接原生调用依赖、自有device/control覆写表、已收集静态函数入口、32张当前RTTI/虚表及最小读取字段。派生类没有独立Hook时明确写出共享Entity路径，不凭类名编造安装点。原始反汇编中的每条指令不当成一个Hook；未确认地址/外部动态目标明确UNKNOWN。

- `CURRENT_SOURCE / SOURCE_INSTALLABLE`：源码可安装的声明；不等于本次运行安装，也不等于所有模式同时启用。
- `STATIC_CONFIRMED`：被引用研究的当前二进制静态结论；本次不重新运行IDA或游戏。
- `STATIC_EXPORT_INDEX`：原始函数导出可定位；旧标签不覆盖正文修正或当前ABI限制。
- `RUNTIME_OBSERVED`只沿用原记录的限定场景；本次没有新增实机证据。
- `UNKNOWN / USER_ACCEPTED_ASSUMPTION`原样保留；字段可读不等于可恢复、原生事件Hook不等于网络消息。

## 目标与地址

Win32/x86 `isaac-ng.exe`，研究横幅 `Repentance+ v1.9.7.17.J460`；当前静态记录的IDA imagebase为`0x00400000`。tx-03-04所读目标为PE timestamp `0x69E6E3A7`、SizeOfImage `0x008FE000`的ApprovedUnpacked。旧canonical/relocated profiles保留，不能把版本名相同当成任意文件适用保证；不计算或校验哈希。

`RVA = IDA VA - 0x00400000`；实际进程地址使用实际模块基址。call site、callee、IAT/vtable槽、返回地址、对象相对偏移分别登记。IAT按符号查找，Lua/Steam外部目标不编造主模块RVA。

rel32/入口表中的字节是源码expected指令；vtable指针字节以IDA imagebase下的目标VA编码，仅用于理解静态表，运行时必须使用实际模块基址处理重定位，不能拿静态指针字节直接匹配ASLR进程。JSON另标mode_scope：当前C/S owner/共享catalog、自有对象或历史/可选研究模式；当前源码仍有安装路径不表示该历史模式允许本次运行。

## 功能模块

| 文档 | 覆盖内容 |
| --- | --- |
| [Entity 公共对象与实体列表](entity.md) | 源码记录10条；静态函数51个 |
| [Player 玩家与本地合作](player.md) | 源码记录54条；静态函数47个 |
| [NPC 敌人与 AI 状态](npc.md) | 源码记录1条；静态函数8个 |
| [Tear 泪弹](tear.md) | 源码记录0条；静态函数5个 |
| [Projectile 敌方弹体](projectile.md) | 源码记录0条；静态函数3个 |
| [Pickup 拾取物](pickup.md) | 源码记录1条；静态函数9个 |
| [Familiar 跟班](familiar.md) | 源码记录0条；静态函数5个 |
| [Effect 效果实体](effect.md) | 源码记录0条；静态函数4个 |
| [Bomb 炸弹实体](bomb.md) | 源码记录0条；静态函数0个 |
| [Slot 机器实体](slot.md) | 源码记录0条；静态函数0个 |
| [Laser 激光](laser.md) | 源码记录0条；静态函数0个 |
| [Knife 刀实体](knife.md) | 源码记录0条；静态函数0个 |
| [Text 文本实体](text.md) | 源码记录0条；静态函数0个 |
| [Dummy 占位实体](dummy.md) | 源码记录0条；静态函数0个 |
| [Room、Grid 与门](grid-room.md) | 源码记录14条；静态函数28个 |
| [游戏循环、模拟阶段与生命周期](game-loop.md) | 源码记录17条；静态函数10个 |
| [输入管理与物理设备](input.md) | 源码记录54条；静态函数0个 |
| [自有 Virtual Device](virtual-device.md) | 源码记录18条；静态函数5个 |
| [Lua VM 与回调](lua.md) | 源码记录35条；静态函数6个 |
| [传输与原版联机历史观察](network.md) | 源码记录11条；静态函数13个 |
| [RNG 与原生诊断记录](rng.md) | 源码记录0条；静态函数5个 |
| [Windows 消息与平台接口](platform.md) | 源码记录4条；静态函数0个 |
| [Hook 安装与恢复](hook-lifecycle.md) | 源码记录0条；静态函数0个 |
| [共享访问器与研究辅助入口](research-support.md) | 源码记录0条；静态函数136个 |

## 安装家族和重叠

| 源码家族 | 本索引记录数（含依赖/动态项） |
| --- | --- |
| active_runtime.cpp | 13 |
| active_targets.cpp | 13 |
| boundary_runtime.cpp | 20 |
| coop_active_runtime.cpp | 19 |
| coop_passive_runtime.cpp | 24 |
| input_runtime.cpp | 11 |
| loop_runtime.cpp | 32 |
| offline_attestation_runtime.cpp | 9 |
| owned_virtual_device.cpp | 18 |
| raw_apply_runtime.cpp | 18 |
| runtime.cpp | 23 |
| world_record.cpp | 19 |

相同site可被不同模式复用，记录数不是可同时安装的唯一地址数量。`active_targets.cpp`是共享声明，`active_runtime.cpp`和coop等使用它。loop基础32＋coop19＝51；可选world19使组合为70。passive/device、mode、boundary、offline是其它历史/可选组合，不能直接叠加。

## 机器索引与原始资料

- [安装声明与调用依赖](installed-hook-inventory.json)：219条，含源码位置、RVA/IDA VA、原目标、字节跨度和ABI说明。
- [逆向入口、字段、RTTI及证据目录](reverse-point-inventory.json)：335个合并静态函数入口、75项字段、32张RTTI表。
- [固定Hook与ABI最近复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[实体生命周期与原生诊断生产链](../../docs/refactor/cs-t0/t0-06/tx-01-71-lifecycle-and-native-records.md)。

## 历史目录

以下catalog原样保留，可能含旧地址/流程；当前安装以源码和正文为准，不重新执行旧gate/哈希校验。

- [cs-t0/archive/t0-04-history/raw-apply-catalog-v1.json](../../cs-t0/archive/t0-04-history/raw-apply-catalog-v1.json)
- [cs-t0/archive/t0-04-history/raw-apply-catalog-v2.json](../../cs-t0/archive/t0-04-history/raw-apply-catalog-v2.json)
- [cs-t0/coop-catalog-v1.json](../../cs-t0/coop-catalog-v1.json)
- [cs-t0/coop-catalog-v2.json](../../cs-t0/coop-catalog-v2.json)
- [cs-t0/coop-catalog-v3.json](../../cs-t0/coop-catalog-v3.json)
- [cs-t0/coop-catalog-v4.json](../../cs-t0/coop-catalog-v4.json)
- [cs-t0/coop-native-join-catalog-v1.json](../../cs-t0/coop-native-join-catalog-v1.json)
- [cs-t0/input-active-catalog-v1.json](../../cs-t0/input-active-catalog-v1.json)
- [cs-t0/input-active-catalog-v2.json](../../cs-t0/input-active-catalog-v2.json)
- [cs-t0/raw-apply-catalog-v3.json](../../cs-t0/raw-apply-catalog-v3.json)
- [cs-t0/raw-apply-catalog-v4.json](../../cs-t0/raw-apply-catalog-v4.json)

- [源码原生定位常量](source-address-constants.json)：153条RVA/对象偏移命名记录，与各模块末尾补录表对应。
二进制布局版本40（2026-11-09）：[Effect后续五项证据](../design/binary-state/effect-next-five-evidence.md)核定传播／Timer等VarData复用、Timeout局部与公共递减、Rotation／Scale动态写、Trail尾部裁至127和三种ExtraData RTTI闭合范围。未新增Hook或实机操作，未运行测试。

