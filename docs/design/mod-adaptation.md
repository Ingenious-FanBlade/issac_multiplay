# 第三方 Mod 适配：后续方案

日期：2026-11-10（Asia/Singapore）。状态：从[原生二进制重构](binary-state/README.md)迁出的设计，后续另行推进；不是本轮 0—15 步的实现或验收条件。本次只迁移方案，没有实现、构建或测试。

## 当前顺序与范围

用户决定当前先跑通原版内容的采集、传输、Client 原生应用和统一整体验收。官方 REPENTOGON 加载器、J273 目标及原版需要的官方扩展 owner 继续沿用；“原版”指内容范围，不改变加载方式。本项目自带界面与 E2E 夹具继续按原职责管理，不构成第三方 Mod 兼容声明。

第三方 Mod 自定义状态、角色动态身份、专用资源／绘制、玩法 Lua 与表现隔离、生命周期及 StageAPI 等具名适配集中保存在本文。它们不延后原版必要功能，也不要求本轮预留可工作的通用 Mod 框架。后续开展时再确定实施范围和验收安排；本文不自动启动适配或实机操作。

## 迁入内容与已有事实

| 原位置 | 迁入内容 |
| --- | --- |
| 二进制重构第 8 步原 8.4 | 只读 Lua adapter、自描述二进制状态、分块比较、预算及 schema 完整性 |
| 第 9 步原 9.7 | Mod 值格式草稿、共享节点与循环引用编码 |
| 第 10 步低频采集、第 14 步产品边界 | Mod 低频读取及 `domains/mod` 正式来源的设想 |
| 第 15 步原 E31 及补充矩阵 | 循环／共享／超预算／adapter 出错等适配验收；原 E31 在本轮不执行、不计通过，其余编号不变 |
| Player／HUD 跨域交接 | 第三方回调最终值、角色／武器／资源、额外 History 展示等具名适配需求 |

已有 Mod 观测使用具名只读 adapter（`mod/observation/isaac-cs-observation/domains/mod/collect.lua`）和有界对象图（`domains/mod/graph.lua`），已有 StageAPI adapter。旧 `stream-catalog.json` 的 mod 域有 12 个公开成员，其中 2 个 `ReplicatedState` 成员原计划映射到具名 adapter schema。它们是已有来源／目录事实，不表示新的二进制传输和 Client 安装已经完成。

[tx-04-14 实现与验收](observation-expansion/tx-04-14-implementation.md)、[Mod 记录／适配合同](observation-expansion/run-mod-schema.md)及[StageAPI 观测设计](observation-expansion/run-presentation-mod-observation.md#51-stageapi自定义房间与customgrid必测适配)保留其原时点事实、失败和覆盖限制。观测成功不等于已能同步任意 gameplay Mod。

## 状态来源与完整性草稿

- 具名 adapter 在合法 ordinary 主线程窗口低频只读 Lua 状态；不额外推进随机、创建玩法状态、调用玩法生产器或写持久数据。网络／后台线程只处理自有数据。
- Lua 自定义状态没有可直接采用的原生字节布局，按 adapter 声明的 schema 判断完整性。未知闭包／私有对象不得宣称自动完整捕获；原生对象仍须遵守字节归属与已核生命周期规则。
- Host 由 C++ 读取 adapter 的有界输出，直接编码为二进制，不再构建 JSON 树；Client 解码后由对应适配器安装状态／展示结果。不得在 Client 重演 Host gameplay Lua 来补值。
- 每个 adapter 的输出作为一个块，用 memcmp 判断变化。稳定输出的顺序、共享节点编号与字节规范化仍须定稿，避免无变化时反复重发。
- 沿用 `graph.lua` 的节点、项数、字节、深度预算，并单独计时；超预算、抛错、缺来源或读取超时必须报告具名缺口，不能截断后宣称完整。原草稿提出超时降频，具体失败后的发布／ACK 语义仍待设计。

## Mod 值编码草稿

以下是从第 9 步迁出的草稿，不属于当前原版线格式；后续实施时再与布局版本、成员／资源表及有界解码规则接合。

| +0x0 u8 tag：0=nil，1=false，2=true，3=i32，4=f64，5=字符串，6=表，7=实体引用，8=Grid 引用，9=Vector，10=Color，11=KColor，12=共享节点回指 | +0x1 值：按 tag 编码，i32 为 4 字节，f64 为 8 字节，字符串为 u16 长度＋字节，表为 u16 项数＋键值对，引用为 u16 成员序号，回指为 u16 节点序号 |
|---|---|

表节点按首次出现顺序编号，再次遇到同一节点写 tag 12，以表示共享或循环。嵌套深度不得超过 adapter 预算。Vector／Color／KColor 的具体展开、非法 tag／回指／浮点／长度处理及单行生成表仍须定稿；本草稿不授权复制原生地址或 Lua closure。

## 具名适配交接

- **StageAPI**：自定义房间类型、带原生载体及无原生载体的 CustomGrid、Lua 附加状态和生命周期。既有观测保留，正式采集、资源身份、Client owner、切房与退休应用仍需具体适配。
- **角色与武器**：动态身份映射、自定义状态、专用资源／绘制及生命周期；未知动态类型不得伪装成原版基类或已知武器。本轮原版角色／身体族所需项仍归原版同步。
- **Player／HUD 回调结果**：官方 1072／1073 等第三方回调的有效结果、healthtype／自定义 cache 的实际显示结果和 Mod HUD 展示；Client 不执行玩法 Lua，也不恢复整套 Host 模拟输入／通用查询字典。原生语义与 UNKNOWN 继续参考[装备与 Plus 复核](binary-state/player-equipment-plus-synchronization-review.md)、[Player 字节账本](binary-state/player-byte-ledger.md)和[第 5 步设计](binary-state/5-player.md)。
- **额外查询与 History**：声明的界面 Mod 如需要原版同步未发送的历史或查询结果，单独声明 schema 与用途，不据此恢复整个原生 History 或任意 gameplay getter。
- **随机相关结果**：适配只传具体最终状态／展示结果，不把 Lua RNG、闭包、CRT／MT 引擎或原生地址跨进程复制，不在 Client 重新抽取。

## 后续验收与未决问题

原 E31 的覆盖要求保留在这里：具名 adapter 的编码／解码须覆盖循环、共享、超预算、抛错、超时、引用失效和生命周期变化；超限不得伪装为空或完整，安装失败不得提前确认。StageAPI、角色、资源与绘制还须各自覆盖真实 Client 行为。后续按明确的完整适配目标统一 E2E，连续修复与续测；本次不执行这些用例，也不把迁出记为通过。

仍未确定：仅保留现有具名 adapter，还是提供公开注册接口；各 Mod 的 schema／版本与支持清单；稳定编码、读取耗时与降频／失败发布语义；Client 具体安装／展示 owner、资源寿命及隔离。本文不将这些未决项转为本轮原版验收前置条件。
