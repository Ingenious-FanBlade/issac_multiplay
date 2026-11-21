# tx-03-18：最小单 Sprite 的资源就绪与固定姿态适配合同

日期：2026-10-23（Asia/Shanghai）。承接 [tx-03-17 的正常 owner 路径](tx-03-17-independent-sprite-owner-lifecycle.md)，本轮研究最终图像关联、资源结构与姿态写入的检查顺序，定义自有实例关联和退休接口。GPT-6 Luna / max 研究资源就绪，主智能体独立取证、研究 descriptor／姿态合同并复核子结论，见 [子报告](tx-03-18-luna-resource-readiness.md)和 [主审记录](tx-03-18-primary-review.md)。

本轮是静态逆向与接口设计，未实现 adapter、registry、资源清单、wire schema 或 Replica。没有执行游戏、解包／加载资源、native helper、构建或运行测试；所有研究及编辑结束后只做一次整体静态文档审阅。历史失败记录保留。

## 本轮结果

资源就绪必须检查 **LoadGraphics 结束后实际持有的 image/control pair**。`Sprite+109` 只代表 Load 到达尾部，layer+30 来自赋值前候选的 loaded 检查；最终赋值还会尝试 retain，不能让这两个标志代替终态关联。已知 PNG 的 loaded 槽只读 image+8 bit0，上传 helper 正常尾部固定返回 1，因此本轮能定义的是正常 owner 下的“原生资源状态满足”，不能据此报告 GPU 上传、像素或 context 有效性已验证。

姿态也有明确边界：动画 descriptor stride 为 `0x13C`，但 layer→visual track 映射只有 **64 项**，不是把剩余空间除以四得到 65 项。候选只在 reset 选择动画后使用已构造的本地索引数组；验证所有名字、track、layer 与 key 关系后，再填入完整固定姿态，不用 SetFrame／Update 重新计算索引或事件。

下文的接口、支持 profile、错误和发布顺序均为 `DESIGN_PROPOSAL`；原始指令为 `STATIC_CONFIRMED`，在指定正常条件下的依赖结论为 `STATIC_DERIVED`。没有运行时成功或故障观察。

## 新鲜证据与地址

活动 IDB 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，记录 input path 为同目录 `isaac-ng.exe`，imagebase=`0x400000`。`RVA = VA - 0x400000`；对象字段、虚槽 offset 与函数地址分别解释。未比较文件身份，未计算或校验任何哈希。

| 本轮原始证据 | 内容 |
| --- | --- |
| [primary-pose-validation.json](../../../artifacts/cs-t0/tx-03-18-static/primary-pose-validation.json) | Load／ANM bind、reset、精确名字选择、直接 track 绘制、layer 初始化 |
| [primary-descriptor-origin.json](../../../artifacts/cs-t0/tx-03-18-static/primary-descriptor-origin.json) | ANM parser、RenderLayer／SetLayerFrame、几何除法及外部环境依赖；parser 是完整大函数，不代表其所有分支已研究 |
| [primary-mapping-bound.json](../../../artifacts/cs-t0/tx-03-18-static/primary-mapping-bound.json) | descriptor 构造及 native string 清理；精确确认 64 项映射 |
| [primary-readiness-review.json](../../../artifacts/cs-t0/tx-03-18-static/primary-readiness-review.json) | 主审独立重取 pair 赋值／getter／retain、PNG loaded／上传及两张不同 receiver 的虚表 |
| [primary-acquisition-notes.json](../../../artifacts/cs-t0/tx-03-18-static/primary-acquisition-notes.json) | 采集范围、散装资源搜索无结果及限制 |

只读采集源为 [export_static.py](../../../artifacts/cs-t0/tx-03-18-static/export_static.py)，通过 `TX0318_OUTPUT` 选择新文件，保留 create-new；完整指令 bytes、伪代码和静态 xrefs 在各 JSON 内。子任务原始导出见其报告。路径和符号不保证磁盘文件身份或动态调用全集。

| 函数／表 | VA | RVA | 当前用途 |
| --- | --- | --- | --- |
| descriptor 构造 | `0x40BEC0` | `0xBEC0` | 名字、track 等初始化；从 +38 填 64 个 DWORD=-1 |
| state reset | `0x408830` | `0x8830` | ECX=state，栈传 descriptor，ret 4；重建本地索引数组 |
| main 具名选择 helper | `0x40A5D0` | `0xA5D0` | ECX=Sprite，名字 C 字符串、reset 两栈参，AL，ret 8 |
| overlay 具名选择 helper | `0x40AAF0` | `0xAAF0` | 相同名字查找，目标 state=Sprite+50 |
| 整套 state 绘制 | `0x40A030` | `0xA030` | 从本地 selected-key 数组逐项读取，按 visual track stride 10 分派 |
| 单 visual track 绘制 | `0x409120` | `0x9120` | 检查 key、可见性，再按 track 的 layer ordinal 取实例 layer |
| pair 复制／retain | `0x40C550` | `0xC550` | 先令目标两个字段为零；control tryretain 为真才写入源 pair |
| ImagePng vtable | `0xB9FF84` | `0x79FF84` | 本轮重取 24 槽；与 control vtable 不同 |
| ImageBase control vtable | `0xB1A6E0` | `0x71A6E0` | 六槽；control+14 是管理的 image |
| PNG loaded 槽目标 | `0xA123B0` | `0x6123B0` | 只返回 image+8 bit0 |
| PNG 上传 helper | `0xA69B30` | `0x669B30` | GL 创建／上传调用，正常尾部固定 AL=1 |

## 1. 将“资源存在”收敛为可解释的终态

以下检查只适用于完整构造、原生 helper 正常返回、拥有有效引用的本地候选。检查期间必须处于已理解的主线程／当前图形 context、共享 cache 稳定、无重入／并发退休的窗口；这些运行 guard 尚未实现。读出非空地址或某次计数非零不证明任意悬空地址可以安全解引用，也不能让网络线程执行这些检查。

**结构层检查。** 新候选只做一次 `Load(ANM,false)`，并检查非空资源声明与实际 filename、非空 layer 表及正数 layer count、animation 表及 count、每个 layer 的 owner=本候选和 definition／路径关系。借用表的范围、字符串容量和整数乘加先按已声明资源边界解释，不从 `+109=1` 推导 ANM 成功。fresh candidate 的 missing ANM 不会获得正常成功绑定的结构，不能用旧 live Sprite 的残留结构代替。这些后态检查不保护此前 Load 内部的任意恶意资源解析；本合同只针对可信的本地固定支持资源。ANM 引用仍按 filename/key 查找，计数与普通失败退休依赖 tx-03-17 说明的稳定窗口；本轮没有发明 acquisition token。

**覆盖及终态图像检查。** 对声明内的 layer ordinal 应用 Replace，返回 false 时区分同路径与无效 ordinal；随后调用一次 LoadGraphics。检查所有实例 layer，而不只检查当前可见 key：最终 layer+98 的 image 和 +9C 的 control 都非空；control 属于已解释表、管理的 image 与 pair 的 image 一致；正常既有强引用关系仍成立；image 属于本轮已解释的 PNG 后端，loaded 逻辑位为真。layer+30 和 +31 可以记录为诊断，不能替代这组条件，也不能由网络覆盖。空 image／非空 control、错误 backend、关联不一致均不满足候选。

这组条件设计名为 `FINAL_PAIR_NATIVE_LOADED`，仅表示当前受支持 native 结构和逻辑 loaded 状态，不是引擎现有状态枚举，也不证明具体路径已匹配远端的资源内容。资源声明须由本地支持清单映射；不开放任意网络路径、cache key、vptr 或 raw pointer。清单可以记录逻辑修订、精确名字、layer／track／key 数量、映射及所需路径，不使用哈希；这些元数据相同仍不能证明任意文件内容或像素相同。

`40C550` 先把目的 pair 清零，再经源 control 的 +4 槽 tryretain，成功才复制两个字段。`40C000` 在此之前就计算了将写入 layer+30 的标志。该顺序是“候选 loaded”与“终态已关联”必须分开的直接依据，不等于正常有效引用下已观察到 retain 失败。control 的检查与递增分两次锁，16 位计数也没有当前已见的溢出拒绝，跨线程关闭和异常计数不在本合同正常范围。

不要用 `408590` 充当无副作用的 readiness getter：它在复制引用前，可能经 image+44/+48 更新共享 image 的 sampler；PNG 路线会进入 GL。候选检查优先在既有 owner 保护下读最终关联和已解释字段，不额外借此触发图形状态变更。其未来读取代码和错误处理仍需实现。

PNG `A12B80` 先设置 loaded 位，再调 sampler／完成 hook；`A123B0` 不查询 driver，`A69B30` 尾部固定返回 1。非零纹理名、非零尺寸或 loaded 位即使增加检查，也不能替代当前 context／GPU／实际像素的后续运行验收。其它图像格式及后端不因共享 loaded helper 而自动进入支持集合。

## 2. descriptor 的真实边界与名字选择

`40BEC6: B9 40 00 00 00` 设置 64 次，`40BECB: 8D 7A 38` 指向 descriptor+38，`40BF17: F3 AB` 填 DWORD=-1。于是映射范围为 **+38..+137，共 0x100 字节**；stride `0x13C` 中的末尾 +138 不是第 65 项。parser 在 `413927` 以 descriptor+138 为映射交换终点，并在 `41397F/413985` 单独交换 +138，补强这个边界。本轮不赋予尾字段未证实的产品语义。

`40A5D0/40AAF0` 从 Sprite+74／78 借用的 animation 表／count 查找，步进 `0x13C`，按 native string inline／heap 选择缓冲，并进行区分大小写的 C 字符串比较。第一处名字相等即 reset 所选 descriptor；AL=true 不证明图像成功或所有资源有效。候选应事先使用资源声明的精确无内嵌 NUL 名称，并确认唯一映射；不做不明大小写折叠，不接受网络字符串在首个 NUL 后仍含另一逻辑身份。原生 parser 的所有异常输入并未闭合。

首个固定姿态 profile 将 layer count 限为 1..64；每个 visual track 的 ordinal 必须在实例 layer 范围内且小于 64。为避免一个 layer 对应多个 track 的歧义，首版进一步限定同一 descriptor 内 ordinal 唯一，并核对 `mapping[ordinal]==trackIndex`；没有 track 的 layer 以 -1 映射表示。这些是受限支持决定，不宣称所有合法原版资源都满足，也不调用 SetLayerFrame 来“试出”映射。

`409120` 虽保护 selected-key 的正负与上界，却在 `4091A7..4091AF` 直接用 track.layerOrdinal 计算 layer 地址，没有本函数内的 ordinal 范围检查。因此必须在进入 draw 前解释本地资源关系；只检查网络 key index 不够。`RenderLayer`／SetLayerFrame 还另用映射，不列为此最小合同的写入入口。

## 3. 固定姿态的验证与应用前缀

目标是 tx-03-16 提出的停止姿态；不是从全局 frame 重新模拟所有 layer。候选输入为资源声明引用、main 名称和完整 selected-key 向量、显式可空 overlay 名称／向量与绘制顺序、Sprite／layer 的声明外观，以及本地相机产生的 draw 位置和零 crop。未发送的 raw 资源、owner、指针、native strings、playing／event masks 都不进入该 DTO。当前没有协议或采集端实现。

1. **只读规划完整描述。** 在调用改写 state 的 native helper 前，按已支持资源校验两个动画名、完整向量数量与每个 track 的范围／映射；key 仅允许明确的 -1 或本 track 内有效索引。count×stride 和地址加法必须有界。输入重复 ordinal、缺字段、非有限值或不支持 flags 在本阶段返回不支持／不合法，不能在部分写入后才逐项发现。
2. **重建本地 state。** 对未发布候选调用 main 的 `40A5D0(name,true)`；有 overlay 时调用 `40AAF0(name,true)`，无 overlay 时 `40AC60` 清掉它。检查返回的匹配结果与实际 descriptor 是否等于先前解析的本地目标。`408830` 按 descriptor+1C／24 分别创建 state+8／C 数组，填 -1；每个正 count 都需有正常构造出的相应本地数组。正常返回不外推 OOM／EH 安全，因为可见分配后直接 memset，没有在这里完整拒绝空指针。
3. **按完整向量写本地索引。** 在同一无 draw／重入窗口，把经验证的 main／overlay visual key 向量写入各 state+8 所有项；不写共享 descriptor／track／keyframe。第二类 track 索引维持 reset 的 -1，不作为 direct Sprite Render 的驱动；本合同不消费这些 track 或事件。state+10 保持 reset 的 f32=-1，+14 playing=0、+18／1C 事件=0；该总帧是本地停止状态，不声称 GetFrame 返回 Authority 的诊断帧。
4. **应用显式外观。** 写入候选的 Offset、Scale、Rotation、Color 的 11 个声明分量（共 44 bytes）、Flip、overlay 顺序、layer 可见性／scale／rotation／offset／flip／额外偏移。tx-03-16 的 flags=0 profile 不使用 layer Color 分支，可保持其构造值；第二 Sprite Color／特殊 shader 不开放。省略字段必须有声明默认，不能继承未知 live 状态；来自全局的构造默认不硬编码为已知常量。
5. **检查完整后态，才允许发布。** 两套 owner=本候选、descriptor 与数组数量关系、完整索引值、停止及零事件、外观／flags 和所有最终图像关联均满足当前描述。失败记录已执行前缀，不进入 draw，不报告已应用该对象或推进整批 AppliedAck。

第 3 步是待实现的受限本地字段写入方案，当前没有直接写数组的代码。`40A030` 直接从 state+8 读每个 key 并按 visual track 调 `409120`，其已读 direct-draw 路线不靠 state.frame／playing／event 位推进姿态，这是该方案的静态依据。它需要后续运行验收证明图形结果，不能从地址和偏移存在宣称适配已成功。

所有被使用的资源 keyframe／实例变换／颜色分量须满足声明的有限数域；尤其 `4098A0` 用 keyframe.scale×layer.scale 作 crop 除数，**即使 crop=0 也必须有限且非零**。还应对支持范围内的几何组合结果作有界解释，单个输入有限不保证乘法不溢出。合并特殊 flags 必须本来属于 flags=0 支持集合；Load 生成星号分类或其它 flags 时报告不支持，不清位伪装成忠实复制。renderer 的颜色栈、镜像 byte、Game offset／视口依赖仍按 tx-03-16 的有限 pass 约束处理，本轮没有通过写全局来消除它们。

## 4. 最小接口与本地实例关联

以下为设计签名，只描述 owner 关系，不是新增可执行 API：

```text
PlanFrozenSprite(ResourceDeclaration, FrozenPose) -> Plan | Failure
PrepareCandidate(OwnerWindow, Plan) -> OwnedCandidate | FailureWithPrefix
InspectCandidate(OwnerWindow, OwnedCandidate, Plan) -> NativeStateReport
PublishCandidate(OwnerWindow, VisualKey, ExpectedRevision, OwnedCandidate)
    -> LocalInstanceToken | FailureWithPrefix
Retire(OwnerWindow, LocalInstanceToken or OwnedCandidate, Reason) -> RetirementReport
```

`Plan` 是有界逻辑数据和受支持本地资源描述，不持有跨退休窗口的裸 descriptor。`OwnerWindow` 是未来主线程／context／不重入的本地检查依据，不能从网络包传入几个 true 冒充。实现时按实际已加载模块基址解析本地 RVA，不把本文 IDA VA 当作任意运行地址。Prepare 只构造新的未发布对象，记录 raw allocation、完整构造、一次 Load、覆盖／图像处理、动画 reset、姿态写入各前缀；Inspect 不借助有 sampler 副作用的 getter。对象外层分配与原生内部引用分配保持各自配对。

本地 registry 用 session、run_epoch、room_epoch、visual_id 及 shape/resource revision 关联自有 Sprite；原生地址仅在本地当前实例寿命中有效。另分配不复用的本地 instance token，防止旧队列／回调因地址复用命中新对象；token 耗尽不回绕复用。具体字段编码尚未写入协议。与现有 ClientId／输入 ordinal 的语义无关。

| 关联操作 | 合同 |
| --- | --- |
| 发布候选 | 同一 owner 窗口确认 generation、期望 revision 和无在途自有 draw，再切换自有 registry 关联；不写 Entity／Room 成员，不调用 Spawn／Init |
| 完全重复的逻辑 revision／描述 | 在本地直接比较规范字段后幂等返回现有关联；不重 Load，不创建第二个 owner，不计算哈希 |
| 过期代次／revision、相同 revision 内容冲突 | 拒绝该应用，失效候选不得复活旧关联；不凭 pointer 相同认定同一对象 |
| 资源／shape 更换 | 新候选准备，切换关联后退休旧实例；native swap 不代替完整 pose 提交 |
| 姿态的高频原地更新 | 本轮没有定义已发布实例的事务式写回；后续实现应另行解释中途失败与 draw 可见性，不能自动套用候选合同 |
| apply 部分失败 | 停止相关 Replica 发布／显示，不推进 AppliedAck；现存实例即使内存未被改动也不代表可以继续冒充目标状态 |

发布动作只定义本地关联的可见时刻，不提供 CPU 并发原子写回或共享 cache 隔离。资源 content／搜索前缀稳定仍是正常范围；不对热重载、worker 或其它 owner 原地重解析作保证。整批 baseline 的 AppliedAck 需所有对象／引用／状态满足，单 Sprite 发布不是整批应用成功。

## 5. 失败报告与正常退休

Failure 至少记录逻辑 key、代次、资源声明／精确名称、层／track ordinal、失败原因和最后已执行前缀。候选原因包括 `UnsupportedResource`、`NameOrMappingMismatch`、`FinalPairMissing`、`FinalPairInconsistent`、`PoseOutOfRange`、`OwnerWindowUnknown`、`GenerationRetired`、`NativeOutcomeUnknown`；这些是诊断设计，不是 gate 或解锁字段。不能把所有 native void 或正常返回统一转成成功。

正常退休顺序为：停止 publication 与新 draw → 撤销 registry／队列关联，确保没有在途自有使用 → 在资源 context 尚有效时对完整构造 payload 调一次 `408170` → 自有外层存储由原分配者释放 → 标记 token 退休。若外层是 Lua userdata，则保持 Lua 外层 owner，不能手工 free userdata；本最小候选优先自有原生稳定存储，不借任意 GC 时刻承担最后图像释放。不要先调完整析构再重复 Reset／析构，也不要把 virtual device 的 NEVER 策略移植到 Sprite。

对于完整构造且 helper 正常返回的普通资源失败，在同一稳定 cache 窗口退休未发布候选；清理的引用条件沿用 tx-03-17。context 已丢失、部分构造、异常指针、计数损坏、EH／OOM 或并发关闭时，报告未知前缀并停止相关修改；本合同不提供猜测性 dtor／强卸载恢复，也未保证这些情况均可安全清理。正常析构不证明 cache 立即清空、GPU 已 idle 或纹理立刻删除。

## 完成范围与下一步

本轮已形成最终 pair 的有限就绪语义、descriptor 的 64 项边界、无事件的固定姿态应用方案及 Prepare／Inspect／Publish／Retire 合同。支持清单的具体资源、Authority 姿态采集、Native adapter、runtime thread/context guard、draw 队列、registry 与 E2E 均未实现。

磁盘资料搜索时 `resources` 下只看到 `packed`／`scripts`，未得到散装 `.anm2/.png`；指定的 `resources-dlc3` 目录不存在，保留该读取错误，未解包或试加载。故本轮不虚构可用资源样本或宣称双方内容相同。

下一项建议优先逆向**最小独立 Sprite 的实际绘制接入窗口**：现有 RenderHook 覆盖范围、加载循环旁路、线程／WGL context 的运行 guard 所需读取、caller 位置／crop 与 renderer pass 的状态保存恢复。先把调用 ABI 和可用窗口对应到现有 C/S owner，再选择具体资源实现最小适配。Player 组合外观、派生几何、Room／Grid apply 与全套失败恢复仍保留为后续任务。

唯一一次整体静态验收结果见 [主审记录](tx-03-18-primary-review.md)和 [final-static-review.json](../../../artifacts/cs-t0/tx-03-18-static/final-static-review.json)。没有新增生产 Hook 或机器点位索引，没有改变 C/S 与原版 Online／Isaac Sync 的隔离。
