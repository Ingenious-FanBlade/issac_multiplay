# tx-04-19 I10—I14：资源与 NPC／Tear／Projectile／Bomb 应用

日期：2026-11-03，Asia/Singapore。范围为同一 tx 的 I10—I14 代码、正式来源、类别注册与静态采用合同。Win32 Release 已纯编译链接；没有运行分项测试、E2E、游戏或部署。统一运行验收仍在 I34。下列“已实现”指明确的代码路径，不表示整个 Entity 域、全部特殊 NPC 绘制或整局客户端已经完成。

## 实现落点

| 项目 | 已实现行为 |
| --- | --- |
| I10 | `client_resource_model.*`、`client_resources_native.cpp`、`client_resource_shaders_native.cpp`共用主／overlay／各层／贴图／颜色／blend／完整32位flags／shader／分数phase与独立selector安装。正式`SpriteRuntimePose`增加11-float冠军ColorMod、播放标志和两类shader路径。主、overlay、null layer的当前selector按实际AnimationData检查，不调用Update推进动画。 |
| I11 | `client_entity_class_model.cpp`与`client_entity_adapters_native.cpp`注册NPC；同一NPC的Delirium专用状态按u8／u16／u32分别安装。状态、I/V、group、scale、冠军及Boss颜色、当前boss/关门属性、护盾、dirt color、controller和官方Plus可空flying override有原生写入／后态。补同use的`+CD8/+DEC`双ANM与内部关系正式来源。具名私有draw缺口见下节。 |
| I12 | Tear公共派生字段、128位flags、scale/baseScale、sticky状态、ParentOffset/PosDisplacement、强度／旋转／计时与三份独立辅助ANM均接入。命中数字只保留Authority事实。`Bounced`自Repentance已删除，只接受Null／NotApplicable，绝不编造字段。 |
| I13 | Projectile使用自己的float/u64/i32布局；Damage在`+41C`，WiggleFrameOffset在`+448`，两份flags各为64位。DepthOffset与公共同名值必须相同；不套用Tear布局或运行弹道／落地／分裂。 |
| I14 | Bomb的128位flags、伤害／范围、计时／fall／rocket／scale／fetus／prism与五份costume分别安装。补`NativeBomb.CostumeDrawEnabled +462`真实Source原值；`+463`只表示本地懒重建请求，安装Source已有最终资源后置false，避免Render重跑`6A2420`。Source的pending事实仍在纯值payload中。 |

`client_entity_class_fields.inc`是派生解析、宽度约束、原生写入／真实后态与功能目录的共同声明。每个已发送公共类别字段都有明确去向：原生最终量、资源、具名关系、Authority查询／命中历史、已删除旧API。未解释字段、缺失必需值或不适用占位均在写前拒绝；不会靠保留池旧值满足检查。

版本化补充使用`application_links.links`与`application_runtime.values`，区别于公开API的`fields`白名单。正式投影可以保留这些纯值结构，再按专用路径提取关系名；若误用公开`fields`结构，未知Native成员会被投影为Unsupported。完整Key／Null及Bomb启用位都有正式采集路径，不靠测试夹具填值。

类别解析在模型阶段完成一次，SDK边界仅传自有不可变`ClientEntityClassState`。`ClientObjects`默认注册四类内置adapter；外部owner配置仍可在候选前显式替换整组合同。没有注册I15—I22的类别继续拒绝，不降级为NPC。构造及实例／池use仍由I05—I09同一执行器管理，没有第二套Scene或实例表。

## 资源与姿态

主／辅助Sprite按实际成员逐个借用；文件名相同不合并实例。NPC两份、Tear三份、Bomb五份都有完整性要求，Bomb索引按0—4检查重复／缺项。Null代表明确卸载：用本方已登记的空ANM候选与成员`SwapANM2`，让原生交换重绑layer owner；只析构本方候选，禁止delete或就地析构后重建实体内嵌成员。

Shader公开接口只暴露Has查询，没有路径getter。现有早期Lua绑定owner在RegisterClasses后、bundled/mod脚本前包装Sprite／LayerState两种SetCustomShader／SetCustomChampionShader：原setter恰调用一次，成功路径存于VM registry的有界集合。正式采集以当前Has(path)核对真实成员，再复制路径；不保存ANM地址、不扫描REPENTOGON私有map。任意未观察路径或预算超限的当前shader明确ReadFailed，不冒充“没有shader”。原生安装通过固定白名单的Set/Clear/Has执行并核对，Lua错误由pcall收口。旧来源明确false的Has查询也会清掉上一目标shader。

冠军ColorMod与Sprite.Color分开。播放标志同样与本地推进分开：NPC辅助draw读取`_isPlaying`，所以保存Source该位，仍不执行Entity／ANM Update；原有动画事件位保持清零，独立事件消费由I26负责。分数phase与main／overlay／null层selector可以分别安装。J273 null descriptor是16B，frames/count为`+8/+C`；实际null-frame步长54h，公开NullFrame未命名尾部不被复制。负selector只接受-1，空层允许原生无帧哨兵0或-1；正count必须有帧数组，越界正值拒绝。

先准备私有候选并核定动画／层／图像pair／shader，再写当前成员。任何原生写入沿用公共前缀并撤销展示资格；换实例或资源变化重新借用。正常候选退休等待主线程context和CPU提交队列为空。原生异常保留不确定owner，不能盲目重试析构；通用异常恢复和唯一提交继续由I28／I29收口。

## 引用与生命周期

正式Source增加`body.application_links`，其固定槽位来自`client_entity_links.inc`：公共`+310`；NPC `+F50/+F54/+F58/+F28/+F38`；Tear `+488/+48C/+490/+494/+498/+49C`。只在同use完成创建owner的读取窗口复制稳定Key／Null，不传原始指针。公共模型同时修复了Lua通用Null节点未建解除边的问题；Sprite Null及optional bool Null不被误当关系。NPC QueryNPCs／PathFinder结果保留纯值，不安装为原生出边。新来源必需`entity-class-application-v1`，旧消费者不能把新增结构误当成已经应用。

NPC EntityRef `+BEC`使用官方EntityPtr helper；ParentNPC／ChildNPC只是公共父子别名，写前要求两份Source引用一致。进一步主审发现NPC `+F40`在原生清理中会被虚ClearReferences后Free，不能当作普通roster借用；Source增加owned-actor存在性事实，非空明确不完整并拒绝。Siren的非空关联因此继续交I25／Player20，不回退首玩家，也不写空未知owner。Tear StickTarget的公开Lua setter裸写指针，但原生`6B17C0`和`663570`有反向登记协议；本方采用官方EntityPtr helper维护同样的4B槽，而非复制Authority地址或依赖裸setter。

先撤全部旧出边，再Remove／替换。类别退休还会显式撤公共／专用边，防止NPC866的Child、NPC907的双向改链、Tear附属／bit127链式主体和base`+310`的连带Remove重复退休T中仍存在的独立Key。这是来源与owner已具名后的结果安装合同，不能将其外推为任意未知附属对象都可写空。ESSM Delete、列表摘除、valid与类型缓存仍归唯一公共owner；正常派生Remove/ClearReferences继续执行，不调用Kill／爆炸／掉落。实际vtable核对确认NPC清理为`6B4E60`、Tear为`663570`，Bomb／Projectile为base`6A8060`，不能以ZHL的skip注释推定继承关系。

NPC912／903等清理还会调用全局SoundEffects Stop。新增薄wrapper只经唯一Hook清单注册并常驻，仅本方类别Retire线程作用域跳过这些Stop；不计成原调用完成，其他用途原链一次。入口VA`908140`，ECX=this且一个int栈参数，两返回均ret4；没有复用缺失的C++ SDK声明，官方Lua Stop存在。通用瞬时音画及整个Scene退休音效归I27／I28继续集成。

Init前清旧use关系，安装当前Source位置／速度，再进入真实动态Init及官方Plus链。已核Tear／Projectile临时global RNG调用只在这个Init线程作用域由既有常驻Hook返回Source seed，不调用原随机源、不伪造观察Returned，Source最终量随后覆盖临时初值。Host／Off及其他用途仍原链。Bomb自身drop RNG初始化结果不充当同步状态；命中编号／RNG历史不写成Client身份或开启玩法模拟。

## 具名边界与后续责任

I25仍负责整个类别的原生分层draw／shadow／light／water／mirror、临时姿态恢复及复合owner pass；I29负责唯一Scene提交。本次未修改代表presentation去调用未知类别Render，也未设置完整Entity域提交位或发网络ACK。

以下NPC目标在写前返回`entity_npc_private_draw_owner_pending_I25`，保留完整成员表而不删除对象：Type835／906／920的`+BF4..C00`私有层容器，Type921的`+C04/+C10/+C14`容器，Type951 Variant3的私有曲线／ring，以及Type965 Variant10的Player minecart乘客pass。额外Source记录`+BF0`私有引用容器的当前count与`+F40`拥有的actor存在性；非空／拥有状态明确使Source不完整并写前拒绝，绝不复制容器header或只写count。静态读到地址不足以证明容器元素、分配／清理和完备Source；I25必须补完这些合同后删除对应拒绝，不能长期缩小原版内容范围。Delirium变到这些具体形态同样不能绕过拒绝。

Type245、411／816的EntityRef复合draw，865／909:10／912的owner颜色和部分资源，NPC OverlayEffect及Minecart的具体输入位置见[静态研究](entity-i10-i14-native.md)。本次双ANM、公共引用、EntityRef、完整null selectors和播放标志提供了已核输入，仍不代表这些pass已经被调度。Bomb未命名`+414`计时语义、NPC特殊容器、所有异常和任意第三方Mod回调均保留原UNKNOWN；不通过调用Update获得缺失结果。

I15起继续其他类别；I23／I28补变形、取消与不完整前缀，I25补上述draw，I31随新增消费者复核发送优化，I33／I34完成统一E2E。Player完整状态仍归20。此处的具体未决项属于同一tx的后续集成工作，不是新gate、测试批次或额外tx。

## 构建与运行状态

完成Win32 Release `zhlIsaacCS`纯编译链接，生成`out/repentogon/zhlIsaacCS.dll`。普通执行权限下MSBuild停在文件扫描，已终止该构建进程；构建所需执行权限下完成。SDK／RapidJSON及既有警告未隐瞒。没有运行测试、游戏或创建tx/E2E artifact；不将编译成功写成运行通过。
