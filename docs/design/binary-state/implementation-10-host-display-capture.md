# 第10步：显示委托后的Host采集实施

日期：2026-11-13（Asia/Singapore）。当前布局版本74。依据：[显示委托方案](client-display-delegation.md)第14节第6项。这里记录功能实现和接线边界，不作为单独测试或交付artifact。

**同日续接**：[第11步worker／Peer发布](implementation-11-worker-peer-publish.md)已接真实票据消费、二进制正式Host发布、每Peer事件队列及AppliedAck回收。下文记录本页上一轮的审阅与停点，保留其真实历史；完整Client链路仍未接通、未测试。

## 上一轮实际状态

工作区此前已经有版本73的第二批布局、Host观察Hook、状态值树和事件编解码，但主要进度入口仍写版本72／Host采集待做。本次在此基础上静态审阅、修复并继续接线。没有回滚原有未提交工作，没有启动游戏、部署DLL、运行E2E、单测或校验脚本。第15步仍是唯一统一整体验收。

当前代码已经具备以下来源和接口：

| 范围 | 实现与边界 |
|---|---|
| Q8／Q9／Q11／Q12 | `binary_entities.cpp`、`run_source.cpp`、`scene_source.cpp`从同一合法ordinary窗口采集Run／Room窄字段、当前descriptor SpawnSeed、种子效果ID、raw诅咒、SFX乘数、paused和后态分支。按Q12保留后态选择器，不改成Host调用历史回放 |
| 第二批状态 | `binary_display_capture.cpp`准备GameRun自有值树：438注入输入、439音乐请求、440／441循环声、442／443演出owner、444／445程序道具显示定义。实际原生字段和资源仍受同窗资格约束 |
| 正常生产 | `binary_production.cpp`／`binary_intentions.cpp`及统一Hook清单观察正常SFX／音乐／文字／演出／Boss／转场。显示来源按Q7排除，混合子根挂起显示身份；SFX在正常选样本与pan尾叶后接受，生命周期期间只保存自有路径和参数 |
| 事件 | 448非循环声、449横幅、450／451fortune、452Boss、453转场关联。序号与原调用帧保留，外观事件可按容量／15帧期限丢弃并计数；非外观不能静默丢弃 |
| 交接 | ordinary packet携带完整事件范围；worker可用`QueueCapturedStateEvents`复制到每Peer队列，再通过`BinaryEntities::EventsTransferred`归还采集副本。Peer仍由已发送目标ledger和AppliedAck确认，worker接管不等于播放或确认 |
| 显示生产退役 | 当前普通发布不再要求旧HUD／D0／显示结果缓存，Host场景离屏图像观察停止采集。旧实现和常驻透传Hook仍保留到第14步统一退役 |

**正式运行链仍未切换。** `BinaryEntities::Background`当前只消费控制／退出排空；没有消费普通二进制票据、为真实Peer调用事件接管接口或发送新协议。旧DTO仍是正式网络来源。第12步完整validator、主线程投递、各域原生安装和第二批Client adapter也没有因本次Host接口入源而自动完成。

## 本次静态审阅与修复

审阅覆盖需求／生成器与Hook清单、Host采集窗口和来源、生产缓存、事件编解码与ACK，并沿调用链复核Client安装／显示消费边界；生成的逐类型字节账本作为参照，没有把文档生成或编译当成运行验证。

| 原问题／后果 | 来源与修复 |
|---|---|
| 后绑定SFX改成协议事件layout后，ordinary根资格仍只接纳Game／Room／descriptor，导致事件值树被拒绝 | `BinaryEntities::Impl::Current`明确接纳锚定当前Game／run、已有自有值树的448—453事件根；不扩大实体或任意原生对象的可读范围 |
| packet只带仍存活记录，丢弃／过期外观后无法表达序号空隙，空事件批次也无法推进覆盖水位 | `CapturePacket`／`CaptureValueWriter::EventRange`携带首序号与水位。worker先核完整范围，Offer成功后才Cover；缺失范围不能被下一批覆盖 |
| 可靠事件在主线程永不回收，长期转场最终耗尽缓存 | 增加32槽SPSC接管回执。仅consumer提交仍有效的ordinary票据；主线程按session／run消费回执并退休已接管副本。只有u32原子，64位数据由release／acquire槽保护。FIFO满返回失败供重试，不等待主线程 |
| 采集端256KiB只估正文，遗漏CaptureRecord、紧缩引用表、对齐与页；Peer也漏算事件头 | 采集端按1KiB缓存页计保守上界，Peer把22B队列头及每条22B事件头计入总额。仍先丢最旧外观；必要事件超额使本次来源失败 |
| 已ACK事件仍随latest包重带，worker会先解析其旧房成员／资源，可能阻断新world | `QueueCapturedStateEvents`跳过已确认序号的正文解析，保留未确认事件的严格重复比较；增量段拒绝越过Client未覆盖前缀，baseline可建立新的起点 |
| 转场缓存保存原Player地址，到ChangeRoom后才解析，退休／地址复用可关联错误身体 | 接受Start时冻结稳定成员身份并清除缓存中的Player地址；后续事件按该稳定身份登记引用 |
| 换层活跃时Start被原生拒绝，但旧mode／index／animation恰好匹配会被误判接受 | Start前保存`Game+0x1BA78`，按J273“换层活跃且animation不为15则拒绝”判定；不从旧转场字段制造新接受 |
| animation 11／16等同房结束不调用ChangeRoom，缺最终目标关联 | 第一个合法ordinary后态观察mode=0后发送同序号的实际最终关联；事件成功封存后才清active，重试不丢失 |
| 普通音乐观察和生命周期意图各有一套状态变换，Fadein参数校验与请求语义可能分叉 | 两个入口统一经`BootstrapMusic`更新同一请求状态，并核所有实际浮点参数 |
| 正常音乐tick的`Play(queued,0)`被当成新外部静音Play | 新增统一清单中的`binary_music_tick`观察作用域，原函数仍恰好一次；内部Queue消费进入版本74专门的mode=5，见下节 |
| 外观丢弃只有内部计数，无法从已有诊断入口观察 | `world_commands.cpp`增加事件水位、worker接管水位、待交接条数／页字节、生命周期意图数、丢弃数和incomplete位 |
| 某个观察／Q7来源入口未安装时，采集可能把漏采请求误当成没有事件 | 启用采集前检查统一清单中对应Hook的实际trampoline；缺少来源只停用该采集功能，不新增推进gate或运行测试 |

本次没有把Q12“按Update后状态计算分支”视为缺陷；也没有在房间变化时清空按只追加资源目录登记的循环声资源。两者保留现行合同。

## 版本74：Queue自然接续

布局438—453及事件段最初在版本73入源。本次改变439的互通接受合同，整数版本递增为74；既有字段顺序和宽度不变。

`MusicRequestState.mode`现在只接受0—5：0静止、1Play、2Crossfade、3Fadein、4Fadeout、5Queue自然接续。额外收紧paused／enabled／jingle_active为bool、layers为六位mask。没有哈希或旧版本兼容旁路。

J273静态依据：音乐tick `0x7D7DA0`在`0x7D8303`调用`Play(queued,0)`，随后直接把当前组gain置0、target置本机enabled／musicVolume、fadeRate置0.08，并清queued。0是本地准备的起始gain，不能成为新的网络静音请求。Hook入口ABI与特征字节已登记到`v1_9_7_12-static.json`，并沿用RE-10确认的无显式栈参调用方式。

mode=5保留当前目标id与逻辑播放代次，parameter=1、fade_rate=0.08，不下发Host Options。后续Client adapter必须区分它与外部同曲重播：已由本地tick消费同一queued目标时不再Play；冷恢复或本地曲目尚未收敛时准备目标，再按本机Options进行相同淡入。不能把parameter=1直接写成绕过本机音量的后端gain。**该Client adapter尚未实施**。

## 后续接线与未决

1. 第11步：ordinary／body票据领取、成员／资源目录、向所有当前Peer复制事件范围、接管回执及AppliedAck回收已[接线入源](implementation-11-worker-peer-publish.md)。新Peer按base=0指定事件起点，已交接旧事件由队列／目标快照保护；首次晚出现而已无当前成员的必要引用仍明确报缺口。
2. 第12／13步：完整候选接收与原子安装、第二批注入、音乐mode=5、循环声样本级协调、文字本地化／路由、ItemOverlay新代次Show、事件幂等和实际回放。
3. Boss肖像／额外资源与活跃演出恢复仍需真实资源owner；只采到bossID不是完成。没有合法生产窗口时，非音频瞬时请求也尚无完整的生命周期后绑定来源。
4. 跨房未确认必要事件中的旧成员／资源闭包，仍须由第11／12步的在途目标owner固定；只登记稳定身份不表示对端已经具备该成员。不能把缺引用改成Null。
5. 第14步统一删除旧采集／手工显示／旧包络代码与不再需要的Hook注册；常驻Hook不在会话结束时恢复字节。
6. 第15步再统一验证显示、同房／跨房／快速转场、暂停组合、音乐Queue与重播、事件容量／重传／ACK、断线恢复、Host采集耗时、Client安装和带宽。当前未运行这些测试，不宣称FPS、延迟或功能通过。

## 纯编译记录

最初沙箱内构建停在CMake子进程并被终止；改用可正常执行子进程的构建环境后，最初修复批次的产品Release／Win32纯编译和链接通过。新增音乐tick观察后，首轮构建因尚未登记该自有Hook的特征字节而停止；已从J273 IDB读取静态入口字节并补入统一绑定源。这是构建生成错误，未运行游戏或测试。

最终同一份版本74源码的产品与E2E配置下`zhlIsaacCS` DLL均完成Release／Win32纯编译和链接。构建仍有SDK等既有警告；本次没有运行测试，纯编译不代替功能或性能验收。临时构建日志用后清理；原有`.scratch/host73-*`材料未删除或覆盖。
