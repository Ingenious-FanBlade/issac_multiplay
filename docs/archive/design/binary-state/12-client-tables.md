# 第12步第2项：成员表与资源表管理

日期：2026-11-11。上级：[第12步](12-client-decode.md)、[接口与寿命合同](12-client-contract.md)。状态：**第2项纯值表管理已实现并编入状态库，正式Client运行接线尚未完成**。先登记下列失败方式，再编写实现；第15步唯一统一验收，本项没有运行测试。

**后续交接**：第3项[影子／候选事务](12-client-shadow.md)已复用本项快照及账户，表／影子共用ClientStateWorkspace；先回收无外部引用的旧影子，再回收表快照。共享工作区Host桥随后已修正构造问题，第3项产品纯构建成功；下方保留本项当次整体构建失败，不改写历史。

本项未修改布局源或版本。开始时共享工作区布局版本为66，期间既有Host相关改动将其更新为67；最终状态库编译使用当前生成版本67，不回退其他改动。下面源码实现不依赖硬编码版本，始终读取LayoutVersion与同源预算。

## 实现前列出的失败方式

- 解码或合并返回的entries／update／payload仍指向调用者正文或scratch，下一消息覆盖后旧快照损坏。
- 新资源快照通过完整旧快照链固定payload，导致无限保留旧epoch；相同内容却复制五份、漏记capacity／索引／控制块。
- 同epoch重传不核原update，旧epoch按最新表解释，完整恢复省略既知资源，或同run复用资源号／改变内容。
- 成员空间／布局未在本地注册仍接受；Leave／Reshape身份不符；退休use复活；形态／use倒退或同shape改布局。
- 新成员占用仍被候选／安装根／在途借用的旧序号；full恢复和跨route重绑绕过历史。
- 合并中途修改当前表／生命周期历史，后续资源图、分配或合法性失败留下半个事务。
- 新快照超过每类5份，仍覆盖有引用的旧快照；旧registry强引用永不回收。
- 资源收到即标ready，缺依赖图／本地文件失败变成Null；迟到准备反馈把新route／新后端代次标ready。
- 缺声明、环、深度、空间或图形用途错误未由正式资源codec检查；空目标资源根误检查整个目录。
- 原生对象退休与表安装混在一起，提前删除已展示对象或发布AppliedAck。
- 共享池与Peer子额度分开建账；分配失败后额度不归还；正常关闭未等外部借用／准备回报就清空历史。
- 普通Open／Resync偷偷重编号资源epoch；显式重编号丢掉同run既知内容身份。
- 错线程安装／回收，主线程最后释放大缓冲；资源图工作区并发运行或逃逸到持久视图。

## 实现与所有权

新增模块均为C++20纯值代码，不依赖游戏／SDK／Lua、原版Online或Isaac Sync，不创建线程、不发送协议包、不写原生对象：

| 文件 | 已实现内容 |
|---|---|
| [`client_memory.h/.cpp`](../../../../mod/src/state/client_memory.h) | ClientStateMemory／ClientStateAllocator、类型化ClientTableError；表与后续同Peer影子／缓存共用一个额度账户，底层仍扣进程复制三池 |
| [`client_tables.h/.cpp`](../../../../mod/src/state/client_tables.h) | ClientStateTables、只读成员／资源快照、精确epoch查找、成员历史与序号借用检查、资源图／准备状态／票据、Close／Collect／Drained与显式资源epoch重编号 |
| [`resource_protocol.h/.cpp`](../../../../mod/src/state/resource_protocol.h) | StateResourceBounds公开同源资源容量；不改payload或接受语义 |
| [`CMakeLists.txt`](../../../../mod/CMakeLists.txt) | 两个新cpp编入isaac_cs_state产品库 |

ClientStateTables构造时必须由产品owner提供本地`ClientMemberLayoutBinding{space,layout}`注册，复制、排序并冻结后使用；不接收网络注册，不缺省放行任意非protocol布局。每项检查已存在布局／非protocol／真实块及重复绑定。具体全域产品注册在第4／6项接线时提供，不能把库可接受注册解释为游戏已完成全部域支持。

### 成员表

InstallMembers接收准确StateRoute和当前调用内稳定的正文span；分配前通过生成的固定头核版本／完整scope及count，随后复用DecodeStateMemberTable与MergeStateMemberTableInto。成功后的entries和原update各有独立稳定数组，返回快照没有借用输入正文／scratch，调用结束即可复用输入。

同epoch针对保留的准确快照比较完整目录或原update，不按最新生命周期历史重新执行旧操作；精确重复返回Duplicate，冲突返回EpochConflict。已不再保留的较旧epoch返回Stale，不假装保存了全部历史正文。非零previous必须匹配当前构建epoch；full恢复仍经过真实生命周期与借用检查。

按`space,id`保留本run的最高use／shape、当前布局及最新表中的存活状态。已经Leave／被完整表省略的use不能再Join，use与shape不能倒退，同use／shape不允许改布局；合法新use及真实后继形态可进入新快照。full恢复与同run换route首次完整表也核此历史，普通Close／Open不清空。历史仅属于元数据，表到达不退休已提交影子或原生对象。

序号改绑为不同Key或use之前，检查所有仍由外部持有的成员快照；旧安装根、候选、inflight或主线程借用尚在时返回MemberReuse。相同Key/use的合法shape前进仍由精确快照隔离，不把旧shape的内容混进新根。最后按完整结果构建独立下一历史；目录、历史和current在全部成功后无分配提交，失败不留下半次退休。

### 资源表与数字图

InstallResources复用正式资源codec核全部typed payload、table／kind／用途、完整表及不可变内容／单调编号；旧条目不可遗漏或改绑，同run资源编号不回收。每个新内容项只复制一次到自有Values字节；已有项按独立不可变内容owner共享。新快照持有自己的目录／update元数据、内容引用数组及edges／offsets，不持有完整旧快照链。

安装前构建ResourceGraphPlan并由正式BuildResourceGraph检查已知目录的环／深度／图形用途与总量。缺声明保留为未闭合，不改成Null。持久edges／offsets属于快照，state／heights／order及解码元数据属于短期Workspace；成功／异常退出均不逃逸临时span。边容量按实际payload的u16引用保守上界及同源最大边数分配，不预占全部字段的最大容器。

Resources(epoch)只返回本session/run内保留的精确快照，Members(epoch)还核当前完整route。Prepare针对指定快照及实际资源根复用闭包检查；空根直接给出合法空闭包，不调用“空roots表示全目录”的底层默认分支。调用者提供的order／state／heights不足时先返回类型化Budget；结果中的顺序span仍属于调用者scratch，须在复用前消费／复制。

2026-11-16 应用频率修复顺序 1 接通上述增量合并路径。Host 用可靠完整回执更新各表的增量基准。在启用 `state-table-delta-v1` 的连接中，资源表未变化时，接收器可在安装成员表后用当前资源代次回报 ContextReady。布局82起，Host 对基线和真正联合仍凭实际完整发送的精确目录对推进；普通 World 可并行发送。Client 后台回收后报告实际外部借用的最小成员代次，旧目录的编号借用检查继续执行。传输取消重发时，接收线程按路由内的表消息序列跳过迟到的旧传输，非零 `previous_epoch` 仍须匹配当前代次。

### 本地准备与过期反馈

每项资源的Unprepared／Pending／Ready／MissingLocalResource／Failed与元数据分开。MetadataReady仅表示两类current元数据存在，绝不代表资源／完整候选／原生ready或AppliedAck。

BeginResourcePreparation核精确epoch和声明项，生成包含route／key／epoch／资源后端generation／不回绕serial的票据，并固定其资源快照。Pending报告保留票据与借用，最终Ready／失败／取消才释放；ResourceReadyContext查询只读这些纯值反馈，不能自行加载或访问游戏。确定MissingLocalResource／Failed不允许在同代次循环重新准备，已Ready也不因重复Begin降回Pending。

InvalidateResourceGeneration只接受更大代次，撤销旧就绪资格；普通同run新route可复用同后端代次的确切已准备内容。旧route／旧generation最终反馈只排空原准备票据，不给新代次授Ready；迟到Pending不提前释放尚在使用的输入。第13步必须根据真实主线程物化／失败／排空回报这些接口，当前没有假造准备成功。

### 快照上限、关闭与重编号

registry为每类至多5个实际快照预留固定位置，包含current、新建及外部借用；Collect只删除非current且没有外部引用的项，所有最后大块释放发生在后台。满时抛SnapshotLimit，不覆盖旧span，不丢事件／在途借用，不自行扩大容量。具体候选淘汰与恢复由第6项调度。

Close停用route查询并保留本run身份／内容历史；Drained只证明本模块没有外部快照和待完成准备借用，**不证明原生／GPU已排空**。生命周期owner须正常收回全部借用，最后在显式指定的后台线程销毁表管理器；不能提前销毁后期待shared_ptr析构自动满足主线程无等待要求。新run只有前一run借用排空后才能重置历史及内容；旧history／local容量一并归还。

RebaseResources只在新route尚未接纳任何资源表、匹配上次已Close的retired route、CPU快照／准备借用排空后显式调用；生命周期owner还须先证明旧原生借用已排空。它复用RebaseStateResourceEpoch逐项比较全部既知编号／kind／payload，允许epoch=1重新编号但不丢同run内容身份；事务提交时替换旧epoch registry，防止旧epoch=1影子遮蔽新目录。普通Open／Resync不调用此路径，同一新route也不能反复重编号。

## 内存与失败接口

ClientStateMemory的Values／Wire／Workspace子额度读取StateBudgets，Allocate先扣Peer子额度和既有进程池，再执行原生分配；失败回滚。分配器用于vector容量、allocate_shared对象及其STL控制块，按实际allocate数量收费；账户／既有Charge自身管理对象另有冷准备的保守1024B余量。分配失败诊断保留类型、池、requested与available，不从错误文本猜调度原因。

后续同Peer owner必须共享传入的账户，不能每个模块再Create一份领取子额度。该类不自行推断网络Peer身份，唯一会话账户与正式worker的绑定在第6项接入。当前表解码／图构建／Prepare通过本模块单一后台工作位置互斥，竞争返回WorkspaceBusy；与整套编码／解压owner统一的工作区调度也在第6项接线。

安装方法只在指定后台线程执行；错线程／错route、表链断裂、精确epoch冲突、身份倒退／退休、序号仍借用、缺表、快照满、资源准备busy／确定失败和预算分别给类型化ClientTableError。低层codec的非法输入Gap在边界转换为InvalidEncoding，并保留原原因作诊断，不匹配what()字符串决定Wait／Resync／Terminal。实际会话动作在第6项驱动。

## 编译记录与边界

新模块首次编入isaac_cs_state即编译成功。静态复核随后补齐显式重编号只准新route首次、相同epoch registry替换、确定准备失败不循环、跨run释放旧capacity、预算现场及重复epoch冲突分类；最终Release／Win32状态库纯编译成功，新模块未报告编译警告。命令为`cmake --build build/repentogon --config Release --target isaac_cs_state -- /m:1 /v:minimal`，E2E构建选项为OFF。

本项也尝试整体产品纯构建，**该次未通过**：共享工作树Host采集桥的WorldCapture隐式构造在`game/observation/world_bridge.h`遇到`unique_ptr<BinaryEntities>`不完整类型（C2027／C2338；可见调用点包含ops/world_commands.cpp），当前新状态库在此前已编译成功。未改写正在推进的Host采集桥来掩盖此结果，也不把状态库成功写成整个DLL成功。整体输出另有官方SDK RapidJSON C5054和remote_life.cpp既有未引用参数警告。

本项没有修改布局源、部署、启动游戏、运行E2E／单元测试／独立校验脚本或任何试跑，也没有新建测试。正式网络接收／控制发送、全域注册、影子候选／结果与事件、主线程准备反馈接入和原生安装仍由第3—7项／第13步完成；不因表管理可编译而宣称同步可运行。第15步仍是唯一统一整体验收。
