# tx-01-71：参考吸收与剩余静态分析补全

日期：2026-10-18。本轮已阅读用户指定的旧 REPENTOGON 参考，并完成 [tx-01-70 任务书](tx-01-70-static-analysis-handoff.md)中 **A–E 共43项所需的静态依据和实现前方案**。按用户要求使用一个 **GPT6 Luna / max** 子智能体进行当前 IDA MCP 分析，主智能体复核其指令与结论、纠正误读，并继续补齐原生详细记录与摘要的实际生产链。

这里的完成范围是所选原生正常路径、当前字段与采样接入、身份及比较设计；**采集器、身份事件、编码器和比较器尚未实现，T0-06整体尚未完成**。没有运行游戏、注入、构建、代码测试或哈希计算/校验，没有修改IDB、运行时代码、正式协议或参考项目。遵循[现行项目规范](../../project-conventions.md)；最终只进行[一次整体静态文档审阅](tx-01-71-review.md)。

## 1. 可直接使用的交付

| 文件 | 内容与使用顺序 |
|---|---|
| [参考吸收记录](tx-01-71-reference-integration.md) | 原文全文快照、19项有用信息及其对A–E的用途；旧源码事实统一限定版本 |
| [生命周期和原生记录补充](tx-01-71-lifecycle-and-native-records.md) | 主智能体复核后的A/E结论、八类覆盖、18个入口ABI/完整指令跨度、生产链与覆盖差异；作为最终原生结论 |
| [采样接入、读取与比较设计](tx-01-71-sampling-design.md) | B/C/D的20项逐题回答；同次更新token、Authority tick、owner、字段表、缓冲、固定profile和差异格式 |
| [子智能体分析报告](tx-01-71-native-analysis.md) | 请求的GPT6 Luna/max分析成果及更正记录；早期原始JSON保持不变 |
| 本文第3节 | 全部43题的结论、类型、证据、实现影响与未知处理索引 |

原始材料均独立保留：

- [指定参考全文快照](../../../../../artifacts/cs-t0/tx-01-71-static/reference-source-static.md)、[12个源码片段及原行号](../../../../../artifacts/cs-t0/tx-01-71-static/source-evidence.json)、[源码导出脚本](export_tx_01_71_source.ps1)。参考原路径为`D:\projects\REPENTOGON\docs\research\repentogon-t0-06-source-static-reference-2026-10-18.md`。
- [子智能体IDA原始证据](../../../../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json)、[子智能体静态导出脚本](../../../../../artifacts/cs-t0/tx-01-71-static/export_native_evidence_tx0171.py)：28个请求地址、27个唯一函数；`903670`在`9032D0`内部。
- [主智能体IDA补充](../../../../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json)、[补充导出脚本](export_tx_01_71_review.py)：40个函数请求、9段重点指令、18个入口候选及其调用者；`41C8E0`请求解析到函数起点`41C890`，不能把内部地址当入口。函数记录包含完整伪代码和逐code-item的VA/RVA、字节与指令。
- 既有当前依据继续引用 [tx-01-63 生命周期](tx-01-63-entity-lifecycle.md)、[tx-01-64 类别](tx-01-64-category-coverage.md)、[tx-01-65 HP](tx-01-65-hp-analysis.md)、[tx-01-66 阶段](tx-01-66-readonly-sampling.md)、[tx-01-67 state/timer](tx-01-67-state-timer.md)、[tx-01-68 seed/归属](tx-01-68-seed-owner.md)、[tx-01-69 偏移](tx-01-69-offset-reconciliation.md)，不重写历史证据。

当前IDB为32位`isaac-ng.exe`，imagebase `0x400000`，横幅VA `0xB7E1B4`为`Binding of Isaac: Repentance+ v1.9.7.17.J460`。所有本轮VA/RVA与ABI仅限该目标；旧REPENTOGON声明不代替当前指令。原始数据中的反编译类型、名字与十进制下标仍须按对应指令解释。

## 2. 新增依据与重要更正

1. 普通Entity工厂取得、Init、发布、生成owner返回是不同阶段；只计一次确认的新逻辑使用。池满可Remove/ClearReferences后同址复用，不经重新构造。Remove不等同摘表或物理释放。Player与Grid另有创建、保留和退休路径。
2. NPC Morph `6C0F30`直接修改分类/冠军相关字段；`this[793]`是`+C64`，不是seed。Pickup Morph `6E30A0`的`!KeepSeed`可写`+3EC`并在同一对象虚Init。已知Morph保持同use；未知重Init输出连续性未知。
3. Player Restore `7AE4B0`写同对象，Manager Restore `9BCAA0`可能创建/复用backup与重建Esau槽。Replace `9BEE80`检查目标`+170` valid（十进制368）；已登记的保留目标重新发布延续自身对象ID。新旧地址不同不能单独证明目标刚出生。
4. 完整样本应在`g_update()`正常返回后、既有coop TopExit前同步复制，且同token普通Room和标准尾部均正常完成。`955231→6FADC0`是LoopGameplayHook；StandardBodyHook是内层`6FAE09→708630`。odd Player-only提交不产生全世界样本。
5. 当前patch engine只支持rel32 call与pointer，不能直接安装18个函数入口候选。所需入口指令边界/ABI已交付，实现时仍须增加entry patch/trampoline并纳入同一owner/有序事务；现有余量各5槽不足以假定全部容纳。
6. **详细framestate真实producer是`4186C0`**，`Room+1310 = EntityList+F8`是同一个32槽ring。24-byte frame头指向56-byte entry vector；entry ordinal不是Entity.Index，Seed是`Entity+3DC` DropRNG当前首字，不是`+3EC` InitSeed。文本GlobalSeed是全局MT状态摘要。
7. **12-byte Online summary是另一条链**：`90B150→910980→90D640`，内容为native frame、`EL+F0`字段摘要、序列化online/lobby流CRC；第三字不是已证明的world-save或全局RNG摘要。`95999D`仅比较frame，并非写入点。
8. **RNG history由`6EEF60`生成276-byte节点**，含随机返回值、native frame、最多64个栈地址及数量；rand早返绕过该trace，内联对象RNG亦不保证经过它。它不是完整RNG状态、Entity关联或可重放日志。
9. HP、完整flags/state/timer、引用、InitSeed、完整DropRNG、Grid及Player心量/保留关系没有在这条详细记录的数据流中直接覆盖。原生producer/dump还会分配、写缓存/表、捕获栈、落盘；只作研究依据，不由C/S采集器执行。

上述结论已落入读取规格、事件语义与比较规则；未采用“seed唯一”“地址稳定”“单一列表代表全世界”等假设。各Entity类通用HP表示生命仍保留用户接受的假设标签，Player心量独立保存。

## 3. 43项逐题回答

类型简写：**S**=`STATIC_CONFIRMED`（本轮或明确引用的既有当前指令）；**I**=`STATIC_INFERENCE`；**C**=`CURRENT_SOURCE`；**R**=`REFERENCE_VERSION_SOURCE`；**D**=`DESIGN_PROPOSAL`；**U**=未证明性质，按列明状态输出。S/C不表示真实进程已运行该实现，D不冒充native行为。下表的“完成”表示已交清实现需要的事实、方案与未知处理。

| 问题 | 结论 | 类型 | 具体依据 | 对实现的影响 | 未决项/处理策略 |
|---|---|---|---|---|---|
| A01 | factory取得仅建候选；Init/发布/创建正常结束合并一次；保留Player允许无普通发布完成 | S/D | 生命周期补充§2；6F0090/9B9CD0/7EBCA0，tx63 | 分physical/use/membership，设置candidate及完成事件 | 未正常完成不确认活跃出生 |
| A02 | 工厂空返/类型拒绝、Grid失败保持旧槽；Player普通与−1分支完成条件不同 | S/D/U | 生命周期补充§2；6F0090/9B9CD0/7EBCA0 | ABORTED/UNKNOWN与已发布分别记录 | 不保证所有OOM/异常安全；不复活已清理旧use |
| A03 | 覆盖路径Remove→ClearRefs→返回同址→新Init/发布；构造不一定再次发生 | S/D | 生命周期补充§2；tx63工厂、6ACB00/6ACB60 | 同址新use_generation，旧关系按事件退休 | 地址相同和valid值均不能合并跨代 |
| A04 | Remove请求、成员提交退休、物理析构分开；池内存和保留Player可继续存在 | S/D | 生命周期补充§2；4186C0/418230/6A8850 | 分层退休且保存短命事件 | 一次缺枚举不是销毁证明 |
| A05 | NPC是同对象分类/config变更；Pickup可同this改InitSeed并虚Init | S/D | 生命周期补充§3 A05；6C0F30/6E30A0 | MORPH reason栈、同use加revision | helper/Mod全副作用不作全称保证 |
| A06 | Player本体Restore、Manager重建/复用、Replace交换/重新发布各自不同 | S/D | 生命周期补充§3 A06；7AE4B0/9BCAA0/9BEE80 | 已知保留目标保留自身ID；关系/membership另记 | 无来源连续性时撤销比较映射、重新FIRST_OBSERVED |
| A07 | Reset并非全析构；room_epoch与物理对象延续分开，run结束退休整个观察域 | S/C/D | 生命周期补充§3 A07；采样设计B06；418230与既有lifecycle | 合并嵌套房间事务，显式run/room代次 | 未观察边界历史不猜持续身份 |
| A08 | 嵌套Spawn、Morph内Init、重复发布与多来源必须带调用上下文去重 | S/R/D | 生命周期补充§3 A08；参考吸收§1，采样B02 | TLS有界reason/token栈；独立nested candidate | 栈/队列溢出→UNKNOWN_CONTINUITY/SAMPLE_GAP |
| A09 | 启用前仅FIRST_OBSERVED，当前分类/seed/来源可读，历史出生序号不可补造 | S/D/U | 生命周期补充§4 A09；采样C01/C03 | bootstrap本地ID；A/B语义锚仅限已有证据范围 | 不把本批唯一指纹升级为跨tick/进程身份 |
| A10 | 七类Entity正常池/Player路径与Grid独立域已列八类观察集合 | S/D/U | 生命周期补充§4 A10覆盖表；tx63/64 | 通用factory/Init/publish加Player/Morph/Grid补源 | 任意Mod绕入口、未知间接路径显式不宣称覆盖 |
| A11 | 分类/seed/current index可变；本地ID与跨进程候选key分层 | S/D/U | 生命周期补充§4 A11；采样D03 | 冻结出生描述+对齐创建事件序号，当前值另存 | KEY_COLLISION/IDENTITY_UNRESOLVED；禁止地址补key |
| A12 | 引用必须有同代事件依据；唯一地址映射不足以排除旧关系指向复用槽 | S/D | 生命周期补充§4 A12；tx68；采样D03 | NULL/KNOWN_CURRENT/UNRESOLVED/RETIRED/AMBIGUOUS_REUSE | 后台不解引用、未知目标不强连 |
| A13 | 全局唯一、跨端事件顺序、实际无漏事件、任意Mod/worker/异常未获静态证明 | U/D | 生命周期补充§4 A13 | 明确退化状态，保留已知局部结果 | 不输出完整相等或伪造静态运行PASS |
| B01 | 阶段复用Update/LoopGameplay/Room/PostUpdate现有wrapper；身份18入口另需新patch类型 | C/S/D | 采样B01；生命周期补充§5 ABI/字节表与primary JSON | 同owner内部观察，函数入口按真实ABI保留原执行 | 候选跨度不是已安装或恢复证明 |
| B02 | 同outer token需标准owner/Room/tail正常且各一次，生命周期污染即无效 | C/S/D | 采样B02；loop_runtime现有PhaseScope，tx66 | 新增调用栈/完成标记，消费后销毁 | 缺失、重复、异常、嵌套outer→gap |
| B03 | g_update返回后、既有coop TopExit前复制；现有单槽已被coop占用 | C/D | 采样B03；loop_runtime UpdateHook与coop TopExit | 增内部sampler槽/组合owner，保留observer和original一次 | collector错误仅停自身，不覆盖既有owner |
| B04 | Authority普通/odd提交都可增tick，只有同次普通提交凭据与完整阶段才对齐世界样本 | C/S/D | 采样B04；Authority/PlayerOnlyUpdate | 同线程传值token/tick/kind/scope；capture与attempt独立 | odd/暂停无完整样本；不可用tick不借最近值 |
| B05 | 现有clock建立主线程，wrapper与coop检查；只支持该同步正常路径 | C/S/U | 采样B05；EstablishOrCheckMainThread，tx66 | 主线程直接有限复制，后台只处理自有值 | 不证明全进程无worker writer；读变动标INCOMPLETE |
| B06 | 生命周期进入即污染当前token；嵌套按事务合并，不靠前后指针相等保活 | C/S/D | 采样B06；LifecycleCallback与tx66 | mutation/room/run epoch、活动深度和健康状态 | 失配/异常保持不可用，明确边界后bootstrap |
| B07 | 现有E8/pointer事务、容量和逆序恢复可沿用；入口detour需新增实现 | C/S/D | 采样B07；生命周期补充§5的18组完整指令、entry_callers | 扩容量/ordinal，原字节/trampoline/在途计数入同owner | 冲突不覆盖；strict_slot_snapshot_before保持nullptr |
| B08 | 未启用/错模式/错线程/缺标记/溢出只影响采样，自有回调noexcept且不等待后台 | C/D/U | 采样B08与C04/C06 | original按原路径一次、返回值保留，不调用coop::Fail | 实际调用次数/时延留实现后统一E2E |
| C01 | 八个EL视图、roster/保留关系、448Grid+8door按来源登记并同批合并别名 | S/D | 采样C01/C02；tx64/69 | membership与active分开，池/保存结构不是active来源 | UnknownEntity保留已知公共头；未知源不当空 |
| C02 | 逐字段raw宽度/偏移、心量、专用state、flags、RNG与引用规格已列 | S/D | 采样C02读取表；tx65–69；SubPlayer getter6DAE10 | 按已证对象域有限字节复制；Grid不套Entity字段 | Tear/Projectile专用State/Timeout为UNCONFIRMED |
| C03 | valid/exists/dead/initialized独立属性；发布证据不等于每tick完整初始化 | S/D | 采样C03；生命周期补充§2 | membership/init/lifecycle/field状态分别存 | 仅知头部仍留对象，派生字段NOT_READY，不补0 |
| C04 | EL与vector边界不同；448/8固定槽，所有长度/预算失败须显式报告 | S/D | 采样C04；tx64/69容器指令 | 预分配对象/来源/关系预算并检查溢出、前后描述 | READ_FAILED/BUDGET_EXCEEDED→INCOMPLETE，不静默截断 |
| C05 | 采集只复制字段，不调用查询、EntityPtr登记、lazy getter、RNG、修复或虚函数 | S/R/D | 采样C05；参考吸收§1；原生记录§8 | 去重/规范化/解析仅作用自有数据 | 不明副作用helper不进入collector调用图 |
| C06 | native同步复制结束后才能release发布，consumer acquire后仅访问值副本 | C/D | 采样C06 | 三buffer FREE/WRITING/READY/READING；无空闲即gap | 容量/时延未验证；关闭publication/drain后回收 |
| D01 | v1内部记录有run/room/mutation/attempt/capture/token/phase/tick与完整性 | D | 采样D01 | 显式跨端观察域及tick/阶段映射，不改正式wire | ALIGNMENT_UNAVAILABLE/SAMPLE_GAP，不拿近邻帧替代 |
| D02 | min-read-v1固定字段集合/ID、LE/TLV与JSON raw位串；finite精确比较、±0比较视图归一 | S/D | 采样D02编号/encoding/profile表及C02 | flags完整保存、u64字符串；未知/缺失/不适用分开 | NaN/Inf/required未知不相等；不逐样本删字段 |
| D03 | 地址/vptr/thread/墙钟仅诊断；引用用有事件依据的目标key和解析状态 | S/D | 采样D03；生命周期A11/A12 | 冻结出生描述，不因Morph重写key | bootstrap或目标未匹配保持不可比较 |
| D04 | 唯一对齐key才逐字段；冲突/身份不明/覆盖不足各有独立输出 | D | 采样D04 | DIFFERENT/EQUAL_WITHIN_PROFILE/INCOMPARABLE三态 | 同缺项或同冲突不算equal；可同时报已知差异与未知范围 |
| D05 | run→room→类→实体→field定位，首次差异按已对齐tick及固定顺序 | D | 采样D05与JSONL示例 | 保留capture/gap/event/collision/comparison与manifest | 更早有gap则标first_observed_difference，不称全局首次因果分歧 |
| D06 | 冻结副本重复编码、自有字段变更、10k采集、生命周期、双进程与恢复观察量已列 | D/U | 采样D06 | 实现完成后整合一次整体E2E并保留输入/记录/差异 | 本轮仅设计，未运行或编写实现后单测 |
| E01 | 详细4186C0、summary90D640、MT trace6EEF60分别是真正producer；dump是consumer | S | 原生记录§6；primary完整函数/9段指令，agent消费者链 | 分三链解释，不以文件名或Checksum名猜覆盖 | 不研究未知apply或触发desync |
| E02 | 详细记录要求Manager的+4B3D8/+4B3DC roster begin/end不同；summary在ready处理；MT主体无Online条件 | S/I | 原生记录§6/8/9；41870C..720、910980、6EEF60 | native frame与Authority tick分离 | 普通/启动/Reset/早退不强配成统一世界采样阶段 |
| E03 | 详细取updateEL原生访问范围；无Grid独立entry，不保证main/persistent-only或备用Player | S/I | 原生记录§6；4186C0更新/记录分支 | 多来源自有采集仍必要 | 原生初始尾/size边界不等于全体分配对象 |
| E04 | 24头/56entry及12summary逐偏移数据流已恢复；DropSeed/GlobalSeed含义更正 | S | 原生记录§7两张字段表；418BE2..D50/4196C4..72D/90D640 | 自有字段与native覆盖逐项对照 | 未直接含HP/flags/state/timer/InitSeed/owner/Grid，不以间接影响代覆盖 |
| E05 | 详细按更新顺序raw复制，无该路径量化/稳定key排序；序列化online流另有变换 | S/I | 原生记录§7；683410/683580/91ACC0/A89DE0 | 自有排序和±0处理明确是设计，不冒称native规范 | 不恢复整个压缩/lobby schema或假定CRC可逆 |
| E06 | 32详细槽重用vector，NetInputDevice+3BC是深拷贝；summary按窗口淘汰 | S | 原生记录§8；418230/417E50/8FE3A0/901B70/910980 | 生成时刻与dump时刻、live与owned buffer分开 | 原生覆盖/窗口不替代自有队列完整性 |
| E07 | trace node是276字节：输出u32/frame/最多64栈地址/count，无Entity或完整MT状态 | S | 原生记录§9；6EEF60/904E70；flag916E51..71 | RNG history仅诊断旁证，不用作重放或tick实体关联 | rand早返及对象内联RNG可绕过；unsigned窗口边界不外推 |
| E08 | producer有模拟、分配/释放、缓存/CRC表写、栈捕获；dump另有文件/符号/报告副作用 | S | 原生记录§8/9；6EF1F0/8FE3A0/904E70 | collector不执行producer/dump/getter获取状态 | 当前静态阅读未执行这些函数或算法 |
| E09 | 全部只研究覆盖，不借NetManager/NetInputDevice/原版FrameStore或等待条件做C/S状态 | S/D | 原生记录§6/10；采样D01 | 自有身份、缓冲、结构化编码与逐字段比较 | 不修改原版Online或Isaac Sync路径 |
| E10 | 原生记录只部分运动/分类/DropSeed及摘要；不提供本项目完整字段或可逆snapshot | S/I/D | 原生记录§10覆盖差异表；本表E01–09 | 保留生产链作为参考，采用自有min-read-v1 | 不以原生摘要相等证明世界相等、不执行apply或哈希校验 |

表中“生命周期补充/原生记录”均指[同一A/E详细报告](tx-01-71-lifecycle-and-native-records.md)；“采样”指[B/C/D详细设计](tx-01-71-sampling-design.md)。两篇内的函数VA和源码链接可继续定位到原始证据。E02表项的全局Manager两个偏移完整写法为`+0x4B3D8 / +0x4B3DC`。

## 4. 已闭合范围、保留限制与下一步

已有tx63–69的字段、类别、阶段、布局依据继续有效。本轮补齐身份选择实际依赖的NPC/Pickup Morph、Player Restore/Replace、Grid/door创建移除、入口ABI/跨度，以及原生记录生产者、字段映射、窗口和副作用；B/C/D从问题清单形成了可实施方案。

静态范围没有扩成“穷尽整款游戏”：全部Mod旁路、所有间接writer/异常/OOM、整个online/lobby序列化格式、所有专用timer语义仍未恢复。其中未确认专用字段不纳入固定profile相等结论；未知重新Init、未解析引用与旁路新对象有明确退化输出。已知正常路径的实现不依赖先证明这些全称命题。

仍只能在实现后观察的是：实际事件是否丢失、未知worker并发、跨进程顺序/匹配、采集时延、连续10,000次无修改、真实加载Hook链与恢复。没有静态PASS替代它们；未来按现行规范在整个目标实现完成后统一E2E，保留可重复的输入、事件、样本、对齐和差异产物。

下一步可据此实现：同owner的内部采样观察与Authority值传递；受支持入口的事件记录及entry patch类型；本地身份/代次和bootstrap；主线程字段复制及自有buffer；`min-read-v1`编码、对齐与逐字段比较。此清单只说明实现输入已交清，不是本轮已写入的功能，也不自动执行实机操作。
