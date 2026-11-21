# Client转场拆分、Host目标关联与等待

日期：2026-11-12。**静态研究后的实施交接／建议，尚未接线**。Q5保持：用Host已决定的路线回放原生过渡，由原生对象／Render绘制。新发现的Start／Update混合副作用见[原生证据](native-transition.md)，不把定位成功写成整根可放行。

## 1. Host采集载荷与关联

Start入口`0x823990`是比Game外层更完整的请求来源，但仍不是最终房间事实。一个转场记录需要：

| 信息 | 来源与用途 |
| --- | --- |
| session／转场序号、Host frame、source room／dimension／scope | 主线程正常启动窗口；区分同房再进、快速连续转场和旧会话事件 |
| 五个请求参数 | index、direction、animation、Player稳定成员身份、dimension；Player*只用于当窗身份解析，不上线 |
| 接受／归一化结果 | original正常返回后，核对本次owner／字段变化与scope；记录owner的+14/+18/+10/+1C及实际关联。StageTransition忙时可拒绝，不能每个入口请求都发已开始事件 |
| 最终world目标 | Host ChangeRoom正常完成后真实current room／dimension、目标world版本／结构事务、与本次转场序号的关系；Maze可能在Level里才选出，不能拿请求房号替代 |
| 必要阶段恢复信息 | 当前mode／counter／duration／Boss标记和相关Sprite／资源的状态身份；用于中途连接／恢复，不能只重放旧Start。具体有界状态由同源布局定义，不序列化strong／map头 |

Host普通原链保持exactly-once，入口捕获和返回观察不修改它的选路／随机。可以先发送已接受的启动及尚待关联的目标token，后续随实际world补齐最终目标；Client在换房点只接受同一转场token的完整目标，不凭同一个数字roomIndex猜测对应关系。

animation11／16的同房结束等不经过ChangeRoom的路径，记录明确的“未切换world／已结束”结果和当前目标关联；Client不能对这类事件永久等待一个不会发生的ChangeRoom回执。取消／被新转场替换同样需要显式终态，不能仅靠超时猜成成功。

采集不能只挂`Game::StartRoomTransition`：内部Start另有caller。也不能通过重跑getter／Level／RoomInit为请求预先求目的地。Host真正发生的失败、拒绝、取消、同房转场和替换分别表达；不伪造Start成功或回滚记录。

## 2. 为什么只hook ChangeRoom 不够

原生mode2到期在`0x826BDC／0x826BE3`先重置counter并切mode3，再`0x826BE9`调用void ChangeRoom。hook里“目标未就绪所以直接return”，会让外层把它当成已换房；mode1两个callsite也可能继续后续逻辑。函数没有可向caller报告pending的返回契约。

此外mode1／3／4和Start本体直接写Player，现有Player Update hook只挡住子调用，挡不住这些内联写入。把world字段事后写回不能撤销随机、生成、进度、资源引用或生命周期副作用。

因此Client在拆分接通前继续拦截完整Start／Update／ChangeRoom。实际进入Manager显示推进的是**具名转场显示阶段**，不是无条件`CallOriginal(RoomTransition::Update)`；Q7的Host显示作用域也按这份实际表生成。

## 3. 建议的显示驱动

> **2026-11-12 审阅后标注：本节由 Q11（用户决定）取代，原文保留。** Client 不写自有显示驱动，转场显示字段（mode、counter、duration、progress、direction、roomIndex、dimension、animation、Boss 标记）按 Host 当前值随 world 原子安装；Client 只做旧／新 surface 捕获、滚动补偿、versus／肖像 ANM2 推进、HUD 刷新请求和原生 Render。下文第 3 步“换房准备／等待”由原子安装天然得到；第 1 步“启动”不再在 Client 上准备转场字段；第 4 步中的新 surface 捕获、第 2 步旧 surface 的“旧 world 尚合法时捕获”仍适用，捕获点改为安装新房间 world 之前。第 6 步“不写 Game+22E90”改为：该字段是 HUD `_isVisible`，按 Q8 同步。见[方案 4.5](../../client-display-delegation.md#45-原生切房过渡q5)。

对内联部分使用自有有界显示驱动，复现已经核定的纯mode／counter／progress与surface准备顺序，调用完整合法的原生显示leaf、ANM2推进和`RoomTransition::Render`；不跳进原函数中段，不运行Player Update、传送／治疗／复活、原版换房／回溯、环境音生产或进房选曲。此为实施建议，未新增用户决定或运行保证。

1. **启动**：验证事件身份、源world、参数与资源，在正常owner中准备转场字段和versus资源。原生Start中的Player动画／Velocity／+410写入不执行；这些结果由Host当前Player姿态／状态供应。Host转场内的身体姿态也须有正常采集来源，不能靠Client补模拟。
2. **旧画面**：在旧world尚合法且安装没有进行到一半时，正常图形上下文捕获本地旧surface；这是转场功能本身的本地显示资源，不是测试artifact。保存strong引用和内容代次，不能在旧world退休后再尝试绘制它。
3. **换房准备／等待**：按native-transition阶段表到对应换房点时，只推进到可等待的显示阶段；保持mode、counter、progress和已捕获旧surface。目标不完整时不调用任何原生换房／Update，不推进事件消费水位为已显示；网络接收／资源准备可继续，不让主线程阻塞等待。
4. **目标安装**：同token的完整world与资源就绪后，在既有NativeBuild／NativeApply事务内退休旧原生对象、安装目标、F／锚点／SpawnSeed／Q8与注入；成功后才建立新surface。失败按既有部分应用恢复，不显示混合房间，不提前ACK。
5. **入房显示**：按实际方向／两房几何和本地camera scroll建立+22C/+230补偿，继续纯显示counter与原生Render。门、背景、光照和相机资源按RE-2窄冷构建；不调用完整RoomInit。Host选择的音乐／循环声／Boss演出由RE-8状态／事件接管。
6. **完成**：以显示状态及Host必要姿态结束，按正常owner退休本方surface／map借用。mode4不能等待本机Player模拟跑完；禁止在结束时擅写保护帧、恢复与Host不符的可见性或触发道具。Boss结束只推进显示mode，不写Game+22E90或Player+418。

若实现选择保留某个原生Update正常子分支，必须同时隔离该分支的全部直接玩法写入和调用，不能仅凭mode编号白名单。当前证据已足以否定“仅替换ChangeRoom”的实现，尚未实现这种替代方案。

## 4. 冷构建与本地资源

`0x823820`按owner正常创建两张surface；`0x703E40`的本机resize路径也会重建它们。资源存在性、内容已捕获和GPU可消费是不同状态；本机分辨率／画质保持本地。旧／新surface不得网络化为Host原地址或像素包，不与已退役的旧显示结果协议混用。

恢复camera／door／backdrop时复用RE-2与已有NativeBuild，不运行Game::ChangeRoom包裹的额外RoomUpdate。普通Room::Render和RoomTransition::Render遵循Game::Render的原生二选一，不能从MenuRender补第二套场景。图形flush／present只在已理解边界按需要使用，不能把含显式SwapBuffers的完整ChangeRoom当作纯资源函数。

Boss Start API还会选音乐、停环境声并直接清Game+22E90；Client具名BossReplay范围需暂停这些子生产，让同步音乐／循环集合成为唯一owner，并对直接标记写入与Host当前状态安装作窄协调。（2026-11-12 审阅：`Game+0x22E90` 是 HUD `_isVisible`，按 Q8 同步，下一次安装即覆盖本地写入，无需另作协调。）只拦子调用不算完成隔离；接通前不放行完整Boss Start。其余回调按Q6放行；原生StartBossIntro虽只有两个参数，依然需要合法roomIndex／配置／Player资源，不把官方Lua安全包装误当成C++函数内自带的检查。

## 5. 断线、替换与失败方式

以下列为第15步E39及实施前失败清单，本轮未执行任何实验：

- 目标晚到、缺资源或无目标：保留仍有效的旧显示／等待状态，禁止void跳过换房后继续原FSM；不能无限增长队列／surface。
- 快速连续转场或重传：以session＋转场序号幂等；后一个转场明确替换或排队，不将旧目标挂到新事件。像素化RenderTarget按真实owner退休，不反复Start泄漏栈。
- world安装／新surface捕获部分失败：停止受影响显示，沿13.3的Resync／完整重建，不把旧ACK作为新目标成功。
- 断线／会话结束：停止回放与publication，排空在途图形和引用，再退休本地阶段；不向已退休Game／Room写旧mode。重连用当前状态／完整目标恢复，不只重放旧事件。
- 活跃转场中resize：两张新分配surface不包含旧画面；必须选择合法的重新捕获／显示中断与资源恢复，不能假定透明继续或回读已退休旧world。具体运行表现仍需E39确认。
- 特殊动画（同房、Hourglass、镜子、minecart、复活、Death Certificate）、Boss开场：只回放Host结果与显示段，不能沿动画编号恢复对应玩法命令。
- 帧号停顿／跳帧与本地30Hz推进：不按ΔF补跑完整Update；Game F仍由world安装，Manager M仍本地推进。转场等待不得隐式暂停Authority或改输入调度。

尚未完成的工作是实际窄驱动、资源／目标接线、故障路径和整体验收，不是继续穷尽每个Render尾部helper。本轮没有修改玩法授权、C/S与原版Online／Isaac Sync隔离，也没有新增gate或哈希要求。
