# 第12步：StageFields 私有字段暂存

日期：2026-11-13。承接主线程资源准备和根对象暂存；本轮目标是嵌套容器、共享 payload 和五个 Weapon 槽的实际私有承载。字段赋值、关系安装及发布仍由后续原子安装器完成。0—14 步不运行测试。

同日续接：[原子安装器组件](implementation-12-atomic-install.md)已新增通用写入、安装租约、header交接及整批保管。完整特殊字段／Init／发布owner尚无具体实现，下文保留字段暂存当时的事实，不将其改记为全域安装已完成。

## 实现前列明的失败方式

- 把不同 root/use、嵌套元素或 Weapon 槽合并，或将同一原生容器的多个投影重复构造；world/body 投影不一致却继续准备。
- 主线程再次解释变长正文头；将条件缺席、共享 Null、空容器、未准备混为一类；空目标不能撤销旧内容。
- vector/deque/map/string 的本地承载不满足实际 ABI、元素构造或数量；仅分配原始字节就将 ANM／LayerState 当成已构造对象。
- 将私有字段提前写入现有 Entity/Player；共享 payload 使用错误的 Effect/Familiar owner，浅复制 holder 或重复析构。
- Weapon 根据基类大小分配、猜动态类型，调用玩法 Fire/Update，或构造时借用现有身体造成可见写入；退休时攻击引用或 owner 未清空。
- 局部失败、预算失败和取消导致前缀遗失、重复 native 调用、后台释放或假 Drain；字段承载比根对象／资源晚创建却先被依赖者使用。
- 尚未完成字段暂存或生命周期已变化时返回 Ready；准备成功误推进安装版本、事件水位或 ACK。

## 实施记录

`client_field_staging.*`消费安装计划的数字字段／路径／标量叶，`client_field_staging_native.*`提供C++17 SDK边界。`ClientSceneMainPreparation`增加`StageFields`、`FieldsReady`和`VisitFields`；调用顺序为`Prepare → StageObjects → StageFields`。正式Manager／session仍未实例化完整安装owner，二进制能力声明保持原状，布局仍为74。

### 身份、投影与承载

- 字段身份是完整目标的object索引和canonical父链，保留每层layout、字段、元素、choice及Weapon槽。world/body和多块投影映射到同一个私有字段owner；数量、present／payload或槽mask冲突直接失败。原始field索引保留，供后续安装器精确取回对应承载。
- 字符串、集合键和聚合计数取后台已经定界的标量叶，主线程不重读长度头、不解码正文、不用JSON补值。空字符串、空容器、共享Null、条件缺席和未准备分别保留。
- 实际暂存vector／连续数组、string／C-string、deque、ordered map、blocked hash set、seed tree，以及可选draw record的真实list节点。ANM、CostumeSpriteDesc中的ANM与LayerState执行实际构造；没有将整份原始字节缓冲冒充已构造的非平凡元素。
- `ItemSpoof`有效innate合计放入一个本地展示组的persistent，temporary与totals为空，遵守现行聚合合同，不伪造Host组名／时限。map／set／聚合和树保持输入元素到实际本地节点的有界索引。
- 无需独立分配的内嵌路径，以及没有原生对象的纯显示协议聚合，用明确的`Group`表示；不虚构协议记录的原生对象。需要独立承载的后代仍逐字段暂存。

### 共享对象与Weapon

- Effect的MrMe／SpriteTrail／PickupGhost，以及Familiar的IsaacsBody／AbyssLocust／UmbilicalBaby，分别使用已核ctor或inline构造和真实动态虚表，建立候选私有的强引用holder；不写Entity的`+4B4`或Familiar的`+ECC`。
- 静态复读发现`430E40`／`606310`的替换helper即使传Null也会分配新的24B控制块。最终释放因此走真实counter虚槽`+0C`和各自最后释放回调，不能用“再Assign Null”丢掉一个新控制块。捕获并复查同一回调，回调缺失时不进入构造；异常或不确定前缀继续保留。
- 2026-11-19 修订：玩家使用 Mr. ME! 后，客机连续以 `binary_fields_shared_release_callback` 拒收 World，幽灵消失后恢复。J273 的控制块虚槽 `+0x0C` 自行减强引用并在需要时析构 payload，正常最终释放也可返回 0。Familiar 与 Effect 原生 setter 只在该槽返回非零且回调可用时通知。暂存和扁平安装均按此规则释放，允许回调为空。
- 五个Weapon槽由计划中的mask、具体布局和实际type创建；type10／11共用Bone布局，不能从布局号反猜type。只使用官方注册factory和`Init(Reset,type,nullptr)`的新对象路径，没有现有Player／Familiar owner，也不写Player槽。逐个核动态vptr、大小、type、空owner与空攻击引用；不调用Fire／Update。
- 退休先通过真实`EntityPtr::SetReference(nullptr)`拆边，保持owner为空，再执行具体动态Free。没有调用会Remove攻击实体、且Urn还会停音的`ClearReferences`。基类析构的Player／Familiar回写分支因空owner不进入。

### 预算、用途与排空

可控的原生容器分配使用游戏CRT，32位Release容器头、tree／list节点偏移和非平凡元素尺寸有编译期ABI断言。大数组按游戏MSVC约定保留32字节对齐前缀；每项真实分配连同前缀计入同一Peer Values额度，释放实际归还。字段owner另持有分配记录；共享payload／控制块及已核固定内部存储也预先记账。SDK内部资产仍归其原生资源owner，不据此声称完整Client episode计费已经实现。

每个Manager边界最多进入32个字段owner；字段内部有限元素仍按其正式上限处理，这不是单次耗时保证。准备与退休沿用`NativePreparation`窄分派，复查主线程、Game／Room／Manager／VM及图形context，要求CPU raw队列无借用。用途visitor退出后，先逆序退休字段，再退休根对象、具名资产及缓存资源。普通预算失败清理已知责任；进入原生构造／释放后异常的前缀不重复执行，也不假报Drained。

### 本轮边界与后续

这里完成的是字段**私有承载**，包含构造和必要的容器形状／键。ANM实例目前取得构造责任；文件／选择器／图层等完整状态仍须由后续具名资源与字段writer装入，不能将其称为完整姿态已安装。普通标量、配置与实体引用、根对象Init、共享字段接合、容器／Weapon所有权及账目接管、Render发布和事件播放仍未执行。所有detached承载都由准备owner继续拥有；后续安装器必须显式接管生命周期，不能裸swap原生header后遗失分配记录或重复析构。

`FieldsReady`只表示本批次的根、资源及私有字段承载当前有效；不是整个`ClientStateNativeOwner`就绪、原子安装成功或ACK资格。下一步是实际原生安装器，随后接完整显示、事件、发布、退休和Manager／session运行链。

## 本轮静态依据与纯编译

除既有J273字段／原生owner资料，本轮只读复核了`9CADB0`工厂、`9C5200`的Reset／type／owner顺序、各动态Reset与ClearReferences、`9C4FC0`及动态Free，和`430E40`／`606310`的控制块构造、`430EB3—430EB6`／`5CCF84—5CCF87`的cdecl最后释放调用。`9E0D50`无条件Stop 22／23的事实继续保留，未放行该函数。

产品`build/repentogon`与E2E配置`build/repentogon-e2e`的Release／Win32 `zhlIsaacCS`均完成纯编译、链接。没有运行游戏、E2E、单元测试、probe或校验程序；第15步仍是唯一统一整体验收。
