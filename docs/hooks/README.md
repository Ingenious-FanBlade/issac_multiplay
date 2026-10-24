# Hook 与逆向点位总入口

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
