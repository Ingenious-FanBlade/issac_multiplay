# 第8步：地图交接、随机显示结果与Presentation正式源

**2026-11-12 已被取代**：显示结果、D0 和随机 alpha 退役；地图只保留描述；Random 只保留 Host 选出的结果。原文、事实与 UNKNOWN 保留。见[修改方案](client-display-delegation.md)。

**2026-11-12 HUD后续职责修订，仅文档**：[HUD本地生成合同](hud-local-presentation.md)调整低血周期／护盾、普通图标／中心、材料／队列、Drop渐变、基础属性格式与多人布局的后续实现。下文版本58源／生成／编译记录保持原事实；315等旧结果布局、用途依赖及schema须在后续正式迁移，不手改生成表或声称本地化已实现。特殊结果、三色相位、有序列表、Stat三值／概率保持Host职责；本次没有生成、构建或测试。

日期：2026-11-10（Asia/Singapore）。整数布局版本 **58**。本轮落实第8步的正式布局源、生成表、逐行回映射和具名结果合同。运行采集／缓存、批次封装、图形与音频资源物化、Client安装和旧链退役分别留给第9—14步；不把本轮写成运行功能已完成。第15步仍是唯一统一整体验收。

## 唯一输入与资料依据

新增[`presentation.json`](../../../../mod/binding/state/presentation.json)的74个布局和[`random.json`](../../../../mod/binding/state/random.json)的24个原生事实视图，编号 **310—407**。`version.json`登记两个正式域和`audio-table`资源空间。共享RNG布局8继续复用，只补入其17条正式目录行；Vector、ColorMod、KColor、ANM2等也不重复建立生产定义。设计账本／来源目录只作证据，不是构建或运行时的另一份布局输入。

本轮先读本地原版Wiki的HUD、ItemPool，以及REPENTOGON的PlayerHUD、PlayerHUDHeart、HistoryHUD、Minimap、LootList／Entry和SourceQuad；再对照官方Minimap、MinimapConfig、PlayerHUD、HistoryHUD、StatHUD与SourceQuad的ZHL声明。具体J273消费者沿用已完成的[Presentation核对](presentation-synchronization-review.md)、[资源／屏幕来源](presentation-resource-screen-sources.md)、[音频来源](presentation-audio-sources.md)、[职责复核](presentation-source-synchronization-review.md)及[Random核对](random-synchronization-review.md)，没有新增实机结论。

## 地图唯一owner

Minimap325的world结果引用原有MapFocusPair301、MapBadgeList302和MinimapQuestionImageBinding303；MapFocusResult300仍是其元素。原生Minimap+614不再发送第二份问号资源号。两份MinimapConfig324各持自己的边框资源；icons和itemIcons各自保留真实资源、overlay、颜色与图层外观。

MapDisplaySprite355复用ANM2原生边界与UNKNOWN，只裁掉已由地图轮廓／房间图标／L2／徽标用途每次明确选择的main状态；不把Host最后一次SetFrame当作整张图的状态。实际overlay、图像／shader选择及其他可变外观保持。原生Minimap中的pickup Sprite没有已核地图消费者，不产生发送投影，其完整原生ANM2事实仍展开。

Minimap的最终shake_offset直接发送，shakeDuration与随机生产器留Host；另登记world转场的最终地图淡出乘数，本地展开alpha不混入。state、holdTime、FBO、bounds、分辨率、Options及地图查看操作留各端本地。

后续确定性地图adapter必须共同消费完整L0—L3、双配置、300—303及必要资源；挑战32未正常产生或169没有合法描述关系时是Gap，不能改成present0或另抽。NO_HUD、有效curse、Home的Data.StageId35／Type16等复用既有唯一事实。地图Time／Score复用Run缓存，不调用Refresh、Update、UpdateVisibility或Render内分数重算。实际地图合成与来源Hook尚待10／13步。

## 随机显示正式布局

| 正式类型 | 状态负载与用途 |
|---|---|
| PresentationDrawClock356 | D0固定5B：u32 ManagerFrame＋u8 paused；GameFrame复用帧头，subframe派生 |
| PickupRandomAlphaResult357／FamiliarRandomAlphaResult358 | 各1—5B：u8 state；只有state1带有限f32 alpha；state0只表示Host明确无覆盖 |
| PillDisplayResult359 | 1—13B：无显示／未知／已揭示三分支；实际外观、该身体effect及名称／外观图像；未知不读effect |
| PickupPreviewItem360／PickupPreviewResult361 | 有序visual_id与实际图像，正常已确认隐藏与空集合分开；最多256项，超限报Gap |

两个alpha只接正常6E3805或61E604已经产生的结果，身份包含subject/use/shape、用途、pass、分支／递归贡献和来源修订；多次调用不折成最后值，重复展示不再抽取。不存在seed、shift、guard、原生指针或Client按访问次序分配样本的正式字段。药丸不恢复ItemPool映射／鉴定位或执行GetPillEffect，预览不调用GetLootList(false)或解引用未知RNG*。

Random的24个新增视图与共享RNG8均无发送块。玩法池、保存体、MT／CRT／Lua模型、BD0CA4／BD0674和两份SFX随机流仍留Host／本地；模型证据转为明确的推断说明，不冒充实际目标ABI已核。地图和音效实际结果分别归其Presentation owner。

## HUD与其他显示结果

原生字节表覆盖43个新增Presentation类型；全部原孔、弱证据与动态对象边界保留，未用最终结果抹掉UNKNOWN。可直接读的当前值进入复制计划；复杂内嵌继续递归展开，指针／strong／STL头不作为裸值复制。

- PlayerHUD315、HistoryHUDPlayer321和StatHUDPlayer317随body目标；全局HUD323的资源／11条消息／Boss结果及StatHUD318共享资源随world。原生八HUD槽与History／Stat两槽没有变成系统容量。
- ControlGroupHudSelection364登记组与真实身体、公共／详细／History／Stat／必要关联健康显示角色和顺序；各端按自己的已确认组选择详细主体，其他组公共HUD保留。归属与输入／roster编号分开。
- VisiblePlayerHUDHeart353只有在心格可见时读取；eternal成立才读overlay。隐藏格不解引用旧动画指针。StatHUDRows354固定0—7三值，第8行由Host星象房资格控制，不发送第9行、previous／threshold或可伪装缺行的空count。
- History保留完整有序有符号visual_id与实际Image，不按库存重排；PlayerDrawImageBindings362承接Player两份实际Image，动态HUD按真实身体owner处理。
- 365／366为具名Sprite／Image贡献元素，保留逐layer／主叠加、实际临时变换、SourceQuad空间、完整ColorMod、逐顶点KColor及实际shader／blend／sampler选择；只在固定owner用途消费，不成为通用GPU命令流。
- 367—380登记bloom、pixelation、retrovision、hallucination、Room glitch、有序snapshot／fade、Room／Stage转场、Nightmare、死亡、ItemOverlay、BossOverlay和共享Prompt。只发送最终贡献；Run颜色／时钟、Nightmare路线／Scene帧／Dogma、转场FSM与结果不重复下发。图像资源无法映射或坐标未闭合仍报Gap。
- AudioVoiceParameters381、AudioVoiceState382与AudioWorldEvent383表达实际选中资源、Options前gain／pitch／pan、loop／world pause、逻辑段和实例事件。原生两组×七声部只是Host生产结构，不整组复制；jingle保留本地SFX音量类别。个人设备失败、抢占与静音不成为world Stop，baseline不重播历史one-shot。

各组完整History／Stat来源、全部音乐Options前writer与逻辑生命周期、segment／seek恢复、视频音轨、纯world程序图像内容与各用途坐标等仍未闭合。来源未完成与正式值布局已经登记分别报告；不能执行额外HUD／概率／分数重算、Lua或Render补齐。

## 生成合同、回映射与预算

schema新增`presentation_contract`。生成器要求合同与实际块序列一致、共同提交范围是所属world/body目标、依赖类型确实存在且有总记录预算；同时生成28条`PresentationContracts()`和数字布局依赖`PresentationDependencies()`。owner、subject、producer、adapter、窄依赖、失效条件、贡献身份及明确来源Gap与布局一起生成，不是已绑定的运行函数。

141条Presentation及54条Random公开目录保留原一基来源行、重复ID、重载和null／空签名。真正的心格、药丸、Prompt、Overlay及音频结果关联其正式字段；本地派生、Host命令、跨owner引用分别说明，不恢复Getter查询字典。RNG17行仍归共享布局8。

资源、消息和最终贡献有独立数量／编码上限；身体结果沿用整个body组64KiB保护，超限按既定ordinary body-joint处理。单记录与同world总预算还须同时满足；表中的各字段最大值不能同时取满。首次准备≤1MiB／≤10秒和三端总持续7.5Mbps目标不变，所有资源／blob／结构均计入，未作性能实测。

第9步仍须登记批次公共上下文、贡献／事件身份、Clear范围、精确外层长度、音频资源payload与固定shader／blend／sampler允许集；本步元素表不是已可接受的完整报文。第10／11步封存正常结果并映射各Peer目标；第12／13步校验完整依赖、准备资源并共同提交。缺必要结果不安装／不ACK，旧完整目标只能在其依赖仍合法时展示。Authority不等待显示或任何Peer确认。

## 构建与实际边界

正式入口`python mod/tools/gen_state_layout.py --out build/repentogon/generated`已经完成生成，文档见[第8步生成索引](../../../../mod/binding/layouts/08-run-presentation/README.md)。迁移中的声明错误包括纯值嵌套归属、条件字段不能进入无条件复制、条件selector必须先行且无条件，以及共享RNG目录行未写回；均在普通生成阶段修正。心格因此拆出可见payload，Stat行改为固定具名投影；没有把这些构建声明错误写成测试失败。

Release／Win32／v143的`isaac_cs_state`纯编译和`zhlIsaacCS.dll`产品链接均已通过，`ISAACCS_BUILD_E2E_TOOLS=OFF`。普通沙箱中的产品目标长时间无输出，已中断；随后通过既有MSBuild工程在沙箱外完成链接，产物仅写仓库`out/repentogon/`，没有安装到游戏目录。

没有新增单元／隔离测试，没有运行游戏、E2E、smoke、校验脚本或其他测试，没有部署／注入。正式Hook、缓存、运行adapter与未知来源继续按第9—14步落实，第15步再进行唯一统一整体验收。
