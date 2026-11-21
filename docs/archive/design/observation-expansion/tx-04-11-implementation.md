# tx-04-11 Grid 实现与统一验收

日期：2026-10-30。状态：全目标实现完成，构建通过，同一 tx 的统一 E2E 连续修复后最终 **5/5 PASS**。

## 实现范围

正式来源仍为 `mod/observation/isaac-cs-observation`。Grid 在既有域内扩展，不新增协调器、身份表、Hook owner 或录制管线。公共值复制器移至 `domains/common/read.lua`，Entity 原采集规则保留；两域复用 Sprite／图层／动画／资源、表图、整数、非有限值及预算语义。

[逐成员目录](../../../../mod/binding/catalog/grid-catalog.md)／[JSON](../../../../mod/binding/catalog/grid-catalog.json)由两套本地 Wiki 转换，登记 GridEntity、全部17个公开派生接口、GridEntityDesc、相关类型／变体／碰撞／门／房形关联枚举与真实回调。接口没有额外公开成员时仍记录全部转换结果及派生类型，不把类型枚举、Lua 接口和 native RTTI 数量互相替代。操作进入目录但采集器不执行玩法方法；不声称逐条拦截了所有 Lua 玩法调用。

Grid 普通状态在相同 native 复制窗口内读取，和 Entity 顺序共享 native 批次、原生 RNG 见证及末尾来源／root 复核。Grid／Door 的所有实际 source/index membership 来自该批 owned 值；不按 width×height 缩减实际448格存储及8门槽。448是当前目标已核定的真实数组边界，派生完整性另由公开转换、字段、描述、资源与生命周期提供。门的目标房号是当前门公开原值，完整楼层连接语义仍归tx-04-12。

保存描述以 `GridEntity.GetSaveState` 为规范字段，`Desc` 是同义来源；它不替代运行字段与资源。全部派生运行状态、门的主／额外 Sprite、压力板 TimerPlate、有限 RoomType／BackdropType 参数查询均复制。`GetRenderPosition` 使用官方SDK签名和栈Vector缓冲，避免官方Lua包装分配临时Vector但未释放的问题；`IsBreakableRock`按官方绑定的boolean返回值修正文档的void误写。

Grid RNG及GreedModeRNG只复制当前seed／shift，并带主体／成员限定；不调用Next／Random，不制造新随机数。统一随机流目录仍归tx-04-14。搬起后的`EntityEffect.GetGridEntityDesc`在`lifted_descriptors`及显式查询中使用Entity主体，不伪造仍活动的Grid槽或源Grid身份；公开API没有提供源Grid身份时保留明确Unresolved。

真实post-update复制运行字段变化；render每30次真实回调独立取样，spawn／rock-destroy／post-grid-hurt／post-grid-hurt-damage和统一native GridFactory／Init／Spawn／Door／GridFree事件各保留自身scope。独立事件不借用旧Authority tick；稳定run／room instance可附实际观察器身份，加载中未知room保持Unknown。floor／dimension／descriptor的规范身份由tx-04-12补全，当前不以本地房号冒充这些身份。

Grid新增`isaac.cs.grid`、`.lua-event`、`.render`、`.query` v2 schema；旧`.legacy`和min-read-v1字段编号／判据保持。录制v2附带grid目录，Rust reader逐schema归约，未实现域仍使world_complete=false。正式消费者的latest_grid只接受普通Grid后态，render／query／legacy不覆盖它。四个pending普通投影及临时编码／最新值计入128MiB扩展预留，总预算仍512MiB；主线程不等待写盘，超预算或失败明确给出不完整。

## 统一 E2E 与事实记录

入口：`scripts/accept-tx-04-11.ps1`。全部功能和场景编排完成前未构建、运行程序测试或probe；无实现后单元测试。用户在本会话明确要求测试无需询问，每次启动分别告知停止手动操作。

场景编排：全部1—27类型（0为工厂拒绝的NULL）、17派生接口、主／额外资源、水面裁切、受损／破坏、坑桥接／梯子、压力板接触、门锁闭／开启／目标、同格删除再生、同回调短命Grid、搬起描述、RNG不推进、显式查询、暂停、离开回访、2×2与L房、停录后正式采集继续、v1/v2同读取投影、wrapper／native body配对、正常停止／退出与部署恢复。请求和实际结果分别记录，不强制variant匹配请求。

- 沙箱内首次MSBuild未推进，已终止；审批通道构建DLL和Rust harness通过。
- 第一次实机进入第二存档种子局，harness漏识别tx11日志前缀而在seed完成等待超时；实际种子命令已执行，未进入Grid场景。退出、部署／Helper／配置恢复及存档范围检查成功，无新崩溃文件。已补全前缀解析，继续本tx验收；原失败保留。

- 第二次实机按原判据5/5通过：27个非空类型及17派生接口进入25个Complete后态。收尾发现房形夹具错误地从BASEMENT配置取variant，而种子局实际为BURNING_BASEMENT；请求8／9房形，实际进入其他房形。原判据只检查多房形，不能证明预期2×2／L场景。已使用`Isaac.GetCurrentStageConfigId()`修正来源，并增加实际形状及触发／破坏／短命对象断言；第二次的局部通过不作为最终结论。
- 第三次实机完成所有场景，新增判据发现TNT伤害后态缺失；Grid普通后态25/25 Complete，但Grid render有1238条Partial、Lua事件有573条Partial。官方`CustomCallbacks.cpp`用字符串`GridTNT_MT`包装部分回调，而目标Lua类使用vanilla元表键，导致回调userdata不具备可用的基类读取接口。此前catch保留了失败状态但没有具体回调；现补充回调名和净化错误文本。
- 修复不改官方全局元表／原回调userdata：`GridCallbackView`只对大小符合官方UserdataPtr布局、受保护读取确认exact Grid RTTI、initialized／type／index有界且未退休的对象，构造自有基类借用视图。ToX仅改变这个临时视图，不影响其他Mod收到的原userdata。新增`MC_POST_GRID_HURT`捕捉TNT Destroy(false)实际转入Hurt(4)后的原值，并保存真实伤害参数；伤害作用于实体的post回调也已接入。没有新增Hook owner或改原调用次数。
- 第四次 **5/5 PASS**。25条Grid普通后态全部Complete，所有Grid schema的Partial为0；主线程采集损失与发布损失均0。进程正常退出0，输入／观测均正常停止，部署／Helper／配置恢复、存档范围检查通过，无新增崩溃文件。成功场景与前序失败的临时材料均清理，不保留本tx artifact。

- 收尾为v2 Grid生命周期补齐typed生成／移除／Init请求、真实GridSpawn正常返回值及已确认出生描述；读取的是同一次native事件参数和身份表已冻结的自有值，不补调游戏Getter。FIRST_OBSERVED不补造observed_birth，非正常返回及非调用事件不制造返回值。
- 第五次完整续测最终 **5/5 PASS**，新增native_spawn_request_return及short_actual_birth断言均通过；所有Grid记录Complete，采集／发布损失为0。正常退出与恢复再次通过，临时材料已自动清理。以下数字以第五次最终结果为准。

## 最终实际覆盖

| 目标 | 实际结果 |
| --- | --- |
| 类型／派生 | 1—27全部非空GridEntityType与17个公开派生接口均在Complete普通后态出现；0为工厂拒绝的NULL，不伪造对象 |
| 普通状态／预算 | 25条Complete，最大197014值节点，低于262144节点预算；native legacy Grid亦25条Complete |
| 保存描述／资源 | 当前Desc类型／State与运行原值一致，Sprite Rotation=31／FlipX=true、水裁切57、门主／额外Sprite及派生字段完成复制，压力板TimerPlate保留实际nil／存在语义；RNG seed／shift为主体／成员限定的当前状态 |
| 变化过程 | 坑桥接／梯子、便便受损750及破坏1000、岩石破坏State=2、TNT伤害／引爆State≥4、压力板实际触发、门锁闭及开启均被实际状态或独立回调见证 |
| 替换／短命 | 同格旧／新主体不同且Index相同，两个主体均有普通后态；同回调短命Grid的BirthConfirmed与GridPhysicalRetired均出现 |
| 原调用／事件 | 619组Grid wrapper和native body进入／返回一一配对，每组各一次；3229条native Grid事件、29373条Lua事件、4478条render及3条显式查询均Complete |
| 搬起描述 | Effect136使用Entity身份记录原保存描述，实际SpawnSeed=43001、VarData=37；不保留伪Grid membership，不猜测源Grid身份 |
| 房间 | 实际1×1／2×2／L房形1／8／9，包含448格L房；离开／回访及五个实际room instance代次均出现，重复房号／索引不恢复旧Grid身份 |
| 兼容／录制 | 2175个Grid对象的v1/v2同读取tick、身份／use／revision、字段状态及原始字节一致；停录后文件不增长、正式Grid scope继续且epoch不变 |
| reader／健康 | 47339条v2记录，无未知schema，delivery_complete=true；采集／发布损失、trace_dropped／query_mismatches／cleanup_failed均0；正常退出及恢复通过 |

最终Grid各schema均无Partial。录制同时保留Entity的实际Partial与odd-only／未调度gap，不将这些记录删去或外推为整个世界完整；reader的world_complete=false。完整Grid观测不是Replica、世界写回或状态同步。

公开API没有源Grid身份时，搬起描述的source_grid仍Unresolved；floor／dimension／descriptor身份、非活动房间保存集合、统一随机流目录分别归12—14。Fire类型13在受控工厂生成中观察到，未声称自然房间会生成它；原生会对其未配置动画输出警告。Greed模式波次及非nil TimerPlate、伤害作用于实体的post回调路径、所有变体／所有随机种子／所有模组组合、所有显式参数调用和极端负载没有逐一运行。目录覆盖全部公开语义，有限参数查询按当前公开枚举实现；不承诺枚举所有私有字段、逐条Lua玩法API调用历史或性能验收通过。


## Grid v2 可读记录合同

Envelope沿用公共v2合同；普通后态有真实Authority tick，独立事件／render／query不填旧tick。整数、可空值、参数结果和资源树共用公共值编码，资源规范成员见entity-catalog，Grid独有成员见grid-catalog。

| schema | payload内容及范围 |
| --- | --- |
| isaac.cs.grid | objects主体／type／variant／index／全部casts／common／bodies，逐source/index的memberships，lifted_descriptors，coverage及预算原因 |
| isaac.cs.grid.lifecycle | native原始kind／observation／call／parent_call／entering／exceptional／主体local_id/use/revision/readiness；相关事件可带typed request、真实returned及observed_birth |
| isaac.cs.grid.lua-event | 实际event名、subject、profile、按已知类型复制的真实arguments及object；runtime profile明确不含资源或参数查询 |
| isaac.cs.grid.render | 实际event／subject及render-sprite profile；仅该次实际渲染的Sprite姿态，不合并普通后态 |
| isaac.cs.grid.query | 只读member、真实parameters／result及owner；搬起描述保留Entity身份和无法公开确定的source_grid关系 |
| isaac.cs.grid.legacy | 相同owned批次的旧profile投影；不包含新公开字段，不按新schema改写旧字段含义 |

request与observed_birth相互独立：请求variant可以是0而实际构造后variant为其他值。observed_birth的sequence／owner／type／variant／seed来自已观察创建owner完成后的冻结值；当前对象状态仍由普通／事件字段表达，不拿出生描述覆盖后续变化。
