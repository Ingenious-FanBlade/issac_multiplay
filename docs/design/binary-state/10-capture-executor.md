# 第10步第三项：计划执行器

2026-11-11。接续[公共采集基础](10-capture-foundation.md)，实现按生成数字计划执行整根采集的公共层；布局及协议保持版本66。真实各域来源和Collector／worker接线继续后续项，第15步统一验收。

## 编写代码前列出的失败方式

- 只处理CopyRange，漏掉父类、条件、嵌套投影、共享presence、容器或动态Weapon槽。
- 条件未满足仍读取旧尾值；选择器缺失当false；Item配置身份未解析便选择704分支。
- 规范零分支调用原生来源，或将必要来源失败编码成合法空值。
- protocol／synthetic／companion当根偏移；容器、共享holder或动态类未核实际owner便跟指针。
- 数量先窄化、计数组重复计数或投影不一致；复制元素后才发现合计上限已超。
- 递归丢失canonical父路径／实际投影；动态槽选择未登记布局或借别的身体身份。
- handler没有写值、写错路径、额外写子树，或者返回后留下非有限数／非法枚举。
- 仅核暂存容量，遗漏字段／块／布局实际线上字节及递归访问预算；极多空节点耗尽CPU。
- 读取前合格但owner／容器／共享来源已经变化；失败只丢字段而没有回卷整根并禁止整包发布。
- scratch在递归中互相覆盖、逐帧分配或锁账本；重入／错线程复用执行器。
- 将通用遍历完成写成实际原版来源已闭合、正式采集已启用或测试通过。

## 实现与边界

新增[`capture_executor.h/.cpp`](../../../mod/src/state/capture_executor.h)，编入`isaac_cs_state`产品库。执行器只依赖公共采集层与第9步同源数字表，不依赖SDK、Recorder、Lua或JSON。唯一布局源、生成器、349布局／2806字段／85条件及协议版本66均不变。

### 1. 整根执行与操作覆盖

`CaptureExecutor::Execute`在指定主线程上复用冷准备的scratch，RTTI根经`CapturePlan::Identify／SelectArm`选臂，无RTTI／protocol根由具名owner指定。生成字段已经包含父类；按真实块序执行整个world／body／value流，递归时按`nested_block`收窄子块，并保留canonical父字段、逻辑元素及实际choice布局。PlayerPost只允许有required_revision的body采集，Production只允许value；ordinary允许正常world及body-joint。

| 操作／规则 | 实际行为 |
|---|---|
| Copy | 复用公共层合格CopyRange；标量原生／线上宽度必须相等，特殊源不默认memcpy |
| 内嵌Copy／Nested | 对真实Object、非protocol、无特殊handler／storage的内嵌视图检查父extent、子大小和32位加法后递归；ColorMod等Copy形式的结构也递归，不遗漏子字段 |
| Convert／Reference／Resource | 当前路径交给冷注册数字handler；必须恰好写一条类型正确的值。引用／资源保留Null／Identity／Pending自有handle，不提前转Peer索引 |
| Container | 具名来源先提供原生宽count；检查limit、u8／u16宽度、固定槽、同组投影和去重总数后才复制元素。按逻辑序递归；标量容器写一条完整数组；确证count=0时不调用元素／旧负载handler |
| shared-owner | 第一项payload复制前预读同canonical全部选中投影，核同一非零来源证言及presence；缺席只计生成最短零编码，不访问payload；尾投影同样写纯值presence事实 |
| Choice | 先核mask有效位，按实际槽序取得真实view；严格检查生成LayoutChoice允许集，写槽布局后递归，不按type猜测或退回基础类 |
| 条件／规范零 | Equal／NotEqual／AnyBits／OneOf使用同实例先行整数；ItemConfig使用同来源资源句柄的已核配置，缺配置报ResourcePending，704零除数失败。空主动槽／Null costume按同源规则直接写零，跳过旧尾值来源 |

`BodySourceFieldInfo`扩展只读暴露storage、数字谓词、selector／operand／choices、规范零selector及nonnull规则，继续来自现有`state_body_codec.inc`。`ValidBodySourceScalars`与正文codec复用同一个无异常scalar校验核心，检查有限值、布尔、带符号范围、枚举、value_mask、计时编码、nullable-water；字符串同时检查C串内嵌零／显示UTF-8，set检查严格升序。有序map以每个元素真实首字段作严格整数键比较，不排序或重排原版结果。

### 2. 来源owner接口与尚未接通边界

`CaptureTraversalBinding`在冷准备按`(operation,handler)`绑定open／element／close，逐帧使用全局字段索引分派。open提供只读游标、宽count／presence／mask及当前root/use内的非零来源identity；element只解析一个逻辑元素或动态槽的临时`CaptureRoot`；close重新核原容器头、holder、动态类／资源和use。callback不能写writer或私自递归整棵子树。scalar／link叶handler继续通过`CapturePlan`绑定，由执行器检查恰好一条输出、当前路径、实际数量及资源表空间。

游标的opaque和identity仅在主线程当前同步遍历借用，可承载实际头／holder／生产修订等证言，不进入packet或后台。相同计数组slot必须同count／同identity，shared投影必须同identity／同presence；不同slot才参加合计。实际owner仍须正确检查元素顺序、共享具体RTTI、容器头和原位变更，不能返回恒真close或用空容器掩盖缺源。

`CapturePlan::Qualify`将原有窗口／字段资格检查复用于open、child和close，读取前后均核完整来源；整次Walk结束再次复核所有已打开来源。窗口current必须能按同一subject/use检查当前根及已核嵌套视图，不能只认顶层地址，也不能因子视图无独立身份而放宽生命周期。直接读取仍只经`CaptureWindow::Copy／read`；不可读访问捕获由真实MemoryReader适配负责，执行器自身不解引用原生地址。

**本项完成的是通用计划执行，不是各域原生来源。** vector／deque／string、官方map／set／aggregate、资源、Plus、共享payload及正常显示缓存等具体open／element／close和叶换算绑定，仍按[来源缺口G04—G15](10-contract-source-gaps.md)逐域实施；没有根据storage猜测任意来源ABI。当前缺绑定即报MissingHandler／MissingQualification，不生成最短空样本。Collector／PlayerPost桥、真实成员清单、identity预约、全组HUD／音频缓存、快源选择及ordinary joint重采、正式worker均尚未切换。

### 3. 容量、失败与所有权

递归字段状态按`(capacity.depth+1)×BodySourceMaximumFields()`冷分配，默认深度32、复制scratch64KiB；当前字段最大数117来自同源生成表。绑定表、字段状态和复制区实际capacity计入Workspace总账（128MiB共享池），逐根只重置游标和已用状态，不分配、不加锁、不等待。执行器固定唯一线程，拒绝重入；plan必须比执行器活得更久，整个owner只在安全退休阶段销毁。

每根默认最多64KiB实际线上字节、262144次节点／字段／标量访问和32级父路径；与正文相同，普通执行不超过64KiB，可靠执行最多32MiB保护范围，仍受实际buffer与layout约束。字段、块、layout record及body专用预算分别复核，包含动态count、共享presence、槽mask／布局号和最短零字节；嵌套容器连最短合法长度都放不下时在首个元素前失败。handler输出的9B自有link暂存与实际2B线上索引分开计费，packet存储仍受CaptureBuffer容量限制。body专用预算超出返回NeedsOrdinaryJoint；可靠ordinary body-joint可显式跳过fast body_limit，record／整帧／网络配额保持。PlayerPost不能用reliable绕过快源预算。实际调度回退由后续owner落实，执行器不重标流、不截断。

全部字段、来源close及最终窗口成功才FinishRoot。失败保留subject／layout／field／规范父路径及首个原因，AbortRoot回卷全部cell／path／byte游标，writer不能再Seal整包。后台继续使用既有CapturedBodySource及精确成员／资源快照解析句柄；执行器不发布、不发送、不ACK，也不证明预期成员集合或全部必要原版来源已经闭合。

### 4. 纯编译记录与完成情况

首轮Release／Win32产品纯编译在`capture_executor.cpp`遇到C2248：同步遍历辅助类未声明为CapturePlan私有分派的friend。已补正；第二轮产品纯编译／链接成功。后续静态复核补齐内嵌Copy结构、共享／容器来源一致性、逐字段窗口与阶段／流约束、body回退原因及容器最短长度预检；两次对应增量产品纯编译／链接均成功，新增执行器无编译警告。共四次必要纯编译，不包含任何行为测试。

构建入口为`cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /v:minimal`；配置`ISAACCS_BUILD_E2E_TOOLS=OFF`，产物`out/repentogon/zhlIsaacCS.dll`。整体构建保留官方SDK RapidJSON既有C5054警告，不将编译成功写成行为或性能通过。

本项不新增或运行测试、独立校验脚本、probe、游戏或性能试跑，不安装到游戏目录。第10步第三项公共计划执行器完成；各域真实来源与运行接线继续后续项，第15步仍是唯一统一整体验收。
