# 第10步第四项：Entity公共部分、全部子类与Player采集

2026-11-11。接续[计划执行器](10-capture-executor.md)。目标为把Entity公共E0—E9、已登记全部派生与变体、Player各块及逐身体来源绑定到真实只读原生来源；不调用Lua、getter、生产器、随机或额外Update，不切换第11—14步尚未完成的发布／Client链。测试仅第15步统一验收。

## 编码前列出的失败方式

- 使用_type猜RTTI、把变体臂vtable独立登记、在Delirium重配owner内选臂；错型payload折成空。
- 将Companion／Synthetic偏移加到Entity，跨root/use借Player证言，绑定失效后保留旧body／拓扑。
- 容器数量先窄化、deque物理环当连续数组、未设置optional仍读payload；共享投影count／来源不一致。
- Scalar／Convert语义不符、非有限输入或窄化溢出；Familiar／Pickup未知计时方向伪装Frozen。
- Sprite文件选择、实际Image及替换路径混同；资源handle来自原生地址或资源登记分配内存。
- Text Init当writer证言；读取未终止串或旧尾值；独立Render结果拼进ordinary。
- 以原版两HUD槽／8个fast槽限制身体；漏掉未激活但有独立初始化依据的身体与P7关系。
- 使用PlayerPlus模型偏移冒充实际官方ABI；聚合读取timer／组名或超界／负计数被抵消。
- 动态Weapon不核实际类型、清空有效尾引用、调用虚getter；Incubus公式浮点精度／截断错误。
- 半根／半批发布、Clock提交前交付、撤销后旧纯值复活；为新源引入主线程分配／锁／等待。
- 把新增读取绑定、纯编译或未触发来源写成原版覆盖／性能已通过。

## 实施记录

本项已实现并接入产品的Entity／Player只读源，布局版本67。没有运行测试或游戏；编译不构成运行覆盖或性能结论。

### 原生读取与同源计划

`mod/src/world/binary/entity_source*`冷准备全部Entity派生／变体及递归子布局的数字绑定，通过PE的COL／TD／层级描述定位真实虚表，variant仍按生成selector选择。Hook可替换虚函数入口，因此身份不要求函数指针仍落在游戏PE。不同RTTI、未知payload和selector失败均保留具体诊断，不退回基础臂或伪空对象。

直接字段继续按合并复制计划读取；特殊绑定覆盖E0—E9以及Effect、Familiar、Knife、Bomb、Tear、Pickup、Laser／HomingLaser、Projectile、Slot、NPC／Delirium、Text／Scaled、Dummy与Player。只复制已登记的状态，不改变各类型原有UNKNOWN字节归属，也不据此宣称原生对象逐字节完整。

`entity_containers.cpp`接通真实vector、固定数组、MSVC deque、树／无序链表、官方Plus、typed共享payload、AnimationState及Weapon槽。标量容器消费执行器已经预读的游标，不重开来源；deque按逻辑顺序解环，Entity*在同步段内转为纯值身份。数量在宽整数下检查，头／节点／holder前后复核，超限或来源变化整包失败。字符串只读有效前缀，不复制SSO旧尾、未设置optional、nil节点内容或未选择的绘制帧。

### Player全部身体与专用状态

`entity_player.cpp`接通P0—P8、固定13个Sprite、15个costume映射、有效装备／队列／效果／残影、真实动态Weapon与可选当前绘制节点。D先证明cursor属于完整当前链，再读取选中节点；D=0不读旧Sprite／节点，负second_animation规范化为`{-1,0}`而不读取旧second_frame。Incubus按已核slot6／7纯f32公式换算，并复核实际Weapon来源，不执行虚getter。

官方`EntityPlayerPlus.itemSpoofs`实际位于+10h；新增ABI编译声明核对optional、官方类尺寸、Release容器及节点value偏移。Innate只读persistent和temporary.cachedTotals，合并为有序有效计数，不读组名／timer，不以负数抵消或截断溢出。

P0／P6比较同一身体的实际资源绑定、costume有序目录和映射等自有结构字节，维护单调拓扑版本；相同拓扑ordinary实际提交后才允许快速body。P7由完整初始化／逐身体证言、原生列表、roster与真实C/S缓存拓扑共同供给，保留激活、活动、输入消费和身份归组的区别。四参与者及空间9／10的Participant／ControlGroup按实际session／binding生成；anchors保留当前注册顺序。

所有合法身体进入ordinary world＋body联合根。8个fast槽仅决定额外快速更新机会；槽不足或结构未获ordinary提交时继续走联合根，不裁掉非活动／存储体。物理退休回收拓扑槽、Text证言和Sprite依赖；普通绑定撤销不把同址新身体当成旧身体。

### 当前窗口、提交与后台所有权

`Collector::NativeOrdinary／NativeBody／NativeWindowCurrent／NativeCurrent`提供真实owner、tick、outer、mutation、生命周期和逐身体资格。普通源自行枚举非模板Entity列表、roster及Esau存储，并在采集结束和提交前复核完整指针序列；不借用Recorder批次槽、Lua枚举结果或旧字段complete标志。原生异常／外线程故障计数在窗口内变化立即使读取失效。

`game/observation/binary_entities.cpp`在现有world准备后台预分配，在Manager的prepare同步读取，只有Clock提交且post精确匹配才发布。普通候选与各快速身体分别保存证言；游戏对象、借用头和原生指针不交后台。未提交、切房／换run、改型、绑定撤销及已选Sprite资源修改会撤销相应候选。依赖已安装的常驻资源Hook，逻辑启停不卸载Hook，不改变Authority输入或调用次数。

交接分成4份ordinary大缓冲和16份body缓冲，并保留cold／latest／8个body mailbox及256控制。单身体退休采用独立lane代次，使旧consumer票据失效，同时允许同窗ordinary完成；来源全局失效仍用整体代次／水位。引用目录按候选包保存纯物理身份快照，帧间重用主线程工作表，避免长期游玩按历史Entity累计耗尽引用编号。资源只追加不可变描述，以packet stamp冻结目录前缀；worker通过有效票据与`Catalog()`读取自有目录，不读主线程可变计数。暂存／目录／交接共同检查shared 64MiB，并计入已有进程三池；超额具名失败，无临时扩容。

现有world工作线程接收／排空控制，退出时排空票据；本项没有增加线程。数据保留在cold／latest供第11步消费，本项不编码／发送ICSR，不切换旧网络链。`world_status`新增binary_entity采集计数、failure布局／字段及exchange失败状态，不把新源缺口混成旧Lua诊断。

### 资源与两项用户决定

`entity_resources.cpp`从实际ANM2选择、AnimationData、有序普通／null层、ImageBase实际对象、ItemConfig及已选EntityConfig生成本地资源描述，不调用配置查询、getter、加载、Render或GPU读回。路径按VFS规范化并检查UTF-8／设备名／边界；空选择与读取失败分开。

本轮静态复读确认PNG虚表`B3BA80`的slot10—13在`A34900／10／20／30`读取+80／82／84／86的u16；PCX虚表`B3B8EC`的对应实现`A346B0／C0／D0／E0`读取+100／104／108／10C的u32，不能复用PNG偏移。ImageBase名称是+3C的char*。未加载配置图片只给实际路径和显式metadata_pending，不补零几何；不明程序图保持ResourcePending，按后续资源／显示来源项补齐，不用原始媒体兜底。

版本67包含三项正式源修正：

- 用户明确选择Familiar FireCooldown采样原始i32。`60E2C0`的公共射击更新可在trinket141分支同次递减两次，其他重置也不能用原±1锚点表达；当前不伪造时钟，Client后续普通帧更新该值。
- 用户明确选择Pickup DropDelay采样原始i32。`6E0685—6E06AE`正值且collisionClass=0才减1并早退，否则直接清零；原条件锚点无法完整表达。移除本项临时计时handler，保留旧证据文档的历史记录。
- QueueItemData.item改为`resource/u16/item-config`，与实际选中的trinket配置身份一致，宽度仍2B；非trinket不读取无效尾部。

### 产品编译与后续边界

执行Release／Win32产品目标`cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /v:minimal`，E2E工具关闭。过程中的真实失败包括容器局部auto混合类型C3538、桥接缺Runtime／PackWord声明，以及新增引用目录类型与既有CaptureReference函数同名的C2070；均已修正。WorldCapture构造／析构移到BinaryEntities完整定义处，处理跨编译单元不完整类型清理问题。末次增量编译还遇到共享构建目录PDB被另一构建占用的C1041；未终止其他任务，待占用结束重建。最终产品DLL纯编译／链接完成；没有部署、启动游戏、运行测试或校验哈希。

本项只接Entity／Player来源。当前packet的floor／dimension尚未由后续Room／Level来源填齐，不能作为完整WorldFrame或提前ACK的依据；其余域、正常HUD／显示／音频缓存、程序资源内容和完整发布依赖闭包仍按第10步其余项及第11—14步推进。旧Lua／JSON链在第14步统一退役，现阶段仍并存，其开销不计为已解决。实际原版覆盖、稀有分支、容量、长期生命周期、线程交错和p95／p99全部留在第15步唯一整体验收。
