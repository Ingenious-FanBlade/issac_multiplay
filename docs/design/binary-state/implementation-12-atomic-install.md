# 第12步：原生原子安装器

日期：2026-11-13。承接数字计划、主线程准备及 StageFields。原子性指同一个 Manager 写入窗口内完成全部安装；失败撤销展示资格，实际排空前不释放承载，不把内存写入误当成成功发布或 ACK。第15步仍是唯一统一整体验收。

## 实现前的失败方式

- 没有逐项覆盖完整数字计划；把特殊字段、协议状态或资源交给通用 memcpy，或漏掉条件缺席／合法空集合。
- 根、引用、嵌套路径、canonical 投影、元素或 Weapon 槽关联错误；同一目标出现相互冲突的重复写入。
- 原生写入后返回 Waiting，Render 看到半个目标；world/body 只成功一个却发布 joint。
- 安装前后根、VM、线程、context 或 ticket 已变化，仍按旧地址继续写；取消后迟到成功继续发布。
- vector/deque/tree/shared/Weapon 的暂存账目提前释放，header 裸复制造成双重析构；本地显示修改／重分配后仍按旧承载拆除。
- EntityPtr 当裸指针复制，丢失反向登记；环形引用在对象尚未齐备时绑定。
- 失败后重复执行已进入的原生 Init、资源或发布；不确定前缀被清零，或只清逻辑 busy 就报告 Drained。
- 特殊 owner 没有真实覆盖却默认 Ready；安装器自行启用正式能力、推进影子版本／事件水位或发送 ACK。

## 实施记录

`client_atomic_install.*`新增`ClientSceneAtomicInstaller`，`client_atomic_install_native.*`是C++17 SDK边界。布局版本仍为74，未改变线上格式或正式能力声明。

### 数字字段与写入

安装器从现有`ClientSceneInstallPlan`取得完整目标；一次借出根、字段与资源，所有原生地址只留在主线程。父链按canonical字段、元素、choice和root归并；动态元素使用私有承载，固定内嵌对象按可信布局的实际边界定位。主线程没有重新解释正文长度头或构建JSON。

通用执行器实际支持原位Copy、已核的同宽转换、u32掩码、ExpireFrame还原、CostumeMap空索引、queued flags，以及唯一通用EntityPtr叶布局10的`SetReference`。同址相同投影合并，冲突／部分重叠拒绝；写入索引用有序表，不逐次扫描全部旧指令。容器键、聚合值和字符串直接使用StageFields已构造的内容，不对有序键再作原位改写。固定内嵌数组与原生指针数组明确区分，后者不能被当作连续对象写入。

`Stage`可等待资源和私有构造；全部字段归属及原生地址预检成功后才取得安装租约。`Install`在同一个Manager窗口执行Init、值／容器、关系及显示字段；进入原生步骤后不接受Waiting。每步重新检查ticket、线程、Game／Room／VM、图形context和实际owner资格。`Publish`还核同一个Manager owner token，不能把半个目标留到下一帧再发布。引用调用失败而反向登记结果不确定时保留前缀，不尝试盲目重放或假报排空。

### 容器保管与退休

vector、string、deque、map、blocked set、innate、Seeds、共享holder和Weapon数组的接合保留完整字段owner、分配账目和原header；它们不是移交给游戏任意析构的无主header。deque同时维护真实proxy的本地owner。原根必须在字段拆除之后才能析构；准备层显式拒绝释放仍接合的字段，以及尚未撤Init／发布的Entity／Player。

同地址后续批次接合时，注册表保留前一租约；新批失败可归还前一header。成功发布才折叠租约，把最初的原生header责任传给新字段，上一字段不再写回旧header。注册表及节点计入同Peer Values额度，不把原生分配账目丢给另一个字段。

`HoldInstallation`在写入前预留保管记录及逐根转交位置；`CommitInstallation`将复用根的构造责任转给成功发布的后继，再移动本批owner，不复制带所有权的header，也不在发布后临时申请保管数组。准备层最多保留五个实际安装批次，资源缓存继续由同一主线程owner持有；没有提前ACK或绕开既有内存预算。旧批转为普通借用，退休时只解除本批用途；当前构造owner仍须等待全部借用结束才能撤Init或析构。关闭会话也不能越过安装租约强制析构。

2026-11-13审阅修复：此前复用根会固定最初构造批次，连续几批各留下一个存活对象即可占满五个保管槽，后续`HoldInstallation`永久失败。现在按完整身份、原生地址和instance逐根转交，未提交候选不能接管，也继续阻止其所借根的owner退休。退休开始后禁止新借用；若当前构造批次先退休，则把责任交回仍存活且未退休的已提交使用者。两遍预检／转交不分配、不重跑Init。`ClientAtomicObject::retire_root`在实际退休前刷新，`Quiesce`／`Retire`必须据此区分本批用途清理与根的撤Init，不能再由本批原来的Join／Reuse推导退休责任。字段承载仍按各自接合租约拆除，不随根指针转交而提前释放。

失败后的已知通用写入按逆序归还；成功发布的普通值不在旧批退休时倒写，以免覆盖后续目标。特殊owner先排空实际用途，再由安装器拆除字段，随后撤销Init／特殊前缀，最后由准备层析构字段和根。生命周期或析构结果不确定时继续保留实际前缀。

### 2026-11-13 真实原生 owner 续接

`ClientSceneAtomicInstaller(memory, preparation)` 现在构建实际的 `ClientAtomicInstallBindings`。安装前核对真实 Game、Room、VM、实体地址、配置闭包、EntityList 容量和现有成员归属。结构事务借用 Game 内嵌的描述槽，用本地描述配置和格点关系安装 Room／Level。Player 基类 Init 先执行，随后执行非 Player 类别 Init。Grid 使用实际工厂承载及本地索引。已提交根按完整身份、原生地址和 instance 重借。

ANM2 文件、动画和图像由已准备的本地资源物化。依赖动画文件及官方 Plus 的地址在 Init、资源装载之后解析，然后检查原生地址的重叠投影。原生绘制记录的 list 头与游标一并移交。Plus 通过官方 holder 的实际对象及动态类型取得承载。地图描述、FX 可见计数、门光位及实体／装备配置通过其原生类型安装。水面裁剪及同 Z 排序从当前候选或已提交成员读取。音频、Overlay、地图图标和其它显示派生结果保留在不可变目标中供显示 owner 消费。

真实 Player roster、EntityList、Grid 数组和门表在同一个 Manager 窗口安装并核对。准备 owner 接收批次后才公布后继 world 及折叠字段租约。已知失败先撤候选成员，再按反序归还字段。退休按实际根保管责任撤引用及 Init。正式 Manager 邮箱、Start 资格、完整显示和事件播放仍需接线。

### 2026-11-14 costume跨流容器修复

此前单流计划只含P6或P0一侧内容，StageFields却重建并替换整个costume vector，导致另一侧丢失。现在后台补入配对影子的costume投影和资源闭包，现有canonical暂存将四个投影合并为一个完整容器。新批次独立拥有该容器、嵌套承载和资源快照；没有复制旧原生header、借用旧costume元素或延长历史安装器链。

回读分为普通字段与costume字段，两条流各自保留轮转游标。普通字段仍由各流安装器负责；costume两流字段只由最近共同提交成功的批次检查及修复。该选择在worker确认Committed后与批次切换一起更新，失败候选不取得回读责任。旧配对流不会再检查已被替换的costume父容器，也不会把旧Sprite状态修回新容器。

退休继续使用原有接合租约：发布前失败可撤下候选并恢复前一header；发布成功后最初的原生header由后继保管，旧容器已经脱离游戏对象，旧批只析构其私有承载。根的构造责任仍按实际使用者转交，关闭时先拆字段再释放根。静态复读覆盖联合后连续body、连续world、两流交替、空目录、失败回滚及关闭顺序；运行结果留到第15步，不将静态推导记为E2E通过。

本次修复后产品与E2E配置的Release／Win32 `zhlIsaacCS`均纯编译、链接成功，仍有类型转换、局部名称遮蔽及RapidJSON枚举运算警告。沙箱内首次MSBuild停在文件扫描，已中止后在正常构建环境完成。没有运行游戏或任何测试。

### 早期交付范围（保留原时点）

本轮交付的是**原生原子执行器、通用字段writer和所有权保管机制**，不是全域安装owner或完整正式运行链。`ClientAtomicInstallBindings`是必须由真实域owner实现的接口，目前没有具体实现：

- `Preflight`必须核真实Init、结构关系、成员集合与最终发布容量；`Initialize`、特殊字段安装、`Publish`和退休必须返回实际事实。
- 资源／ANM选择器、Companion／Synthetic、布局10以外的引用、纯协议状态及其它特殊转换没有通用memcpy后备。每个完整字段必须通过`Covers`明确交给具名owner，否则在写入前失败；不能以接口存在或通用计划存在声明Ready。
- 暂存根、完整字段的具名ABI绑定、原生成员集合发布、完整显示／事件、Manager邮箱、正式Start和后台资格快照仍需串联。外层协调器须在`Install`后完成真实Display／Replay，再调用`Publish`；只有最终共同提交成功才能由后台提升版本与事件水位／发送AppliedAck。
- 每个安装器实例只处理一个ticket。实际字段用途及配对流可能使已提交实例继续保留，外层须退休不再使用的批次；根构造责任可以转交，但不能把“后台已收到Success”解释为可以销毁安装器／准备owner。

因此不能把本轮写成“全字段原生安装器已完成”或“第12步全部完成”。

## 纯编译

真实原生 owner 续接后，产品和 E2E 配置的 Release／Win32 `zhlIsaacCS` 均完成纯编译与链接。没有启动游戏或执行测试。局部显示派生结果仍需由第13步的显示消费者接线，运行行为统一留到第15步验收。

同日P1修复后，产品与E2E配置的Release／Win32 `zhlIsaacCS`再次编译、链接成功；本次默认构建环境正常完成。仍有编译警告，未运行任何测试，不能据此声明原生运行验收通过。

产品`build/repentogon`与E2E配置`build/repentogon-e2e`的Release／Win32 `zhlIsaacCS`均已纯编译、链接成功。没有启动游戏、运行E2E、单元测试、probe或校验程序，第15步仍是唯一统一整体验收。

沙箱内MSBuild子进程停滞，已终止并改用正常构建环境。首次产品编译发现临时vector声明被C++解释为函数声明，改用花括号初始化后通过；静态复读另修正了逻辑根的Null承载不应算作被借用、Weapon choice路径的布局选择及已构造容器键不应重复写入。没有以运行结果替代这些静态事实，也没有把纯编译写成功能验收。
