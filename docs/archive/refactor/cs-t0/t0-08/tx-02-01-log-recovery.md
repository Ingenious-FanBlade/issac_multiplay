# tx-02-01：协议日志消费与失败收尾修复

2026-10-20，Asia/Shanghai。用户要求修复 tx-02-00 实机的 `protocol trace overflow`，然后继续测试。本轮完整目标：及时释放协议共享环槽、减少日志写入阻塞、在采集失败后正常停止并落盘恢复结果、报告原始故障；全部修改完成后一次统一 E2E 覆盖离线协议与真实 Authority。历史失败文件不改写，不预跑、不增加单元测试。

## 修改前列出的失败方式

1. 消费者把共享槽占用时间延长到逐字段 JSON/hex 转换和磁盘写入之后，生产者看到旧 consumed 而报满。
2. 未缓冲 File 的大量小写入和固定 20ms 轮询使 256 槽协议环积压；只扩大容量会掩盖消费问题。
3. 复制后未核对 publication 或过早释放槽，使生产者重用共享字节时消费者仍在读取。
4. 缓冲后不 flush，实时执行器看不到 commit/script-complete，导致迟停、超时或不完整 JSON 行。
5. overflow/I/O/probe 错误通过 `?` 提前退出，仅 Drop 发停止命令，没有收集停止后的恢复事件与 restored/pinned。
6. 正常停止与异常 fallback 各执行一次 stop，违反单 owner/单次清理，或错误地重注入/卸载常驻 DLL。
7. 清理失败、最终 drain/flush 或 world.finish 错误覆盖最初错误；有恢复成功但错误未保留，误报 PASS。
8. 已丢 trace 在清理时被错误当作完整；即使保存剩余记录，overwritten 非零仍必须失败。
9. 缺 restoration.json 时分析器抛 FileNotFoundError，遮蔽控制器的真实错误且不生成逐项报告。
10. 对旧常驻进程再次注入；输入准备标记晚到或工具拒绝仍被当作成功；在修改期提前测试。

## 设计与验收安排（运行前）

保持 wire v2、IPC v6 和 256 槽布局。消费者先将稳定槽复制为有界本地批次并发布 consumed，再转换/写日志；事件/协议/loop 日志使用缓冲并在每轮 flush，协议模式 2ms 轮询。失败退出先记录原始错误，统一完成唯一正常 stop，独立尝试最终 drain、恢复 JSON、world 收尾和日志 flush；每个清理错误单独保留，不解除失败。

沿用原来的 17,373 corpus 和生产生命周期场景及真实 Authority/outage/恢复检查，在一个入口内运行一次。另在同一入口内部，用第三次旧失败的只读日志副本调用报告器，要求保留 `protocol trace overflow` 并以明确缺文件检查失败，不能再抛 FileNotFoundError；不重放游戏。原始旧产物不修改。所有测试只在本轮全部实现及纯编译后执行。

实机前需要用户重启为没有旧常驻 DLL 的唯一隔离游戏并进入安全新局，只有 A；必须先询问用户准备情况。需要物理动作时仅使用已授权短闭合移动。预期产物包含完整原始包、事件、恢复逐槽结果、控制器主错误/清理错误/环计数、源码/二进制/构建日志与统一结果。若失败，不自动重跑。

## 已实现内容

- `coop_active.rs`：协议记录先通过两次 publication 读取检查，复制到最多256条本地批次，每条复制后 release-store consumed；JSON/hex/File 不再持有共享槽。事件/协议/loop/操作日志各使用256KiB BufWriter，每轮显式flush，协议模式轮询间隔2ms。wire和IPC布局未改变。
- 采集循环错误保留为controllerError；正常stop子进程或异常fallback只执行其中一个。最终事件/协议/loop drain互不阻断，先写restoration.json，再world/probe finish和最终flush；cleanupErrors及produced/consumed/lost进入结果。关闭后即使保存剩余排队trace，lost仍使本次失败。
- `report-protocol-e2e.py`：缺原始文件时输出明确的失败检查及控制器原始错误；完整运行增加零loss、produced=consumed=落盘行数、无controller/cleanup错误的检查。
- `accept-protocol-e2e.py`：支持推进编号，初始物理输入必须提供实际成功工具结果；历史缺恢复文件的真实失败副本作为同次入口内的报告场景。控制器非零退出保存原始错误并标live=FAILED，最终汇总始终落盘。
- `build-protocol-e2e.ps1`仅新增构建记录的Transaction参数，仍是纯编译，不执行产物。

最终构建目录：`out/cs-t0/tx-02-01-build-02`，Rust/Native/Helper/ProtocolFullScenario纯编译完成、退出0。第一次`tx-02-01-build`的Rust编译通过，但Native因沙箱拒绝读取Windows SDK目录而停止；原日志保留，未运行产物。允许读取SDK后的第二份构建成功。预定唯一验收目录：`artifacts/cs-t0/tx-02-01-e2e-01`。用户已在本会话确认完整重启、隔离新局与前台准备。统一执行命令：

```powershell
python cs-t0/accept-protocol-e2e.py --transaction tx-02-01 --build-dir out/cs-t0/tx-02-01-build-02 --artifact-dir artifacts/cs-t0/tx-02-01-e2e-01 --prior-failure-artifact artifacts/cs-t0/tx-02-00-e2e-03 --authorization "User requested fixing protocol trace overflow and continuing E2E; fresh isolated safe game confirmed in this chat."
```

状态：唯一整体验收已完成，**23/30 FAIL**，详见[结果](tx-02-01-results.md)。原trace溢出未再发生，4007条完整、lost0；70槽恢复与产物落盘通过。新暴露A原生输入全0和停止末尾tick271 publication失败，未自动重跑。新代码的异常采集错误收尾分支在本次未被触发，报告器历史失败场景不冒充该分支的实机运行证据。
