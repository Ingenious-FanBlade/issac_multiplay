# 第12步：二进制接收、事件账本与主线程事务组件

日期：2026-11-13（Asia/Singapore）。布局版本74，没有改线上字段。依据：[显示委托](client-display-delegation.md)第7节第12行、[第12步](12-client-decode.md)、[线程与寿命合同](12-client-contract.md)。没有运行测试，第15步仍是唯一统一整体验收。

同日续接：[原子执行器／安装保管组件](implementation-12-atomic-install.md)已入源；完整Init／特殊字段／成员发布的`ClientAtomicInstallBindings`尚无具体实现，不能把通用writer或成功保管接口记为全域原生owner已接通。下文较早停点保留原时点。

## 实际状态

**整条正式运行链仍未完成。** 接收owner、公共候选校验、事件账本、不可变批次、主线程事务协调器及网络邮箱已入源；随后补齐[完整数字安装计划与Scene描述](implementation-12-install-plan.md)，实现[主线程资源物化、原票据反馈与根对象暂存](implementation-12-main-preparation.md)，并补齐[StageFields私有字段暂存](implementation-12-stage-fields.md)。仍缺完整字段安装／发布、真实原生资格owner及事件播放器。现有`ClientScene`没有构造新的协调器，正式`Client::Start`没有传入完整准备owner，Client继续不声明二进制能力。不能把组件已编译写成原生整批安装、事件实际播放或联机成功。

现有第13步显示组件只覆盖显示片段。Entity／Player／Grid／Room承载仍大量使用旧字符串DTO，不能把这些DTO的提交版本冒充二进制world/body，也不能用空原生计划、默认成功或仅调用显示Commit补齐缺失的原生owner。Boss肖像与恢复来源、同sample回放、音乐mode5、loop与ItemOverlay协调仍保持此前未完成状态。

## 已入源组件

- `game/client/scene/client_preparation.*`与`client_preparation_native.*`：主线程精确资源物化、资源邮箱消费与原票据反馈、具名音乐／Boss资产准备、根对象私有构造／复用、容量检查、用途借用及逆序排空；后台资源描述由同源codec生成。随后接入`client_field_staging.*`／`client_field_staging_native.*`的`StageFields`、字段就绪／借用与先字段后根对象排空。它们仍不执行正式安装／发布，也不自行启用能力。详见[主线程准备](implementation-12-main-preparation.md)和[字段暂存](implementation-12-stage-fields.md)。
- `state/client_receive.*`：接收线程独占`ClientStateTables`、`ClientStateShadow`与事件账本；表只生成ContextReady；完整候选在Begin前重查公共校验与具体域准备。world／joint使用两个等待槽，替换不续期，普通body直接执行或丢弃；一个真实native busy保持到原票据完成／排空。没有默认Ready或默认成功的具体域接口。
- `state/client_validation.*`：对新提交序列的完整影子递归检查引用空间／类型、同快照资源图、资源用途及真实物化状态；按生成字段号比较P0与P6拓扑。body借用准确已安装的另一流，不重查无关NPC／地图。旧显示结果组在版本74拒绝。该公共层仍不能代替各域跨成员关系、原生布局支持范围及实际安装准备。
- `state/client_plan.*`与`game/client/scene/client_install_plan.*`：完整数字计划、跨域结构检查、现有Grid／Room／Level／显示描述的具体生成、事件／转场关联及有序命令；具体`ClientSceneStatePreparation`只委托真实原生资格owner作Ready判断，没有默认成功。批次退休与子描述借用排空由后台最终回收。详见[安装计划实施说明](implementation-12-install-plan.md)。
- `state/client_events.*`：事件独立于latest候选持有原始候选与准确两表；序号／正文冲突明确失败，乱序不倒退覆盖范围。容量按512条／256KiB正文及头限制，先丢未播放的最旧外观；必要事件无法容纳时失败。外观按Host帧超过15帧标记过期。实际播放回执独立记录，部分失败后已播放前缀不再重播；只有世界目标及所需事件都成功才提升事件水位。
- `state/client_apply.*`：唯一创建线程的writer按Prepare→Stage→Install→Display→Replay→Publish执行；全部具体操作必须由实际`ClientStateNativeOwner`实现。写入后不能跨帧等待半个目标，失败先撤展示资格；原生资源准备失败也先实际排空再回报FailedBeforeWrites。成功上报后仍等后台Committed／Drain决定，取消后的迟到Success不会提前销毁存储。回放结果不确定时等待清理后停止连接，不自动重播不确定事件。
- `net/live.*`：增加独立批次／资源工作与反馈SPSC邮箱；二进制Frame／Members／Resources在网络线程进入receiver，ACK／ContextReady／Resync用flags=1二进制正文返回。旧Range控制及OperationAck不解释这些确认。可选准备owner必须明确声明已有实际完整安装能力，接口存在本身不构成声明。socket终态之后只收主线程真实排空反馈，正常释放前保留在途存储。

新持久容器和不可变批次使用本Peer的`ClientStateMemory`；邮箱容量另计现有Values池。网络不持有或解引用游戏对象，主线程接口不解析报文。

`ClientStateTables::MetadataReady`另补本route确实接收资源表的条件，防止换route后把同run历史资源目录误当作新route已收齐。

## 尚须实施

1. 绑定真实`ClientSceneInstallReadiness`：安装计划及具体准备接口已实现；下一项须由实际原生owner提供数字字段覆盖、本地资源实际形状／代次与生命周期资格，不能只返回公共校验或计划存在的Ready。
2. 实现并接入`ClientScene`的`ClientStateNativeOwner`与Manager邮箱消费。已有事务调度、资源／根对象准备与`StageFields`；Init／字段／关系、显示Stage／Commit、事件回放与最终发布仍须接成同一真实事务。
3. 主线程资源准备、根对象与字段暂存已实现，见[准备](implementation-12-main-preparation.md)和[字段续接记录](implementation-12-stage-fields.md)；仍须由Manager创建owner、消费邮箱并纳入真实生命周期。原子安装器和暂存所有权／账目接管尚缺，不能以私有字段承载就绪代替完整原生状态已安装。
4. 实现真实SFX／横幅／fortune／Boss与转场关联播放器，以及音乐／loop／Overlay恢复，核定Boss缺失资源与sample选择。`ClientDisplayReplaySfx`现有leaf不等于完整事件播放器，不以void返回冒充实际消费成功。
5. 把Client准备／恢复episode的实际分片进展、双向字节及原生准备进展接入预算。现有Host episode、重组限额、候选期限与连接超时继续有效；本次没有新增完整Client episode计费。
6. 上述具体owner实际接通后才从正式Start传入准备owner并切换声明；随后第14步退役旧路径，第15步统一验收。

## 纯编译

状态库和网络库、产品与E2E配置的Release／Win32 DLL均已完成纯编译／链接；没有运行游戏、E2E、单元测试、probe或校验脚本。沙箱内一次MSBuild子进程停滞，已终止该构建，改用正常构建环境后成功；保留原失败事实。构建仍有既有警告，不把纯编译等同于功能验收。

同日安装计划续接：初次编译发现一处同声明auto类型不一致，修正后，最终产品与E2E配置的Release／Win32 DLL均纯编译／链接成功。安装计划、Peer描述分配器和后台退休索引均已包含；没有执行任何测试。

同日主线程准备续接：资源具名输入、实际加载／资格核对、根对象暂存和准备专用隔离作用域已包含于产品与E2E配置DLL，最终纯编译／链接成功；修正过程及剩余范围见[实施记录](implementation-12-main-preparation.md)，没有执行测试。
