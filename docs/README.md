# C/S研究与状态同步文档

> **tx-04-05 完成（用户接受，D23，2026-10-24）**：用户决定将第四次 E2E 算作验收通过（V1、V3–V10 PASS；V2 的判定方向缺陷不修复，原报告保持 FAIL）。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 第四次 E2E（2026-10-24）：**FAIL**，V1、V3–V10 PASS；V2 的 S10 已按 D22 向左原生移动约 198 px，但判定仍要求向右（D22 修改遗漏），未重跑。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 第三次 E2E（2026-10-24）：**FAIL**，V1、V3–V10 PASS；只有 V2 的 S10 因 harness 固定向右移动、被随机房间的岩石挡住而失败，DLL 没有接管输入，未重跑。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 第二次 E2E（2026-10-24）：**FAIL**，S0–S11 全部执行，V1、V3–V9 PASS；V2 因 S1 落在开局约 3 秒的未模拟区间、V10 因 F9 触发原版手动截图而失败，均为 harness/夹具问题，未重跑。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 第一次 E2E（2026-10-24）：**FAIL**，S0 启动加载超过 5 s 时 harness 强制 focus 失败，未进入 run，未重跑；DLL 侧能力、28 个 Hook、8 个槽补丁与 Odd shim 安装正常。另发现 V10 的 D18 历史基线规则无法匹配。见[结果](design/tx-04-05-results.md)。
>
> tx-04-05 测试代码（2026-10-24）：剩余 Lua F8/F9/只读夹具、Rust 单启动 S0–S11 编排与 V1–V10 归约已编写，见[实现记录](design/tx-04-05-test-harness.md)。未部署或启动游戏，没有执行测试，唯一整体 E2E 尚未执行。下方各阶段记录保留其当时状态。

> tx-04-05 game 适配（2026-10-24）：第 3.2 节七个主线程适配模块、必要的常驻 Hook/Odd shim 接线、solo 命令与状态/trace 已编写并纯构建成功，见[实现记录](design/tx-04-05-game-adapter.md)。测试 mod/harness 场景尚待实现，没有部署或运行测试，唯一整体 E2E 尚未执行。

> tx-04-05 core 阶段记录（2026-10-24）：第 3.1 节的 Session、TickClock、TickPipeline、input_core、Participant 已编写，Win32 Release 纯构建成功。代码、Solo 适配和后续接线见[移植记录](design/tx-04-05-core-port.md)。该阶段尚未接入 game/命令；后续实现见上方最新记录。没有运行测试，唯一整体 E2E 尚未执行。下方静态报告与 tx-04-04 内容保留历史语境。

> tx-04-05 静态工作完成（2026-10-24）：T1–T8 已由 GPT-6 Luna / max 逆向并经主智能体独立取证审阅；修正了 Odd 分类、设备 id 映射、采样时点、屏蔽观察、持续 held 提交和字段宽度等方案。T6 不把 new-room 回调当作所有路径最终位置稳定的证明。见[静态报告](research/tx-04-05-static-confirmation.md)；功能尚未实现，没有构建、实机操作或测试。下列 tx-04-04 记录保留历史语境，其第八次整体 E2E 已 PASS。

> tx-04-04 最新（2026-10-24）：退菜单已改为观测暂停状态/退出选项后确认，第七次 E2E 已正确返回主菜单、Game::Exit 计数增长，两次启动正常退出。整体仍 FAIL：Lua run 回应、UI 渲染标记缺失，以及启用/关闭配置两次都有第一、三槽存档字节变化；详见[结果](design/tx-04-04-results.md)。

2026-10-22，tx-03-09；本工程从原研究仓库独立复制，原目录保留。

1. [现行项目规范](refactor/project-conventions.md)：唯一当前流程要求；用户会话指示优先。
2. [按功能模块查Hook与逆向点位](hooks/README.md)：全部当前安装家族、调用依赖、候选函数、字段及虚表，明确证据等级。
3. [状态同步方案](design/state-synchronization.md)：完整基线＋增量状态＋表现事件，首版单房间、无预测；列出实际缺失能力。
4. [迁移说明](migration/README.md)：复制范围、保留的历史产物位置、路径差异与使用限制。
5. [历史T0任务书](refactor/cs-t0/isaac-cs-t0-task-plan.md)、[旧总体规划](refactor/isaac-cs-architecture-plan.md)、[T0代码入口](../cs-t0/README.md)：保留原始时点与失败，不覆盖为新状态。
6. [tx-03-10：Replica更新与展示分派研究](research/tx-03-10-replica-update-presentation-boundary.md)：状态同步迁移的首项实际静态逆向，GPT-6 Luna / max子智能体研究、主智能体复核；没有实现或运行Replica。
7. [tx-03-11：实体绘制与动画事件的逻辑／展示分离](research/tx-03-11-logic-presentation-separation.md)：继续区分 Entity 绘制 pass、Sprite 动画与事件消费者，追到动画事件的弹体创建路径；含 GPT-6 Luna / max 结论及主审。
8. [tx-03-12：派生绘制与 Sprite 资源生命周期](research/tx-03-12-derived-render-sprite-lifecycle.md)：三类实体独立 Lua render 入口、额外图形与独立 Sprite 构造／资源引用／释放；Luna 研究及主审，尚未实现展示 adapter。
9. [tx-03-13：Sprite 图像资源、引用与图形后端](research/tx-03-13-sprite-image-resources.md)：ImageBase control 六槽、两层图像缓存、最后释放与 PNG 纹理上传／删除；Luna / max 结论已由主智能体独立取证复核，实际线程／context 仍未确认。
10. [tx-03-14：renderer 线程来源与 GL context 生命周期](research/tx-03-14-renderer-context-lifecycle.md)：连接正常主入口初始化、TLS current、帧内呈现和资源先于 context 的关闭顺序；补充绕过现有 RenderHook 的加载绘制入口。仍无运行时 thread／context 观察或 Replica 实现。
11. [tx-03-15：Sprite 换图、Reload 与 ANM 引用](research/tx-03-15-sprite-replacement-reload.md)：区分路径更新、逐层加载与 layer 重建；记录正常失败不回滚、限定条件的 Reload 额外引用及 Load 失败借用风险。GPT-6 Luna / max 子结论已由主智能体独立复核并修正 Lua wrapper 描述；下一项为最小展示字段与特殊标志支持边界。
12. [tx-03-16：独立 Sprite 最小展示字段](research/tx-03-16-independent-sprite-display-fields.md)：区分 main／overlay、layer ordinal／track／关键帧索引，解释 setter 与特殊绘制依赖，提出固定姿态的有限支持描述。Luna / max 结论已由主审独立取证复核；下一项为资源准备、姿态应用和失败退休的 owner 顺序，尚无 adapter 实现。
13. [tx-03-17：独立 Sprite owner 顺序](research/tx-03-17-independent-sprite-owner-lifecycle.md)：构造、未发布资源准备、固定姿态应用与正常退休；区分 copy／swap 的字段合同及 native／Lua 外层 owner。Luna / max 结论已由主审复核；新审阅器修正地址分类，保留 tx-03-16 原 FAIL。adapter／资源就绪 predicate 尚未实现。
14. [tx-03-18：最小单 Sprite 适配合同](research/tx-03-18-single-sprite-adapter-contract.md)：最终 image/control pair 就绪语义、64 项 layer 映射边界、reset 后逐 track 固定姿态与自有实例发布／退休接口。Luna / max 结论由主审独立取证复核；原生 loaded 不证明 GPU／像素成功，合同与资源清单尚未实现。下一项为实际绘制接入窗口及 thread/context guard。
15. [tx-03-19：独立 Sprite 绘制接入窗口](research/tx-03-19-sprite-render-entry-window.md)：定位最终 pre-present callsite 的屏幕叠加候选，解释 Hook／TLS／WGL guard、Sprite 参数 ABI、batch 独立强引用与状态版本。Luna / max 结论经主审独立取证并修正初始化状态及地址分类；未安装 Hook／实现 adapter，下一项为具体资源与连续姿态闭环。
16. [tx-03-20：具体资源与连续姿态闭环](research/tx-03-20-sprite-resource-pose-closure.md)：从现成 extracted_resources 收敛 Poof 单层 ANM／RGBA8 PNG 样本，明确 Authority 同窗口采集、同 descriptor 原地更新和变更候选替换。Luna / max 结论经主审复核；支持声明未实现、未加载或绘制，建议下一轮进入最小独立 Sprite adapter／本地 fixture 实现。
17. [tx-04-01／02：REPENTOGON 适配重构设计](design/repentogon-refactor.md)：把原型改为由 REPENTOGON 加载的 `zhlIsaacCS.dll`。Hook 常驻、会话逻辑启停；网络放入 DLL，Rust 只做测试与中继；旧 ISCS／Isaac Sync 兼容约束不延续；会话通过 REPENTOGON ImGui 界面发起。tx-04-02 定下目标为官方 REPENTOGON 与 v1.9.7.12.J273，并修订了现行规范。目前只有设计，没有代码。
18. [tx-04-03：REPENTOGON 目标版本点位核对](hooks/repentogon-binding.md)：在 v1.9.7.12.J273（从 `Repentogon\` 子目录的 exe 复制一份，用 Steamless 脱壳）上静态核对原型首轮点位，产出 43 项 [binding 清单](hooks/repentogon-binding.json)。37/39 个调用点有对应；输入设备子系统变化较大，自有虚拟设备要重做；4 处调用点补丁要按新寄存器分工重写；字段偏移已用 REPENTOGON `.zhl` 印证。只做了静态分析，脚本和中间数据见 `artifacts/repentogon/tx-04-03-static/`。
19. [tx-04-04：实现与结果](design/tx-04-04-results.md)：开发包与[项目代码](../mod/README.md)已实现并编译；27 个入口 Hook、8 个设备槽补丁、`cs` 命令桥、有界命令队列与测试控制口、纯界面 mod 和 Rust i686 harness。唯一 E2E 在注入启动阶段因 Launch 等待 60 秒超时失败，未进入菜单，未重跑。存档无变化，临时测试 mod 已移除、配置已关闭。tick、输入接管和网络不在本轮范围。之后做了静态审阅，并修订了[实现方案](design/tx-04-04-implementation-plan.md)和设计文档；修复沿用 tx-04-04（D16），已实现并编译；第二次 E2E 仍启动超时 FAIL，新增诊断捕获 Use The Launcher! 对话框。两类存档均无变化，未自动重跑；Lua 捕获偏移经 IDA 更正为 Game+0x680D8。 按 D18 修正后，第八次 E2E E1–E7 全部通过。
20. [tx-04-05：实现方案](design/tx-04-05-implementation-plan.md)：循环阶段与 tick 判定、本地玩家 A 经统一队列输入、生命周期和单人会话。U1–U4 已确认（D19）；T1–T8 的[静态确认与主审](research/tx-04-05-static-confirmation.md)完成，原始资料见[artifact](../artifacts/repentogon/tx-04-05-static/README.md)。第 3.1 节 [core 移植](design/tx-04-05-core-port.md)已编写并纯构建成功，[game 主线程适配](design/tx-04-05-game-adapter.md)及必要 Hook/solo 命令也已编写并纯构建成功；[Lua 夹具与 Rust 单启动 S0–S11 / V1–V10](design/tx-04-05-test-harness.md)也已编写并纯构建成功。共执行四次整体 E2E（D20–D22 修正 harness），第四次 V1、V3–V10 PASS；按 D23 由用户接受为验收通过，tx-04-05 完成，见[结果](design/tx-04-05-results.md)。

`refactor/`与`isaac-sync/`为迁入的历史资料；旧gate、哈希、独立角色与重复验收条款已由现行规范覆盖。新增`hooks/`与`design/`用于后续状态同步研究，不表示对应Replica生产功能已经实现。
