# tx-01-83：安装诊断与异常结束汇总

日期：2026-10-19。用户明确选择先补诊断与异常汇总、只纯编译控制器，准备阶段不运行测试或游戏。Native、Helper、Mod、IPC/ISCS/Lua/loop 事件布局不改；tx-01-82 的 FAIL、21 PASS/12 NOT_COVERED 原始账本及全部产物不重判、不重写。

后续运行：用户明确要求开始剩余 E2E 后，2026-10-19 17:32–18:25 完成一次统一验收，**整体 FAIL，25 PASS / 1 FAIL / 7 NOT_COVERED，报告完成**；八段恢复均为 70 槽。详见[实际结果](tx-01-83-e2e-results.md)。下文“E2E_NOT_RUN”和等待确认均为本次修改准备阶段的历史状态。

## 修改前登记的失败方式

- 安装失败事件的 action（阶段）和 device_key（Win32 错误）未写入 JSONL，只有索引不足以定位原因。
- helper 退出发生在本轮 drain 之后，立即 break 会遗漏最后发布的诊断或恢复事件。
- 后续采集抛异常/用户中断，正常路径末尾的 record_results 和最终人工审阅没有到达，早先已完成证据没有汇总。
- 部分证据损坏、缺失或汇总自身报错，不能抹掉其它阶段，也不能用汇总错误覆盖最初失败。
- 缺少入口/场景观察不等于已观察到不变量错误；不能将未覆盖改成 PASS，也不应仅因没运行场景而捏造功能 FAIL。
- 清理、汇总或关闭某个句柄失败，仍须尝试其它自有资源的既定收尾并导出原始失败；不能偷偷重试注入、故障、比较、replay 或清理请求。
- 最终汇总可能正常路径和 finally 都经过；只允许一次写入/判定，不覆盖已存在文件，不以新结果擦除先前 FAIL 或人工审阅。
- 既有受控 controller-loss 的控制器最终 flush 原本就是未确认；汇总必须保留该事实，不能要求被主动终止的控制器生成完整 result 而引入新的假失败。

## 实现范围

1. coop-active 原有 loop-events.jsonl 增加原始 action/deviceKey，不改二进制布局；收尾再 drain 已发布事件一次，不等待、不重启生产者。
2. 统一执行器在最终人工审阅前汇总已有文件；若提前异常或中断，finally 走同一个一次性汇总入口。只读取本次已有产物和内存中的完成记录，不运行任何新测试、场景或 CLI。
3. completed-evidence.json 保存各采集段原始退出/采集/世界/probe/独立恢复结果，以及已执行命令的原始 exit；缺失/损坏和汇总错误明确保留。
4. instrumented 模式把已完成 primary 的自动阶段/读取撤销检查写入总表，缺入口/缺场景保留 NOT_COVERED；不变量错误保留 FAIL。人工项继续需要实际审阅。受控故障的原始 exit=1/CAPTURE_ERROR 不改写。
5. 最初异常、收尾和汇总异常分别保留；整体失败不因部分通过变成 PASS。未开始或未完成的后续阶段保持未覆盖。

## 已完成的代码修改

- `cs-t0/src/coop_active.rs`：日常及末次 drain 共用 loop JSONL 写入函数，保存现有事件的 `action`、`deviceKey`。helper 退出后、生成恢复/采集报告前，再读取一次已发布的 coop 与 loop 事件；不重试安装或停止。
- `cs-t0/world_e2e/runner.py`：正常人工审阅前或异常的 finally 中，只汇总一次；扫描本次采集目录，包含尚未登记完成的阶段。写出 `completed-evidence.json` 与 `finalization-diagnostics.json`，保留原异常、原退出码、缺失文件和收尾错误。汇总、控制器既定清理、观察器关闭分别处理异常，避免一个失败跳过其余步骤。
- `cs-t0/world_e2e/instrumented.py`：将已有内存中的 primary 分析结果与故障记录写入一次性收尾，不重新运行 evaluate、比较或故障；缺入口/缺撤销场景记 NOT_COVERED，实际不变量错误记 FAIL，先前 FAIL 保留。逐个关闭自有观察资源，分别收集关闭错误。

### 安装失败字段的含义

以现有 loop 安装失败事件为准：`errorCode` 为 HookTransaction（4）或该分支的 UnsafeShutdown（8），`action` 是 `InstallFailureStage`，`valueBits` 是失败索引，`deviceKey` 是 Win32 错误码。阶段 1–6 分别为 CallTarget、IatLookup、IatRva、IatOwner、VtableCol、VtableQuery；补丁事务阶段从 32 起编码，按 Native 的实际阶段定义解释。其它事件中的同名字段仍保留原语义，不能统一当作安装错误。

这些字段只改善未来取证，无法补回 tx-01-82 没有保存的阶段/Win32 错误。该次安装失败根因仍为 UNKNOWN，70 槽外部字节相等也不升级为完整安装/原生恢复通过。

### 收尾产物与边界

| 文件 | 内容 |
| --- | --- |
| `completed-evidence.json` | 本次已有各阶段结果、命令 exit、比较与对齐来源；缺失/损坏逐项保留 |
| `remaining-review-evidence.json` | 已有 primary 分析、故障实际观察与尚未尝试的故障；人工义务不自动判 PASS |
| `finalization-diagnostics.json` | 汇总是否尝试/完成、最初失败与各收尾异常；不自动重试 |
| 原 `result.json` / `finalization.json` | 导出本次账本与报告完成状态，整体原始失败继续生效 |

受控 controller-loss 保留 `UNCONFIRMED_AFTER_CONTROLLED_LOSS`；不要求已终止的控制器生成最终 flush 结果。汇总文件本身写入失败仍记录为报告失败，不覆盖原始材料。

## 构建与交付状态

直接纯编译命令：

```powershell
& 'C:\Users\114514\.cargo\bin\cargo.exe' build --release --locked --offline --manifest-path cs-t0/Cargo.toml --bin IsaacCsT0 --target-dir out/cs-t0/tx-01-83-build/rust
```

结果：退出码 **0**，`Finished release profile [optimized]`，耗时 **2m 14s**。产物为 `out/cs-t0/tx-01-83-build/rust/release/IsaacCsT0.exe`，完整日志为 [controller-build.log](../../../../../out/cs-t0/tx-01-83-build/controller-build.log)。只编译控制器，未调用旧构建脚本，未编译或更换 Native/Helper。

状态：**实现完成、控制器纯编译成功、E2E_NOT_RUN**。Python 执行器仅作静态代码审阅，未执行 Python/py_compile；未执行控制器、测试、replay、注入、进程探测或游戏操作，没有新增运行 PASS。异常分支及诊断落盘的运行效果仍待整体验收，tx-01-82 根因也仍未知。

新的统一验收需准备完成后由用户确认。此修正没有增加从 tx-01-82 失败点续跑的入口，现有 instrumented 编排仍为完整流程；不能把已结束的执行器当作仍可继续的会话。
