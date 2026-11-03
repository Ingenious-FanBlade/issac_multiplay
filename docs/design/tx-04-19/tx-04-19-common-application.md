# tx-04-19 I05—I09：身份、计划、构造、公共值和依赖

日期：2026-11-03，Asia/Singapore。范围为同一tx的I05—I09公共实现。`entity_plan.*`、`client_entity_model.*`、公共字段表和原生执行器已接入CMake与`ClientObjects::PrepareTarget/Step`；没有运行分项测试或启动游戏。全部类别adapter、资源、展示、唯一Scene提交和统一E2E仍按I10—I35推进，不把本次公共实现写成整个19已完成或全Entity域可ACK。

## 已实现的公共路径

| 任务 | 代码与最终行为 |
| --- | --- |
| I05 | 复用`ClientObjectKey`与同一原生entries表，分别保存shape revision、进程单调instance、池use、地址/vptr、本地Index和InitSeed见证。新实例不继承旧use引用。`PlayerOwner()`提供按Key解析的普通Isaac窄接口，没有首玩家回退。回调token由消费端共同持有，owner删除前先失效。多Player Key/多身体逐Key解析并拒绝同地址别名；完整形态由20替换`player_ready`接口。 |
| I06 | `PlanEntityTarget(T,A)`从最后原生A规划Unlink/Remove/Acquire/Initialize/Fields/Resources/Publish/Link/Check。完整同scope枚举才按缺席删除；Retired行、Exists=false但仍在EL、dead-but-existing分开。旧revision、同版本异值、shape倒退或不换revision变形被拒绝；同内容无操作；Null、清零、空容器和回到B的值均参与A→T比较。未写候选可取消或替换，执行前缀一旦进入则须退休，不重放Init/remove。 |
| I07 | `FormalExecute`把取得、构造前置/官方动态Init、最终值、资源、成员发布分成可预算步骤。取得与Init进入前先登记责任；`Create(type,true)`不覆盖活池项，Dummy必须提供具名专用acquire。保留官方Factory/Init链，不伪造Plus或重复New通知。按J273 main/persistent/render/update/wisp规则核对真实成员，变化bit8只使用Add的valid分支刷新update/cache。类别回调须在写前解释派生初始化、连带主体和专用清理；缺回调不执行。 |
| I08 | `client_entity_fields.inc`是模型解析、native写入、native后态及功能目录的共同声明。覆盖位置/速度/偏移、目标位置、HP/MaxHP、exists/dead/visible、flags、碰撞、质量/大小/SizeMulti、Friction、状态计时、颜色、Sprite别名及渲染层。FrameCount通过游戏帧减实体基准安装，不把Authority tick当帧。WaterClip通过官方leaf接口安装并pcall保护。check真实读取当前owner；Source Index/命中编号/DropSeed/生成归因保留纯值，不覆写本地身份；ColorParams保留权威时间线，安装当前Color，不推进本地状态机。 |
| I09 | 整component图解析覆盖graph_ref与owner-qualified依赖。Entity/Player/Grid/Room/descriptor、历史/查询值分类型。Familiar Spawner、Knife Parent构造前置单独拓扑排序，普通后置环采用两阶段绑定。五个公共关系走EntityPtr反向登记；类别关系通过同一解析器取得当前实例。旧非Player出边先解除，再退休/替换，最后重绑新实例。Effect136使用Grid owner的同scope描述依赖检查，不构造活动Grid。 |

SDK边界用不可变共享组件文本提供派生完整值和图解析域，不传Lua对象或模型DOM借用。`ClientEntityClassAdapter`只由产品代码注册，不接受网络函数地址或任意方法名。未注册I11—I22类别时，`PrepareTarget`返回`objects_entity_class_adapter_pending_I11_I22`，不会试调用不明Init来补表现；公共执行路径完成与类别支持分别表达。

`player_ready`核对本次所需形态/状态。17 owner只接受真实拥有且已checked的普通Isaac、当前Key/shape/instance、基础统计及主Sprite后态；不能声明覆盖20全部字段。相同scene中的Player成员集合变化仍交给唯一Scene的Player阶段，19不创建或退休第二份Player。

## 构造、退休和借用合同

Init通过真实虚方法进入官方链，基类Plus更新与Factory的ESSM通知保留原位置。Acquire、Init、Fields、Resource、Publish、References、Remove、Detached等前缀在可能产生副作用前记录。异常使owner不确定，后续不盲目重试；地址复用必须产生新instance/new pool-use。字段安装不改本地Index/vptr，成员发布才分配本地Index。

类别`prepare`在首次写前检查完整派生值、具体变体、资源及已核Init/退休分支；`before_init`只做已核前置；`fields/resources`均有真实后态模式；`relations`区分绑定/检查/清除；`retire`承担派生Remove/ClearReferences和附属owner。公共层再完成ESSM Delete、列表摘除、valid=false与类型查询缓存失效。类别retire不能重复Delete通知，不能用通用ClearReferences掩盖子对象副作用。未完成派生Init或异常时保留进入记录；完整异常退休由I28继续，不猜测释放。

各列表中的每个实际地址必须对应唯一当前目标实例或外部Player。effect/temp等临时视图不冒充主owner，Add不会自动建立这些专用成员，实际使用类别须具名处理。Add的敌人/Boss临时累计`+A8/+B0`与更新汇总`+AC/+B4`不同；基Remove不会对称减计数，Add也不是完整缓存重建。完整展示/派生缓存及必要汇总随I25接线，不启用本地玩法Update。

原计划F01—F28的本次具体处理包括：不完整枚举不删除；旧scope/revision不复活；无新shape的nonexisting→existing拒绝；构造前置环写前拒绝；后置环先建实例；Null真实解除；Player迟到/错形态/复用或同地址多Key不由首玩家替代；池不足不覆盖活项；无专用Dummy owner不落NPC池；已写取消必须退休。没有为这些分项编写/运行单元测试，运行覆盖仍归I34。

## 必要静态逆向与主审

先读原版本地`Entity.md/EntityPtr.md`、REPENTOGON本地Entity文档，再读官方`Entity.zhl/EntityList.zhl`、`EntityManager.cpp/EntityPlus.cpp/LuaEntity.cpp`和ColorMod/ColorParams布局；源码没有的vanilla属性绑定才查J273。一个GPT-6 Luna max子智能体完成必要逆向，主智能体独立复读发布函数、Lua注册邻域、访问器及Remove/ClearReferences指令。下列均为静态事实，非运行观察。

地址为VA，基址`0x400000`，RVA为VA减基址。

| J273位置 | 主审采用结果 |
| --- | --- |
| `413D00`，`413D0B—413EB1` | valid=0时，Type1/3及Type8且Subtype0进入persistent，其余main；按GetRenderZ插入render；Type3:206进入wisp；flags bit8控制update。valid=1只刷新update及类型缓存，不重进其它owner；EAX不是成功合同。 |
| `431380/4313A0`，注册`84F33F—84F349` | 公开DepthOffset写/读float `+358`。ZHL `_depthOffset +2B0`不能代替公开属性；正式路径与17代表路径均改用公开绑定，`+2B0`真实语义仍UNKNOWN。旧验收事实不追溯改判。 |
| `4313B0/4313D0`，`84F54C—84F565` | HitPoints/MaxHitPoints是float `+380/+384`；只安装结果，不调用Damage/Kill。 |
| `452210/4521F0/429DA0/43A840` | SizeMulti Vector `+374`、SpriteOffset Vector `+DC`、SpriteScale Vector `+E4`、SpriteRotation float `+EC`；对应RVA为`52210/521F0/29DA0/3A840`。 |
| `4313F0/43C740/565250/43C750` | TargetPosition Vector `+334`、RenderZOffset int32 `+354`、SortingLayer int32 `+35C`、FlipX byte `+148`。RenderZOffset实际视觉效果不从setter存在外推。 |
| `84F751—84F75D`，`8A45E0/8A4640` | Friction闭包捕获偏移`+36C`，按float读/写，不与速度乘子混用。 |
| `6A5290/6EB980` | FrameCount=`Game+25A70 - Entity+328`；SpawnGridIndex读`+32C`。前者安装本地基准再真实核对，后者保留Source事实。 |
| `84F62D—84F641`，`4521B0/4DF5D0/502750/8585E0/858680` | SplatColor是44B ColorMod `+1E4`，有直接拷贝setter；GetColor方法返回`+F0`。Color属性getter复制`+1B8`，setter会经虚SetColor，故安装已核当前值。Source read.lua用handled首canonical id，Entity.Color先采Color属性，GetColor别名不会覆盖；Sprite Color由资源owner处理。 |
| `6A8000/6A8060` | Remove分开写Exists/Dead并释放`+408` LuaBridgeRef，不是释放Plus。ClearReferences撤EntityPtr反向登记，可能Remove `+310`附属对象并清资源；Plus析构和ESSM Delete各有不同责任。 |

主审纠正子结论中的SplatColor/GetColor配对、漏位RVA及把LuaBridgeRef释放称为Plus释放的中间推断，没有将其用于写回。Color别名通过实际read.lua去重规则核定；公开DepthOffset与SDK旧名区别明确登记，不默认为同义偏移。

## 构建与后续接线

本轮Win32 Release纯构建已成功编译、链接`out/repentogon/zhlIsaacCS.dll`并复制正式资源；保留RapidJSON/SDK及既有源码警告，不声称零警告。普通沙箱MSBuild停在文件扫描已中止，之后在构建所需执行权限下完成。没有部署游戏、运行测试或创建tx/E2E artifact，不提前执行I34。

I10接资源owner；I11—I22注册类别完整adapter；I23/I28补具名形态和异常退休；I24接Grid descriptor值；I25接完整分层draw/缓存；I29纳入唯一Scene提交；I33/I34再整体E2E。本次没有把非Player类别能力、Player20能力或完整Entity提交位改为true，没有第二套Scene或网络ACK。
