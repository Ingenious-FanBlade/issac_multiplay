# 9：线格式

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。以下版本46／草稿／待生成表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

定义原版状态帧、记录、块、成员表、资源表的二进制格式，以及 Client 的有界校验规则。传输层（通道、分片、补片、合包、zstd、确认）沿用现有实现，只替换正文。第三方 Mod 值编码已迁至[独立后续方案](../mod-adaptation.md)，不属于本步实现或完成条件。

## 输入事实

### 现有传输层

- **路由头**：固定 64 字节（`mod/src/replica/stream_codec.h:20-28,57`）。
- **报文与分片**：报文最大 1200 字节，分片正文 1136 字节（`stream_config.h:21`）。
- **单条消息上限**：world 32 MiB，fast 64 KiB（`stream_config.h:22`）。
- **通道**：`Control`、`Baseline`、`Context`、`WorldDynamic`、`PlayerFast`、`Input`。其中 `Baseline` 和 `Context` 是可靠通道（`stream_transport.cpp:62`）；`WorldDynamic` 和 `PlayerFast` 只保留最新。
- **消息种类**：`ScenePrepare`、`Range`、`ReceiveAck`、`ContextReady`、`AppliedAck`、`Resync` 等（`stream_codec.h:6`）。
- **现在的正文**：`Range` 的正文是带字段编号的树形编码，固定 payload schema=2（`stream_codec.h:47`），必须先还原成范围模型才能使用。
- **身份的数据类型**：key 是 `{run u64, space u32, id u64}`（`mod/src/game/client_objects_native.h:12-14`）；use 和 shape（revision）都是 u64。

## 设计

### 9.1 通用编码规则

- **字节序与对齐**：小端，紧密排列，不对齐。解码一律用 memcpy 读取。
- **基本类型**：
  - 浮点按原样位模式发送。Client 校验时拒绝 NaN 和无穷大，除非布局声明该字段允许。
  - bool 写成 u8，只允许 0 或 1。
- **可变长内容**：
  - 字符串写成 `u16 字节数 + 原字节`。
  - 容器写成 `u16 元素数 + 元素`。
  - 两者的上限都来自布局，超出上限即为整帧失败。
- **标识编码**：
  - 实体或格子引用写成 `u16 成员序号`，`0xFFFF` 表示 Null。
  - 资源写成 `u16 资源序号`，`0xFFFF` 表示无。
- **不计算哈希**：兼容性只靠布局版本号；完整性依赖传输层现有的分片和长度校验。

### 9.2 消息种类

| 种类 | 通道 | 用途 |
| --- | --- | --- |
| `StateFrame`（world） | `WorldDynamic`；Baseline 走 `Baseline` | 普通世界状态帧 |
| `StateFrame`（body） | `PlayerFast` | 逐身体状态帧，每个合格身体一条记录 |
| `MemberTable` | `Context` | 成员加入、形态变化、离开 |
| `ResourceTable` | `Context` | 资源序号与路径的对应 |

- **取代关系**：现有 `Range` 的树形正文由上表取代。场景准备 `ScenePrepare` 改为“成员表 + 资源表 + base=0 的状态帧”。
- **沿用的消息**：`AppliedAck`、`Resync`、`Cancel` 等控制语义不变。`AppliedAck` 中的确认量改为“各序列已安装的目标版本”。

### 9.3 状态帧头（26 字节，草稿）

| +0x00 u16 layout_version：布局版本，两端必须相同 | +0x02 u8 stream：0=world 序列，1=body 序列 | +0x03 u8 flags：bit0=base 为 0 的完整帧，其余位必须为 0 | +0x04 u32 member_epoch：帧中成员序号依据的成员表版本 | +0x08 u32 resource_epoch：帧中资源序号依据的资源表版本 | +0x0C u32 base_version：此帧以哪个已确认版本为起点，0 表示从空状态开始 | +0x10 u32 target_version：应用此帧后达到的版本 | +0x14 u32 game_frame：采集时的 Host 游戏帧；Client 用作展示时钟，由帧锚点还原计数（第 13 步 13.6），也用于诊断和插值 | +0x18 u16 record_count：随后的记录数 |
|---|---|---|---|---|---|---|---|---|
### 9.4 记录头（12 字节，草稿）

**Player P6—P8与C/S结构（2026-11-09，未生成）**：[完整草稿](5-player-p6-p8.md)规定world物理16／17／18分别为P6的`111+4C` B、每身体21B P7、P8的disabled／6或10B效果／残影／当前记录及ANM2分支。效果仅资源身份为type1／3／4且id704时附4B cooldown；其余按6B消费，不能套通用14号旧10B。P7不带sub／replaced／703四槽，group引用纯C/S空间10、participant空间9，group另有`21+2N` B有序active_anchors记录；真实layout号待注册生成，不创建原生对象。P6／P0采用统一Player绘制拓扑revision保护跨身体借用，不改变本节12B记录头／32位mask；当前46不接受这些草稿。

**Player P3—P5后续布局（2026-11-09，未生成）**：按[定稿](5-player-p3-p5.md)P3为70B前缀＋有界容器（格式下界88B），P4固定96B，P5为u8低5位存在mask＋按槽序的`u16 view_layout＋固定分型值`，总1—91B；每个view按注册布局和真实type精确消费，不能由报文提供偏移／native大小。二级布局标记仍用整数号，实际编号待生成，不改这里12B记录头／32位块mask。Player物理13／14／15为world，body不得夹带。当前版本46不能提前接受这些草稿负载。

| +0x0 u16 member：成员序号 | +0x2 u16 layout：布局编号，必须与成员表中登记的一致 | +0x4 u32 block_mask：本记录携带的块，第 n 位对应块 n | +0x8 u32 record_bytes：记录头之后的正文字节数，解码必须恰好用完 |
|---|---|---|---|

- **块的排列**：正文按块号从小到大依次排列。
  - 定长块的长度由布局给出；变长块以 `u16 元素数` 开头，元素布局见第 2—8 步。
  - 一个布局最多 32 个块。超过时由生成器拒绝，要求重新拆分。
- **块的格式**：块的正文就是该块的状态布局，例如[总计划 5.1](README.md#51-示例entityeffect草稿) 的 EntityEffect 子类块。

### 9.5 成员表项（38 字节，草稿）

| +0x00 u16 member：成员序号 | +0x02 u8 op：0=加入，1=形态变化，2=离开 | +0x03 u8 space：身份空间 | +0x04 u16 layout：布局编号 | +0x06 u64 run：身份 run | +0x0E u64 id：身份 id | +0x16 u64 use：同地址获取次数 | +0x1E u64 shape：形态版本 |
|---|---|---|---|---|---|---|---|

`MemberTable` 消息结构为：`u32 epoch` + `u32 前一 epoch` + `u16 项数` + 若干表项。

- **按 epoch 安装**：Client 必须按 epoch 顺序安装；前一 epoch 不符就请求 Resync。
- **序号复用**：成员离开后，它的序号要等所有 Peer 的已确认 epoch 都越过该次离开后，才能重新分配。

### 9.6 资源表项（草稿）

| +0x0 u16 index：资源序号 | +0x2 u8 kind：0=anm2，1=贴图，2=shader，3=其他具名资源 | +0x3 u16 length：路径字节数 | +0x5 u8[length] path：资源路径原字节 |
|---|---|---|---|

`ResourceTable` 消息结构为：`u32 epoch` + `u32 前一 epoch` + `u16 项数` + 若干表项。资源序号只增不改：同一 run 内一个序号始终对应同一路径，从而避免旧帧中的序号被误读。

版本32的资源字段允许`0xFFFF`（`kNullIndex`）表示Host未选择资源，不占资源表项。ANM2文件选择据Host loaded形成这一可空资源号，不再追加独立loaded布尔。已选择非Null但Client尚未准备／失败时仍为该目标资源，不能改成Null或提前AppliedAck；图层路径与实际Image资源号保持独立，具体身份限制见[动画／资源复核](animation-resource-synchronization-review.md)。

### 9.7 当前格式边界

原 Mod 值编码草稿已移至[后续方案](../mod-adaptation.md)。当前只接收已登记的原版类型布局，不引入 Mod 自描述值、共享节点图或 Lua 状态记录；未知布局按 9.8 拒绝。保留本节编号以免后续校验规则的引用错位。

### 9.8 Client 校验与接受规则

| 检查 | 失败处理 |
| --- | --- |
| 版本相符：layout_version 不同 | 断开会话，报布局版本不符 |
| 表版本就绪：member_epoch 或 resource_epoch 还未安装 | world 帧暂存最新一帧，等表安装后再处理；body 帧直接丢弃 |
| 帧版本连续：base_version 不是本端已安装的版本 | 丢弃；仍未收敛时由 Resync 恢复 |
| 帧未过期：target_version 不大于已安装版本 | 丢弃（重复或乱序） |
| 记录与成员表一致：成员序号未登记，或布局编号不符 | 整帧拒绝 |
| 字节数一致：block_mask 含布局未定义的块，或 record_bytes 与解码消耗不符 | 整帧拒绝 |
| 字段取值合法：容器超上限、枚举越界、浮点非有限、bool 不是 0／1 | 整帧拒绝，按具名原因记录 |

- **整帧接受**：一帧要么整帧接受，要么整帧拒绝，不部分接受。
- **AppliedAck 的时机**：接受后先更新影子状态，再由第 13 步完成原生写入；写入成功后，才对该 target_version 回复 `AppliedAck`。

### 9.9 版本序列

- **独立序列**：world 和 body 是两条独立的版本序列。
- **body 序列的范围**：body 序列按 Peer 计数，覆盖该 Peer 收到的全部身体记录。如果用户或后续设计要求每个身体独立确认，需要在本步定稿前改为按身体分别计版本。
- **body帧依赖**：依赖成员表／资源表，不笼统绑定整个world的具体版本。第5步[P0—P2设计](5-player-p0-p2.md)增加Player的窄依赖：P0携带render_topology_revision，须与已安装P6有序绘制目录版本相等；未就绪或已越过该拓扑时丢该body候选，不ACK、不按旧count／顺序安装。形态变化（换角色、加入或离开）仍必须先经过成员表。这是待Player布局生成时落实的设计，当前布局版本仍46。

## 改动范围

- **新增**：状态帧编码器与解码器，成员表、资源表的编码器与解码器。
- **修改**：`stream_codec` 新增消息种类，路由头不变。
- **退役**：`Range` 的树形正文、`EncodePayload`、`DecodePayload` 等，在第 14 步移除。

## 失败方式

- **记录越界**：`record_bytes` 与块长度不一致时没有拒绝，导致读越界或把后续记录错位解释。
- **序号过早复用**：成员序号在旧帧还会被引用时就被复用，Client 把旧帧写到新成员上。
- **资源序号复用**：同一序号被重新分配给另一路径。
- **body 帧早于成员表**：成员表还没安装就接受了 body 帧，造成身体错配。
- **版本回绕**：u32 版本号回绕。60 Hz 下大约 2.2 年才会回绕，但仍要在会话内检测，回绕前强制 Resync。
- **压缩前后长度不符**：zstd 解压后的长度与声明不一致。沿用现有的解压长度检查。

## 完成内容

- **单行表格**：状态帧头、记录头、成员表项、资源表项都由生成器输出单行表。
- **接受规则**：校验和接受规则定稿。

## 未决问题

- body 序列按 Peer 计数，还是按 Peer×身体分别计数。
- 帧头中 game_frame 是否需要 u64。
**版本40 Effect布局引用**：见[后续五项处理](effect-next-five-evidence.md)。记录与帧头格式不变，整数布局版本递增为40；新增137—141及102移出113由生成布局表分派。所有Effect FX0=16B、FX1=13B，FX2空预留，Trail节点块为u16 count＋count×64B、count≤127（最大8130B）。不将原生VarData／holder地址塞入扩展块，不复用旧8／21B或16384上限。其他容器的独立上限不受影响，真实记录／帧字节预算保持；未运行测试。

