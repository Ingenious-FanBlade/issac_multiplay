# 第9步第一项：公共报文结构定稿

**2026-11-18 平铺记录 L8（用户已决定，尚未实施）**：[设计文档](10-flat-record-encoding.md) 3.2 节、[规范第17节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)。已迁移布局取消块：记录头改为 `member u16 | layout u16 | record_bytes u32 | set_mode u8` 加字段集合，按字段增量上线，撤销下文 2026-11-11“保留 u32 块掩码和 12B 记录头”；协议版本 85。本文旧合同按时点保留，实施后以设计为准。

**2026-11-17 最终一致修订（E1—E9 已同意，尚未实施）**：[设计文档](eventual-consistency-revision.md)、[规范第17节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)。成员身份随帧携带，成员表不再可靠先行、ContextReady 等待与 `member_floor` 退役，普通帧取消 joint（E4）；AppliedAck 改为接收确认（E1）；窗口改为 N=8／B=64KiB（E2）。本文下列旧合同按时点保留，实施后以修订为准。

**2026-11-16 布局 83**。普通 World 发送窗口常量为 2，Client 保留一代成功前代解码根。公共头中的整数布局版本用于握手和逐帧兼容性检查，布局 82 与 83 混用时拒绝连接，线上字段形状保持原合同。[发送和接受语义](13-client-world-cadence.md)

**2026-11-11 版本66补充**：仅新增[world/body独立zstd](9-world-body-zstd.md)的可选共同协商及收发支持；固定头、状态字段与ACK／恢复均保持，普通64KiB同时保护压后及解压完整消息。其余布局／资源／动画／音频／预算不改。

**2026-11-11 第9步收尾，版本65**：[现行预算／容量／时间](9-budget-capacity.md)覆盖原75%额度和“后续核算”表述：用户允许模组占90%，持续9Mbps，三端各375000B/s。准确成本含ICSR／ICRF／IP／UDP与实际中继，初建／恢复只借本Peer停用流；64KiB／32MiB完整消息先扣40B信封。公共头尺寸保持，细化等待、episode、账本／事件／缓存／池配额，静态设计负载不等于实机通过。

**2026-11-11 第五项完成，版本64**：[最终接受与恢复合同](9-acceptance-recovery.md)覆盖下方版本59时点的“详细规则／完整消息后续”：完整Frame、成员表、ContextReady／AppliedAck／Resync编解码及纯值状态机已实现；表／来源／完整候选／原生owner由10—13步接线。公共固定头保持，严格整数版本；旧base补原ACK、实际发送账本、恢复期间旧ACK不能解除恢复。普通Resync不归零；只有排空退休旧复制route后的显式新代次重绑定才可重置epoch，资源身份／编号历史仍完整保留。

**2026-11-11 第四项实现，版本63**：[完整本地资源协议](9-resource-protocol.md)完成table/kind矩阵、typed payload、有限recipe、固定图形规则、资源消息和不可变目录／闭包。builtin shader使用1B官方类型而非路径；309／366补16B ColorOffset上下文，分别212／213B；公共固定头保持。像素／blob禁用，运行接线仍待10—14步。下方版本59的资源payload“后续”表示原定稿时点，当前由版本63合同覆盖。

**2026-11-11 第三项实现，版本62**：[显示外层](9-presentation-envelope.md)已实现414—417及显示区域的编解码、用途／pass数字规则、真实主体和贡献定位、缓存epoch及事件水位／冲突检查，尺寸仍42／32／16／40B。Reference只复用相同epoch的已提交缓存，换表以Replace重新映射并保留原produced_revision；完整Frame及资源／图形允许集和运行接线仍待后续。下文“后续”与版本59／61实施状态保留其原时点。

**2026-11-11 资源职责修订及版本61实施**：[本地生成合同](local-media-presentation.md)覆盖本文旧通用内容块、Host切房／hallucination图及文字图必需下行的设计。[第二项实现](9-domain-body-codec.md)已迁移文字／有限样式和本地历史结果，374收窄为BossVersusResult，正文层拒绝305／306及资源kind5；公共固定头字节与Kind24编号保留。完整本地资源payload、允许集及运行接线仍待后续，不启用原始媒体blob／像素payload。

2026-11-11，用户采用本轮公共报文方案，并明确保留 **u32 块掩码和12B记录头**、守住Host三端总上行10Mbps预算。本项登记固定封装、身份、共同提交、事件和预算语义；不把它等同第9步完整编解码或第10—13步运行接线完成。

唯一源为 [`wire.json`](../../../../mod/binding/state/wire.json)，本合同定稿时布局整数版本 **59**；正文首次实现递增至60，本次本地生成迁移递增至61，公共结构字节尺寸保持。第7步已登记的 ResourceEntryHeader304／ResourceBlobChunkHeader306 在原源 [`room.json`](../../../../mod/binding/state/room.json) 中更新，不另建同义头。固定头及常量的正式字节／数值表见[第9步生成索引](../../../../mod/binding/layouts/09-wire-format/README.md)；所有表由同一次生成产生。原生对象账本、UNKNOWN、来源Gap和运行adapter缺口保持。

## 1. 封套、公共上下文与消息编号

保留现有 ICRF 64B分片头、ICSR 40B逻辑消息信封、1200B报文／1136B分片正文及合包、补片、zstd能力。公共正文不重复写连接、路由、消息种类或长度；完整长度取信封，分片收齐后才解码逻辑消息。旧树形 Range 正文不作为新状态格式的兼容分支。

每条本合同状态、表、blob、ContextReady、AppliedAck、Resync消息都以 **StateCommonHeader408（50B）** 开始：`layout_version:u16 + session/run/floor/dimension/room/scene:各u64`。这六项沿用 WorldScope 的稳定身份／代次语义，不能替换为地图格子、维度编号或原生地址。三端内容可共享，但路由及逐Peer目标不因此共享身份。

| Kind | 名称 | 通道与职责 |
|---|---|---|
| 21 | StateFrame | 单world增量走WorldDynamic，单body走PlayerFast；完整初建／恢复、结构配对与ordinary body-joint走Baseline可靠承运 |
| 22 | MemberTable | Context可靠发送成员元数据，不直接创建／移除原生对象 |
| 23 | ResourceTable | Context可靠发送不可变typed资源声明 |
| 24 | ResourceBlobChunk | 既有数值／固定头保留；当前不启用通用原始媒体正文，不作为缺资源兜底 |
| 8／9／10 | ContextReady／AppliedAck／Resync | 保留既有Kind编号和控制职责；新正文分别按418／419＋420／421编码 |

新Kind的数值只在源与生成常量中登记。本项不修改旧stream_codec的KnownKind、Lane分派、握手、合包或运行解析；这些入口必须在第9步后续一并接通，不能仅扩大KnownKind便接受未完整实现的消息。新状态能力使用整数布局版本协商，版本不符明确失败，不猜读旧Range。

ScenePrepare的场景准备职责由成员表、资源表和base=0目标承接；既有路由退休／取消／停止保留。改变路由不伪造run或成员身份，原版Online与Isaac Sync继续隔离。

## 2. StateFrame与两个分段

消息正文顺序固定为：`公共头50B + StateFrameHeader409(2B) + 1或2个完整分段`。Frame头为`section_count:u8 + flags:u8`；一个分段flags=0，两个分段flags=1，按world随后body排列，不允许重复stream。双分段必须共同准备、共同提交、共同确认，不允许安装其中一段后再等待另一段。

每段为：`StateSectionHeader410(52B) + record_count条记录 + group_count个结果组 + event_count个音频事件`。不再给后三个数组额外添加容器计数。段边界由三个计数、各项精确长度和生成布局顺序得到；必须恰好消费整条消息。

| +0x00 u8 stream：0 world，1 body | +0x01 u8 flags：仅bit0 baseline | +0x02 u32 member_epoch | +0x06 u32 resource_epoch | +0x0A u32 base_version | +0x0E u32 target_version | +0x12 u64 source_revision | +0x1A u64 presentation_batch | +0x22 u32 game_frame | +0x26 u16 record_count | +0x28 u16 group_count | +0x2A u16 event_count | +0x2C u64 event_watermark |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

- 继续只有world／body两条确认序列；body按Peer覆盖其全部身体，不按身体另计ACK。传输通道与确认序列分开，可靠承运的body仍确认body版本。
- 普通帧要求base等于本端该序列已成功安装版本；target必须更大。baseline当且仅当base=0，包含本序列完整合法目标，不是新格式；允许在恢复时替换已有目标，但target仍必须前进。普通Resync不能把Host target计数重置为0。
- Host在已理解窗口封存后分配不回绕的u64 source_revision，先于任何Peer发布存在；同ordinary封存的配对world/body指向同一来源窗口。各Peer的base/target由发布端映射，不能把缓存来源改写为某Peer的target。
- presentation_batch是配对显示来源身份，不是第三条传输／确认序列；值不变与来源有效分别判断。缓存保留实际produced_revision，不能只因新帧GameFrame或批次号变化便重造来源。
- GameFrame保留原生u32位模式，不用它判断消息新旧，也不扩成u64模拟tick。原生计数按既定模运算／锚点还原；恢复导致来源时间线改变时撤销受影响候选，不能当作网络乱序继续拼旧显示。
- baseline和结构变化必须携带完整配对的P6／P7、同拓扑P0及必要结果。body保持成员／资源／明确绘制拓扑等窄依赖；不等待完整地图或无关HUD。ordinary body-joint用两个分段显式表达，不能把body块伪装成world块。
- 空段可以推进时钟与版本，但仅在其全部必需状态／结果均可合法继承时成立；缺瞬态来源不能借record_count=0绕过。body的event_count及event_watermark必须为0。

## 3. 记录头与块

**StateRecordHeader411保持12B**：`member:u16 + layout:u16 + block_mask:u32 + record_bytes:u32`。record_bytes只计后随块正文，块按编号升序消费，条件、固定槽及容器按同源生成布局编码，不再附任意字段字典。

一个对象布局最多32个块号0—31，包含父类和显式空预留块；空预留块不发送、不置位。当前323个旧布局中最大为Player197的25块（0—24，含1空预留），眼泪125为18块（14块有负载），保留位图有实际余量。这不限制一帧的对象数：100颗眼泪可以是100条独立记录，各有自己的u32掩码，能否同帧取决于总字节与发送预算。

一段内同一member不得出现两条记录；只允许所属stream的真实块。baseline收齐本序列全部必要块；增量未携带块只能从明确base继承。shape/use改变、来源／资源／拓扑失效时不得继承旧块。产生显示结果的同一投影只保留一份编码，不能同时放成员记录和结果组重复下发。

## 4. 成员表、资源表与blob

两表共用 **StateTableHeader412（10B）**：`epoch:u32 + previous_epoch:u32 + entry_count:u16`。previous=0表示完整快照；非零必须与当前用于构建新快照的前一版本一致。epoch非零且前进，不允许原号改内容。重复同一传输幂等，冲突快照失败。

**StateMemberEntry413（33B）**：`member:u16 + op:u8 + space:u32 + layout:u16 + id/use/shape:各u64`。run由公共头继承，Key仍为run/space/id；space不窄化。op=0加入、1换形、2离开；完整表仅包含加入项。离开项带原身份／代次和布局，不能只凭member删除另一个代次。

表安装是后台构建不可变元数据快照，不立即删除当前已提交影子、原生对象或仍合法的旧画面。成员操作随完整目标在主线程提交；换形不把旧块拼到新布局。引用按精确member_epoch解析，不用“最新表”覆盖在途目标；已退休身份不得借旧快照重新激活。成员号回收等待所有仍存活Peer越过离开版本并退休旧在途引用，断开的Peer随路由退休退出等待。

**ResourceEntryHeader304由7B改为8B**：`table:u8 + index:u16 + kind:u8 + payload_bytes:u32`。资源字段若生成布局已固定table，仍只发送u16 index；表项、blob及公共依赖明确传table。FFFF表示未选择资源，不占表项；非Null尚未ready不能改为Null。

table的固定编号0—7依次为anm2-animation-table、image-table、anm2-files、item-config、entity-configs、room-config-visual、shader-table、audio-table。kind与table不是同一编号空间，0—9的typed分派tag在源中登记；**完整payload、table/kind矩阵、shader／blend／sampler与各用途允许集由第9步后续完成**，有固定头不等于已经可接受任意tag正文。按2026-11-11[本地生成合同](local-media-presentation.md)，本次后续分派以本地资源声明、配置与有限生成描述为主；已有像素tag／blob头不加入当前接受允许集，文字缓存与本地历史图不再声明为Host媒体内容。

资源表是同session/run下的不可变内容目录，可跨当前场景复用已准备内容；公共WorldScope仍标明此消息服务的目标。变更分配新号，同run不回收、不原号覆盖；恢复全量表也必须保持已知内容身份一致。资源号耗尽不能靠普通Resync原号复用，需明确失败／新的合法命名空间生命周期。

**ResourceBlobChunkHeader306由14B改为15B**：`resource_epoch:u32 + table:u8 + index:u16 + offset:u32 + bytes:u32`，随后恰好bytes字节。offset针对声明的完整内容；先检查乘加、上限、缺块及重复内容冲突。相同重传可幂等复用，冲突、错空间或越界失败。表收到、内容收齐、依赖图无环就绪、本地物化成功分别记录；不因表头到达便ACK状态目标。单chunk原256KiB是保护上限，实际分块仍受速率、突发、时间和首次准备总额约束。

上述306字节是保留的生成事实和旧内容块设计；版本61正文入口明确拒绝306及305像素描述，304的基础kind校验排除5。现阶段不实施通用图像／音频分块传送；冷连接发送描述／文字／当前状态，Client使用本地资源生成或播放。真正必需的文字／样式／本地资源未就绪仍不得提前ACK；取消的切房surface与Host反馈图不再形成Wait。文字与历史结果字段已沿唯一源迁移，完整资源payload、精确用途允许集及运行消费者仍须后续接通，不能把正文通过当目标已完整。

## 5. 结果组、缓存与依赖

**StateResultGroupHeader414（42B）**的生成表固定op／flags、owner_member、subject_member、purpose、layout、owner_instance／generation、produced_revision、dependency_count／contribution_count、payload_bytes。组键由完整scope、owner及其显示实例代次、subject完整身份和purpose共同组成；member_epoch解析真实use/shape。实例id由Host在同session/run内唯一分配给该用途实例，不能把同一实例代次改绑另一个subject；同一原生owner的不同身体／用途分别分配实例。它不是地址、roster槽或Peer序号，因此依赖条目可由owner／用途／实例代次唯一解析到完整subject身份。

purpose复用已登记具名结果布局号；同用途多次实例由owner_instance/generation分开。layout必须与用途匹配，不允许报文自选adapter。音频voice组以382为用途，owner_instance/generation对应voice_id/generation；声音事件payload为383，并不改变voice用途。

- op0 Replace：正文依次为32B依赖条目数组、16B贡献定位数组及已有具名结果布局的完整值。payload_bytes覆盖这三部分；typed值本身的现有容器count仍保留。
- op1 Reference：显式引用同组键、layout与produced_revision的已提交不可变缓存；可以重述当前必要依赖，contribution_count必须0，不重发typed值。只有合同允许缓存且来源／全部依赖仍有效时成立；瞬态结果不能借此改归新批次。
- op2 Retire：该用途实例明确退休；dependency_count、contribution_count及payload_bytes均为0。不能把未正常产生或读取失败编码成退休。
- 增量未列出的组仅能从明确base继承，且依赖必须继续成立。合法隐藏／空集按typed布局表达，Gap／Wait／失败不能折为它们。源资格失效即撤销候选；旧强引用仍在不等于允许新world/use借用。

**StateDependencyEntry415（32B）**：`kind:u8 + table:u8 + member:u16 + purpose:u16 + index:u16 + revision:u64 + instance:u64 + generation:u64`。按kind解释，不适用字段必须规范空值：table=FF，member/purpose/index=FFFF，三个u64为0。

| kind | 有效字段及检查 |
|---|---|
| 0 Member | member；按本目标member_epoch核身份与生命周期，其余规范空值 |
| 1 Resource | table/index；在本目标resource_epoch内解析并检查实际依赖ready，其余规范空值 |
| 2 PlayerTopology | member/revision；revision须非零且高32位为0，与已安装P6及本P0的u32拓扑相等 |
| 3 ResultGroup | member为owner、purpose、instance、generation、revision为实际produced_revision；匹配完整已提交组身份；table/index为空 |
| 4 PairedTarget | index为0 world或1 body、revision为非零u32 Peer目标；只允许生成合同明确要求共同提交或严格配对的用途，其余为空 |

同类同键依赖不得冲突。组依赖和共同提交范围由本地正式合同约束，报文不能自行省掉必需依赖，也不能任意给所有body增加完整world目标依赖。世界几何、描述拓扑、L2／焦点和必要显示结果继续共同提交；身体HUD、History／Stat的结果与其body目标／绘制拓扑配对，本端详细主体仍取本端已确认控制组。

## 6. 贡献定位与有序payload

**StateContributionEntry416（16B）**：`contribution_id:u32 + parent_id:u32 + pass:u16 + field:u16 + element:u16 + flags:u16`。id按数组顺序从0稠密分配；FFFFFFFF表示组根，其他parent必须是先行项；父链深度≤16，不能成环。pass只取当前用途允许的固定集合，flags当前为0。

field是父节点生成布局的字段索引，element是它已有容器中的实际有序索引；FFFF分别表示当前根值或非容器值。父链定位嵌套／递归上下文；定位必须落到当前typed payload，不允许越过count、指向不存在的条件分支或重复占用同一贡献位置。不同pass可以声明对同一已核内容的独立贡献身份，保持Host提交顺序。

365／366已有branch／layer／track以及资源／姿态／图像空间字段，公共定位不重复发送这些值；标量随机结果的分支由固定用途、pass和父链定位。贡献payload只编码一次。versus逐layer、Nightmare逐项及Pickup／Familiar多pass分别保留，不能只留最后值。Hallucination按本地生成合同保留必要world参数，历史采样／叠加由本端adapter管理，不再要求Host反馈图或逐次像素贡献一致。Client不按本地第N次访问猜Host随机结果身份；重复展示同一批不重新抽取／推进玩法事件。

## 7. world音频事件与可靠语义

**StateAudioEventHeader417（40B）**：`event_sequence:u64 + logical_frame:u32 + owner_member:u16 + purpose:u16 + voice_id:u64 + voice_generation:u64 + clear_scope:u8 + flags:u8 + layout:u16 + payload_bytes:u32`，随后是既有AudioWorldEvent383的精确1—19B值；不再套12B成员记录头。

event_sequence在同session/run内稳定、递增，只有已确认world逻辑事件才分配；本地硬件提交重试不另造Start。logical_frame是原生逻辑时点，不是播放游标。voice身份和代次独立于soundID、sample资源、原生handle及自然后端生命周期。flags为0，layout必须383。

kind0 Start、1 Retrigger、2 Stop、4参数调整必须clear_scope=0且voice_id/generation有效；只有kind3 Clear可声明范围：0精确voice、1当前owner的该用途、2当前world的该用途、3当前run的该用途。非精确voice Clear的voice_id/generation为0；world/run范围owner_member为FFFF，owner范围须指定真实owner。purpose必须登记，范围均只影响C/S world voice，不能清个人UI／设备音频，不能省略范围成为隐式全局Stop。

普通world仍可latest-only发送，但**未确认事件不能latest-only丢弃**：Host按Peer水位保留未确认事件，后续帧带齐从已确认水位之后到本目标event_watermark的连续事件。客户端按完整scope及事件号去重，按序处理；同事件号内容冲突失败，存在缺口则不提交／不ACK。候选淘汰不退休未确认事件。event_watermark只有原生提交成功才前进，不能单独ACK事件或另建Presentation序列。

Baseline以当前world音频状态恢复仍有用途的voice，水位明确建立新的起点，不重播该起点以前的历史one-shot；重复状态不重启voice。事件积压超过共享内存／速率／恢复时间约束时显式请求完整恢复，不能无声跳水位继续称增量完整。个人静音、设备失败、抢占及自然硬件完成不广播world Stop。音乐逻辑包络、seek／segment、重连续相位及视频音轨来源Gap继续保留。

## 8. 确认、基线、回绕与接受边界

布局82的 ContextReady 为公共头 50B 加布局 418 的 12B，依次为 `member_epoch:u32 + resource_epoch:u32 + member_floor:u32`。`member_floor` 在 1 到 `member_epoch` 之间，用于释放编号租约。资源代次只确认元数据可解析。AppliedAck为公共头＋419的2B＋1或2个420的21B条目：`stream:u8 + target_version:u32 + member_epoch:u32 + resource_epoch:u32 + event_watermark:u64`。双分段成功后一次回报world、body两项及joint标志；不回报半个共同目标。

Host只接受当前connection/route、完整scope和实际已发送目标的确认，表epoch、事件水位须与该目标一致；不能用Peer回报任意大版本退休未发送内容。重复确认幂等，已确认版本只前进。接收完成／解码接受／表安装／资源ready／原生提交分别计状态；原生提交失败不ACK，也不把已接受影子当继续差分的已安装基线。

Resync为公共头＋421的11B，说明stream mask、具名原因及本端已安装world/body。普通恢复保留Host目标单调性，以base=0完整重建所需序列；结构有关时共同恢复。成员／资源epoch或u32目标将耗尽时，先退休旧路由和候选，再建立新的复制代次并完整准备；不能只发普通Resync后同路由从1复用旧版本。资源index仍受同run不复用约束，route变更不自动解除。

表快照、已提交目标、构建候选和待安装内容均有界，不能无限缓存缺表或拼凑跨代目标。world可以只保留最新候选等待依赖；单body未就绪即丢，joint按整个共同目标等待／淘汰。详细错误分类／来源用途允许集／纯值reader与writer已由第9步第二至第五项完成，实际原生reader／writer按10／13步接线；容量／时间以版本65预算合同为准。

## 9. 带宽、消息保护与准备预算

| 范围 | 已定持续预算 | 说明 |
|---|---|---|
| Host向三个远端总上行 | 10,000,000 bit/s | 不是每Peer10Mbps；物理链路统计含实际IP／UDP／中继封装 |
| 三端模组持续总额 | 1,125,000 B/s＝9Mbps（90%） | 所有通道、ACK、重传、状态、表、资源、准备和恢复共同计费 |
| 每Peer | 375,000 B/s | world150000＋body150000＋control25000＋reliable50000 |
| world 30Hz／body 60Hz | 平均5000B／2500B每次 | 含分片／消息公共封装；不是每次固定额度或性能通过结论 |
| 首次准备 | 每Peer≤1MiB、≤10秒 | 状态、表、资源和协议封装合计；发三端按三份发送流量计费 |

实际发送按最终合包／分片产生的字节计费，包括重复传输；共享编码只省CPU和内存，不能把三个Peer的网络成本记成一份。可见的网络／中继额外开销纳入链路统计和限速余量；不能用压缩前体积冒充实际线上带宽，也不能忽略解压内存保护。

总／逐Peer／逐通道桶共同生效，保持既有总24KiB、逐Peer8KiB、world/body各4KiB突发上限。令牌按分片消费，大逻辑消息可跨轮次发送，收齐后才原子提交；令牌不足不阻塞Authority。持续积压须定位真实字节、变化粒度和来源／资源表达，不能仅延期后声称满足延迟与频率目标。

进一步核对现有EncodeBlob：非可靠world/body实际已受64KiB未压缩逻辑消息保护，并非32MiB；本合同沿用该保护界限，**64KiB也不是普通帧发送额度**。旧32MiB只描述可靠大消息／解码内存保护及旧布局理论容器预算，不能作为正常world产品帧许可。单条头部／布局预算、整逻辑消息、解压内存和实际速率须同时满足，不能把普通超限目标改走可靠通道绕过。

首次准备及合法恢复的可靠目标按完整目标和其资源闭包计费，不截断／降质／漏发必要内容。必要完整目标超过实际预算时报告真实未满足项；版本65已按当前布局完成[静态账本及有效峰值／积压期限](9-budget-capacity.md)，第11步落实实际调度，第15步唯一整体验收验证。这里没有把“保护上限可容纳”写成频率、带宽或准备时间已达标。

## 10. 失败方式与本项交付

- 把32块当作每帧32对象；或发预留／未知mask位，导致错读后续记录。
- 表快照到达便退休已提交对象；按最新表解码旧epoch，使member序号命中新use/shape。
- 双分段只提交／确认一段；或把快源超限body改计world，造成P0/P6错配。
- 混用source_revision、Peer target、GameFrame和显示批次；缓存内容没变便伪造新生产来源。
- 资源序号跨表混用、内容原号覆盖，或收到资源声明便当作物化成功。
- 将缺源／资源Wait编码为空、Reference跨world/use借旧结果、Retire冒充读取失败。
- 贡献父链循环／错字段／越界索引／本地访问次序匹配，丢失多pass及递归结果。
- latest-only候选淘汰同时丢未确认事件；事件水位跳缺口；重复状态重启voice或Clear影响个人音频。
- 普通Resync重置计数、旧route ACK退休新内容，或来源接受成功便提前AppliedAck。
- 用32MiB保护替代发送额度，按共享编码少计三端流量，或把Context／重传移出10Mbps统计。

本项交付为：版本59唯一源、14个公共固定头408—421、更新304／306、公共Kind／资源空间／用途／预算常量及同源C++只读目录和单行文档。生成器支持公共常量和第9步目录。这里只完成第一项公共结构定稿；完整typed资源payload、用途允许集、严格接受规则和运行编解码仍是第9步后续工作，Host／Client运行接线仍按10—14步完成。

本次完成静态审阅与普通生成；`cmake --build build/repentogon --config Release --target isaac_cs_state -- /m:2 /v:minimal /nologo`的Release／Win32状态库纯编译成功，配置中`ISAACCS_BUILD_E2E_TOOLS=OFF`。没有运行游戏、E2E、单元测试、独立校验脚本或任何提前试跑，也没有安装产物到游戏目录。第15步仍是唯一统一整体验收；不设gate、不校验哈希、不新增tx编号，也不创建推进artifact。

## 11. 被本合同替代的草稿

原9.3的26B单帧头、9.5的38B成员项、资源7B头／14B blob头及body确认粒度、GameFrame宽度未决，均是2026-11-10及以前的设计时点。本次以50B公共上下文、2B分段外壳、52B分段头、33B成员项、8B资源头、15B blob头和按Peer body／u32 GameFrame替代；12B记录头及u32掩码保持。旧源304／306版本57的生成事实、失败和UNKNOWN仍按原时点保留，不把新合同回写成当时已实现。
