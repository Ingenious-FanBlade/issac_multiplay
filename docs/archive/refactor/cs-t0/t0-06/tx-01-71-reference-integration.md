# tx-01-71：旧源码静态参考的吸收记录

日期：2026-10-18。用户指定参考为 `D:\projects\REPENTOGON\docs\research\repentogon-t0-06-source-static-reference-2026-10-18.md`。本轮全文阅读并保留[原文快照](../../../../../artifacts/cs-t0/tx-01-71-static/reference-source-static.md)，以免后续该会话的新编辑改变本轮依据；另读关键原源码片段并保存到[源码证据](../../../../../artifacts/cs-t0/tx-01-71-static/source-evidence.json)。不对提交号、文件或目标做哈希校验。

该文档自己明确：研究的是旧 REPENTOGON 源码，没有当前 EXE VA/RVA/ABI 或运行时新证据；原文 `STATIC_CONFIRMED` 仅针对旧源码。本项目吸收时统一限定为 `REFERENCE_VERSION_SOURCE`。声明、注释、静态推导、设计与 UNKNOWN 分别保留，不把旧源码事实直接升级为当前原生事实。

执行要求见[现行规范](../../project-conventions.md)。本篇是同轮资料分析记录，不执行额外验收；实际新增原生研究与最终完成状态由本轮合并报告说明。

## 1. 对任务书有用的内容及采用方式

| 参考内容 | 具体依据 | 本项目采用方式 | 仍需当前依据的内容 |
|---|---|---|---|
| 工厂/Player ctor/删除通知是不同阶段 | 参考S01；[EntityManager.cpp](</D:/projects/REPENTOGON/repentogon/Patches/EntityManager.cpp:146>) | A01–04分开allocation、Init、publication、retire；不直接把通知当出生 | 当前普通/Player/Grid发布成功点与失败分支 |
| 提交移除存在分列表补偿 | 参考S01/S25；旧CommitRemovedEntities94–144 | 保留来源membership；Remove、ClearRefs、valid=false、Free、摘表各自记录 | 当前EntityList正常与reset/destructor路径，不照抄Mod补偿实现 |
| 构造与Init对EntityPlus处理不同，getter可lazy分配 | 参考S02；[EntityPlus.cpp](</D:/projects/REPENTOGON/repentogon/Patches/EntityPlus.cpp:22>) | A03不计ctor代替复用；C05采集不调用这些getter | 当前native逻辑使用与Init重入，不能用扩展数据重建证明本体新生 |
| Grid一条补丁路径先入槽、再写Desc、再Init | 参考S16；[ASMGridEntitySpawn.cpp](</D:/projects/REPENTOGON/repentogon/Patches/ASMPatches/ASMGridEntitySpawn.cpp:83>) | A01/A02/C03不能以容器出现或Initialized单标志证明完整发布 | 当前Grid的两个创建入口、发布顺序及立即移除 |
| 多EL、玩家保留形态、Grid/door不能用单一来源替代 | 参考S04/S10–19；§4 | C01列来源状态并保留视图/成员身份，不把池或保存记录算活跃房间实体 | 以tx-01-64/69当前规格为准；全进程分配全集仍不在目标内 |
| Grid447声明与0..447实现存在旧差异 | 参考S14/S16；§4.1 | 明确采用当前已证448槽，保留8个door来源 | 已由tx-01-64/69闭合，不重复当本轮未知 |
| Morph PRE可取消/改参，POST可看同this前后；KeepSeed参数不是执行结果 | 参考S07；[CustomCallbacks.cpp](</D:/projects/REPENTOGON/repentogon/LuaInterfaces/CustomCallbacks.cpp:1192>) | A05追本体；冻结出生描述，当前分类/seed可变 | 当前NPC/Pickup内部Init/保留/发布/替换 |
| Pickup选择请求参数使用文件全局，非重入栈 | 参考S08；[CustomCallbacks.cpp](</D:/projects/REPENTOGON/repentogon/LuaInterfaces/CustomCallbacks.cpp:5417>) | A08/B02采用TLS有界调用记录，不复制该暂存设计 | 本项目实际嵌套/重复事件处理 |
| Spawn后仍可能字段补写、重设Parent、立即Update | 参考S09；§5.3 | C03不把POST_INIT当最终每tick快照；继续沿用tx-01-66顶层普通完成窗口 | 不恢复所有writer；当前阶段入口与最小依赖另证 |
| owner关系有多种槽，Familiar.Player不是统一EntityPtr | 参考S18/S20；§6 | C02/D03分关系记录；NULL、已知同代、未解析、退休、复用歧义分开 | 当前raw槽/宽度已由tx-01-68证实；同代解析依赖A组事件 |
| ESSM ID可回收，保存marker及StabilityChecker会主动写/修复 | 参考S03/S05；[ID管理](</D:/projects/REPENTOGON/repentogon/SaveStateManagement/EntitySaveStateManagement.cpp:492>)与[检查调用](</D:/projects/REPENTOGON/repentogon/SaveStateManagement/EntitySaveStateManagement.cpp:3395>) | 不拿ESSM ID作跨进程身份；C05不复用修复流程采样 | 当前Restore/replacement只追身份需要，不恢复全部扩展存档算法 |
| Lua删除幂等、同地址新通知清旧记录 | 参考S06；§6 | 借鉴本地事件幂等与同址代次分离，不当全局顺序证明 | 实际通知无丢失只能运行时观察 |
| Update可被PRE跳过，仍做补偿提交/尾部操作 | 参考S25；§7 | B02必须同token确认标准Room与尾部，不因Update返回就完整 | 当前Hook/marker关系在同轮设计补齐 |
| super可能是前一层Hook，不能凭单wrapper推出全链 exactly-once | 参考S24；§7.1 | 保留现有owner，不覆盖未知第三方Hook；区别局部代码性质与实际加载链 | 当前补丁事务及实际链范围 |
| Lua查询、EL操作、RNG、引用登记、lazy getters存在副作用 | 参考§7.2 | C05只复制明确字段，不调用这些路径；后台只收值副本 | 所选实现的运行时无修改仍留最终E2E |
| Checksum::Generate已见用途是文件流 | 参考S21；[SaveSyncing.cpp](</D:/projects/REPENTOGON/repentogon/SaveSyncing/SaveSyncing.cpp:342>) | E01区分文件校验、存档与每帧世界生产者；不执行函数 | 必须另找当前framestate/checksum/RNG history生产链 |
| onlinestub只消费控制台desync文本且默认未加载 | 参考S23；§8.2 | 不把UI脚本当native生产入口或RNG历史格式 | 当前原生字符串xref与实际数据流 |
| RNG四字布局不等于history，诊断dump不等于可逆snapshot | 参考S20/S22；§8.3 | 记录覆盖差异与限制，不研究/执行apply，不借用Online状态 | 当前生产的模式、时机、字段、buffer、转换与history语义 |
| D组没有现成完整schema/匹配/分层比较器 | 参考§9 D01–06 | 作为新DESIGN_PROPOSAL编写[本轮读取与比较方案](tx-01-71-sampling-design.md) | 方案尚需实现和统一E2E，不能以文档代替运行验证 |

## 2. 已有当前证据对旧参考的限制

当前 [tx-01-69偏移核对](tx-01-69-offset-reconciliation.md)仍优先：Game.Factory为`+0x26580`，PlayerType/PlayerIndex/Twin/backup/replaced为`+0x13C0/+0x161C/+0x1E68/+0x1E6C/+0x26C8`，Familiar stride为`0xF48`，Pickup池起点为`+0x357C30`。旧参考表不能覆盖这8项更正，也不为池槽赋予房间成员资格。

旧Lua Variant setter会清Effect.VarData；当前tx-01-69原生setter `0x6A8180`只单写Variant。二者分别陈述，不把Mod扩展副作用加入当前直接读取设计。旧源码未命名通用HP不否定当前`Entity+0x380/+0x384`；所有Entity类用其表示生命仍保留用户假设，Player hearts独立。

旧声明没有确认当前ToTear或精确NPC分类；继续使用tx-01-64当前RTTI/转换依据。FrameCount为Game counter减Entity出生counter，不能由旧表或名称改作独立raw字段。Grid与Entity字段域、基础/Tear/Projectile flags宽度和独立RNG继续按当前研究分别读取。

## 3. 本轮吸收的结果

参考文档对A/C提供了明确反例与入口线索，对B提供了跳过更新/Hook链的限制，对D明确现成方案缺失，对E排除了“文件checksum或UI脚本就是帧生产者”的错误捷径。它并没有完成当前43个问题。使用它的正确方式是缩小当前原生补查范围，并把其副作用与未知处理写入实现方案；不能把其43项索引视为43项完成证明。

本轮后续补查与设计会在独立新文件记录，不改参考项目、不覆盖旧报告和原始证据。实际运行时无修改、并发、事件覆盖、跨进程匹配和最终Hook恢复没有被本篇证明。
