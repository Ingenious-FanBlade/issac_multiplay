# tx-03-08：T0-09-C 工作设计静态审阅

日期：2026-10-21（Asia/Shanghai）。状态：**本轮唯一整体静态文档审阅 PASS。**

本轮目标：交付 [C 工作设计](tx-03-08-c-design.md)、[实施前失败方式](tx-03-08-c-failure-modes.md)，同步 T0-09/任务计划/代码工具入口。只进行文件检索、代码阅读与文档编辑，不运行构建、程序、游戏或代码测试。

## 审阅方式与范围

在设计、48 项实施前失败方式和主要入口编辑完成后，一次集中读取新文档及相关入口，核对前期已读源码事实；在同一次静态审阅中检查三份新文档的相对文件链接，全部可定位。没有执行代码测试、构建、游戏、离线 replay 或任何哈希校验。以下 PASS 只表示文档一致性，不表示运行时功能通过。

| 项目 | 结果 | 依据 |
| --- | --- | --- |
| 用户选择 | PASS | 六场/35,000 tick、用户逐场准备新进程、普通采样与 odd gap 三项明确写入；完整多 seed 留 D。 |
| 已有事实 | PASS | VM worker 无本地 A/B、Host script_count=0/world 空、P1 gap publication、256-step 上限、旧序 Sequence 和 relay 启动时计时等均对应已读生产代码；无运行推断冒充已观察。 |
| 输入与身份 | PASS | 共用 24-byte state 与逻辑命令身份；各端绝对 target 显式翻译；静默/恢复/edge、物理源与脚本源、accepted 高水位用途区分。 |
| 时标和启动 | PASS | Host/VM/相对 tick 三域、普通 r0、启动无源中性 300 tick、固定本地/网络 lead 和发送 deadline 明确；日常 Authority 不等待 B。 |
| 采集资格 | PASS | 复用普通 Room 正常尾部、同 token 自有拷贝、current-player witness 与正常 publication；普通 incomplete 不得降格伪报完整 subset。 |
| odd 与覆盖 | PASS | 每 gameplay tick 留记录，odd 有原生来源且不算完整相等，普通样本零遗漏，phase 不匹配失败，禁止全 gap PASS 或删 gap 后重编号。 |
| 比较规则 | PASS | finite exact/signed-zero 规则、局部身份隔离、F0 严格位置相等、F1–F5 有限因果分类、UNKNOWN 失败均明确。 |
| 兼容与 IPC | PASS | 保留 wire v2、state schema1 与既有 prefix；新内部配置显式版本；reason 非零、10 values、局部 id/use、独立映射和有界脚本表符合现有边界。 |
| 重复与乱序 | PASS（澄清后） | T09-only Guard 保留 decoder/身份验证及 scheduler Stale 语义；正常 256 条历史淘汰与真正越界/分配失败已明确区分。 |
| relay/outage | PASS | 实际 xorshift 算法、非负 jitter、tick 触发/实际控制延迟、双向 forward 静默、排队包去向和前后 1,000 tick 明确。 |
| 线程/生命周期 | PASS | 主线程唯一模拟与 Hook owner、worker 不写世界、双端实际 catalog、终点封闭、双方完成后 STOP、NEVER/DLL 常驻及 staging 保留明确。 |
| 唯一验收 | PASS | 一个实现目标末尾一次统一 E2E，6 个必要 case 不拆试跑；任何失败收尾一次并停止，不自动重跑；本次仅静态文档审阅。 |
| Artifact/边界 | PASS | 双端/relay/control/脚本/来源/分歧窗口、磁盘与内存预算、共享文件完成边界、隐私与 D 未覆盖项明确。 |
| 引用/入口 | PASS | 三份新文档相对文件引用全部存在；T0-09 README、总设计补充、任务计划和 cs-t0 README 已同步本次用户决定；旧 B 结果和失败保留。 |

## 本次审阅中完成的澄清

1. 接收历史采用正常有界淘汰，不把第 257 条合法包误设为容量耗尽；真正分配/越界/序列问题才是错误。
2. 实施顺序改为“实现1–7”，避免与已删除的对照 C2、后续完整对照 C3 编号混淆。

以上属于本次同一次静态审阅的文档澄清及结果落盘，没有另开第二次验收。

## 结论与未完成

**T0-09-C 工作设计 DESIGN_COMPLETE / STATIC_REVIEW_PASS。** C 的生产实现仍 NOT_STARTED、P2 E2E 仍 NOT_RUN，T0-09 整体 IN_PROGRESS。

SharedV8 等实际字节布局、helper 接线、具体脚本数值与队列限额由实施前静态契约补齐；VM 本地 B、r0/phase 可比性、固定 lead 的实际迟到行为属于待运行观察事实。本次不外推其成功，也不新增用户确认或旧 gate 前置流程。
