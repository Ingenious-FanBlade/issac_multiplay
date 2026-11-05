# tx-04-21 I26—I30：恢复、停止、统计与运行配置

日期：2026-11-05，Asia/Singapore。状态：I26—I30源码实现与静态主审完成，**本轮没有构建、测试或启动游戏**。仍在同一tx内推进；I31—I38完成后才由I39执行P2全部E01—E26及子用例，再执行P4全套。没有新增单元测试、gate、哈希校验或测试artifact。

## I26：同连接新Baseline恢复

`game/client.cpp`的连接级恢复账本跨`ClientScene::Begin`和route换代保留。纯值依赖／预算暂缺、资源错误、写后失败和不确定owner分别分类；热更新写前暂缺可以保留有效A等待。写后失败立即撤显示和新借用，关闭旧输入、边沿、worker历史和世界接纳，使用已有presentation→objects／关系→Grid→native world→retained资源的完整退休链。不会直接重建失败时的旧T。

只有`CanReturnToMenu`确认真实owner、CPU／GPU／Lua／图形借用退尽，才排入可靠`Resync`，请求新的完整采集。Host保持同connection／ordinal，等待manifest真正更新后重新准备route并走I22冷应用。原生失败自动完整重建在连接内最多一次；换target、重发或换route不重置次数。无成功提交的连续恢复共用绝对期限，不能每次新目标重新延长。

首个结果保存attempt、Key／范围、phase、执行前缀、instance／resource代次与owner计数；保留当时retained ticket、释放phase、代次、CPU／GPU占用和失败状态的纯值副本。诊断副本分配失败只标缺测，不阻止停止／恢复。后续清理错误另列。原生退休失败和release回调返回失败后禁止重试不确定析构；仍保持Client隔离并报告，不能为重连强制释放owner。

对应E15、E16、E22；上述均是实现合同，不是恢复成功的运行观察。

## I27：最新完整范围与慢端收敛

`net/stream_session.*::PublicationSender`保留每范围不可变在途发送游标和最新完整候选，不以新帧取消正在有效发送的完整范围／可靠依赖。未确认终值每100ms补发，实际完成的同版本重复不会增加新帧计数；2s无收敛触发该Peer完整恢复。归零、空集合和删除仍由完整范围表达，没有拆掉成员或字段减负。

只有新的真实Applied版本才推进确认进度，旧ACK和心跳不刷新收敛时限；Recovering／Preparing期间的旧确认只作退休处理，不能重新激活旧Baseline。普通完整Player域已经真实提交、且覆盖旧Fast来源tick时，停止重发被覆盖的旧Fast；**不伪造该Fast范围的Applied ACK**。每个慢端独立等待、恢复或退出，Authority与健康Peer继续服务；无成功恢复时最多120s收口。

Client按实际普通范围提交时间计算年龄，500ms标陈旧、2s恢复；Host按未确认范围的本地时间记录对应状态。暂停来自现有生命周期`Pause`事实，含原生暂停、控制台与已理解的失焦暂停；`TickProgress.paused`显式传输。暂停不制造新tick，恢复时重新开始缺口计时。不能把单纯input_enabled=false当暂停。

可靠世界候选、快速世界候选和控制候选独立计数；控制容量不会被世界分片挤占。可靠主线程邮箱满时使用至多两个自有值暂存，仍由唯一producer推送、唯一主线程consumer释放。旧route队列由主线程过滤，不由worker越权消费。

对应E06、E10、E12、E21。频率、带宽、长时慢端和静止后态仍待I39实测。

## I28：停止、断联与安全Off

用户stop、Host断联／stop、超时、协议失败及既有run teardown、窗口关闭／正常退出共用终态封存。先关输入和发布，清候选、控制、transfer及输入历史，随后Manager在完整步骤边界退休原生世界。后续通知不能覆盖首因或重新激活准备事务。网络线程清理自有值并关闭socket／Winsock，不在析构或DllMain等待网络。

网络结束不等于Scene退休。只有worker Finished、两轮物理中性、MenuReady及真实`CanReturnToMenu`同时成立才交还Off；未解释owner继续隔离。既有进程退出进度保护覆盖最后写入者，常驻Hook／DLL只做逻辑停用，不恢复字节、不卸载、不注入。Host单Peer终止沿现有冻结身份表处理，不回收重用ordinal，也不关闭其余Peer。

对应E16、E19、E22；未宣称任意SEH、损坏指针或未知图形／Lua owner可自动安全恢复。

## I29：有界聚合与诊断入口

`replica/stream_metrics.*`使用128个范围槽、固定阶段计数和32个对数耗时桶；范围明细随world／route换代撤销，连接累计量另列。来源、发布、编码、实际发送、完整接收、原生提交和ACK分栏，重复版本不当新帧。实际身体更新／可读采样及缺席另由`body_status`保留source local／use／shape／binding／tick，不把处理输入ACK当身体消费。

Host实际socket成功发送才计UDP payload、IPv4／UDP线上字节、物理包与合包条数；合包按条目长度摊销共有头，lane线上字节之和等于物理总额。选择性补片、重复、冲突、超时和解码另有transfer统计。共享编码器只统计一次实际编码／压缩工作；每Peer逻辑内容尺寸和实际线上费用分开，不能把三份游标当三次编码。

纯值投影、编码／压缩、队列等待、接收／解码、冷预检、原生完整步骤、退休、绘制和命令IPC分别计时。`wall_*`为同机单调区间，`thread_cpu_*`来自GetThreadTimes；CPU捕获失败标缺测，零值可受系统计时精度影响。区间可能重叠，不能直接相加成端到端时间。完整解码量／耗时还需结合transfer的decoded／decode_us，逐片接收成本不是完整模型解码成本。绘制成功另记实际render revision。

正式诊断命令（page为0—31，省略时为0）：

```text
cs stream 1 0
cs stream 2 0
cs stream 3 0
cs stream client 0
cs stream source 0
cs stream ipc 0
cs stream config
cs observe status
```

每页最多4个范围；耗时汇总始终列出，详细桶按`histogram_stage`分页。Client首因资源owner每页至多34项，Source身体每页至多2项。完整范围／owner／身体须翻页读取。`stream source`和`observe status`提供来源统计；`cs status`保留原有实际输入／consumer见证及轻量恢复状态。诊断不自动开启Recorder或逐tick／query日志。

状态同时提供候选、队列数量／高水位、retained数量、复制三池current／high water／limit及进程private／working set。未捕获阶段使用零count及明确missing字段，tick=0不冒充已知采样时刻。不同owner的local clock只在其自身时钟域解释，禁止直接相减不同设备时刻。网络时延继续使用既有回显区间。

统计失败不参与输入／同步接纳；固定槽溢出或邮箱try_lock失败累计missing。详细文本仅在显式命令按页生成，不放到每帧status全量展开。I35仍需实现场景化归约和E2E指定版本详细见证，I29聚合本身不声称性能通过。

## I30：配置、协商和冻结

`replica/stream_config.h::CS_STREAM_SETTINGS`是会话参数、单位、默认值和本地最大值的单一目录，`mod/config.example.json`展示常用覆盖。所有速率为byte/s、burst／消息／池为byte、超时为ms、频率为Hz。固定framing的1200byte数据报／1136byte净荷只能按该组合使用，配置不能改变协议几何。

默认PlayerFast60、WorldDynamic30、roster20、时序60Hz；总937500、单Peer312500，四lane依次25000／120000／120000／47500byte/s，总burst24576、Peer8192byte。默认100ms终值补发、500ms陈旧、2s恢复／进度超时、120s绝对上限；RTO初始200、范围100—1000ms。其他节点、候选、展开和池保护值见配置目录，未配置项直接取该目录默认值。

解析拒绝零／负数、溢出、stream未知／重复字段、lane超Peer、三份Peer超总额、burst矛盾、RTO／超时矛盾和分片容量不足等组合。Player／World／时序降频须整除对应60／30／60来源机会，并与陈旧阈值相容，不假造额外来源。P2／P4验收profile固定同一312500和lane份额；产品profile仍保留三份公平分配、明确禁止跨Peer借用，可显式关闭初建借用本Peer空闲动态lane。不会因只连接一端自动放宽预算。

world_limits与新增fast_limits分别协商最小值；快速范围bytes／nodes／candidates／fragments不再借用可靠世界的两个候选额度。能力`recovery-status-v1`绑定新的暂停字段和严格capabilities形状，旧端明确拒绝。远端不能抬高本地消息、展开或进程池保护上限。

Host worker、Client runtime、Scene和来源owner在启动时复制配置。状态显示requested、effective、协商后的可靠／快速限制及限制来源。编辑文件后可在Off、Manager边界、复制池无在途租约时执行`cs stream reload`，仅更新下次会话的流参数及Host timeout别名，不热改Hook或其他启动设置；重启加载同样有效。非法配置保留原有效设置并报告原因，不悄悄降档。

能力不足、资源错误、预算不足、缺依赖、陈旧、原生部分失败、owner不确定及恢复失败均有具名类别、首因和恢复可用性；简洁用户提示与详细原生／网络诊断分开。对应E01、E12、E17、E21。

## 静态依据与剩余工作

暂停语义先读本地`F:\projects_isaac\IsaacDocs\docs\Game.md::IsPaused`及`F:\projects_isaac\REPENTOGON-1.1.3\docs\docs\Game.md::IsPauseMenuOpen`，再读官方`repentogon/LuaInterfaces/LuaGame.cpp::Lua_GameIsPauseMenuOpen`和本仓库`game/lifecycle.cpp`。实现复用已有Pause事实，没有新增游戏查询／写入口。

退休、线程、借用及副作用继续采用[已主审R05合同](tx-04-21-native-contracts.md)和18—20／I21—I25现有实现；本轮不依赖新的未知ABI或点位，**无需新增逆向，也没有启动逆向子智能体**。主智能体静态复读修正了旧T恢复、重复析构、控制候选挤占、worker越权消费队列、旧Fast被普通提交覆盖后的误恢复，以及60Hz按17ms取整造成降频的问题。

本轮交付不包含I31—I40。没有构建、测试、游戏启动或四端实测，不能把新代码与静态主审报告成失败恢复／退出／30—60Hz／10Mbps已通过；历史I21—I25的纯编译记录保持其原范围。
