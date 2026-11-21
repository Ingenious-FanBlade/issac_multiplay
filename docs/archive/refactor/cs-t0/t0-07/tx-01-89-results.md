# tx-01-89：操作 1 离线验收结果

2026-10-19（Asia/Shanghai）。**T0-07 操作 1 已实现，唯一离线统一验收 PASS：33 个场景、302 条生命周期命令、303/303 检查通过，进程退出 0。** 其中一项检查输入/输出条数及退出码，其余302项逐命令检查。没有自动重跑，没有执行游戏、注入、单元测试或哈希校验。

## 已交付行为

- 生产 AuthoritySession：Dormant → Negotiating → Warmup → Active → Retiring → Retired，未退休状态故障进入 Failed。一次冻结 session 和 A/B ClientId→ordinal，无 reset；两个终态均不复活，首个失败原因不被后续清理覆盖。
- Native 在 Hook 可见前完成身份及 Participant 准备、发布 Warmup。Warmup 保留绑定/原生 Join，仅首次完整 A/B 就绪进入 Active；输入 producer 不消耗脚本或 sequence。Active 不因临时解绑或 B 单独退休重新暖机。
- 附着 session 的生产 Participant 在非 Active 时拒绝命令生成/Submit/Begin，gameplay Read 中性且不提交 previous；旧队列不能在终态继续消费。重复 Initialize 不重置输入序号、队列或已退休的 Participant。
- 主线程正常停止/run teardown 先 Retiring 再沿用 B/A 清理；原恢复线程排空和恢复后才 Retired。恢复失败保持 Failed；没有把原生移除操作转移到 helper 线程。
- 新事件 Kind 31 保存状态、原因及冻结身份，不改变 Shared/Event 的尺寸或旧 state 字段。

源码、边界和静态依据见[实现说明](tx-01-89-authority-session.md)及[GPT-6 Luna / max 分析](tx-01-89-static-analysis.md)。本次不需要新增 IDA 分析。

## 实际覆盖与证据

统一入口运行生产 FSM、V2 codec、Participant 的完整离线生命周期；不重新实现一份替身 FSM。覆盖正常 A/B 入队/Resolve/查询及退休、暖机准入、非法身份、身份冻结、各初始阶段失败、当前/排队输入在终态失效、run teardown、控制器丢失及恢复失败的逻辑转换、B 单独退休、代际重绑、重复初始化，以及16次失败/stop并发顺序。并发场景只访问自有原子会话状态，不访问游戏对象。

- [机器结果](../../../../../artifacts/cs-t0/tx-01-89-authority-session-01/result.json)
- [全部实际事件](../../../../../artifacts/cs-t0/tx-01-89-authority-session-01/events.jsonl)与[输入命令](../../../../../artifacts/cs-t0/tx-01-89-authority-session-01/commands.txt)
- [实现前场景清单副本](../../../../../artifacts/cs-t0/tx-01-89-authority-session-01/sources/cs-t0/testdata/tx-01-89-authority-session.json)
- [源码快照](../../../../../artifacts/cs-t0/tx-01-89-authority-session-01/sources/)、[构建日志和 Native DLL](../../../../../artifacts/cs-t0/tx-01-89-authority-session-01/build/)、[二进制出处](../../../../../artifacts/cs-t0/tx-01-89-authority-session-01/binary-provenance.json)

Native 与离线程序均使用 VS 2022/v143、Hostx64→Win32 Release 直接纯编译并链接成功。唯一离线进程墙钟约0.069秒；stderr 无运行异常，执行器 errors=[]。

**本次绑定是不可解引用的自有标记，控制器失联/恢复结果是显式场景输入。真实 Hook 安装、Join、主线程时序、控制器实际退出及 Hook 字节恢复未运行，不能把此 PASS 称为新的实机证据。** 操作 2–9 未在本次实施；也不宣称整个 T0-07 完成。

## 可重复入口

先纯编译（已有默认构建目录时指定新 OutputDirectory），再由一次入口生成全新的 artifact-dir；不得覆盖本次材料，也不将复现自动追加到本次验收次数。

```powershell
& ./cs-t0/build-authority-session.ps1 -OutputDirectory out/cs-t0/NEW-SESSION-BUILD
python cs-t0/accept-authority-session.py `
  --binary out/cs-t0/NEW-SESSION-BUILD/AuthoritySessionScenario/bin/IsaacCsT0.AuthoritySessionScenario.exe `
  --artifact-dir artifacts/cs-t0/NEW-SESSION-ACCEPTANCE
```
