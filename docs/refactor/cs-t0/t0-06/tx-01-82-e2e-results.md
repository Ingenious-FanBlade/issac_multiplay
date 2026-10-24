# tx-01-82：宿主统一 E2E 实际结果

日期：2026-10-19（Asia/Shanghai）。同一个统一执行器从 12:46 开始，在 14:03 的 `fault-event-overflow` 初始化失败后结束。最终 **FAIL**，`finalization.json` 为 **COMPLETED**。没有重跑验收、故障或注入，没有在验收过程中修改 Native、controller、Mod 或协议。

原始目录：`artifacts/cs-t0/tx-01-82-instrumented-e2e-01/`。tx-01-80/81 历史失败和 tx-01-79 等轮次的历史 PASS 原出处保留。用户仍排除全量 replay 和真实 Online/Isaac Sync 隔离。

## 冻结总表与已产生证据

机器总表为 **21 PASS / 0 子项 FAIL / 12 NOT_COVERED**，整个执行器因异常为 **FAIL**。21 个 PASS 包含历史沿用；可选采集/70 槽子项在 primary 中重新检查，不能称为 21 项本轮新增 PASS。

十个剩余目标在 `authorized-scope-result.json` 中仍全部为 NOT_COVERED：异常发生在 `record_results()` 和最终人工审阅之前。以下新运行证据已保存，报告不改写机器总表，也不将未汇总解释成没有产生证据。

| 阶段 | 实际结果及范围 | 原始证据 |
| --- | --- | --- |
| 宿主环境 | 同一入口枚举 445 个进程，选中唯一游戏；与 tx-01-81 沙箱内仅 3 个进程的观察并存 | `process-queries/`、`manifest.json` |
| primary | 30 分钟上限后正常结束；5,020 次普通采样均有同 token 阶段匹配，trace_integrity=true、errors=[]，背压 0；70 槽外部核对一致 | `primary-evidence/evaluation.json`、`captures/primary/` |
| primary 场景 | 原自定义命令没有启用/action 记录；只读 Lua 记录 callback count=1。用户改用显式 Isaac.RunCallback 后，首次 enabled 仅在采集冻结后出现、players=1，不能补算 primary 场景完成 | `command-static/`、`continuation-after-primary.json` |
| align-left | RD2W XWC9、以撒、Normal；实际 seed=1998345781、房间 84；frame 193 设置目标，实际记录 frame 6000；正常停止、native error=0、70 槽外部核对一致，probe 全部消费且无溢出 | `captures/align-left/` |
| align-right | 相同设置及实际 seed/房间；frame 379 设置目标，实际记录 frame 6000；正常停止及 70 槽外部核对一致，probe 全部消费且无溢出 | `captures/align-right/` |
| 精确帧比较 | 用户确认映射后，生产 world-compare 执行一次、exit=0；DIFFERENT 定位 B 的 position.x、position.y、birth_counter、frame_count；保留未映射 Grid 和更早未知范围 | `cross-process/` |
| field-read | 健康样本后仅一次 command=4/ack=4；world error=4/READ_FAILED、enabled=0；独立观察 coop tick 4351→4363、state=2/error=0；正常停止，70 槽外部核对一致 | `fault-field-read.json`、`captures/fault-field-read/` |
| source-changed | 健康样本后仅一次 command=5/ack=5；world error=17/SOURCE_CHANGED、enabled=0；独立观察 coop tick 4437→4449、state=2/error=0；正常停止，70 槽外部核对一致 | `fault-source-changed.json`、`captures/fault-source-changed/` |
| event-overflow | 初始化失败，尚未取得健康基线或发送故障命令；执行器停止后续实机操作 | `captures/fault-event-overflow/`、`result.json` |

用户明确选择继续本轮后续阶段、保留 primary 场景未覆盖，没有重采 primary。短命/嵌套生成、池复用、Morph、Grid 生命周期、Esau/Restore/Replace 等 primary 场景和四个非终止故障，没有因后续比较或两个终止故障而补算覆盖。

## 对齐依据及限制

双方都存在唯一普通样本对应实际 Game frame **6000**；从 run 开始的原始输入连续且中性，canonical Submit/Resolve 中性并覆盖 ordinal 0/1。明确对齐双方 run_epoch=1/room_epoch=1，左 Authority tick **11951** 对右 **11545**，A→A、B→B，实际 B 出生事件 1→1。

B 出生 counter 为 25/228，加入时机不同。提案、依据、用户确认均已保存；不声称初始世界相同，不将这次 DIFFERENT 解释为确定性缺陷。其它出生不自动匹配；Grid 的 IDENTITY_UNRESOLVED、earlier_coverage_unknown=true 和 uncompared_regions=true 均保留。没有最近帧、地址或位置匹配，没有哈希校验。

## 受控故障的原始退出状态

field-read/source-changed 的 controller exit=1、capture status=FAIL、world status=CAPTURE_ERROR 是受控观察错误的原始结果，不改成 exit=0。各自同时保留 scheduler_continues=true、coop error=0、probe error=0/closed=true/produced=consumed/overflow=0 和实际 70 字节恢复证据。这支持两个单场景的预期反应，不能据此把整个 W12 判为 PASS。

## event-overflow 阶段的初始化失败

游戏 PID=28120，controller PID=47132：

- helper：stage=REMOTE_INITIALIZE、native_error=8；controller transportFailure 为 initialize helper exit 8。
- coop：nativeError=3，即 Error::Hook；没有 B 创建完成记录。
- loop：Failure kind=30、errorCode=4，即 HookTransaction；valueBits=0 是诊断索引，缺少阶段时无法说明是哪类槽/检查。
- world：0 次采样/0 事件，CAPTURE_UNCONFIRMED；probe produced=consumed=0、issued_faults=[]、closed=false。event-overflow 命令没有发送，不能称为事件溢出故障通过或事件溢出导致失败。
- external/after-normal-stop.json 的 restored_count=70，70 槽实际字节与安装前一致；原生 restoration.json 为 nativeRestored=false/targets=[]。不能把外部字节一致改成原生完整安装/恢复通过，也不能据原生标志为 false 宣称已有字节未恢复。

静态代码：coop_active_runtime.cpp 在 StartEmbeddedObserver 失败时 Fail(Hook) 并保守返回 8；loop_runtime.cpp 在 InstallHooks 失败时记录 action=安装阶段、value_bits=索引、device_key=Win32 错误。当前 coop_active.rs 保存 loop-events 时遗漏 action/device_key，而 loop_ipc.rs 已有这两个字段。本轮落盘数据不能还原具体失败分支；根因保持 UNKNOWN，不猜测线程指令位置、字节冲突、权限或游戏操作问题。

## 未完成及后续范围

event-overflow 未触发，wrong-thread/root-changed 未进入；primary 场景按用户选择未覆盖。全量 replay 和真实 Online/Isaac Sync 隔离仍为用户排除，历史失败不改判。

本轮已结束；当前进程不重注入、不强制卸载、不自动重跑。建议后续先补齐 controller 对现有 loop 安装失败诊断字段的持久化，保留部分运行证据，再根据诊断准备剩余统一验收。该建议尚未实施、未纯编译、未运行新测试。
