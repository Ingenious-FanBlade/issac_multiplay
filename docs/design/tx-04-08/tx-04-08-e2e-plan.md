# tx-04-08：一次整体 E2E 设计

日期：2026-10-26，Asia/Shanghai。状态：设计，全部运行项 `NOT_RUN`。范围与用户 U1–U4 见[主方案](tx-04-08-implementation-plan.md)。本文件只规划未来功能完成后的单次整体验收，本次不编写或运行执行器。

## 1. 唯一入口和运行安排

拟定入口：`harness e2e tx-04-08 --scenario <json> --artifact-dir <new-dir>`。一个入口组织准备、两个顺序游戏进程、离线生产 CLI、归约与清理；保存参数、完整场景、随机种子、源码/二进制出处、构建日志和所有退出码，不覆盖历史目录，不计算或校验哈希。

第一个游戏进程取得同一稳定 run/room 内连续 **2,000 次普通完整样本**，之后做生命周期、背压/故障与停止；第二进程采用相同 seed、角色、输入脚本的短对照，默认计划 256 次完整普通采集。第二段的长度是实现参数，若静态确认显示需要更长初始化窗口，在实现说明中明确调整，不扩大第一个区间的 2,000 次要求。

这是两个独立 Authority 捕获的离线比较，不是两个联网 Replica。远端仍为本机 Rust 输入客户端，使用既有 Host 协议；不增加 STATE lane，不进入原版 Online/Isaac Sync。

## 2. 每次启动前的用户配合

第一次启动前明确说明：将临时部署 DLL/测试 mod/配置，使用第二存档，后台运行本机脚本远端；测试期间 harness 接管键盘，用户须停止手动操作。`SkipIntro=1`，不需要手动跳动画；Download Helper 按 D20 临时禁用，结束后恢复。需要用户配合时先取得“准备好”的确认，再启动。

第一进程正常退出、初步产物收口后，**在第二进程启动之前单独提示**下一次启动和继续停止手动操作的要求；若需要重新确认准备状态则等待确认，不在后台静默启动。入口支持记录可恢复的人工检查点，等待用户不算测试重跑，也不以超时当成已确认。

设计不预先授权所有 fixture 改世界动作。实现完成后的启动说明须列出本轮实际动作：spawn/morph/remove、受限 Player Replace/Restore、Grid 创建/替换/移除、B 死亡复活、切房/换层和保存/Continue；超出现有授权范围的具体动作先确认。静态确认不支持的动作不能靠猜 ABI 执行。

每次启动独立记录 PID、部署、配置、存档 before/after、游戏/REPENTOGON 日志、crash before/after。失败按正常已理解路径停止和清理，不自动重启。每日备份按 D27 原样复制但不参与变化判定；其余 D18/D20 规则继续适用。

## 3. 夹具与观察原则

测试 Lua mod 只用于 E2E，允许已授权的 gameplay 回调和世界动作，不构成产品 mod 支持声明。沿用 F8 动作队列，尽量用既有控制口/token 和检查点协调，不使用 F9。实际 API、variant、参数和副作用由 ST1–ST7 核对后写入 scenario。

2026-10-26 ST6–ST8 的[静态核验与主审](../../research/tx-04-08-st6-st8-static-confirmation.md)已完成：Game 显式 seed Spawn、NPC/Pickup Morph 和 Grid 公开入口已定位，但安全的具体类型/对象/格子/PathTrail 与 Replace 自然触发动作仍有 UNKNOWN，当前夹具尚未实现这些新动作。首次菜单 Continue 保留、B 复活后的读取依据重建和部分 B Invalidate 返回未共同撤销须纳入新 bridge/scenario；不能把已有 metrics 当 world witness。S11/S13 的健康退出在显式 world stop 真正 Closed/flush 后安排；原生 Shutdown 位于 Manager 循环之后，不能预设还有十秒 Manager 排空窗口。所有直接退出的完整协调仍须实现，不以该健康路径冒充 V9 全覆盖。所有运行项继续 NOT_RUN，没有在静态工作中执行或删减必需场景。

普通区间使用受支持的 A/B 和可读来源；先记录初始化和 FIRST_OBSERVED 的 NOT_READY，再满足当前 Player 读取规则。稳定段避免主动投弹、激光和未支持类别；它们另在明确负向/未知场景记录，不能删实体或缩字段让稳定区间通过。必要时用已授权的确定房间夹具提供所需七类和 Grid，但不能把“生成了一个对象”当成全部变体已支持。

Lua 独立读取只作限定字段和场景交叉证据；它可能位于更新内，不能拿帧尾 `cs status` 的旧快照直接当实时采样 tick。需要对齐时使用已有 `cs witness`、真实阶段记录及新的 world checkpoint watermark，并给出时序关系；不补调用游戏查询来制造重复覆盖。

每例生命周期必须指向真实原生调用/事件链和前后成员状态。不能自然观察到的池复用、短命或特殊 Player 路线保留未覆盖，不能把离线修改文件当作这些原生路径的运行证据。

## 4. 两个进程的场景顺序

### 4.1 第一进程

| 场景 | 操作 | 必须保留的事实 |
| --- | --- | --- |
| S0 | 环境准备，第一次启动到第二存档的新局；world 未开始 | Hook/能力、默认透传、进程与存档基线；未开始录制不产世界数据 |
| S1 | A 已存在时 `cs world start primary`；待 Recording 后发起 Host/远端握手，原生 Join 创建 B | A 的 FIRST_OBSERVED、B 创建 owner/Init/Publish/返回、exact pair 和 session/participant generation |
| S2 | 初始化与普通 A/B 输入；随后稳定采集 2,000 次完整普通样本 | 明确区间、所有 gap/attempt、真实 tick、读取依据、输入推进、copy/queue/落盘统计 |
| S3 | 暂停/控制台屏蔽、测试 MC_PRE_UPDATE 跳帧、恢复 | 无完整世界误发布、不重放边沿；OddPlayer 自然样本保留为 gap |
| S4 | 受支持七类/来源和 Grid/门别名；短命/重复发布/移除与可达复用 | 各类字段、来源、event 链、birth/use/revision、原生调用/返回和别名 |
| S5 | 受支持 NPC/Pickup Morph、Grid 替换/移除、受限 Player Replace/Restore | 已核对动作参数；同对象/新对象、保留 Player、成员退出与物理退休的区别 |
| S6 | 切房、B 死亡/复活、换层 | 污染窗口、epoch、读取依据撤销/恢复、同 K/ordinal、旧关联失效；不要求迁移期间有完整样本 |
| S7 | 主动暂停 raw reader，触发背压；恢复后继续 | 主线程零等待、独立 gap 范围和 Authority/输入继续；仅暂停本 DLL 自有 reader，不暂停游戏线程 |
| S8 | 先正常关闭 primary，再分别在新的录制上下文触发采集器自有故障 | 每个故障独立归属，真实终态收口后才开下一个；不复活旧身份，不重装 Hook、不重启游戏 |
| S9 | 新建健康的正常录制，重新取得当前 Player 读取依据后正常 stop；Host 保持，A/B 继续输入，再正常 stop Host | world 停止与会话退休分开；Hook 常驻且透传、B/K 仍由原有 owner 退休 |
| S10 | 已授权的退菜单保存；新录制观察首次菜单 Host/Continue，再正常退出 run | world 不破坏既有 Preserve 决策；加载 gap、run epoch 与 B 再创建/退休；保存窗口/文件证据 |
| S11 | 最后一次正常 world close、游戏退出、清理第一进程环境 | callback/reader/writer/drain/flush；无新 crash；已完成文件与明确未完成项 |

顺序在实现时允许为原生依赖作调整，例如 Replace/Restore 需要受限角色/预备对象，但 S2 的稳定区间先于主动生命周期扰动，正常 stop 的证据不被故障场景取代。S4/S5 的 UNKNOWN 必须在实现说明中列为未实现/未覆盖，不能删掉原型功能后仍称完整迁移。

### 4.2 第二进程

| 场景 | 操作 | 必须保留的事实 |
| --- | --- | --- |
| S12 | 第二次启动前提示与准备检查；按同 seed、角色和脚本建立短对照，A 已存在后再开始 world，随后 Host/Join | 与第一进程同场景的实际依据，独立 PID/run/room/local IDs，非地址或数值碰巧相同 |
| S13 | 初始化后取得计划的 256 次完整普通样本，停止 world/Host，正常退出 | 双方时间线、实际输入消费/出生/阶段记录；正常 drain，第二次完整环境清理 |

不要求两个原生世界状态相等，不声明同 seed/input 保证确定性。第一进程 S2 的起始子区间与第二进程显式配对；无法建立某个映射则该区域 INCOMPARABLE。没有可配对的已知字段时，双进程工具链覆盖不足，不能仅凭 CLI 退出成功判通过。

## 5. 自有故障的作用域

先列明失败方式，再实现故障入口。入口仅在专用 E2E 配置、匹配 token 和当前 recording generation 下可用；故障参数不能通过远端输入协议传入。所有合成情况标为 `TEST_INJECTED`，不得描述成原生游戏实际发生了相同故障。

| 故障 | 允许手段 | 判定边界 |
| --- | --- | --- |
| 背压 | 有界暂停 raw reader；生产者正常运行 | 触发实际无 FREE 分支、记录缺口，游戏不等后台；恢复后不补旧世界 |
| 对象/membership/关系预算 | 当前实验录制采用较低的自有预算，让真实枚举触发上限 | 不写游戏指针，不改固定字段集合；在报告列出故障参数 |
| 字段读取失败 | 读取封装在测试分支访问 collector 自建 PAGE_NOACCESS 区域 | 经过真实受保护 Read 失败路径；不等于观察到真实游戏坏地址 |
| 来源/root 变化 | 测试分支改变 collector 自有的前后期望值，使真实比较分支拒绝 | 证明拒绝/隔离响应，不冒称游戏实际换了 root 或来源 |
| 引用歧义 | 在自有记录判定点标记歧义，不修改 EntityPtr | 原始值与注入前后状态并存，不把后处理文件修改当 native 覆盖 |
| event overflow | 有界压低自有事件容量或投递明确的测试诊断事件 | 经过实际容量失败路径，记录器停止；不制造假的出生/析构事件 |
| wrong thread | 自有工作线程仅触发 observer 线程检查，携带无游戏对象的测试通知 | 不调用游戏 Hook/original，不碰身份表；Host/Authority 不因 world 策略被误停 |
| 文件失败 | writer 使用 artifact 内的可控失败 sink 或关闭自有文件句柄 | 记录实际 Write/flush 错误，成功报告不能成立；不改用户文件权限或制造磁盘满 |

失败实验之间允许显式关闭并新建录制，等待真实 Closed；不重跑同一个失败实验。若收口未完成，不强行释放/重建上下文，剩余项记 NOT_RUN。主流程错误或游戏崩溃时按正常失败收尾，不用后续故障样本掩盖主失败。

## 6. 同一入口内的离线生产 CLI

两个进程结束后，使用真实完整产物和新建的自有副本调用 `harness world-replay/world-compare`。不是在设计/修改中预跑工具，也不另行启动测试入口。

| 子项 | 内容与期望 |
| --- | --- |
| L1 全量 replay | 对 primary 的全部 captures/gap/lifecycle 记录重编码两份，含稳定段和生命周期，不截取成小样本冒充全量；逐记录字段/字节比较，无哈希 |
| L2 编码边界 | 自有副本倒序 source/object/field/membership 验 canonical；最大 u64、raw128、±0、其他 finite、NaN/Inf；原始值/未知原样保存 |
| L3 明确差异 | 仅修改一个已知有限字段，精确定位 run/room/tick/class/key/field；未知区域和 earlier_coverage_unknown 不被差异掩盖 |
| L4 解码拒绝 | 错 domain/version/profile、重复 ID、错误宽度/长度、u64 溢出、额外字段、截断 JSON、超限记录；保存失败原因，不能生成成功结果 |
| L5 冲突与引用 | 同 key 多对象保留整组；NULL/已知/未解析/退休/复用歧义、NOT_READY/READ_FAILED、缺来源/tick/重复 tick；相同错误不算 equal |
| L6 双进程比较 | 显式 alignment 和依据；分层输出 DIFFERENT/EQUAL_WITHIN_PROFILE/INCOMPARABLE；至少存在可核对的真实 tick 对与已知字段区域，不强制 equal |
| L7 旧格式导入 | 显式 legacy 模式读取一份仓库已有完整旧格式材料并重编码；不参与新版本运行 PASS，不默认允许跨布局 equal |

本轮性能报告记录实时 copy/队列/写盘耗时、内存峰值、背压，及完整 replay 的 bytes/records/耗时/吞吐/flush。以 S2 取得 2,000 次且无其他 gap 为必要行为，文件工具不退回无缓冲的小块写入。历史 5 MiB/s/5 倍是 tx-01-88 的特定基线目标，不直接套成新 C++ writer 的性能承诺；新固定超时和样本预算在实现前的 scenario 明示，超时不自动延长重跑。

## 7. 整体判定

| 编号 | 判定与证据 |
| --- | --- |
| V1 接入与透传 | 所需 world 能力/Hook/ABI 逐项与清单对应；每个被覆盖调用 original exactly-once。未开始和停止后透传；常驻 Hook 不恢复字节 |
| V2 完整阶段/tick | 每份完整样本都能关联同 outer 的 owner/Room/tail/正常返回和同轮真实 StandardRoom tick；odd、内部更新、无提交、加载/暂停不冒充完整 |
| V3 2,000 次 | 同 recording/run/room/mutation、连续 attempt/token/tick 链的稳定区间内完整普通数 ≥2,000；符合现行条件的明确 odd-only gap 不计数也不打断。其他 gap/缺号/生命周期均打断，保存精确边界 |
| V4 字段与来源 | 七类 Entity 和 Grid 的规定字段、实际目标版本 Grid 上限、八 EL/Player 来源与门别名均有证据；未知和未覆盖单列，不删除数据/字段 |
| V5 身份与生命周期 | 外层创建完成、重复发布、Remove/清引用/销毁、可达复用/短命、Morph/Replace/Restore/Grid 退休按真实路径判定；本地身份与跨进程 key 分开 |
| V6 Player 依据 | FIRST_OBSERVED 不补出生；真实 Player body/Authority/exact pair/generation 满足后当前字段可读，失效撤销；复活/加载正常重建依据 |
| V7 只读与线程 | 采集路径无额外原生查询、RNG 推进、Lua、Spawn/setter；copy 前后可核对 RNG 原值、线程与原生调用。限定证据窗口，不声称覆盖所有 worker/未知 mod |
| V8 缓冲与隔离 | 实际背压/预算/读取/来源/root/引用/事件/线程故障各报告覆盖；不覆盖 READING、不阻塞主线程、故障不误停 core，故障之后真实 tick/输入继续 |
| V9 文件与关闭 | 正常 stop drain/flush 且已生产数据可对账；文件失败不会成功；记录器停止不停止 Host；进程正常退出，无在途释放与新 crash |
| V10 离线工具 | L1–L5 真实生产 CLI 结果符合预期，源未改、两次重编码一致、非法输入拒绝、未知不 equal |
| V11 双进程与 legacy | L6 有实际可核对的配对/分层结果和未知；L7 显式导入可复现，拒绝无依据跨版本兼容 |
| V12 既有会话与持久副作用 | A/B 输入、completed/Ack、Join、K 常驻/退休与 loading/Continue 语义不被采集破坏；D18/D20/D27、窗口/文件/崩溃记录，未授权变化如实失败 |

一个最终 `report.json/report.md` 汇总 PASS/FAIL/NOT_OBSERVED/NOT_RUN 和限定理由。未观察到的必需功能子项不能冒记 PASS，也不能用原型历史账本补足。场景动作完成、录制 Closed、CLI 退出 0 均不等于整体验收通过。首次失败保留已采证据，后续无依赖的离线归约和清理可完成；禁止自动重启游戏或重做失败场景。

## 8. 可复查 artifact

目录至少保存：

- `invocation.json、scenario.json、environment.json、sources/、binaries/` 和构建日志/版本可读出处；两次启动前提示与人工准备确认记录。
- `process-1/、process-2/` 各自 PID/部署/输入/网络包、常规 trace、REPENTOGON/游戏日志、关键截图、退出/crash 和存档窗口/文件变更证据。
- 每个 recording 的完整 `world/`、Hook/布局清单、生产/消费/写入计数、callback/worker 终态、失败前缀和缺口范围；稳定段的逐记录索引与 2,000 次区间证明。
- 逐类/字段/来源、identity/use/birth、readiness、生命周期 episode 与故障账本；每个结论能回到原始序号，不以摘要代替全部记录。
- `offline/` 中所有生产 CLI 参数/退出码、未修改输入、自有变更副本和变更说明、两份 replay、逐字段/字节对照、alignment/依据、冲突和比较结果。
- `report.json、report.md、cleanup.json`：实际完成与失败/未覆盖、两次环境恢复、Download Helper 恢复、配置/测试 mod 清理；不回滚用户存档、不扩大白名单。

复现入口使用同一 scenario、新 artifact-dir、相同支持范围与明确的动作授权。保留失败历史；复现是将来用户另行发起的运行，不是本次失败后的自动重跑。
