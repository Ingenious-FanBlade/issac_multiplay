# Laser／HomingLaser／Bezier 采样、运动与计时同步复核

日期：2026-11-09（Asia/Singapore）。第4步，布局版本 **37**。范围为J273官方展示消费者及其调用链；仅资料阅读、只读IDA静态分析、布局／计划修订和生成。没有构建、测试、游戏、注入、部署或IDB修改。版本28／29已排除的伤害、命中历史和内部关系保持原职责。

## 1. 采用结论

**保留 optimized／unoptimized 两组最终点列；indices 中间点缓存、Bezier输入和六项采样标量留Host。Laser另有19项更新／运动／计时输入撤销传输。** 当前角度、长度、完整TearFlags、原始有符号采样模式、MaxDistance、公开EndPoint、+4F0不透明开关和GridHit有直接展示消费者，继续同步。

| 原生范围 | 当前归属与理由 |
| --- | --- |
| Homing `+1C`／Laser `+480`，vector头12B | **保留 optimized**：Laser Render／Shadow／PostRender使用最终点列 |
| Homing `+28`／Laser `+48C`，vector头12B | **保留 unoptimized**：Room Light的Type7分支使用它；不能只传优化点替代 |
| Homing `+34`／Laser `+498`，vector头12B | **Host中间点缓存，历史别名indices**；J273实际为8B双f32点对，旧i32／4B步长记录错误，详见第3节 |
| Homing `+48`／Laser `+4AC`，vector头12B | **Host Bezier输入**：24B六f32记录，无展示消费者；系数成员的业务名仍未知 |
| Homing `+04/+0C`，各Vector8B | origin／end_point留Host；后者与Laser公开 `+4D8` EndPoint不同，公开端点仍传 |
| Homing `+14/+18/+40/+44`，各4B | parameter／sample_num／cursor／spacing留Host；sample_num不是vector长度，cursor／spacing是旧适配器命名，不推成物理游标／间距 |

| Laser范围 | 排除项及已核用途／边界 |
| --- | --- |
| `+410/+418/+420`，Vector／f32／f32 | ParentOffset／StartAngle／LastAngle为Host父体跟随和更新历史输入；最终位置、当前角度与几何独立传输 |
| `+458/+45C/+45E`，i32／bool／bool | Timeout／FirstUpdate／Shrink为更新生命周期／收缩输入；撤销Timeout同步锚点，最终长度、Sprite及生命周期结果保持 |
| `+4B8/+4BC`，各f32 | CurveStrength／Radius不传；Radius用于更新几何。CurveStrength已核构造存储和公开字段对应，本轮没有证明其实际曲线算法消费者，不能从名字编造生产公式；无展示读取 |
| `+4C0/+4C4/+4C8/+4CC`，bool／i32／f32／f32 | ActiveRotating／RotationDelay／RotationDegrees／RotationSpd为Host旋转更新；撤销Delay锚点，Client安装最终Angle |
| `+4D4/+4E0/+4E4`，u32／f32／bool | HomingType／Scale／DisableFollowParent为追踪、尺寸及跟随更新输入；保留最终点列、公共尺寸／偏移与Sprite变换，不重算 |
| `+500/+514/+515/+560`，bool／bool／bool／i32 | MultidimensionalTouched／PrismTouched／OneHit／ChainedLasers为更新、接触与派生生成状态；Client按最终独立成员展示，不重演接触／分裂／伤害 |

19项Laser范围共59B原生字节，对应旧线上67B（两项i32计时曾各编码u64）；六项Homing标量32B及两个撤销容器的count前缀4B再减少36B。**EntityLaser最小状态542→439B，减少103B**，另停发indices／Bezier变长内容。旧indices的i32步长已被纠正，不能把旧4N载荷当作正确原生数据的带宽基线。HomingLaser自身最小40→4B，仅两组u16 count；Bezier布局23最小24→0B。

原布局号23／24／127及全部块号保持：Homing子块0／3／4、Laser L2／L4／L7／L8显式预留；原L9也保持预留。父Homing投影只列子块1／2，生成器只要求所有**实际产生发送字节**的子块各投影一次，允许省略预留子块；兼容Effect等已有空投影，不要求改动无关源。本次Laser不制造零长字段。Bezier撤销只供线上完整copy使用的`pure_value`标记，原生六f32类型事实保持。撤销相应wire、读取资格、validator、换算声明及公开成员状态回映射。Laser未知421B及原有证据等级保持；最小字节数是布局算术，不是实测带宽或类型完整声明。

生成过程的修正记录：首次仅以非空子块构成允许投影集合，导致既有 `EntityEffect_SpriteTrail.extra_data` 的空子块0被报为非法。随后区分“必须包含的非空子块”与“允许声明的全部子块”，保留既有空投影合法性；最终生成正常完成，没有改动Effect布局源，也没有运行测试。

## 2. 实际展示消费者

目标 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`；以下地址为VA。本地公开语义参照 `IsaacDocs/docs/EntityLaser.md`，存储／旧分支事实参照[原始证据](tear-pickup-laser-evidence.md)与[内部关系复核](internal-entity-relations-review.md)。

Laser vtable `B2D81C`：Update `950B90`、Interpolate `956B50`、Render `9514B0`、Shadow `955290`、PostRender `956F80`、GetRenderZ `945E00`、GetForwardVector `6B1FD0`。GetRenderZ只使用公共Position.y与DepthOffset，不读本轮参数。Render大函数使用IDA完整反编译及原始指令，未将工具截断预览充作完整函数。

| 保留字段 | 正面读取证据 |
| --- | --- |
| optimized，Homing `+1C` | Render `9517B5/9517BB`、`951818/951826` 等取vector范围／点；Shadow `955441/955447`、PostRender `95702D/957033` 等同样读取 |
| unoptimized，Homing `+28` | Room::RenderEntityLight `7FC110` 的 **Type7** 分支，`7FDCC9/7FDCCF` 取Laser `+490/+48C`，`7FDCF6/7FDDA2/7FDE93/7FDE9F` 等遍历点 |
| Angle `+41C` | Render `953044/9536E6/954073`，PostRender `95754B/957848` |
| TearFlags `+428`，完整128位 | Render `9520E6/9522AB` 取址，Shadow `955CCA` 取址后交效果位求值；不能随运动规则一起排除 |
| sample_mode_raw `+45D`，i8 | Render `9514EC/951712/953DCC`、Shadow `9552CC/955351`、PostRender `956FD6/95701C`、Light `7FDC7C`；继续区分-1／0／1，不缩成bool |
| Length `+460` | Render `952EFA/952FB0/9530CD/953645/953727`，PostRender `957889` |
| MaxDistance `+4D0`／GridHit `+516` | PostRender `957113/957103`，实际展示分支不可省略 |
| 公开EndPoint `+4D8/+4DC` | Room Light `7FDEFF/7FDF0B`，独立于Homing的采样输入端点 |
| start_offset_mode_opaque32 `+4F0` | Render `952906` 直接分支；保持原i32与不透明语义，不虚构偏移算法 |

BounceLaser `+4E8` 不属本轮排除范围，继续当前真实成员引用与正常撤边合同。最终公共Position／Velocity／PositionOffset／Size／SizeMulti、Sprite与图层变换、生命周期和Room展示时钟亦各自保持。

## 3. indices类型纠正与采样调用链

`CalculateSamples 9768C0` 的原始指令足以否定旧i32假设：

- `976F08/976F0B` 取得Homing `+34` vector；`976F16` 将字节差 **右移3位**计算元素数。
- 数量变化分支 `976F38/976F3E` 将该vector交 `858EE0`；该helper按8B元素分配、复制两个4B槽。
- 同数量分支在 `976F48` 检查Homing `+40==1`；`976F70—976F9A` 按索引×8读取两个浮点，执行差、乘parameter、加回并分别写两个f32，`976F9E` 前进8B。
- 末尾 `9770A3` 再通过 `858EE0` 将 `+34` 点缓存复制给optimized `+1C`。因此本轮保留历史别名indices，但原生类型改为 `std::vector<Vector>`；它是采样平滑中间点缓存，不能继续按i32索引传输。

Homing构造 `974770` 正常建立本地self及vector；路径生产 `974C60`、Bezier维护 `976230/9766C0` 和 `CalculateSamples 9768C0` 消费采样输入与系数。Laser更新 `94C940` 在 `94D1FF` 调路径生产、`94D25A` 调采样；子类型更新 `954310` 在 `955029` 调采样。`974C60` 另有Knife初始化 `93A330` 的调用，不能因此视为Knife展示消费者。Homing `+44` 在采样中清零、累计、归一化，本轮保持其业务名UNKNOWN，不推定物理间距。

Render `951E5F/951E7E` 与Shadow `955A8D/955AA3` 也调用8B点vector的erase／insert helper `958880/9588D0`，但ECX明确为栈上 `lea [ebp+Block]`，是临时绘制点列；它们没有取得Homing `+34/+48`，不能误判为indices／Bezier依赖。Room Light `8002BB` 的 `+478` 读取属于 **Type1000 Effect、variant0x79**，不是Laser Homing parameter；Effect的+4B4／+478、Player／Bomb同偏移对象同理排除。

复用[Projectile复核](projectile-synchronization-review.md)的26个展示根，补核Laser GetRenderZ／GetForwardVector共28根；停止展开明确的Lua render桥，直接可达集合534函数。目标位移内存、LEA及ADD／SUB取址扫描涉及1005个候选函数，与展示集合交叉25个；另查+500及跨字段宽读取，按真实基址区分类别。路径生产、采样、两个Laser更新helper和Interpolate均不在该展示集合。该静态采用结论涵盖当前官方展示合同，不宣称任意第三方Lua／虚分派／别名的形式化全程序证明。

## 4. 更新与计时事实不因排除而改写

ParentOffset在 `94CA37/94CA5F` 等加入跟随位置；LastAngle在 `94CC6C` 比较，StartAngle／LastAngle在 `94D6FC/94D6D7` 等维护。FirstUpdate `94C98E/94CC4F`、Shrink `94DCF2`、Radius `954440/955164`、HomingType `94CC3D/94D1CD`、Scale `94FF10/95052E`、DisableFollowParent `94CA18` 均属更新；MultidimensionalTouched `94D5A8/94D84E`、PrismTouched `94DCB5/94DE91`、OneHit `94C998/94C9F0`、ChainedLasers `94D933/94E281` 等维护更新／接触状态。CurveStrength本轮只有初始化 `9482BA` 的正面存储证据，不编造消费者。

Timeout的历史事实保留：`950AC0` 正值且公共 `+175==0` 才在 `950ADB/950ADC` 递减，零时 `950B46—950B59` 进入Shrink／dead；Shrink分支 `94FD2A` 可递减到负值，`94FD48` 比较-18后Remove。SubType 1／2／3的 `954310` 仍有0／5／0x2D等重置及未闭合活动条件。RotationDelay在 `94CAF9` 以ActiveRotating为条件，`94CB16` 任意非零值（包括负数）到 `94CC29` 递减，零时转入角度更新；`94CB24/94CB44` 等使用旋转参数。

这些分支UNKNOWN仍是Host研究事实；**不再要求为已撤销传输字段核定同步时钟、产生Gap或在Client恢复计时**。Interpolate `956B50` 包含位置／历史维护、FirstUpdate与子类型分支，并进入更新helper，仍属模拟，不能为了画面平滑放行。收缩、旋转、追踪和分裂的最终角度／长度／点列／Sprite／独立成员／退休由Host输出，Client不重演。

## 5. 采集、安装、退役与唯一验收

第10步只在同一entity窗口采集两组最终vector及真实展示标量；每组独立count≤16384、有限点与实际全局池预算保持。采集前后核定来源／生命周期，不读已排除标量、缓存或系数，也不因其未知采样计时阻止快照。第12步只解码L5／L6真实投影；预留L2／L4／L7／L8及Homing子块0／3／4不作为构造必收负载。

第13步两组点列由各自兼容vector owner构造候选，所有目标版本依赖和真实资源就绪后安装；count各自合法，不强求优化与未优化点数相同，空集合必须清除旧点。保留本地self，indices／Bezier保留正常构造的本地空vector及正常析构；旧实例如含旧绑定／分配须通过owner正常释放，不能memcpy零头。不得调用CalculateSamples、重采样请求、ClearLaserSamples后靠计算恢复、Update／Interpolate、碰撞、接触或级联Remove。两组最终点列不得与采样缓存共用owner，也不能清空保留几何来初始化已排除容器。

第14步按新源退役19项Laser输入、六项Homing标量、indices／Bezier及系数的旧Source、writer、validator、原值／漂移比较与两项Laser锚点要求。旧执行器的indices i32假设也随退役处理；本轮未修改运行中的Lua／JSON链或声称修复旧执行器。布局源／生成链更新不代表第10／13步已接线。

第15步沿用已有编号：E09／E25覆盖直线、追踪、曲线、环形、主动旋转、收缩、父体跟随、接触派生与独立成员，比较最终画面和两组完整点列；兼顾原始采样模式-1／0／1、当前角度／长度、效果位、公开端点及MaxDistance／GridHit展示分支，Light须实际消费unoptimized。E13／E17拒绝真正上线的点数超限、非有限点／标量、来源变化、真实引用、分配失败与预算溢出，不构造已撤销输入的非法负载。E29覆盖两组不同点数、空↔非空／增长／缩短、同目标版本完整安装、失败不部分替换、正常退休及本地缓存／系数空owner；E28确认Client没有模拟／采样，draw借用精确恢复。E34撤销Timeout／RotationDelay锚点及原值比较，其他实际线上计时与展示时钟保持；E27保留全部UNKNOWN。仅安排第15步，未提前执行。
