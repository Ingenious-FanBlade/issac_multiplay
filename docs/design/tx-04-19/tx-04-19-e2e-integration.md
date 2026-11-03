# tx-04-19 I31—I33：投影、构建边界与统一编排

日期：2026-11-03，Asia/Singapore。本文先登记实现前失败方式，再记录实现。运行验收属于 I34；本轮不构建、不启动游戏、不运行测试。

## 实现前失败方式

沿用总计划 F01—F15，新增以下具体编排失败方式：

| 情况 | 必须处理 |
| --- | --- |
| 旧代表投影删掉非 Player 字段／Exists=false 成员 | 19 保留整个正式 Entity 投影、原始成员数及墓碑；Player 实际应用范围单列，不改 coverage |
| 冻结 Source 之前／之后取错快照 | ready 携带 SourceSnapshotToken；出口返回同一 token；库存／图像只比较这个静止窗口 |
| 只数 Source 标签而没有 Client 原生后态 | 每个目标等待唯一 Scene 提交、精确范围及真实逐字段后态，再观察 draw；Source 与 Client 分阶段报告 |
| E2E DLL 被当成正式 DLL 打包 | 两种构建目录／输出隔离；包读取 product 构建说明；产品编译排除文件夹具和原生测试模块，拒绝 token 配置 |
| 故障注入提前遗忘已取得／已 Init owner | 只在真实调用正常返回的具名边界注入，走正式失败／退休；不模拟未知异常或覆盖原生地址 |
| 旧模型／悬空图／四 shader 失败被忽略 | 同一验收入口送正式 codec／parser；写前拒绝保持 A；图及动态层在同源投影中对照 |
| 18 回归仅引用历史 PASS | 统一入口实际顺序执行原 49 Source／49 Client 场景及 12 项检查，保留人工画面复核 |
| Text／Dummy 只用 Type 数字冒充真实类别 | 按真实 RTTI／writer 见证核对，无法取得来源则明确未完成，不能用 NPC 替代 |
| Slot 捐款或奖励修改真实进度 | 默认只生成／设置最终后态；持久动作不自动执行，具体缺少授权的动作保持具名未覆盖 |
| 失败续测丢失清理信息或正常结束永久留 artifact | 中断才保留必要进度；正常验收完成并实际审图后清理该次临时目录，不恢复存档制造通过 |

上述表不是已执行的测试结果。

## I31：实际消费者与发送规则

`replica_projection_policy.py`重新复核了`client_entity_model.cpp`公共字段／关系、`client_entity_class_model.cpp`的Rules／Resource／Collection和动态字段、`client_entity_links.inc`逐槽以及`client_entity_draw_native.cpp`的资源消费。逐项结论写入生成目录，不改变完整观测目录。

| 输入 | 实际处理 |
| --- | --- |
| Bomb.GetCostumeLayerSprite | 原I14已恢复；0..4全部真实成员、参数、Null及失败继续发送 |
| 四个Sprite／Layer shader查询 | 保留当前查询及失败，和已捕获shader路径互相核对 |
| 静态ANM动画表／逐selector帧查询 | 仍省略；主／辅助／owned Sprite全部保留自己的资源、GetAllLayers、完整pose_runtime，本地同版本ANM按selector读静态数据 |
| Laser samples／曲线 | 完整四容器正式application_geometry；公开两份sample别名仍保留并逐值核对，不靠SampleLaser重算 |
| NPC／typed ExtraData／关系 | 三份独立deque、五类typed owner、具名EntityPtr和Player借用已有正式来源；不恢复按索引Get RPC |
| Text／Font宽度 | writer字节、KColor20和同窗口draw输入；Font6在本地按已知字符串读度量，不发整套按参宽度枚举 |

`ENTITY_COMPACT_CONSUMERS`随Entity功能目录生成，未新增Sprite别名。19出口不筛活对象、不删墓碑、不裁任何Entity字段；Player保持源值，实际提交仍只声明普通Isaac支撑，完整Player归20。旧17代表出口保留供18回归使用。

出口保存同一批11域原始envelope、正式发送投影、目标及capture token，记录原始观测字节／投影字节；harness同时读取`cs status`目标字节／节点。该差值明确不是P1—P6实施前后净收益，本轮没有实测数字。

## I32：编译、输出与产品边界

全部已有Entity模块仍由`mod/CMakeLists.txt`纳入DLL和目录依赖。新增测试模块`client_entity_e2e.cpp`与原`client_fixture.cpp`／`client_e2e_native.cpp`／`random_e2e.cpp`只在`ISAACCS_BUILD_E2E_TOOLS=ON`编译；主动随机测试Lua注册、client_test／world_test命令及测试Client启动／所有权练习也受`ISAACCS_E2E`编译约束。正式配置遇到e2e_token明确拒绝。

产品和E2E分别使用`build/repentogon`、`build/repentogon-e2e`，DLL分别输出`out/repentogon/zhlIsaacCS.dll`与`out/repentogon/e2e/zhlIsaacCS.dll`，各有具名build-flavor说明。harness只部署后者；package只接收product说明和显式文件清单，新增Entity功能目录，不复制Rust、测试Lua或fixture程序。默认Off／无录制／无明细／无控制口保持。

Dummy的E2E主动获取登记在唯一Hook清单，启动后常驻；产品／未激活路径恒透传，不在会话启停时改字节。观察wrapper仍各调用一次super。其采用／源码／指令及主审纠正见[来源研究](entity-i31-i33-native.md)。没有新增游戏注入器、第二套Scene或网络ACK。

## I33：统一夹具与编排

交付入口为`scripts/run-tx-04-19.ps1`、`tools/harness/src/tx19.rs`、`tools/harness/scenarios/tx-04-19.json`及测试Lua`tx19.lua`。统一入口在I34内先纯构建两个flavor，然后顺序运行18 Source／冷Client、19 Source／冷Client。18的原场景与检查代码直接复用，没有删掉其49／49场景或12检查。

Source用真实Spawn、状态setter、Morph、Delirium Transform、Knife Shoot、Pickup cycle、Slot结果setter、Grid拾起／落地等产生实际目标；不在导出前持续清理待测对象。Familiar owner换代用真实run退休／新Player Key，避免同一Player重复赋值冒充变化。多Player同场景装备／身体仍由20负责。Text使用DAMAGE_VALUES下真实NPC伤害writer并仅恢复本次添加的debug位；Dummy明确是token限定的专用预构造池桥，未发现自然caller的历史事实仍保留。Slot不执行捐款、支付或持久统计动作。

| 矩阵 | 编排与判断 |
| --- | --- |
| E01—E16 | 每场景Source就绪token／原始库存、完整正式投影、真实Scene提交及精确非Player范围；原生实例库存／逐类字段容器资源只读后态；重复目标不得重建／重载；实际类别／Slot1..18／Delirium变形／dead-existing／lifted描述必须存在 |
| E17 | 真实Lifecycle／Query独立记录单独导出；重复、冲突、旧scope、错误domain及Partial覆盖；不得伪造呈现确认，ordinary A不受改变 |
| E18 | 真正renderer窗口、同一Source token对应画面对、尺寸／空画面检查与像素差观测；画面差不自动判等，逐类draw／分层由I34实际审图 |
| E19 | acquire／Init／fields／resources／publish／references／check／draw前后16个具名边界；只在真实正常返回前后注入；检查命中一次、停显／真实前缀、安全退休、完整T恢复 |
| E20—E21 | 统一输入和Host／Client原调用计数、原生／Lua持久入口拒绝、暂停／恢复、线程／借用拒绝、四阶段stop和展示期正常窗口关闭；文件变化按现有规则核对 |
| E22 | 旧／冲突revision、Partial、来源版本、未知分类、非法值／位集／引用、缺owner、悬空图、资源路径、池超限及Player超范围；实际Negotiate验证旧模型／目录拒绝，写前A保持 |
| E23—E24 | 18完整重跑；同源原始观测／发送投影、逐成员省略、四shader、pose_runtime和组件图；体积口径不冒称净收益 |
| E25 | 密集Entity／曲线／辅助资源、原生池上限负向与稳定重复T；采集／codec／parse-plan／native-step／draw累计和峰值CPU窗口、含IPC的墙钟、进程内存各自记录，不混为每类成本 |
| E26 | product清单／默认配置／flavor，录制关闭的正式采集与应用；关闭录制器自有文件句柄后观察实际FILE_ERROR，正式consumer继续发布并导出可由冷Client应用的恢复T |

独立结果实际消费属于27；不能为了腾队列伪确认“已播放”。编排明确记录128条上限背压／未接纳后缀，必要时通过正常场景退休／完整T冷重建清空会话队列；这不算音画消费通过，也不把Lifecycle短命对象复建成常驻。每份真实独立原始值仍可在该次必要临时材料中审阅。

harness将Source就绪、自动原生检查及人工画面／矩阵审阅分别报告。成功启动/Source样本/场景标签不等于E项全部通过；运行后仍须逐项审阅复杂Laser几何、辅助资源／特殊分层、短命记录、历史UNKNOWN与实际覆盖，不能因为夹具存在就声称测试成功。

## I34使用与本轮收束

本轮只实现I31—I33，完成人工静态审阅后按用户要求提交；没有执行构建、语法／格式／分项校验、游戏或E2E，没有写单元测试，没有创建tx artifact。仅按交付需要重新生成Entity功能目录。Luna max完成必要逆向，主智能体独立复读并纠正NPC池容量笔误。

进入I34时使用：`pwsh -NoProfile -File scripts/run-tx-04-19.ps1 -GameRoot <游戏目录>`。每次启动单独说明只跳过开场，到标题／选档页停止操作，获得准备确认后启动。失败保留必要诊断与阶段目录，修复后在同一tx用`-RunDirectory <同次目录> -Resume`继续；`-RerunStage <阶段名>`明确重跑已成功但受修复影响的阶段，重采Source自动重跑其Client，不比较哈希。失败阶段从完整Source或冷Client重跑；成功前缀只复用明确未受影响的部分。自动阶段结束后执行者继续实际审图及全矩阵归约，正常验收完成后清理该次临时目录，不建立永久artifact要求。

I34负责全部运行覆盖、修复及续测；I35记录真实结果和具名未覆盖。当前不将整个tx-04-19标记为完成，不设置完整Entity域位或发送Applied ACK，不外推Player20、实时下行21／22、音画消费27或整局产品已完成。
