# 第9步第三项：显示批次外层封装

**2026-11-12 显示委托修订**：

- 本项整体退役：显示批次包络 414—417 不再使用。
- 原文、布局与事实保留，作为历史记录。
- 见[修改方案](client-display-delegation.md)第 7 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-12 HUD代码迁移，版本70**：315依赖435；435是独立body具名结果，沿原world/body目标和确认提交。普通图标条件分支不得继续要求重复Image／center；特殊分支、完整输入及精确资源仍须ready。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-11 第9步收尾，版本65**：[预算合同](9-budget-capacity.md)统一整帧1024组／8192依赖／8192贡献／4096事件、完整目标缓存与事件年龄／总额，固定封装保持。带宽改模组9Mbps；动态显示、实际资源、完整候选与真正原生提交继续后续步骤，不早ACK。

**2026-11-11 第五项交接，版本64**：[完整消息与接受](9-acceptance-recovery.md)承接两段边界、逐段窄依赖及连续事件；joint的两段依赖／贡献暂存区互不覆盖，整帧累计数量和真实字段访问预算。编码器可返回真实访问量，避免完整Frame为累计预算重复解码结果。公共尺寸保持，实际来源／完整影子／原生安装仍待后续。

**2026-11-11 资源交接更新，版本63**：[第四项资源协议](9-resource-protocol.md)已完成typed资源和图形允许集、目录及闭包。图像贡献366补齐16B ColorOffset上下文，当前213B；414—417及world/body封装不变。本页版本62结果保留原时点，完整Frame和原生应用仍待后续。

承接[公共合同](9-public-wire-contract.md)及[版本61正文](9-domain-body-codec.md)。本项只实现显示组、依赖、贡献和音频事件的正式封装；完整Frame／表装配、资源payload、网络接线和原生安装仍由对应后续项处理。0—14步不运行测试，第15步统一验收。

**2026-11-11 完成，布局版本62**：新增产品库[`presentation_envelope.h`](../../../../mod/src/state/presentation_envelope.h)／[`.cpp`](../../../../mod/src/state/presentation_envelope.cpp)，同源生成[`数字合同`](../../../../mod/binding/layouts/09-wire-format/presentation-envelope-plan.md)。仍为340种布局、2772字段、84条件；没有新增线布局或改变414—417的固定头尺寸。新增用途约束与音频asset非Null规则需要双方按62协商，不按61猜读。

## 编写前列出的失败方式

- 混淆Host来源修订、显示批次和逐Peer目标版本；Reference把旧缓存伪造成新生产结果。
- 只用member序号或地址作组键，跨scope／use／shape借用；同实例代次改绑subject。
- 接受报文自选layout／adapter／pass；瞬态结果跨批次引用，Retire冒充缺源。
- 依赖非规范空值、同键冲突、遗漏真实资源／成员／拓扑／D0依赖，或任意扩大body对整个world的等待。
- 贡献id不稠密、父指向未来、深度越界、条件不成立／容器越界／重复位置，或漏掉有序Sprite、Image、本地历史和fade贡献。
- 只验证贡献字段号而不验证实际typed值；遍历复杂度、scratch和整批预算无界。
- latest-only丢失未确认事件，序号缺口／重复冲突、voice代次混用，Clear扩大到个人音频。
- 把结构解码当作来源合法、资源ready或已安装，先更新缓存／事件水位或提前ACK。
- 恢复Host文字图、普通切房surface或hallucination反馈图作为必需依赖。

## 资料顺序与边界

本地Wiki优先：`IsaacDocs/docs/SFXManager.md`的SoundID API不能充当voice身份；`REPENTOGON-1.1.3/docs/docs/HistoryHUD.md`的GetPlayer明确0／1为身体槽。随后读取官方SDK的`libzhl/functions/SFXManager.zhl`、`StatHUD.zhl`及`include/IsaacRepentance.h`。本项不新增游戏ABI或调用这些API，不把显示槽当Peer容量；封装编号／pass为C/S自有合同，不声称是原生枚举。

## 唯一源与实际线字节

`presentation.json`的28条`presentation_contract.envelope`登记group／event模式、真实subject类型、可引用缓存、D0／P0—P6拓扑、允许严格配对的序列与pass集合。315／317／321的player、364的group还必须等于外层subject。生成器按真实嵌套布局递归保留具名owner依赖；StatHUD318依赖ControlGroupHudSelection364，HUD323的stats仅为ANM2精灵，不能凭字段名把前者依赖加到后者。实际域状态的窄依赖继续交完整候选owner检查，不能把全部body改成等待完整world。

`wire.json`登记九种有限贡献类别及整批数量保护；`state_presentation_envelope.inc`从同次源产生四个固定头的布局号／字节数／字段索引、用途允许集和聚合依赖目录。固定头也复用正文codec，手写实现不维护另一份字段偏移表。

显示区域紧接Section中的成员记录，线上只有：

| group_count个：414头42B＋dependency_count×415条目32B＋contribution_count×416条目16B＋typed值 | event_count个：417头40B＋383 typed值 |
|---|---|

计数仅取Section头，不增加数组count、记录头或Presentation序列。Replace的payload_bytes覆盖依赖、贡献及typed值；Reference只带依赖；Retire三者全空。所有精确长度、规范空字段、序列／用途／layout／flags和来源修订都在相同入口检查。公共scope、source_revision、presentation_batch、GameFrame及Peer base／target复用分段上下文，不重复写入显示区域；GameFrame原u32不用于新旧判断。

## 接口与所有权

| 接口 | 工作 |
|---|---|
| EncodeResultGroupInto／DecodeResultGroupPrefix | 组头、规范依赖、有序贡献及完整具名值；prefix返回精确消费长度 |
| EncodePresentationAudioEventInto／DecodePresentationAudioEventPrefix | 事件头与AudioWorldEvent383；明确voice代次和Clear范围 |
| EncodePresentationBatchInto／DecodePresentationBatch | 连续groups随后events，检查整区域精确消费、整批计数／字节／遍历预算及事件连续水位 |
| ValidatePresentationBatch | 按精确epoch解析完整Key/use/shape，检查来源资格、缓存、资源ready、绘制拓扑、同批前向依赖／环及已知事件内容冲突 |

全部输入为自有封存纯值／不可变字节；结果span借用调用者缓冲，不保存原生对象。依赖、贡献、路径、排序索引、组键和DFS状态由调用者提供有界scratch，正文observer只借用到当前调用，异常时恢复原scratch回调。没有字段树、JSON、逐元素vector、hash或网络函数分派。编码输入与输出禁止重叠；失败不返回可发布的部分结果。

Validate只说明本批**显式提供**的身份／来源／引用依赖成立。必要组是否全部存在、未列组从base继承是否合法、与成员记录是否重复编码、完整目标／资源用途／图形允许集及真正提交，都仍须由完整Frame和第12／13步检查。此处不启用新Kind、不注册传输处理器、不提交缓存／事件水位，也不发送ACK。

## 缓存与贡献检查

- 组键包含完整scope、解析后的owner／subject Key/use/shape、purpose及实例代次。相同owner／用途／实例代次不能在同批重复或改绑subject；跨批实例与来源资格由只读Context提供，明确Retire也须有真实退休事实。
- 可缓存用途只可引用相同layout、完整组键、实际produced_revision的**已提交**缓存。缓存还记录原member／resource epoch；换表后采用Replace重新映射纯值，保留真实生产修订。不能用新表重解释旧字节，或仅因内容不变补一个新来源。Reference重新检查当前依赖是否覆盖缓存中的真实链接。
- D0、随机alpha和每次产生的有序临时屏幕／versus／Nightmare／GameOver／换层贡献不允许跨来源Reference。持久文字／列表／参数的缓存资格与内容变化分开；SourceQualified必须核正常来源、失效和具名窄状态依赖，不能以旧strong存在或资源声明到达代替。
- 贡献id按数组0起稠密，parent必须先行，父链最多16层。field／element先按生成布局形成真实路径，再由正文实际遍历匹配。条件失效、超count、未知字段／pass、重复同位置同pass均失败。不同pass可保留同节点的独立贡献身份；不按Client第N次访问猜身份。
- Context类别只作父链，Value只指结果根；Sprite／Image／LocalHistory／Fade分别命中365／366／424／372。随机Primary／Secondary／Recursive只用于357／358的当前值；不同实际值必须封存为独立用途实例，不借多pass把不同值合成最后值。所有实际有序贡献节点必须被定位，排序scratch不改变线上顺序。
- 每个实际非Null成员／资源引用必须有明确依赖。身体结果必须声明本subject的P0／P6拓扑；结果依赖只允许本用途及实际嵌套owner的正式依赖。PairedTarget只允许登记的窄用途、实际双分段可靠共同目标，不能由来源修订猜Peer target。

## 音频与预算

事件用途固定voice用途382，typed布局固定383；Start／Retrigger／Stop／参数调整及精确voice Clear须有独立voice id／generation。范围Clear不带voice，仅owner范围指定真实owner，world／run范围owner为空；范围不涉及个人UI／硬件音频。381的实际asset现明确非Null，缺资源不能改成Null。

增量必须连续携带installed_event_watermark之后至本目标水位的事件；缺口、重复号、终点不符或已知同号内容冲突均失败。Context只读提供已接受未确认事件，精确重传可幂等复用；本层不退休未确认事件。baseline不重播历史one-shot，以当前voice状态及水位建立起点，不倒退已安装水位。只有world携带事件／水位，body二者均为0。

整批上限为1024组、8192依赖、8192贡献及4096事件；typed字段／标量元素访问默认合计262144次，父路径32层，结果依赖DFS32层。数量保护、调用者scratch、每字段／布局预算和整区域字节同时生效；默认64KiB，可靠路径的旧32MiB仍只为内存保护。完整Frame调用方必须先扣公共／分段头及成员记录，再传显示剩余额度；这些数字不替代10Mbps总上行、7.5Mbps持续、首次准备1MiB／10秒或第11步实际分片计费。

## 完成方式与后续

普通生成和Release／Win32 `isaac_cs_state`纯编译完成，`ISAACCS_BUILD_E2E_TOOLS=OFF`。首次编译发现新增observer局部stride使用了作用域外width，已改为读取同源标量宽度后重新编译成功；这是编译修复，不是功能测试通过。静态复核另补主体一致、Stat及实际嵌套布局依赖、换表缓存、整批访问／重叠保护和同号事件冲突；曾在说明中误把HUD.stats当成StatHUD，按正式源／SDK核为ANM2并更正，没有据此增加错误依赖。

未运行游戏、单元测试、E2E、probe或独立校验脚本，未向游戏目录安装产物。完整本地资源payload／table-kind矩阵、shader／blend／sampler精确允许集、完整Frame／表／控制报文及最终接受规则仍为第9步后续；来源缓存、逐Peer发布、完整影子和原生应用按10—14步接线。文字／字体／本地历史adapter及音频逻辑包络／seek／恢复相位等来源Gap、UNKNOWN保持；第15步唯一统一整体验收尚未开始。
