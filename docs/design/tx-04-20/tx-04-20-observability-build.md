# tx-04-20 I36—I37：状态观测、真实后态及工程发布边界

日期：2026-11-04，Asia/Singapore。同一 tx-04-20 完成 I36、I37 的实现接线。按用户要求直接提交，**没有构建、运行测试或启动游戏**；I38 编排、I39 全目标统一验收和 I40 最终交接仍待执行。本页是功能合同，不是运行通过记录，不保留推进 artifact。

## I36：纯值状态和首因

- `cs status` 沿主线程帧尾发布，增加 `player_status_version=1`、实际接纳的 `target_revision`、有效 `applied_revision`、角色类型／身体用途位集。参与者、控制组、roster／非 roster／备份及容器／资源／节点／字节预算继续发布；整场重建期间预算取已预检的新目标。
- `player_step_*` 表达最后执行身体的稳定 Key、instance、shape、角色、阶段及实际身体前缀；`player_failure_*` 保留第一次 Player 失败和缺依赖 Key／字段。解析及计划拒绝携带已知主体；未知 Key 保持零值，不猜地址或 roster[0]。
- `retirement_cause` 和 `retirement_failure` 分别保留退休原因和清理失败；`recovery_state` 区分无需恢复、等待安全退休、重建、已恢复、重试失败、禁止恢复和用户停止。恢复成功不覆盖首因。
- 状态是有界纯值／字符串，不把 Player 地址交给 Rust，不依赖录制、逐 tick／query 明细或诊断输出成功驱动同步／权威输入。

## I36：真实 inspect 和故障边界

现有 `cs client_test <token> inspect` 从同一已提交 Scene 检查非 Player 和全部 Player 原生实例。Player 检查实际 roster、Room／render／update 集合和四个镜像备份槽，回读最终量、库存、主动／口袋槽、效果、变身、顺序 History、costume、五槽 Weapon、专用状态、ANM／Image／记录序列、跨域关系及动态 HUD。输出行读取实际位置、存在性、类型、索引、容器计数及槽存在性，Key／instance／shape 仅关联本地 owner，不打印目标缓存冒充后态。

检查复用已审阅安装器的 `check=true` 分支。HUD／装备检查不再写 owner 关联或故障标记；检查分配或读回失败只返回本次错误，不撤销同步提交，不触发恢复。关系 inspector 使用明确只读 resolver，不能借此开放写入重入。

以下命令仅在 `ISAACCS_E2E` DLL 编译，安排故障需要主线程 Manager 和匹配 E2E token：

| 命令 | 行为 |
| --- | --- |
| `cs client_test <token> player-fault <point>` | 安排一次具名故障，拒绝叠加 |
| `cs client_test <token> player-fault clear` | 清除安排及命中记录 |
| `cs client_test <token> player-fault-status` | 返回命中 Key、instance、前缀、slot 和子 owner generation |

`point` 为 `player-allocate`、`player-ctor`、`player-init`、`player-fields`、`player-resources`、`player-hud`、`player-roster`、`player-publish`、`player-weapon`、`player-relations`、`player-check`、`player-draw` 加 `-before` 或 `-after`。after 只在正常返回成功后命中；draw 异常展开不命中 after，武器 after 需要实际非空槽安装路径。

`prefix_kind=body-mask` 表示身体实际累计位；全局 roster 操作记录 owner 累计位，Key／instance 为零。`resource-stage-entered` 表示武器所在资源阶段和实际 slot／generation；`draw-pass` 表示实际 draw pass。后二者不伪造完整身体 mask 或未知 instance。命中立即撤销安排，退休和完整 T 恢复仍走正式 Scene，不模拟未知 ctor／Init／Free SEH。

E22—E25、E28 失败模型沿主清单；本次只补接口，不运行故障场景，不补写单元测试。

## I37：构建和发布

- C++20 纯值模型和 C++17 SDK 单元保持分隔；新增故障实现只进 E2E target，inspect 受 `ISAACCS_E2E` 编译隔离。产品保留纯值摘要和必要生命周期／线程检查。
- 复用统一永久 Hook registry、正式同窗口 observation 和全部 Player native 模块；本次没有新 Hook 或 ABI，没有新增录制导入或测试来源依赖。
- 产品输出为 `out/repentogon`，E2E DLL、辅助程序、目录、观察 Lua 和默认 Rust harness 输出为 `out/repentogon/e2e`；构建树分别为 `build/repentogon`／`build/repentogon-e2e`。现有脚本／harness 同步迁移工具和目录路径，显式 HarnessOutput 仍可选择测试编排器位置。
- build 从对应构建树复制 `player-application.json`，与 DLL 内嵌目录同源；package 产品显式清单增加此目录。构建元数据标明 Player 合同、诊断版本及 I39-not-run。发布清单排除测试桥／fixture／Rust harness／故障实现；默认配置继续关闭录制／详细诊断并拒绝产品 E2E token。
- CMake 声明 Hook Markdown 清单副产物；字段表／角色表／原生合同及观察目录仍参与 Player 生成依赖。目录和产品能力更新到 I37；`native_apply=true` 表示实现能力，`runtime_acceptance=I39-not-run` 明确尚未运行验收。

## 资料和未决范围

先读原版本地 `IsaacDocs/docs/EntityPlayer.md` 的 GetHearts／GetPlayerType／GetActiveCharge，再读 REPENTOGON 本地 `docs/docs/EntityPlayer.md` 的 GetHealthType／GetPlayerHUD 等语义，随后阅读现有安装器和 R01—R16／I11—I35 采用合同。此次没有新游戏行为、布局或未审阅原生调用，现有资料足够，未新增逆向，未修改 IDB。

本批只做静态代码／文档审阅及生成目录更新。E29 双配置构建、产品包／符号边界检查及 E22—E25、E28 故障恢复的实际结果统一留在 I39。原版 0—40 支持范围与 tx-04-27 声明 Mod 分工保持现行约定。
