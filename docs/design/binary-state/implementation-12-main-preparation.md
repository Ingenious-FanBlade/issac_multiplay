# 第12步：主线程资源准备与对象暂存

日期：2026-11-13。承接完整数字安装计划；本轮只实现准备和暂存，原子字段安装、发布与事件执行继续由后续安装 owner 接入。0—14 步不运行测试，第 15 步统一验收。

同日后续：已补齐[StageFields私有字段暂存](implementation-12-stage-fields.md)，包含嵌套容器、共享payload与Weapon槽。下文保留资源／根对象暂存交付时的范围。

同日原子执行器续接：新增[安装租约与有界成功保管](implementation-12-atomic-install.md)，整批对象／字段／原票据一起保留；后续批次仍借用的根不能提前撤Init或析构。特殊字段与完整域owner仍需实现；原准备成功不授予ACK的边界保持。

## 先列明的失败方式

- 网络元数据抵达被误当成本地物化完成；加载失败、图像几何不符、动画实际层顺序或帧数不符仍返回 Ready。
- 主线程重新解析状态正文或使用旧 JSON 补缺字段；资源请求丢失精确 epoch、generation、serial 或取消后误反馈新代次。
- 资源反馈邮箱满时丢失最终结果，重复加载同一票据；资源在使用者之前被释放，或后台析构原生资源。
- Game、Room、VM、图形 context 改变后仍调用旧对象；错误线程、CPU 提交队列非空时进入构造／释放。
- 对象尚未完整安装就出现在 Player roster、Grid 数组或 EntityList；复用地址被当作相同 use／shape。
- Factory 容量不足强抢活槽；两个候选拿到同一空槽；构造／释放异常后重复执行原生前缀。
- 暂存失败后提前释放原生借用，或者未实际清理就宣称 Drain；取消、等待和最终失败混为一类。
- 无真实安装器时启用正式能力，或用准备成功推进安装版本、事件水位及 ACK。

## 实施记录

`game/client/scene/client_preparation.*`提供实际主线程owner，`client_preparation_native.*`通过C++17 SDK桥持有原生资源与私有承载。布局仍为74；没有改线上字段、声明完整二进制能力或启动游戏。

### 资源工作与物化

- `DescribeResourceLocal`复用同源分派、字段号和正式codec，在后台解出具名资源输入；文本由原资源快照延寿。`RequestResources`先完成描述及分配，再创建准备票据，失败不留下没有消费者的Pending。
- `PumpResources`消费专用网络邮箱，保留原route／epoch／generation／serial。反馈队列满时只重发结果，不再次加载；取消工作回Unprepared。首个工作可通过`resources->Memory()`沿用该Peer账户创建owner，再通过`OfferResource`交回原工作。
- ANM真实构造／Load；图像真实LoadImage并检查逻辑、储存几何与路径；动画依附同文件，逐项核实际长度、普通／null层顺序、ID、帧数与数组存在性。shader检查本地内置对象及GL程序。
- Item／Entity配置解析本机精确配置，核ID、类型、cooldown、portrait、shadow位值及文件路径；RoomConfig只构造现行显示合同携带的六个字段，不伪造Name／Variant／Spawn／Merged保存内容。Q14仅支持正式Host的一次白色Shape Clear，不恢复已取消的Host离屏surface协议。
- Audio加载指定文件的本地强引用pair，不调用随机选sample或Play；后端不可用为等待，正常返回但资源不可用为明确失败。释放按本地引用计数与最后释放回调执行，不把路径元数据当声音已播放。
- 音乐／jingle请求准备本机该ID的main、intro与最多六层；Boss请求准备对应屏幕ANM、两个请求ID的有限肖像／name／alt配置闭包及本目标Player类型的演出资产。它们仍是私有资源准备，不执行音乐请求、Boss选择或StartBossIntro；副作用隔离和播放回执归后续事件安装器。

### 根对象暂存

- 暂存全新Entity池use、专用Dummy槽、Player ctor、Grid工厂对象和私有Descriptor；Game／Level／Room／SFX只借用当前根。没有调用玩法Init，没有写roster、EntityList或Room Grid数组。
- 复用／换号要求真实`ClientSceneObjectSource`按完整身份和本地instance重借；缺owner直接失败。body只处理计划命令及引用触及的对象，不重建无关世界。
- 容量使用与既有Entity owner相同的槽数／offset／stride表，取得前核需求、现有use及封存的官方通知闭包。Factory的`force=true`在J273表示池满返回Null，不进入覆盖分支。Dummy使用已核专用池并配平New／Delete。
- 现有owner仍持有的inactive池use若可能被同类Factory选中，明确要求先退休后重建；不写valid占位、抢槽或让新use覆盖旧use。重复地址和构造异常保留真实前缀。
- 每个Manager边界最多取得32个实际需要的私有承载。`VisitObjects`／`VisitResources`只在当前用途栈借用；准备成功仍不推进world／body版本、事件水位或ACK。

### 作用域、计费与排空

二进制准备使用具名`NativePreparation`分派根。只有当前准备owner的实际主线程栈、当前Game／Room／VM／renderer context和单一活动owner共同成立才放行；继续借用既有常驻隔离分派及生命周期票据，不要求旧JSON target存在，也不将私有构造前缀记到旧JSON世界。没有新增Hook或恢复目标字节。

容器、资源／对象owner及自有ANM／RoomConfig固定存储使用同一Peer分配器；SDK内部原生资产由其原生owner管理。资源缓存不长期固定每个历史资源表快照。用途借用退出、CPU raw队列为空后才按对象／需求／资源依赖逆序释放；原生异常或已进入的释放不能重放。实际句柄全部归还后才Drained。未排空时误销毁owner会保留其账户和前缀，不能在后台或错误context强行析构。

### 静态依据与限制

本轮复读官方SDK、`EntityManager.cpp`与`EntitySaveStateManagement.cpp`的ctor／New／Delete／destructor链，并在已连接J273 IDB进行只读静态分析：

| 点位 | 本轮使用的事实 |
|---|---|
| `0x906EF0`、`0x907DEB—0x907E16`、`0x9F8170` | 精确路径加载、hidden out pair；load-on-play不等于随机选择或播放 |
| `0x7D7AB5—0x7D7B08` | SoundManager虚槽`+0x20`的参数／临时pair释放 |
| `0x7D8D40`、`0x9F7CA0` | counter虚槽`+0x0C`减引用，最后释放调用全局pair回调 |
| `0x8D2AB0` | 官方`ModManager::TryRedirectPath`解析本地逻辑路径 |
| `0x6EC540` | Factory扫描、`force=true`拒绝覆盖的分支 |
| `0x823D10` | Boss屏幕ANM与肖像有限配置依赖；没有放行其混合副作用 |

这些是静态依据，不是运行验收。`StageFields`后来已实现，见上方续接记录；Init／字段／关系安装、发布与退休接管、真实事件消费、Manager／session接线、后台资格快照和Client episode计费仍须继续。正式Start没有绑定本owner，`Available`没有改成默认true。

### 纯编译

本轮只进行源码／文档／静态IDB阅读和纯编译。初次发现SDK图像创建函数所属类写错，修正为`KAGE_Graphics_ImageManager`；随后发现Windows `FindResource`宏与状态函数同名，已在无SDK的主线程单元移除该宏。最终产品与E2E配置的Release／Win32 DLL均编译、链接成功。没有运行游戏、E2E、单元测试或校验脚本。
