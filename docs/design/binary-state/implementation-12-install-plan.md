# 第12步：完整数字安装计划

日期：2026-11-13。布局版本仍为74，没有修改线上字段。范围是用户本轮要求的“先补齐完整安装计划”；实际原生资源物化、对象写入和事件播放继续由后续原生owner实现。第0—14步没有运行测试，第15步唯一统一验收。

同日后续：已实现[主线程资源准备和根对象暂存](implementation-12-main-preparation.md)，随后补齐[StageFields私有字段暂存](implementation-12-stage-fields.md)。下文保留计划交付时的范围；完整字段安装、实际发布及事件消费仍未完成。

## 当前交付

`state/client_plan.*`从完整候选、配对有效根、真正成功的旧根及独立事件账本生成不可变`ClientStateInstallPlan`。计划持有原候选及精确快照，另有一次性连续正文副本、数字路径、字段、标量／嵌套实例、已解析绑定、实际资源闭包与执行顺序。它不含游戏地址、原生容器或报文提供的偏移。

`game/client/scene/client_install_plan.*`提供具体`ClientSceneInstallPlan`和`ClientSceneStatePreparation`：在同一Peer账户下生成Grid描述、Room／Level／Descriptor描述以及既有Q8／Q11／Q12显示片段，保留Entity／Player与第二批显示／事件的完整数字输入。body单流不重新构造Grid、地图或world显示描述。

接收器的顺序已经改为：完整公共检查与数字计划→具体Scene描述→真实owner资格Inspect→Begin重查→不可变批次交接。具体描述在本次交接内只构造一次；不能把仅有`DecodeStateFrame`或空原生描述当作完整计划。

`ClientSceneInstallReadiness`仍是必须由真实原生owner实现的接口，查询只消费纯值资格快照。没有该实现时`Available=false`。正式游戏Start尚未传入准备owner，也没有实例化实际原生安装器，因此本轮没有启用二进制能力或宣称完整运行链完成。

## 数字输入与覆盖

| 内容 | 计划中的表示与约束 |
|---|---|
| 全体成员 | Participant、ControlGroup、Run、Level、Descriptor、Room、Grid、Player、Entity、SFX各域；Reuse／Join／Reshape／Remap及独立退休集合；只将本次更新流标为写入，配对已安装流只供依赖 |
| 完整字段 | 布局／字段的可信数字索引、真实递归路径、容器元素／Weapon槽／choice布局、已解码数量与mask；显式区分条件缺席、共享Null、合法空集合和真实非空内容 |
| 稳定寿命 | 字段／节点只保存计划正文的偏移与长度；没有逃逸的解码scratch span；块保留最后变化版本、Host帧与来源修订 |
| Entity／Player工厂 | 实际type／variant／subtype、所选配置、init_seed与authority顺序；不按C/S ordinal猜Player类型，不回退不支持的RTTI布局 |
| 引用 | 逐引用保留目标完整身份与计划对象索引；八门等固定数组额外保留分量号；全部承载先暂存，关系后绑定，允许已核定的环与别名 |
| 资源 | 本次实际引用闭包及依赖顺序、原始资源快照；SFX／loop选择准确sample，拒绝Null或music类资源冒充sample；本地音乐／jingle／Boss演出需求另列数字请求，不冒充已物化 |
| Room／Level | 唯一根、当前描述槽与维度、507项占用表、普通描述稠密顺序、特殊负入口、当前SpawnSeed归属、Grid几何与索引、八门目标、wave条件；描述身份的物理槽来自Host明确的`slot+1`规则 |
| Grid | 工厂type与真实布局共用生成表，不手写第二份switch；检查索引唯一、Fire余烬状态与寿命、Rock rubble状态／动画；复用完整Grid描述生成器 |
| Player关系 | 全局非零P6拓扑、配对P0相等、最多四组／每组64身体／全局256身体、participant／ordinal唯一、同组active anchors及其原顺序、roster唯一稠密、成员资格蕴含及无组中性规则；旧成功根用于检查稳定组分配及binding不倒退 |
| Player外观 | 递归投影／计数组由现有codec和shadow完整校验；costume map非Null索引必须落在本目标有序目录内；不强行拒绝明确Null资源的原生fallback语义 |
| 持续表现 | 注入、音乐、loop、Overlay、程序配置均有完整字段描述；检查音乐／jingle代次、loop instance唯一、Overlay owner唯一及状态／代次、程序定义与配置ID一致 |
| 瞬时事件 | SFX、banner、fortune、Boss、RoomTransition各有独立完整record和精确两表；检查Player用途、banner样式／配置、fortune索引、转场维度／完成标记及请求一致性；转场按Host序号关联，开始／完成可合法单独出现；SFX实际回放FrameDelay明确为0 |

完整字段描述来自同一次正文遍历的`BodyFieldNodeView`。既有`BodyNodeObserver::Node`继续提供标量与嵌套实例；新增字段边界回调记录空容器和条件缺席，避免只有叶节点时漏掉清旧内容。所有回调仍只构建后台暂存，不允许原生副作用。

## 执行顺序与原生责任

计划按以下顺序列出命令：

1. 精确资源闭包及本地具名显示资源准备。
2. 旧房surface捕获位置。
3. 按域暂存全部对象，再由外向内暂存嵌套容器／共享对象／Weapon槽。
4. 按codec后序安装值；容器字段只完成本字段及已准备子元素的提交，不重解报文或递归重复安装子字段。
5. 清空／定长关系字段（包括空anchors），再逐项绑定已解析引用；u16成员号不得当作原生地址。
6. 显示字段与新房surface捕获位置。
7. 按真实事件序号回放。
8. 统一发布；旧承载按实际借用排空后退休。

这些命令只描述顺序。旧／新surface的实际捕获、setter／具名ABI、容器构造交换、本地ANM／图层实际形状、音乐／Boss本地资源、声音实际消费与失败排空，仍由下一项真实原生安装owner完成。原生owner必须在Prepare／Stage阶段完成全部可失败准备；任何可见写入后不允许跨帧等待半个目标，也不能从该命令清单推导成功回执。

## 容量与回收

2026-11-14 costume单流修复：`_costumeSpriteDescs`的P6目录与P0 Sprite共用一个原生vector。计划现在为本次Player从配对成功影子补齐另一流的costume投影；只解码相关根块，只登记costume路径，不重装配对流的其它字段。配对记录保留自己的context、成员表、资源表和块版本，其非空资源引用及依赖闭包也进入本批资源需求。未收到的流不会因此推进版本、事件水位或ACK。合法空目录同样生成完整的空容器安装操作；计数冲突继续由canonical暂存形状检查拒绝。

- 所有持久数组、正文、描述及共享控制块使用本Peer的`ClientStateMemory`；数字路径／字段索引和codec事实使用Workspace。保留既有262144次访问／32层边界及Peer／共享池额度，超额回滚未发布计划。
- 2026-11-13审阅修复：此前计划已取得Workspace后，完整目标校验及资源闭包再次取得同一非重入租约，正常非空闭包也会被误报为Budget。现在顶层持有一次租约，显式传给校验、需求收集与`ClientStateTables::Prepare`；独立调用仍自行取得租约，子调用核对原持有线程。没有放开全局互斥或改成隐式重入。
- 既有Grid／Room／Display描述新增可选Peer分配器。二进制批次用实际分配计费；原独立调用继续用原Values租约。两种方式互斥，真实副本各计一次，不重复估算同一分配。
- 接收器在Begin之前预留批次退休索引，完成回执不再临时分配。主线程及Render借用未放下时继续保留整个批次；后台检查`Releasable`及所有子描述引用后才执行最终析构。Close／Drained同样包含这些借用。
- 计划完成不推进world／body成功版本、事件水位或ACK。此前的实际Completed／CompleteDrain与不确定事件处理保持。

## 尚未完成的运行工作

后续需要实现并绑定真实`ClientSceneInstallReadiness`、资源物化和`ClientStateNativeOwner`，让Manager消费批次并执行本计划，完成第二批状态恢复与事件原生播放器、Client episode计费，再切换正式入口。Boss与准确sample等此前的来源／后端未决没有被本轮计划描述改判为已成功。

## 纯编译

本轮仅进行源码阅读、实现和Release／Win32纯编译／链接。初次状态库编译发现一处同声明auto类型不一致，已修正；最终产品与E2E配置DLL均编译／链接成功。没有运行游戏、E2E、单元测试或校验脚本，运行正确性留到第15步统一验收。
