# tx-04-07 实质修复：启动 trace 丢失与空启动调用

日期：2026-10-25（America/Los_Angeles）。用户在第一次 E2E 失败后明确要求“修复该问题，然后继续测试”。本轮是该目标的实质修复推进：完成以下修改后只执行一次新的完整 E2E，产物使用 tx-04-07-e2e-02；不更改 e2e-01，不在修改中运行测试，不以新编号重复原验收。

## 依据与修改

第一次运行在 Host 前、Manager loop=1 的 PGD 观测中丢失 63 条 trace，见[原始结果](tx-04-07-results.md)。日志原实现使用 512 槽且空队列时 Sleep(10)，不能及时唤醒；仅清除 trace_dropped 或放宽 RuntimeHealthy 会掩盖问题，本轮保留错误停用。

`ops/trace.cpp` 改为 4096 槽的有界队列（约 14 MiB 常驻）和自动复位事件通知。每条事件仍完整写入，保留原序号、单事件长度与截断规则，不合并 PGD 调用。生产者只格式化/入队/通知，文件写入仍在后台线程；正常停止先通知并等待线程，之后关闭事件，进程退出补写仍不等待其他线程。

新增 status/shutdown 诊断：trace_capacity、trace_queue_dropped、trace_write_errors、trace_wake_errors、trace_native_error，分别保留队列、磁盘、通知错误。任一证据丢失仍计入 trace_dropped，并通过现有 RuntimeHealthy 失败停用。

ST4 的 ExecuteStartGame 导出明确原生入口先读 Manager+0x4AD10，零值早退；第一次运行中该 Hook 在菜单每帧到达。原观测无条件打开 Continue 窗口、记录 enter/return 并更新 defer_through，既制造错误加载证据，也可能使启动后稳定重绑一直延后。本轮生成 wrapper 在原调用前只读该位：只有真实 starting 请求才启用 PersistenceCallScope 与 LoadingOwnerScope，原 super 无论该位如何始终调用一次，常驻 Hook 不卸载。不可读时记录 context fault。

`lifecycle.cpp` 的 loading end 改用冻结的 uint64 队列/代次快照，消除 Win32 size_t 直接传给 %llu 造成后续参数错位。首次失败的原始错位日志保留。

harness 在运行时错误检查前保存 last_observed_status，提前失败归约可引用该真实快照，不把它写成 final_status；完整场景结果仍依赖 S21。V16 同时检查新增 trace 错误分类为零。没有新增单元测试或阶段测试。

## 验收安排

先统一纯构建 DLL/harness。用户准备好后，通过原 tx-04-07 单入口、一次启动执行完整 S0–S21/V1–V16；按 D20 临时禁用并恢复 Download Helper。第一次启动失败记录与用户存档不回滚。若此次出现其它失败，记录真实结果和未执行项，不自动再启动。

Win32/v143/Release DLL 与 i686 Rust harness 已统一纯编译链接成功，见 [build-01.log](../../artifacts/repentogon/tx-04-07-trace-fix/build-01.log)。Rust 无 warning，SDK/生成 wrapper 的既有警告保留。已回读生成的 start_game wrapper，空调用仍经过 super 一次，但不启用加载/持久观测 scope。

本轮修改与纯构建期间没有执行测试。用户回复“已准备好，构建成功后开始”后，只执行一次完整入口 E2E（e2e-02）：S0–S12 完成，S13 `cs stop` 后在退休/原生 B 移除路径崩溃，退出码 0xC0000005。最后 S13 checkpoint 的 trace 总丢失、队列丢失、写入/唤醒错误和截断均为零，原启动丢失未重现，严格 trace 解析成功。V11 死亡/复活通过，但整体仍为 FAIL，S14–S21 未执行。崩溃准确指令与根因未确定；本轮未自动重跑。详见[第二次结果](tx-04-07-results.md#实质修复后的第二次-e2e2026-10-25)与[原始产物](../../artifacts/repentogon/tx-04-07-e2e-02/README.md)。测试 mod/C/S 配置已清理，Download Helper 已恢复；第一次失败保持原样。
