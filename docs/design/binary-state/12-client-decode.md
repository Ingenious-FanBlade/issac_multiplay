# 12：Client 解码、校验与影子状态

版本45按[残余结论](projectile-effect-residual-evidence.md)保持Projectile PR0为4B f32 Height，并解码新增PR2的9B：u8 height_render_mode／Vector freeze_render_position，偏移0／1；浮点必须有限，u8按零／非零解释，不能以bool校验错误拒绝2等原始值，也不能未对齐强转。Effect新增148／149，167不再接受102；19／71／194不开放公开TearFlags，FX2仍预留。119 callback专用尾部及其余UNKNOWN不进入影子状态，不接收地址或假造DamageFlags负载。实际帧解码接线仍为本步后续工作。

版本44 Text／Dummy按[逐段证据](text-dummy-evidence.md)解码145／146／147。TX1必须为u16 count＋精确count原始字节，count≤255、无内嵌NUL，非UTF8允许；`StageCStringInto`只在自有256B暂存区补NUL。TX0为四个有限f32，TX2只在146有i32 counter，145的12号块预留；不得等待duration／颜色空间标签或Dummy专有负载。Manager帧／pause依赖同world展示上下文，缺失不可换用本地帧。旧独立Render来源保持原scope，不拼为当前普通快照。实际帧解码／影子状态接线仍属本步后续工作。

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

Client 在后台线程中完成三件事：

- 对收到的成员表、资源表和状态帧做有界解码和校验；
- 维护影子状态；
- 把需要写入的变化整理成应用批次，交给主线程。

主线程不解析报文，也不做字符串匹配。

## 输入事实

- **现有 Client 的分层**：
  - `ClientScene` 只保存纯值目标和提交，不跨线程持有原生对象（`mod/src/game/client_scene.h:27,85`）。
  - `ClientObjects` 按“已应用 A → 目标 T”的纯值计划差分，执行失败后不再按历史 A 继续差分（`client_objects.h:17-18`）。
- **现有解析链**：`Parse{Entity,RoomGrid,Player}Target`、`DomainModel`、`SubjectValues`、`stream_materialize`。它们把范围模型还原成按字符串命名的目标，由本步和第 13 步取代。
- **校验规则**：见[第 9 步 9.8](9-wire-format.md#98-client-校验与接受规则)。

## 设计

### 12.1 影子状态

- **结构**：与 Host 块存储同构，即每个成员、每个块一份字节。另有成员表（序号 → 身份、布局）和资源表（序号 → 路径）。
- **线程**：只由解码线程写入。
- **类型化视图**：生成器为每个布局、每个块产出只读访问函数（`state_client.inc`），例如 `EntityE1::Velocity(const uint8_t*)`，取代 JSON 树和字符串成员名。

### 12.2 接受流程

1. **表消息**：按 epoch 顺序安装成员表和资源表。
   - 前一 epoch 不符时请求 Resync。
   - 成员“离开”时，影子状态中删除该成员。
   - 成员“形态变化”时，清空该成员的块，等待新形态的完整块。
2. **状态帧**：按第 9 步的规则整帧校验。通过后：
   - 把每个块写入影子状态；
   - 把（成员, 块）加入“待应用集合”；
   - 记录该序列的已接受版本。
3. **合并**：待应用集合按块去重。主线程还没取走时，同一块的新字节覆盖旧字节，只保留最新值。

### 12.3 应用批次

- **交接**：主线程每帧以一次交换取走应用批次，主线程不在解码线程的锁上等待。批次内容如下：
  - 成员表操作（加入、形态变化、离开），按 epoch 排序；
  - 待应用的块，含成员序号、块号和字节副本；
  - 本批次对应的 world、body 目标版本，以及两个表的 epoch。
- **内存**：批次的字节放在现有内存池中，有上限。超出上限时停止接受新帧并请求 Resync，不截断。
- **形态完整**：成员“加入”或“形态变化”后，必须收到该成员全部块才算可构造。块不全的成员留在批次中等待，并且有超时，超时后报缺口并请求 Resync。

### 12.4 与旧路径的关系

旧的 `Parse*Target`、`DomainModel`、`SubjectValues`、`stream_materialize` 不再使用，在第 14 步移除。`ClientScene` 的提交语义不变，但输入改为应用批次和类型化视图。

**版本33 Familiar负载**：按[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)只对实际线上字段校验：Incubus amount为i32且0—100；240四个payload投影count仅0／1并一致，非空payload的节点数为1—16384、XYZ有限，全部同目标版本齐备。原始轨道／方向／HeadFrameDelay／Hearts／Keys／RoomClearCount不上线，不为它们构造拒绝负载。节点数0的下限及owner存在性不能只交给通用u16上限检查；不把未知类型／读取失败当缺失。

**版本34 Knife／Bomb／Tear负载**：按[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)只解码实际保留字段；42个Host范围及其三个u64锚点不进入影子状态／校验／应用批次。Bomb B0、Tear T0／T2等reserved空块不作为构造必收的状态块，不因空块存在而等待不存在的负载；其他实际块及资源仍须完整。旧布局负载由整数版本不一致拒绝，不做逐字段兼容补丁。

**版本35 Pickup／Slot负载**：按[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)只解码实际保留字段，原19项字段／容器与Pickup Timeout／Wait锚点不进入影子状态或应用批次。Pickup P3、Slot S0／S1预留且不等待不存在的负载；不再检查已撤销循环列表的线上count，其他真实容器／资源约束保持。DropDelay的条件编码与Price、辅助Sprite／关系仍须同目标版本完整。

**版本36 Projectile负载**：按[Projectile运动与展示参数复核](projectile-synchronization-review.md)只保留PR0的Height及继承状态／资源；八项Host输入不进入影子状态或应用批次，PR1预留不作为构造必收负载。Height有限值和真实资源／引用约束保持；不构造已撤销字段的拒绝负载。私有展示来源尚未闭合，不以合法解码当前已知字段声称画面完整。

## 改动范围

- 新增：解码器、影子状态、应用批次。
- 修改：`ClientScene` 的输入接口。
- 移除（第 14 步）：旧的解析链。

## 失败方式

- **部分接受**：校验通过一部分就写入影子状态，导致影子状态处于两个版本之间。
- **主线程等待**：主线程取批次时等待解码线程，帧时间抖动。
- **批次无界增长**：主线程长时间不取批次（例如加载画面），批次一直增长。
- **半状态构造**：成员形态变化后，旧块和新块混在一起，用不完整的块构造原生对象。
- **ACK 过早**：已接受版本被当作已应用版本回复 ACK，导致 Host 认为 Client 已写入。
- **旧表解码**：资源表或成员表还没安装，就按旧表解码了新帧。

## 完成内容

- **实现与编译**：解码、校验、影子状态和应用批次实现完成，纯编译通过。
- **验证安排**：验证在第 15 步完成。

## 未决问题

- 加载画面等主线程长时间不取批次的情况：批次上限是多少，Resync 的门限怎么定。这些只是运行参数，不是推进 gate。

**版本37 Laser负载**：按[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)只解码L5／L6的optimized／unoptimized最终点列及真实展示标量；L2／L4／L7／L8与Homing子块0／3／4预留，不等待空负载或构造虚拟投影。十九项Laser输入、六项Homing标量、indices／Bezier与系数不进入影子状态，两个Laser锚点撤销。两组独立count各自≤16384，有限值、预算、来源／版本及资源约束保持；不要求优化与未优化点数相等。旧indices i32类型记录已纠正，不再保留其4B线上codec。

**版本38 Effect负载**：按[Effect各臂与payload复核](effect-synchronization-review.md)FX0仅8B、FX1仅21B；全部FX2空预留不作为构造必收负载。已排除六参数、TearFlags与136描述不进入影子状态／validator／应用批次。Ghost只解码有效Sprite前缀，三投影u16 count必须同目标版本相同且≤4，由此重建原生i32 count，不另收count原值；present=0时三个count为0且各投影最短全零。视图134没有Offset／Scale负载，通用ANM2不变；源原生i32负数由第10步编码前拒绝。Trail节点已核完整64B，保持u32绘制参数原位及其余真实浮点、资源、预算校验，不恢复Rotation误名。

**版本39装备／Plus负载**：按[完整复核](player-equipment-plus-synchronization-review.md)ActiveItemDesc24B、Weapon16B、Whip135号18B、TemporaryEffect10B、TemporaryEffects136号disabled＋u16 count及完整列表；四主动槽／五Weapon槽存在性、顺序与动态形态不可缺。只解码真正上线的保留字段与Whip真实引用，已撤销八范围不进影子状态／validator。KnifePlus子块1空预留不等待负载；公共两nullable仍严格保持-1／0／1和0／高32位恰为1编码。Familiar D76只接0／1，新增外层disabled只接0／1；配置资源与type/id、原生f32、真实引用、计数及全局字节预算照常核对，不能把坏资源或来源失败伪装为空槽／空列表。
**版本40 Effect后续五项解码合同**：见[证据与落地](effect-next-five-evidence.md)。整数布局版本40；接受生成表的11个Effect布局，102不再包含113，113只接受141；未知／不匹配形态拒绝，不能按负载相同回退旧臂。FX0按16B解码State／Timeout到期帧／Rotation／Scale，FX1按13B解码LifeSpan／following／ParentOffset；不使用旧8／21B常数，FX2空预留不等待。Trail u16 count只接受0—127，count=128及以上拒绝，真实节点块最大8130B；present=0和present=1且count=0保持不同语义，不能截断或把多余字节丢弃。FX0／FX1浮点及引用／资源／目标版本／累计预算约束不变。137—141中不上线的VarData原值、函数地址和未知字节不进入影子模型，也不为其虚构线上validator；未知非空owner由Host报缺口，不在Client解码成任意新payload。未运行测试。

**版本43 NPC现行合同**：N0仍25B，末字节由DecodeNpcControlledUi仅接受0/2；1及其他值必须拒绝，不能查询本机设备/原版Online来补结果。143布局17/18仍空，NpcOverlaySprite144仍不含Scale，长度和其他依赖保持版本42。整数布局版本43直接比较，不兼容旧版本猜读。见[来源闭合](npc-control-ui-evidence.md)。
