# tx-01-90：操作 2 的结果与复现

2026-10-20（Asia/Shanghai）。**操作 2 已实现，唯一统一离线验收 PASS：30 个场景、174 条命令、175/175 检查，进程退出0、errors=[]。** 其中一项检查输出条数/退出码，其余逐命令检查。Native与离线程序均纯编译成功，未进行实机运行或自动重跑。

## 实际交付

生产 `AuthorityTickClock` 成为 C/S 已完成 tick 的唯一写入 owner。Begin 只预留下一候选；普通标准 Room 正常返回（RVA2FBA52），或已出现普通 Main Player witness（2FD60C）的 odd helper 正常返回（555246），经过同 kind/source/owner/loop/thread 配对后，Complete 才增加一次 completed。旧全局 g_tick/g_completed 已移除，IPC completed 仍是镜像；其它隔离运行时的计数不并入。

loop/render/物理poll不增加tick，loop编号跳跃不补造步数。Warmup、暂停/loading/menu阻断、未命中正常Player路径的odd早退不开始候选。缺输入、B暂时失绑/单独退休不变成全员ready等待；room/death generation变化仍影响绑定与previous，但不抹去已经正常返回的真实模拟步骤。

新增Native pre/post配对记录和EventKind32；odd保存首次真实Player见证，post不重新获取拓扑。错误来源、线程/owner/loop不配对、重入、同loop第二scope、计数耗尽和丢失正常返回均关闭时钟；session终止取消当前候选。不会回滚Participant高水位后重用同一tick，也不吞原生EH或补调original。未新增Hook，Shared/Event布局与现有恢复路径保持。

tick成功仍指既定模拟边界返回。完整世界采集继续在后续owner/Room/tail/outer链完成后进行，odd仍是gap；collector失败或背压不能让scheduler等待。

## 子智能体结论的审阅

已按用户指定使用 GPT-6 Luna / max。主智能体重新读取当前及修改前源码、tx-01-66原始指令和CFG资料，逐项审阅其结论，补齐：真实odd owner保存、合法未准入pre/post区分、停用后的pending取消和outer缺口检查。其答复中的旧行号按分析快照理解，没有直接当作当前代码事实。既有证据足够，本轮无需新增IDA调用。见[逐项审阅与限制](tx-01-90-static-review.md)。

## 离线验收证据

统一入口实际运行生产 Clock、AuthoritySession、V2 codec和Participant，不使用替身时钟。正常返回、暂停/loading/menu状态和绑定都是显式场景输入，绑定仅为不可解引用的自有标记。

覆盖两路径共享递增、空转/loop跳跃、暖机与阻断无提交、A/B缺输入仍推进、B单独退休、generation改变但正常模拟提交、终止/失败取消、异常/缺失返回、重复返回/同loop第二scope/重入、错误Room来源/FSM/线程/owner/loop、main-thread身份变更及u64末端耗尽。边界用例通过构造离线Clock初值完成；Native仍从0开始，没有游戏内计数修改。

- [机器结果](../../../../artifacts/cs-t0/tx-01-90-authority-tick-01/result.json)
- [全部实际事件](../../../../artifacts/cs-t0/tx-01-90-authority-tick-01/events.jsonl)、[实际输入命令](../../../../artifacts/cs-t0/tx-01-90-authority-tick-01/commands.txt)
- [实现前清单副本](../../../../artifacts/cs-t0/tx-01-90-authority-tick-01/sources/cs-t0/testdata/tx-01-90-authority-tick.json)
- [源码及主智能体审阅快照](../../../../artifacts/cs-t0/tx-01-90-authority-tick-01/sources/)、[Native与离线二进制/构建日志](../../../../artifacts/cs-t0/tx-01-90-authority-tick-01/build/)、[二进制出处](../../../../artifacts/cs-t0/tx-01-90-authority-tick-01/binary-provenance.json)

纯编译使用VS2022/v143、Hostx64→Win32 Release；唯一离线进程约0.075秒。无游戏查询、输入、注入、实际暂停/过房、Hook安装/恢复、完整运行时异常或新实机tick见证；不能把此PASS称为Native接入实机验证。操作3–9不在本轮实施，也未重跑操作1的旧验收。

## 复现入口

使用全新构建和artifact目录，不覆盖或自动重复本轮验收：

```powershell
& ./cs-t0/build-authority-tick.ps1 -OutputDirectory out/cs-t0/NEW-TICK-BUILD
python cs-t0/accept-authority-tick.py `
  --build-dir out/cs-t0/NEW-TICK-BUILD `
  --artifact-dir artifacts/cs-t0/NEW-TICK-ACCEPTANCE
```

准备期间先写失败清单与场景，再实现/静态阅读/纯编译，最后只执行上述统一入口一次；没有实现后补写单元测试，没有中途试跑或哈希校验。tx-01-89及更早历史事实和失败保持原样。
