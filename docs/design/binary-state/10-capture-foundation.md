# 第10步第二项：公共采集基础设施

**2026-11-11第三项后续**：[计划执行器](10-capture-executor.md)已经接续实现通用条件／递归遍历、计数组／共享来源预检和预算／整根回卷；下文“后续完整遍历”保留第二项完成时点。各域真实来源与Collector／worker仍待接线，未运行测试，版本仍66。

2026-11-11。依据[当前合同与来源缺口](10-contract-source-gaps.md)，实现G01—G03和G15所需的公共窗口、数字绑定、预分配纯值及有界交接基础。布局／协议仍为版本66。各域真实来源、完整递归采集和游戏桥接在后续项完成；不启用一条缺源的新运行链。

## 编写代码前列出的失败方式

- 用恒真回调、最近tick或其他身体证言替代实际窗口；读前合格但读后root/use／绑定／mutation已经变化。
- 只识别RTTI便读取未初始化字段；selector读取失败退回基础臂；将protocol／synthetic／companion偏移加到根对象。
- 热路径按handler字符串查找、分配容器／诊断字符串、加锁、等待后台或触发预算账本锁。
- CopyRange批量读取跳过字段资格；条件未定便读旧可选尾值；未知处理器默认为普通memcpy。
- 容器数量先窄化，值／路径／记录／缓冲预算越界，失败留下半根记录或把Gap封存成空值。
- 纯值索引丢失规范父路径／动态槽身份，跨根或跨use混用；同一计数组重复投影互相覆盖且不检查一致性。
- 将裸指针当引用或资源身份；Pending在后台尚未绑定时写Null；资源表空间错配或来自另一个epoch。
- 后台持有主线程scratch／原生地址，或者生产者覆盖后台正在读的封存区；票据释放后仍能复用旧视图。
- latest替换抹掉cold首个完整来源；冷请求前开始的候选满足新请求；控制项被普通候选合并或溢出后仍发布。
- Begin／Stop／Invalidate期间旧写入或在途后台结果复活；序号回绕、错误生产／消费线程、双重释放导致ABA。
- 将8个fast槽当身体支持上限；满时覆盖其他身体、吞掉必要来源或不报告ordinary body-joint需求。
- 按Peer／槽重复领取整个内存池，或只计已用字节、不计预分配capacity和索引；最后租约在主线程逐帧释放而触发账本锁。
- 将公共基础完成写成各域来源闭合／采集器已启用，提前改旧运行链、布局版本或运行测试。

## 当前范围

公共层只依赖C++20、现有生成表和复制内存总账。窗口／读取／资格回调借用真实主线程owner；后台只拿自有纯值。实际Collector／PlayerPost／正常生产Hook的回调由后续各域接线，缺绑定明确失败。

本项不新增测试；功能、并发、错误分支与性能统一留第15步验收。

## 1. 公共接口与位置

新增实现编入`isaac_cs_state`产品库，不依赖SDK、Lua、RapidJSON、Recorder、旧Range或游戏对象头。

| 文件／入口 | 本项实现 | 后续使用方 |
|---|---|---|
| [`state/capture.h/.cpp`](../../../mod/src/state/capture.h) | CaptureWitness／Subject／Diagnostic、CaptureWindow、CapturePlan、CaptureCopyScratch | Collector／PlayerPost／正常生产owner提供实际证言、只读回调、RTTI绑定和具名处理器 |
| [`state/capture_values.h/.cpp`](../../../mod/src/state/capture_values.h) | 预分配CaptureBuffer、整根CaptureValueWriter、CapturePacket、CapturedBodySource及CaptureLinkResolver | 主线程封存数字路径纯值，后台用现有BodyValueSource接口编码；引用／资源到准确成员／资源表的解析归发布owner |
| [`state/capture_exchange.h/.cpp`](../../../mod/src/state/capture_exchange.h) | 单生产／单消费的固定buffer池、move-only读写票据、cold／latest ordinary／body及256项控制环 | 后续正式worker与主线程桥连接；本项不创建线程或切换现有CaptureWorker |
| [`state/body_codec.h/.cpp`](../../../mod/src/state/body_codec.h) | DescribeBodySourceField只读暴露同源scalar、components、canonical、资源表和条件／规范零／共享owner／动态槽属性 | 采集层复用现有生成数字计划，不按类型字符串再解释一次，不维护第二份字段白名单 |

没有修改唯一布局源、生成器、线格式、接受规则或布局整数版本；349布局／2806字段／85条件仍保持版本66。新增的句柄暂存和队列元数据均为进程内结构，不是新的网络报文。

## 2. 窗口、资格和数字绑定

`CaptureWitness`保存scope、实际generation／来源revision／batch／tick／outer／owner／mutation、GameFrame、阶段及body的required_revision。它不保存Peer base／target，也不把GameFrame当Authority tick；Production阶段可以没有Authority tick，但仍须真实outer／owner及当前来源。`ValidCaptureWitness`只检查值是否齐全，不能产生读取许可。

`CaptureWindow`显式绑定真实主线程ID、`current(context,witness,root)`与有界`read`。Check核线程及完整当前资格；Copy在地址／extent／32位乘加保护后读，并在读后再次Check。实际回调必须核当前root/use、绑定、成员／资源关系、重配owner和完整窗口的全局失效事实，不能传恒真函数或把上次完成tick当本次证言。读取回调负责沿用MemoryReader的不可读地址捕获；公共层不直接解引用游戏地址。

`CapturePlan`在冷准备期按生成字段索引绑定`read_requirement`回调和`(operation,handler)`处理器，并建立排序后的vtable→基础布局映射。重复注册、非RTTI／protocol映射和将vtable直接绑定某个变体臂均拒绝。Identify只用于真实RTTI对象；无RTTI视图的布局由具名owner给出。SelectArm读取生成selector，已知值选臂，未列值使用基础臂，读取／资格失败不回退。

Dispatch处理一个已选字段：先核writer与窗口证言、根身份、规范路径和实际选中块，再核读取资格。缺资格或缺处理器保留具名失败。**它是公共字段分派基础，不是已经完成的全域递归采集器。** 条件／CanonicalZero、复杂容器、共享payload、转换、引用、资源、Nested和Choice必须由后续具名处理器按当前合同处理，不能默认memcpy。所有回调为同步`noexcept`接口，不保存原生指针，不调用玩法／Lua／随机或分配诊断字符串。

只有生成表明确为Object、非protocol、无条件、无规范零分支、无引用、无嵌套且原生／线上宽度一致的Copy可以自动读取。安全CopyRange须完整由这些字段覆盖，批读前核区间内全部字段资格；同对象、同完整证言和同range复用一次scratch复制。区间不满足条件时回到单字段直接复制，未知操作不降级。CopyScratch只属于当前同步遍历，不能随packet交给后台。

## 3. 预分配纯值与根事务

CaptureBuffer在冷准备期一次分配record、cell、canonical父路径和字节区，实际vector capacity及管理开销计入现有Values总账。逐帧writer只移动有界游标；不创建JSON／map、申请内存、获取账本锁或格式化错误。

BeginRoot固定纯值subject、layout、world／body／value及块mask。PutScalar／Count／Presence／SlotMask／SlotLayout／Links分别写入类型化值。父路径逐级核生成的Nested／Container／Choice、元素范围和canonical身份，多投影只开放本记录所选父投影的子块；不能借canonical别名读取另一个未选块。数量在u32下检查后才按生成限制使用，固定槽数量、动态Weapon允许集、资源表空间和标量宽度均不靠字符串猜测。

FinishRoot将本根cell按规范父路径和数字字段排序，后台以二分定位。重复Count／Presence只在值完全一致时合并；其他重复值和冲突都失败。失败后AbortRoot回卷本根全部游标，首个具体诊断保留，整个packet不再可Seal；不会丢弃失败根后把余下部分伪装成完整世界。Seal还要求没有未完成根，且实际根数等于枚举owner给出的expected_roots。

**根数相等只证明值事务封闭，不证明预期成员集合、全部条件／投影／必要显示均已完整。** 后续域遍历必须建立真实根清单、填齐必要值；后台正文及完整目标校验仍按第9／11／12步执行。不能用空值布局、最短样本或该Seal接口证明来源已经闭合，也不能因此ACK。

引用与资源先存`Null／Identity／Pending + 自有handle`。字节区采用固定9B暂存编码，排除C++结构填充；禁止裸地址充当handle。CapturedBodySource在后台绑定精确记录和只读解析context：确认Null才输出FFFF；其他句柄只有成功解析到准确表项才能输出u16，尚未编号／缺声明保持Gap，不折空。资源空间取同源数字计划，解析scratch由后台预分配、不得覆盖packet值区。调用方必须让解析context对应本目标准确成员／资源快照，不能访问原生对象。

## 4. 封存区交接与生命周期

exchange构造时显式给出唯一producer／consumer线程ID，不能由首次任意调用抢占。所有同步状态采用Win32保证无锁的u32原子；不使用atomic shared_ptr、mutex、condition_variable、等待或64位原子同步。票据共享的是冷分配的自有State寿命，逐帧移动／释放票据不创建新的控制块。

| 状态／操作 | 所有权与行为 |
|---|---|
| Free→Writing | producer有界扫描空槽，写票据独占buffer；无空槽返回QueueFull，不等待或覆盖Reading |
| Writing→Ready | Publish在真实外层提交后再次核实际窗口／证言；body还核anchor与本身体、required_revision和记录流归属；成功后producer不改packet |
| Ready替换 | ordinary只替换尚未取出的latest；相同body身份只替换自己的最新候选；被替换槽释放为Free |
| Ready→Reading | consumer先取控制，再取cold；其余ordinary与body轮转，避免某一流持续占先；一次最多持有一张ReadTicket |
| Reading→Free | consumer正常结束或淘汰后释放；任何撤销均不提前归还后台仍借用的buffer |
| Begin／Invalidate／Stop | generation／watermark撤销旧Writing、Ready及后台结果的成功资格，清待取槽；Reading保留到正常释放；Begin不复用旧序号 |

ReadTicket.Packet只在票据、线程、generation／watermark和冷请求仍有效时给出只读视图。consumer须在把后台结果交给发布owner前再次Current，并一直持票据到所有借用span消费完毕；Reset之后不得保留视图。exchange的会话owner必须保留到正常排空，在安全退休／后台清理阶段才释放最后所有权，不能在逐帧主线程路径销毁整个池或触发最后租约回收。

Begin发起冷请求；RequestCold另增不回绕的请求号并撤旧cold资格。只有请求之后Acquire、仍符合当前请求且合法完整发布的第一份ordinary进入独立cold槽，后续ordinary不覆盖它。请求前已开始的候选只能作为普通候选，不能满足新请求。Invalidate后原冷来源被撤销，下一个合法来源重新满足当前请求；producer不等待cold消费。

控制环容量256，Begin／Stop／Invalidate／ColdRequest／RetireBody保留FIFO且不随数据合并。控制溢出、错线程、状态冲突或序号耗尽撤销成功资格，并在固定Statistics中保留首个failure。溢出／线程错误后仅在旧控制已排空、在途票据释放后显式Begin重新启用；序号耗尽须正常退休整个exchange后另建，不将计数归零复用旧票据。

body默认8个fast身份槽，可冷配置至256；默认20份buffer容纳旧候选、当窗多身体暂存、cold／ordinary及在途读写。这些是保护／存储参数，均不是原版身体数量上限。槽满明确返回NeedsOrdinaryJoint，绝不覆盖其他身体；实际下一次ordinary的完整body-joint重采／发布由后续owner落实。本层不把PlayerPost包改标world，也不因快槽不足断言身体离开；绑定／shape／拓扑变化须先由生命周期owner撤销候选并退休旧身体槽。

## 5. 容量、成本与尚未完成项

默认每buffer为512根、16384个cell、32768项父路径和1MiB字节区；可按实际来源在冷准备期配置，超限具名失败，不截断。exchange同时按请求估计和实际保留capacity检查自身≤第9步shared_value_bytes（64MiB），再受进程Values512MiB总账约束；不能按Peer重复领取。buffer、plan、索引和控制均实际记账，不把只读span当成独立所有权。

上述局部容量不替代第11步所有公共owner／资源目录的合计配额，也不表示默认容量足以覆盖原版所有场景。公共层未分配Wire／Workspace、未编码或发送消息，1MiB暂存区不是首次准备≤1MiB网络总额的证明。来源复制、排序、路径检查、回调和实际并存高水位还须在第15步按stage12与总预算统一测量，当前无p95／p99或带宽通过结论。

G01—G03现已具备公共窗口／绑定／事务／句柄／交接机制；以下仍需各域和后续步骤完成：

- 从真实J273／官方模块构造RTTI映射与具名读取资格，把Collector ordinary、PlayerPost及正常生产点接入这些回调；禁止恒真替身。
- 生成计划的完整条件／递归遍历、同组投影和全部具名转换／容器／共享payload／资源来源；本项没有替这些未核来源补零或开放任意偏移。
- 实际成员枚举与Identity／Pending预约、资源不可变描述及精确快照resolver，全部身体的fast选择／ordinary joint处理和完整来源闭包。
- 各组HUD、文字／字体映射、程序资源／坐标、音频逻辑生命周期等G04—G15工作；原生UNKNOWN仍保留。
- 第11步正式worker、memcmp／逐Peer发布／真实发送账本，第12／13步完整候选／ready／原生提交，第14步旧链退役。现有Collector、CaptureWorker、Lua入口和运行协议未被提前切换。

## 6. 实施与纯编译记录

先列出本页失败方式，再编写上述公共代码及中文注释。首轮Release／Win32的isaac_cs_state纯编译完成，没有项目编译错误。静态复核随后补齐规范路径的选块约束、引用固定编码、资格失败的字段定位、普通／身体轮转、冷请求撤销、实际capacity检查与错误线程／溢出恢复；没有通过执行程序验证这些分支。

静态修订后的Release／Win32产品整体纯编译／链接完成，入口为`cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /v:minimal`，实际产物为`out/repentogon/zhlIsaacCS.dll`。构建配置`ISAACCS_BUILD_E2E_TOOLS=OFF`，新公共源编译无项目警告；整体构建保留官方SDK RapidJSON的既有C5054枚举运算弃用警告。首轮状态库和本次产品构建均无编译失败，不将编译成功写成行为或性能通过。

未运行游戏、E2E、单元测试、probe、并发／性能试跑或独立校验脚本，未新增测试或安装到游戏目录；第15步仍是唯一统一整体验收。本项公共基础完成，各域真实来源、完整遍历与运行桥接按第5节继续。
