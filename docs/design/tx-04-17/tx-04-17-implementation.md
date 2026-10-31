# tx-04-17 实现与统一验收

> **与18的后续调整（2026-11-01已验收）**：本文保留17当时自有Scene及10/10的实际范围。[18原生Room／Grid状态应用](../tx-04-18/tx-04-18-native-state-plan.md#13-最终统一验收结果)已在同一18调整原生展示分派、必要初始化／维护、颜色后处理和正常退休，统一E2E **29/29 PASS**。玩法、Lua、线程、生命周期及访客持久隔离保持；下文旧接线不再禁止合法原生载体，修改后路径以18结果为准。

2026-10-31。目标沿用专项工作流程的完整17范围；18—20的具体域应用和21的网络Baseline闭环不提前记为完成。实现阶段不运行测试；全部实现后使用同一入口连续验收，跳过不受影响的既有通过项。

## 实现前的失败模型

- Client准备前就封闭原生Manager玩法链；连接失败、资源失败、断开和stop不能提前恢复本地模拟。原生Start、Restore、Game::Exit、普通／odd／FSM、Player／Entity／HUD玩法入口不能成为Client场景初始化工具。
- 场景根、Room几何、Camera变换、HUD和有序展示节点由Client独立拥有，只接受Authority纯值。不得创建同seed本地run，不发布半套候选，不把菜单单Sprite称作完整场景。
- scene epoch、resource revision、instance generation与连接寿命隔离；相同路径不延续旧借用。描述重复幂等，冲突和过时代次拒绝；旧节点停止新draw后才进入有界退休队列。
- 每次资源加载、姿态、draw和最终释放均要求主线程、已理解的owner及实际OS current HDC/HGLRC；TLS非空不能代替OS检查。context改变、重入、未知owner或队列未退尽时保持隔离并停止相关操作。
- main／overlay／逐层时间帧先核动画和轨道边界；非有限值、非法layer ID、缺失PNG／ANM／shader不能发布。Replace返回false与图像加载失败分别报告；资源关联成功不声称GPU／像素成功或World Applied。
- 不保留Lua借用Layer／Animation／Image跨revision；使用自有native资源时由明确owner可控释放，不依赖Lua GC时机。保持原生observer original exactly-once，主动隔离入口明确记录未执行original。
- 访客PGD、Continue、成就／挑战／完成标记和Mod持久档在整个Client窗口不得因服务器世界改变；Shutdown直接写入与Manager析构版本同步分别隔离。只读位、最终dirty=0、文件写拦截或事后回滚均不能替代入口隔离。
- 输入、网络及正式消费不依赖调试录制。Host／Off按原生链透传；本地允许设置按来源与字段归属处理。线程退出、draw在途与资源退休完成后才允许Off。

## 当前进度

运行实现与统一E2E已完成，最终10/10 PASS（第四次启动仅续验Mod持久与退出项，其余8项复用第三次不受影响的通过结果）。进入统一验收前未运行测试。必要剩余逆向由一个GPT-6 Luna/max子智能体完成，主智能体独立回读HookSystem、官方保存同步与关键J273指令，接受范围及纠正见[逆向补充](tx-04-17-reverse-supplement.md)。

## 接线与所有权

`game/presentation.*`的`PresentationBegin/Submit/Stop`是公共主线程值接口。Scene独立拥有Room尺寸、Camera origin/zoom、HUD文本和八个明确排序pass中的复合Sprite节点；不构造原生Game/Room/Camera，不调用Start/Restore。具体房间／实体／Player全量外观适配仍分别由18—20提供，网络Baseline与Applied由21提供。这里不把受控场景当作完整联机世界。

描述显式携带connection、scene epoch、resource revision与节点身份；整数main/overlay及逐层时间帧、变换／颜色（含colorize）、crop、flags、blend、root／逐层custom/champion shader均只作用于私有候选。root保留可选分数phase，层时间可带分数或指定有界selected key／隐藏-1；不承诺key内额外插值。图像关联检查与实际像素检查分开。每个场景最多512节点、64层／Sprite、64MiB保守纹理计费；描述256KiB、32层／16384 JSON节点；旧Scene最多两个，成功发布后在原生apply/reset、具体CPU队列为空及OS current检查后执行glFinish，再析构自有ANM2。tracked image索引不被误作额外strong owner。没有持久Lua Image、Layer或Animation借用，也不以GC时机释放图形资源。

`game/client_isolation.*`与唯一清单生成wrapper封闭Client普通／odd／FSM、Start/Restore、生命周期、Player消费者、PGD及GameState修改入口。Manager Render使用-30000外层与+30000原生trampoline，绕过官方PRE_RENDER，原生prelude后由常驻splice分派Scene，再恢复图形tail；Host/Off保留正常完整hook链。四个Mod持久CFunction正文入口也从唯一清单安装；在RunBundledScript入口、官方隐藏_CBindings之前捕获ESSM函数，覆盖Lua局部闭包和别名。三个新常驻patch全部来自`binding/hooks.json`：render分派、Shutdown直接计数块、Shutdown flush/Exit块。Manager析构在保护窗口绕过官方自动PGD版本同步，但保留原生析构。关闭桥从录制配置中解耦，生产默认不录制时也安装。

Client从准备到资源退休保持隔离；失败候选保留旧画面，网络失败保持Client；ESC只控制本方停止UI。Off要求网络退出、两轮真实中性、资源排空和PGD内存值未改变。PGD比较只用于发现非法变更，不回滚dirty或写回对象。窗口关闭将持久保护延续到Shutdown/Manager析构，保留来源明确的本地OptionsConfig保存。

## 验收入口与范围

`scripts/accept-tx-04-17.ps1`统一构建／E2E。真实游戏Client使用受控多pass场景资源，核对像素与native姿态、动态换图、候选部分失败、过时connection/revision、错误owner、普通/odd/FSM入口未进入、通用Lua回调冻结、失败后持续隔离、stop退休、Host/Off恢复和带资源的正常窗口关闭。Rust只编排与读取结果；UDP黑洞仅触发真实连接失败，不冒充Authority、不声称实现网络世界下行。不重复09—16不受影响的观测／输入／codec矩阵。

测试夹具`tests/e2e-mods/isaac-cs-presentation`和Rust harness不纳入正式模组发布。测试专用描述文件只从显式配置的本次临时目录读取，生产空token关闭入口；正常通过后清理临时材料，失败与必要续作如实记录。

## 连续验收中的实际失败与修复

1. 默认沙箱内MSBuild停在检查阶段，终止该构建后使用本机MSVC环境完成完整DLL/Rust构建；未启动旧DLL冒充新结果。
2. 第一次真实启动（e2e-01）Client入口以`adaptation_unavailable`拒绝，场景用例0项。Shutdown `A1 [Manager]`中的绝对地址被ASLR重定位，固定原始字节模式不成立；改为核对A1指令形状，并将实际操作数与`Base+offset::Manager`比较。游戏正常退出，部署恢复。
3. 第二次真实启动（e2e-02）多pass场景、可见像素、main/overlay/逐层姿态通过；旧connection/revision、缺ANM/PNG/动画和越界时间帧六个候选均被拒绝且旧画面未替换。随后layer ID63／实际两层用例崩溃。必要续逆向由同一个Luna/max完成，主审回读官方Lua wrapper：native `GetLayer(int)`越界返回静态sentinel而非nullptr，随后`GetLayerID()`解引用无效LayerData。修复为在native调用前执行明确层范围／身份解析；原崩溃和未完成项保留，不改判为通过。
4. 人工检查第二次截图发现冻结前菜单像素仍残留在Scene未覆盖区域；Client现在在已核定prelude绑定的color target清背景，并恢复GL clear color／write mask／scissor状态。新增独立背景像素检查，不以几块Sprite覆盖菜单作为完整场景背景隔离。

续验复用第二次已通过且不受边界修复影响的资源／姿态／像素项及上述六种候选拒绝，只重新建立后续场景所需运行上下文；越界层、背景隔离、其余资源失败、退休和退出继续验收。新层模型／资源路径若发生影响既有通过项的修改，则相关项仍须重新验证。

5. 第三次启动（e2e-03）自动功能用例10/10通过，但最终汇总仍FAIL：tx17未加入当前`save_fields_tx10`分支，落入旧基线判定。主审逐字节确认第一、三槽两文件差异仅在末尾8字节Counter/Checksum字段，没有计算或校验摘要；Client窗口前至正常退出的进度文件对比严格通过。已让tx17使用既有D20字段规则，不修改其范围。
6. 随后源码主审发现ESSM内部全局表已被LuaInit隐藏，而官方模块保留了Save/Delete的局部闭包。第三次的absent仅表示晚期查找缺失，不能证明API不存在，该次Mod项只实际覆盖Vanilla与其别名。由同一Luna/max核定RunBundledScript的J273 RVA454FD0、thiscall/ret4；主审回读指令及LuaInit/模块源码后，把常驻捕获前移到脚本入口，四个保存入口都成为本目标必需能力。补上实际隐藏CFunction及退出卸载回调的尝试拦截。
7. 第四次启动（e2e-04）仅续验上述Mod与退出修改影响，8项已通过且不受影响的结果复用；最终功能10/10及汇总全部PASS。游戏正常退出，配置／DLL／UI／夹具／本方data目录恢复成功，cleanup errors全空，没有新增崩溃。第二次崩溃新建的DMP/TXT在诊断完成后按确切文件名清理，历史转储不动；测试临时目录／日志随收尾清理。

## 最终验收结果

| 项目 | 最终结果与实际范围 |
| --- | --- |
| 自有Scene／Room尺寸／Camera／HUD与像素 | PASS；三个节点、多个pass、真正native图形提交与可见像素，main3／overlay1及独立层key实读；层选择修改后在03重验 |
| 完整候选与非法描述 | PASS；重复描述幂等；八种拒绝覆盖旧connection、旧revision、缺ANM／PNG／动画、越界时间帧、越界层和坏shader；保持旧发布版本，无半套发布 |
| 主线程／context／owner | PASS；实际OS current检查通过，错误Manager owner调用未执行绘制；并非宣称所有驱动／线程异常都已穷尽 |
| 动态换图与在途退休 | PASS；实际橙色像素取代旧资源；apply/reset返回、普通／透明CPU队列为空及glFinish之后退休旧Sprite |
| 独立背景 | PASS；空白区域不再残留冻结前菜单，clear color／write mask／scissor恢复 |
| 分数phase／隐藏层 | PASS；main3.375、overlay1.25、独立层2.5对应key1、隐藏overlay key=-1；只写本方私有状态 |
| 玩法／通用Lua隔离 | PASS；普通／odd／Room／Player／Start／Restore／Join等入口未增长，通用render／update回调冻结；网络失败仍保持Client与展示 |
| stop／Host／Off | PASS；资源排空、真实中性与网络结束后恢复Off；原生回调恢复、菜单Host与正常停止保持透传 |
| Mod保存与别名 | PASS；Vanilla Save/Remove、预先保存的别名、初始化后隐藏的ESSM Save/Delete正文都被拦截；退出callback再次尝试也未执行original |
| 访客持久与正常退出 | PASS；Shutdown两块和Manager同步各有一次阻断见证，保留正常析构和本地Options保存；Client窗口所有PGD／Continue／同步状态文件字节不变；最终整窗仅按既有D20接受启动Counter/Checksum尾字段变化 |

完整构建通过，未新增或运行单元测试。未重跑09—16不受影响的已通过观测／codec／输入矩阵，也未恢复已永久移除的Online实机测试。必要逆向及主审见补充记录；具体Room/Grid、实体／Player全内容应用、网络世界Baseline／Applied仍按18—21推进，17完成不表示已形成整局世界同步。任意未适配第三方gameplay Mod、未知私有持久协议和驱动／异常路径的无限闭包没有被升级为支持声明。
