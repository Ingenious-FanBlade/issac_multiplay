# tx-04-21 I31—I35：构建部署、场景、损伤、对照和归约

日期：2026-11-05，Asia/Singapore。I31—I35已完成源码／脚本编写与主智能体静态复读。**没有构建、运行测试、启动游戏或部署到游戏安装目录。** 整个tx尚未完成；I36全窗口隔离、I37全实现审阅和回归归并、I38唯一启动／远端编排继续推进，I39再先执行P2全E01—E26及全部子用例，全部通过后执行P4全套。本文记录功能接口，不是测试artifact或运行通过报告。

## I31：同源版本和双配置发布

`replica/stream_version.h`统一声明framing2、stream schema1、compact schema1；codec、压缩分片、合包、CMake发布元数据与目录生成器读取同一源码定义。原有Win32 v143、`/MD`、纯值C++20／已核SDK边界C++17保持一致。zstd静态链接，包中带原许可证，不需要zstd.dll。

`build.ps1`分别使用`build/repentogon`与`build/repentogon-e2e`，输出`out/repentogon`与`out/repentogon/e2e`。开始构建时移除上一轮完成记录，只有整次编译／链接、目录和Lua资源复制成功才产生`build-complete.json`；配置阶段生成的flavor文件不冒充构建成功。没有哈希校验。`build-flavor.json`为v2，明确目标、版本、静态zstd和21运行验收待I39；20历史结果仍保留在原文档。

`package.ps1 [-E2E] -OutputDirectory <新目录>`按显式清单打包，产品不含fixture、Rust工具、inspect、故障注入和逐版本明细实现；E2E工具位于独立`e2e-tools`。正式观察Lua、UI、默认Off配置与目录照常发布。E2E宏同时传到其纯值库，避免DLL有入口而库中没有相应实现。产品代码不依赖测试token。

`deploy-tx-04-21.ps1`提供`Install/Restore`，参数为`GameRoot`、独立的`BackupDirectory`以及安装时的`PackageDirectory`。它使用包的`deployment.json`，逐个备份原文件，在覆盖前写恢复记录，记录真实目标、原文件来源、原来不存在的文件及所用构建声明。还原只复制原文件或删除此次新增的单个文件，不递归删除游戏树；`e2e-tools`不装入游戏。远端设备在各自安装位置使用相同包／清单。实际调用和运行库缺失场景归I39，正常完成后由I38还原并清理临时备份。

## I32：两人完整子用例与四人扩展

`tools/harness/src/tx21/cases.rs::suite/phase_suite/loads`是子用例的唯一代码目录，`scenarios/tx-04-21.json`提供环境、网络档与人工操作约定。E01—E26逐项展开协商错误、完整冷端、稳定负载、多身体、战斗、拾取、结构／资源、物理输入、同版本对照、N档、ACK、故障、退休、产品与回归动作。原有Entity16个、Player24个具名故障边界逐个列入；停止按准备／传输／应用／显示乘四种原因展开。所有P2子项保留到P4；P4增加另外两组输入、三Peer公平及健康端独立性。I37继续将18—20的全部受影响目录项目展开到E25，旧离线成功不能替代网络回归。

`tx21::execute`解释真实控制命令、物理按键、代理、人工窗口、持续采样、对照与截图步骤；观察器必须返回具名实际结果，缺观察器、缺字段或负载未达到均不能通过。`Driver`是I38提供真实进程／远端生命周期的接口，当前不提供启动游戏或单子项预跑入口。P2/P4启动顺序、准备确认和连续修复续验由I38统一组织，不能直接将此组件视为已经可运行的全套验收。

`physical_script`要求实际每节点键位及窗口，保留左射→左＋右→左→中性的真实物理顺序，覆盖后按优先和释放回退；另有移动与bomb／active／card-pill／drop四类边沿。它不使用Submit取代物理源，结束及异常均请求释放。调用方须保持相邻物理步骤的held集合，观察canonical、ordinary／odd逐身体consumer及实际后态。

`tests/e2e-mods/isaac-cs-test/tx21.lua`只在配置为Host时注册玩法回调，Client直接返回。F8按配置的动作序列布置普通／Jacob-Esau／混合身体、资源和装备，补充真实移动NPC及攻击实体，并输出实际成员数量。AI、伤害、拾取及炸弹破坏由Host原版模拟裁决；没有冻结更新、冻结序号、Client造对象或文件导入。NPC／攻击下限和持续时长由来源及真实变化见证判断，夹具打印的标签不算覆盖。

L0a／L0b分别为两体／四活动体，稳定180秒且战斗至少60秒；L0c混合稳定60秒。P4对应L1／L2四体／八体及L3混合；L4损伤30秒＋恢复60秒。实体生成使用从212121起递增的显式非零seed；它不是run seed，后者必须读取实际Game Seeds。Basement普通d.10的实际房间／descriptor在计时前核对，准备失败不得按超时算完成。自然互动至少60秒；每次启动先确认准备，只跳开场，到标题／选档页停止手动操作。Host暂停／控制台和Client焦点／UI均有明确人工窗口及停止信号。

## I33：真实协议的有界UDP代理

`tx21/proxy.rs::Proxy`为一个或三个Client分别绑定前端socket与连接Host的后端socket。配置明确监听地址、Host地址和Client IP；首次真实Client源端口确定后冻结。代理只处理自有报文字节，不能访问游戏对象或生成有效世界。

物理Host出站共用1,250,000 byte/s总桶，单Peer固定312,500 byte/s，反向每端50,000 byte/s；IPv4／UDP28字节计入，每个实际转发副本分别计费。模组937,500 byte/s总预算、四lane及初建借用另由正式socket发送明细归约，不能将代理内环重复算成模组流量。N3慢端为62,500 byte/s，下行中断2秒时停止该端排队下行发送，上行心跳仍可通过；30秒慢端与随后60秒恢复由场景固定。

N1逐方向40ms、±10ms抖动、0.5%丢包、1%重复、2%额外50ms延迟，PRNG序列可重现，记录实际命中而不是只报告配置。规则可以精确选择Peer／方向／kind／lane／transfer及首中尾片，删除ReceiveAck／AppliedAck、补重复和乱序。合包命中记录连带影响的消息数，物理数据报仍只记一次。旧route、错connection／transfer、同片异值、同input sequence不同合法移动、长度／数量／reserved／版本、压缩帧／窗口／展开声明及ACK版本错误从真实包有限变更。错误具体在哪层被拒绝必须由真实端观察；不把某次schema拒绝冒充另一条送达／ACK历史检查已经覆盖。

缓存上限8192个数据报且8MiB；规则最多128条，每条最多20次，额外延迟最多2秒。队列满、未命中、端口错误和停止丢弃均输出实际事实；没有全局网络设置更改。停止通过独立原子标记收口，不依赖挤满的控制队列，也不在游戏DllMain等待代理。

## I34：合格来源、原生回读与真实render版本

E2E显式命令使用现有`world_e2e_token`并要求Manager主线程；产品编译没有实现：

```text
cs stream_test <token> on
cs stream_test <token> arm source 5 0
cs stream_test <token> arm native 5 0
cs stream_test <token> snapshot 0 0
cs stream_test <token> snapshot <id> <page>
cs stream_test <token> trace <cursor>
cs stream_test <token> off
```

source与native命令分别发往Host／Client。kind1是Manifest、5是WorldDynamic、6是PlayerFast；revision0选择该类首个新版本，非0选择精确版本；成功保存一次即撤arm，不在高频路径反复序列化全闭包。I38可在明确的视觉中性段用代理短期延迟，先取得Host版本再arm Client相同revision；未送达／错过版本须报告并重新安排指定窗口，不比较两个独立next。Host在合格`Capture::Make`完成时保留不可变范围与精确闭包；Client只在`CommitStream`真实提交时复制inspect。缓存最多16项、合计64MiB、TTL10秒，单条序列化32MiB；分页已开始且每2秒有进度时给予绝对120秒的有界读取租约；过期／预算不足明确缺测，不用另一最新版本补洞。分页8KiB原字节转hex，避免切断UTF-8；没有每tick全域大DOM永久历史。

`ClientTestInspect`现允许显式token的真实NetworkStream，仍保留原fixture路径。Room核对真实根／容器／descriptor；Grid使用原owner的只读Check并复制真实描述；非Player核对完整公共／类别字段、资源、实际成员与引用；Player继续完整状态／装备／角色／HUD／关系后态检查。新增原值输出使用已有具名字段表读取，不把待应用DTO填进native栏目。复杂资源和容器保留具名完整后态验证，原值与验证事实分别报告。

`compare.rs`逐字段比较Host／Client同一个world、root、sample、完整闭包和实际成员／Key／shape，并比较来源可对齐的原生公共／类别／Player字段。只有原生合同有对应私有语义的项明确列为verifier-only，不伪造公开别名。显式Null／空／零／缺成员保持不同；无全局浮点容差或状态哈希。某次普通目标和更新的身体overlay无法精确对应时应报告不一致／未完成，不能以双方“最新帧”兜底。

render关联仅在真实Render正常完成、当前完整提交账本向量与inspect时完全相同且仍包含该范围时记录。比较器要求匹配connection／route／world／root revision的实际render事实和人工画面审阅；缺画面、未审阅或中性截图段均不变成性能通过。视觉中性／结构／暂停区间在测量前声明。

## I35：真实阶段、预算与性能归约

生产保留I29固定聚合；E2E的16384项有界短期明细记录独立的sample／publish／encode／send／receive／commit／ACK、身体更新与合格读取、各阶段墙钟与GetThreadTimes线程CPU、实际物理包和lane摊销。try-lock失败和溢出累计missing，诊断失败不改变同步接纳或ACK。`stream_metrics.cpp::Fields`原阶段名称表遮蔽CPU计数的静态错误已修正。

`reduce.rs::drain`按cursor分页读取，不把缺页默认为零损失；`window`要求执行前固定的每节点起止、完整预期范围／stage和身体资格。输出真实Hz、逐身体资格覆盖、1秒桶、p95／p99／最大间隔与超过100ms缺口；重发不算新sequence，odd与ordinary分列。N0 WorldDynamic≥29Hz、实际60Hz身体≥58Hz、身体合格覆盖≥97%；没有预期范围、资格、CPU或trace即未完成。网络丢包／预算／解析慢仍留在原分母。

线上预算按成功socket发送的物理数据报和IPv4／UDP费用核对，共享合包头摊销和为实包总量。总桶937500byte/s＋24576byte、每Peer312500byte/s＋8192byte及稳定期lane分别核对；任意连续包区间与100ms／1s／10s窗均按rate×时间＋burst判断。代理损伤副本／反向瓶颈另列，不混入原包计数。低带宽过载不承诺保持30／60Hz，但必须有界且恢复。

`latency`分开处理冷依赖压缩总量／Host回显就绪跨度、ACK同机往返、Authority tick锚点年龄和误差；不减异机时钟。`inputs`统计真实物理命令、过期、target偏差、边沿和逐身体consumer；`memory`区分三自有池、全进程private趋势和多次退休后的平台期。缺事实就输出未完成，不能用工作集替代owned bytes或仅凭一次零增长宣称有界。`pipeline`按版本及各owner分列阶段计数。

`tx21 reduce <I39临时分段目录>`读取`windows.json`与`events.jsonl`，写临时`metrics.json`；单段原文上限128MiB／200万行，超过时按执行前固定的case边界分段，不能截尾。I38负责把真实状态／输入／回显／进程指标按上述schema采入、绑定本次case和每Peer身份；本组件没有自行捏造这些事实。正常整体验收完成后这些临时材料清理，运行中必要中断才保留继续所需部分。

## 资料顺序、主审与未执行项

先查本地原版Wiki的`Game.md::Spawn`、`Isaac.md::GridSpawn`、`EntityPlayer.md`、`enums/ButtonAction.md`和`tutorials/DebugConsole.md`，再查REPENTOGON Wiki的`EntityPlayer.md::GetWeapon/SetWeapon/SwapForgottenForm`及既有18—20源码／21合同。本轮没有新增未知ABI、游戏原生入口或点位需求，复用已核字段、只读检查与现有owner，**无需新增逆向，未启动逆向子智能体**。

主智能体静态复读修正：协议常量替换误触合包头长度、CPU统计名称遮蔽、Raw宏在另一SDK单元缺少对应只读辅助、临时JSON跨allocator必须深复制、快照UTF-8分页及有界读取租约、render必须匹配完整inspect提交向量、诊断异常越出命令边界、相反按键跨步骤保持、下行中断期间旧队列继续发送，以及明细缺失／预算失败未参与归约的问题。以上都是静态发现／修正，不是测试失败／测试通过记录。

I36—I38及I39/I40仍待继续。构建、运行库、跨设备部署、两人／四人动作和负载、频率／时延／CPU／内存、故障恢复及画面均未实测；不将组件代码或历史20验收报告成21已通过。
