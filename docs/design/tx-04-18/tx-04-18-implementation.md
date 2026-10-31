# tx-04-18 Room／Grid 客户端应用

2026-10-31。Room/Grid 专用表示及统一 E2E 已完成，最终 **10/10 PASS**。规范沿用项目现行约定，全部实现之后统一 E2E，连续修复续测，复用不受修改影响的历史通过结果。

## 完整目标和实现前失败模型

交付房间实际构成、全部房形的显式几何、背景／环境、八门槽、全部 27 个非空 Grid 类型和 17 个派生接口的专用客户端展示表示。保持 Listen Server 唯一裁决，Client 不构造随机本地房间、不执行 Grid Init／Update／Destroy／开门／奖励。网络世界 Baseline 和整体 Applied ACK 仍归 21；本 tx 的提交只表示 Room／Grid 域完整发布。

- 448 格实际存储与八门槽不能按 width×height 缩小。几何、碰撞、路径、轨道、遮挡和门目标都是 Authority 显式值，形状枚举不触发布局生成。
- Grid 身份由 run／space／id 构成，同格替换必须更换身份；完整成员集才能删除旧节点。Unknown 类、截断、重复槽、引用缺失、资源准备失败必须拒绝整个候选，旧版本继续展示。
- 保存描述与当前字段分离；全部公开只读成员按 11 的目录保留。计时器、RNG 描述、机关／破坏结果仅保存与展示，不调用对应玩法方法。纯值查询不借用原生对象。
- 主／额外／TimerPlate／岩石 top／水裁切等视觉部件各有明确 owner、pass、资源和姿态；房间地面、墙、环境和前景使用 Authority 明确选择的部件，不能只按 seed／backdrop ID 在 Client 重抽。
- Room／Grid 元数据与所有资源同一个候选准备、发布和退休；失败不暴露一半成员或几何。主线程、OS context、图形队列和停止生命周期沿用 17。
- 输入、正式应用与录制无关；E2E 入口仅显式 token 开放。测试夹具不进入正式模组发布。

必要静态研究由一个 GPT-6 Luna/max 子智能体承担，主智能体回读后记录接受范围与 UNKNOWN。实现与验收状态将在实际完成后更新，不把目录覆盖或准备成功当作可见像素或全内容通过。

## 接口和源状态归属

`replica/room_grid.*` 定义 `isaac.cs.room-grid` v1 及 `ComposeRoomGrid`。`isaac.cs.room-grid-source` 把15规范模型与Authority最终视觉构成送入同一路径：Room/Grid component必须同scope、同revision、Complete、完整membership；状态、主体、保存描述、门别名以及448格坐标／碰撞／path／rail从源模型复制。显示补充提供实际inside／遮挡、场景资源部件、pass／depth与局部裁边，不在Client重演生成。正式主线程可直接提交同一纯值描述，不依赖录制、控制口或夹具。

固定448格和八门槽，全部1—27类型按已核定Factory对应17类；公开只读成员按11的Grid目录检查完整性，保留全部当前值及显式NotApplicable／AuthorityOnly。未知类、缺字段、缺源、重复槽、缺当前Grid引用和资源失败拒绝整个候选。非活动房间、跨房事务及其可靠传输仍由23—24负责；Entity引用保持规范值，实际对象应用由19—20负责，不制造原生借用。

每个视觉节点有唯一Room或Grid owner。floor／shadow／wall／water／main-extra-timer／top／light／foreground-outline分别使用0—7 pass，pass内明确order。地面逐格声明覆盖，不能只有L房标签而画矩形。门额外层、岩石top、TimerPlate和倒计时文字是独立部件。水边距为经过Authority换算的Sprite局部裁边；复杂遮罩由明确分片／资源部件表达，不把原生water flags或world y直接当作clamp。

Room/Grid状态与资源同属17的Scene candidate，准备全部成功才发布；同scene变更run／floor／dimension／room拒绝，旧节点在draw停止、实际CPU队列排空与glFinish后退休。公开只读查询返回owned状态；细胞数据也是自有值，不提供写世界接口。域完成没有设置whole-world Applied，网络Baseline闭环仍归21。

## 逆向主审

子智能体原始[静态笔记](tx-04-18-reverse-notes.md)完整保留。主智能体独立回读官方Backdrop／Door ZHL、LuaBackdrop、GridEntityPlus、Anm2Extras及InterpolV2，并从J273反编译回读Rock RenderTop VA7149E0、PressurePlate Render VA95E140和GetSourceQuad VA404F60。接受不复用原生Room/Grid Render的结论。

主审确认PressurePlate会读Game计时并改Sprite颜色；改用Authority给定文字／颜色。404F60的GLITCH分支实际推进全局XorShift两次；root和每层都要求显式resolved_random_uv与crop结果后清除native随机位，否则拒绝。STATIC/GOLDEN要求Authority选定等价custom shader及colorize相位，清除官方本地时钟分支标志；不改本地Game/Manager时间。关闭本方AnimationState播放，阻止RGON插值按本地奇偶帧改共享AnimationFrame；本方draw的light/reflection/glitch全局模式由有界作用域保存恢复，外部color override未清空时拒绝。

SDK `_gridEntities[0x1BF]` 是447元素的旧声明，但`_doors`位于0x724，相对Grid起点0x24正好留448个指针槽；本次不新增原生读取或修改SDK，不缩小已经核定的0..447观测边界。原声明差异按原笔记保留，不作为新内存布局结论。

原生Room完整渲染树、WaterClipInfo私有float、生成背景surface内部缓存和任意第三方内容仍UNKNOWN；Client不调用这些路径，也不以Backdrop ID/seed补造。专用表示接收Authority已经解析的实际资源／分片／裁边。此tx验收类别合同及专用应用，不把受控资源图等同29全部原版素材／变体组合验收。

## 统一 E2E 入口

`scripts/accept-tx-04-18.ps1` 统一构建DLL／Rust并启动真实游戏Client。复用17的纯回调见证与位图素材，不创建本地run、不进入原版Online。覆盖全部类型／派生完整状态与保存描述读回、448格逐值读回、八门槽与全部12房形、门／坑桥／破坏／TimerPlate／top／引用变化、同格替换、完整删除、资源与不完整候选拒绝、GLITCH拒绝与已解析UV、环境／camera／水边距的真实像素、主线程隔离、stop／重连／带资源正常退出与访客持久保护。

受控Authority模型是本tx的域应用输入；它不伪装网络Authority或实际的全内容房间捕获。09—17已通过且未受影响的输入／观测／codec／持久入口完整矩阵不重跑。生产默认关闭逐帧诊断；仅显式E2E在自有draw窗口读回J273 GLITCH所用全局seed，检查本方准备／绘制没有推进该流。通过后清理临时材料；中断才保留必要恢复信息。

像素入口只在显式E2E token下开放：已核定owner／OS context、实际apply/reset、两个CPU图形队列排空后，读取当前真实native color target，恢复GL pack状态。BMP仅写本次临时目录，不激活游戏窗口。它证明真实GPU目标中的像素，不把被遮挡桌面、draw计数或资源Load成功当作像素通过。

## 连续验收的原始失败与修复

1. 统一入口首次构建失败：新增GLITCH诊断错误引用了另一个编译单元私有的ReadAt；添加context头仍不可见。改用本文件context核验所用的ReadProcessMemory保护读取后，完整DLL／Rust构建通过。没有运行旧DLL代替新结果。
2. 第一次实机（e2e-01）完整候选发布，480个资源／文字节点、27种Grid与17个派生类、保存描述全部逐主体读回成功，draw窗口19次RNG核验未变。随后第0格比较失败：源坐标经适配成为40.0，夹具期望整数40，serde_json按不同number表示判为不等。修复仅放宽几何数值等价比较，身份及64位整数仍严格比较。
3. 第二次（e2e-02）复用第一次27类读回，完成448格逐值读回；停在旧窗口截图函数的前台焦点要求，未进入像素／后续场景。没有把此失败改记为像素通过。已改为上述实际GPU目标读回，不开启自动抢焦点。前两次游戏正常退出、部署／配置／Helper恢复成功，无新增崩溃，必要续测材料暂存至本tx结束。
4. 第三次（e2e-03）沿续验链复用不受修改影响的27类状态与448格几何读回，只重建运行上下文，继续像素和其余用例，最终功能及汇总 **10/10 PASS**。Scene正常退休，停止／重连和带资源窗口关闭成功；Client窗口PGD／Continue／SaveSyncStatus字节不变，整窗存档检查沿既有D20字段规则通过，没有计算或校验摘要。cleanup errors均为空，无新增原生崩溃。成功运行临时目录由harness自动删除，前两次必要诊断及构建日志在收尾清理，不保留本tx artifact。

## 最终覆盖及边界

| 项目 | 最终结果与实际范围 |
| --- | --- |
| 全类型／派生／保存描述／资源 | PASS；1—27和17派生的完整目录状态逐主体读回，15规范模型Room/Grid接入，真实GPU图中出现地面、墙、Grid、extra、TimerPlate、top与计时文字；0为NULL |
| 坑桥／破坏／门／引用后态 | PASS；HasLadder、碰撞、State=1／1000／4、开门和-100维度路由、当前Grid引用值均读回；不调用对应玩法方法 |
| 同格替换／资源退休 | PASS；相同index32的id2→2002，旧主体查询null，新主体及引用正确，旧资源实际排空后退休 |
| 全部房形与几何 | PASS；448格坐标／碰撞／path／rail／遮挡／inside逐值读回；12形状与八门槽metadata一致，1×1／2×2／L缺角实际GPU像素检查通过 |
| 原子拒绝 | PASS；未知class、未知type、Partial成员、少格、缺公开字段、当前引用缺失、缺ANM、重复门槽、未解析GLITCH及本地时间效果共10种，旧域及资源revision保持 |
| 环境与裁边 | PASS；Authority水部件、floor色、camera及Sprite局部裁边使实际GPU像素变化；不外推所有旋转遮罩／私有waterline组合 |
| 已解析随机UV | PASS；root和两层flags含GLITCH、显式crop与resolved_random_uv的候选成功发布，draw窗口累计检查>20且该全局流变化为0；不声称检查了全部玩法随机流 |
| 玩法／Lua／持久隔离 | PASS；Start／普通／odd／Room／Entity／Player／Join等原生计数及通用Lua计数不增长，访客PGD／Continue／同步状态未变；whole-world Applied始终为false |
| 完整删除 | PASS；完整空成员集将27个Grid及其门别名／视觉部件全部退休，旧主体查询null |
| stop／重连／正常退出 | PASS；Room/Grid与资源排空后才Off，再连接带资源正常退出0，部署／配置／Helper恢复及存档范围检查通过 |

没有新增或运行单元测试；实现完成之前未运行测试。上述类别应用使用明确受控的Authority值和图形素材，不能外推所有原版Backdrop缓存表面、所有房间资源、所有Grid变体／特殊shader组合已完成内容差异验收。生成surface与复杂遮罩需要Authority给出完整资源分片，缺失时拒绝；29仍负责原版全内容组合闭合。18没有实现或冒充19—20对象应用、21网络Baseline／整世界Applied、23切房事务或24离房世界。
