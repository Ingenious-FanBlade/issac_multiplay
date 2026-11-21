# tx-04-19 I26—I30：独立结果、隔离、退休与唯一提交

日期：2026-11-03，Asia/Singapore。I26—I30的产品实现与接线已完成，主智能体完成资料／代码／必要原始指令的静态审阅。按本轮会话要求直接提交：没有运行构建、分项测试、游戏或E2E；本记录不表示运行验收通过，I31—I35及I34统一整体验收继续按原清单推进。未新增tx artifact。

## I26 独立scope与短命主体

`ClientEntityEvents`由唯一`ClientScene`持有，接入`ClientAdmitEntityIndependent`／`ClientVisitEntityIndependent`／`ClientCompleteEntityIndependent`。它与Grid队列分开，只接受Entity域的完整Lifecycle／Render／ActualCall记录及正式瞬时Event，不修改普通完整T或成员表，不生成原生Entity。

- 完整T中已退休或缺席的主体不因独立Lifecycle而重建。下一T中的Effect／碎片／奖励仍按自己的Key走正常应用。
- 保留原scope、source_scope、sequence、phase与required_state。Render／Query必须明确绑定当前已提交revision并逐一匹配Key／shape；初始shape=0合法。不把required_state=0猜成当前版本，也不将Query回灌为普通状态。
- 结果事件可只含Authority最终值；包含当前主体引用时，同样核对当前Key／shape。真正消费后才显式Complete；缺依赖保持Pending，过期和旧姿态分别为Expired／Superseded，均不变成Presented。
- 去重按kind／sequence，并比较完整原报文；同序异内容拒绝。单条256KiB、128条／2MiB队列，未消费记录背压，仅回收已有明确终态的历史。没有事件水位或网络ACK。

NativeText独立Render仍不覆盖普通`application_draw`；27负责实际消费独立音画结果。接纳／Visit只表示交接资格，不能作为播放成功证据。

## I27 采用路径与全窗口隔离

保留17的Manager／普通／odd／FSM／loading用途分派、Lua回调保护及PGD／平台／存档／Mod持久保护。Client保护在任何原生准备前成立，直到实际安全退休与最后writer收口后才交还Off；Host／Off原链透传。观测wrapper仍只为实际原调用记一次完成，主动跳过不伪造original完成。

| 路径 | 具体处置 |
| --- | --- |
| NPC／Tear／Projectile／Bomb／Laser／Knife Init | 使用已核动态Init／官方维护链；本用途临时全局Random供应Source seed，不推进本地随机源，随后安装完整最终值／资源。不是运行AI／Update。 |
| Pickup／Familiar／Effect／Slot Init | Factory／ctor保留，采用base-qualified Entity Init及官方Plus维护，主动不执行派生生成／抽奖／支付／创建Weapon等玩法。 |
| NPC Morph／bestiary | A→T退休旧use并换实例，不调用Morph；现有`pgd_encounter`仍保护已知bestiary writer。 |
| Slot／Pickup操作 | 不调用Slot Update、捐款／支付／开奖、Pickup拾取／Morph；现有`pgd_counter`、`pgd_add_collection`及Lua持久保护继续覆盖已知writer。 |
| 派生draw | Source完整姿态／几何；Pickup／Familiar inline RNG与frame在整帧临时借用并归还；Bomb禁lazy重建；Abyss只画最终ANM。 |
| 声音 | 唯一Hook清单新增`client_entity_local_sound`，SFXManager::Play RVA`0x507C60`在ClientBusy全窗口主动不调用，阻断其独立声音RNG及声音状态写入。已核Init直接callee并无Play，新增Hook的依据是全窗口隔离，不虚构Init caller。27未来播放Authority事件须另接具名用途。 |
| 退休 | 先撤关系和类别附属owner，保留ESSM／Plus正常维护；Familiar只走已核base清理，避免Player计数／位置／Grid玩法副作用；Retire内原生Stop由既有窄Hook隔离。 |

新增声音Hook、生成器与startup实际安装检查保持同一注册owner；不增加第二套Hook或停用时恢复字节。具体静态证据和UNKNOWN见[指定Luna研究与主审](entity-i26-i30-native.md)。不宣称全部原版函数或未采用Update均已穷尽副作用。

## I28 真实前缀与安全退休

`ClientNativeObjectsAbandon`现在先等待Scene用途借用、renderer CPU队列及Entity draw owner全部退尽，再逐个撤全图关系，按本地instance取得次序逆序、每泵一个类别退休，最后由Grid／Room／Player原owner收口。已Published对象也走类别adapter，不再直接清map后把Familiar交给不适用的Room通用清理。

| 实际前缀 | 处理 |
| --- | --- |
| 纯值候选，尚无原生写入 | 可替换／取消；有效旧A不被宣称回滚或重新提交。 |
| Factory返回空 | 不存在取得的use；保留实际Factory游标前缀后由Scene正常退休。 |
| 完整Acquire返回，Init未进入 | 记录`acquire_returned`；确认无valid／exists及无冲突后，只配平一次官方ESSM New／Delete，不物理析构预构造池槽，不调用未初始化派生Remove。 |
| Init／BeforeInit进入但未完成 | 保留实例、前缀和Client隔离，不能猜测派生析构。 |
| Init完成，资源／引用／发布部分完成 | 撤已安装关系，由真实类别owner清理，ESSM通知和七列表脱离按显式前缀执行；移除独立Key的附属关系不会连带删除另一主体。 |
| Remove／真实ESSM通知或原生异常不确定 | `cleanup_uncertain`／`retirement_fault`锁住owner，不再次Remove／Delete／Free，不交还Off；返回具体首因。 |
| draw异常／raw借用未收口 | `ClientEntityDrawDrained`保持false；即使CPU队列恰好为空，也不得销毁Entity／ANM owner或重建。 |

Factory返回冲突地址时保留指针及未知责任，不能置null丢掉已发生的New通知，也不能Delete另一个use。正常退休不改变自有virtual device的NEVER策略。未知SEH／VM／context丢失没有自动恢复声明；程序明确保留隔离和owner，而不是伪报安全退休。

## I29 Scene唯一提交与准确范围

Scene按同一attempt编排：Room准备 → 普通Isaac支撑／外部Player owner → Entity取得／Init／最终量／资源／发布 → Grid安装 → Entity后置关系／检查 → Grid双向引用／lifted核对 → 表现缓存与最终native检查 → Scene统一Commit。

`ClientObjects::Step(..., finish=false)`停在实例就绪边界，让Grid先取得；`VisitPrepared(attempt, ...)`只供同attempt内部检查，普通`Visit`不能枚举未提交候选。已检查对象保留`installed_`／`entity_installed_`，只有Scene调用`Commit(attempt)`才提升成后续A→T所用的对象A。旧scope、旧revision、同版本冲突和重复目标仍采用原合同；取消已写候选先安全退休，再从完整T重建。

修补正式目标的Player支撑接线：无外部Player20 owner时，在同一实例表沿17合同安装唯一普通Isaac；形态／roster超范围写前明确拒绝。具名外部owner配置仍可供20扩展，不创建第二个Player世界。

结果和`cs status`明确区分`non_player_entity_applied`、`player_support_applied`、`full_entity_applied=false`。19完整目标才声明非Player合同；17代表profile不冒认完整非Player范围。Room／Grid提交位保留；整个Entity component包含Player，所以不设置完整Entity域位，也不发送网络Applied ACK。失败、重复提交和正常提交采用同一范围口径；运行覆盖仍为I34待验收。

## I30 有界预算与稳定缓存

64M节点、32MiB聚合、深度64和4096实际resource_pose的既有约束保持。对象／render统一16384、命令131073；这些是自有值容器上限，不能扩大真实Factory。额外分别限制262144引用／曲线元素、16384资源成员槽（含合法Null）、128条／2MiB独立记录。

Factory按J273构造立即数独立限制：NPC／Tear／Projectile／Pickup各512，Bomb／Knife／Familiar各64，Laser／Slot／Text／Dummy各128，Effect1024。Native预检只盘点真实空槽及本方可退休use，并核对main／persistent／render／update／wisp实际容量；不改游标或覆写活槽。原生取得仍用`Create(type,true)`，池不足明确失败。

复用T的域解析结果与不可变类别／资源描述：重建预检的候选一直保留至实际退休后，不再整套重新解析；正常接纳也不另造一套临时全目标。最终native检查建立按地址索引的当前描述，换T／写入／退休关闭资格。表现成员／排序每T核一次并放在堆上；稳定draw只检查缓存拓扑、当前use和必要owner见证，避免各consumer重复扫全图和逐帧平方级去重。

`cs status`提供目标字节／节点、Entity成员／Player／退休行、资源槽／引用／曲线、命令进度、独立记录Pending／Presented／Expired／Superseded／字节及原生首因／失败Key／前缀。相同失败不逐泵重复trace。详细诊断与录制仍默认关闭；采集、解析、应用、draw耗时和峰值只在I34实际测量，不预报收益。

## 交接

I31复核新增消费者的P1—P6投影；I32完成产品／E2E发布边界总核对（本轮新增模块及Hook已接CMake／生成器）；I33补完整夹具，I34在全部目标完成后统一E2E，I35记录实际结果。完整Player属于20，真实Baseline／Delta／网络ACK属于21／22，独立结果实际音画消费属于27。Text／Dummy自然业务caller、原先保留的UNKNOWN及未实测特殊组合保持原事实。
