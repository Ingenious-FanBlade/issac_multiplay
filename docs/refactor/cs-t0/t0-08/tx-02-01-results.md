# tx-02-01：日志修复后的唯一整体 E2E

2026-10-20，Asia/Shanghai。**原协议 trace 溢出及恢复产物缺失问题在本次实机已消除，但整体结果为 FAIL：23/30 项通过，controller exit1，未自动重跑。** [修复设计与实施前失败方式](tx-02-01-log-recovery.md)保留；旧 tx-02-00 三次失败原样保留。

用户明确要求修复后继续测试，并在本会话确认完整退出旧常驻进程、重新启动隔离安全新局并保持前台。本轮只执行一次统一入口，修改期间未运行测试或产物，没有新增单元测试。

## 修复与本次通过的事实

- 协议环仍为 256 槽、wire v2 / IPC v6。稳定槽复制后立即发布 consumed，hex/JSON及写盘在本地有界副本上进行；日志改为缓冲写入、每轮flush，协议轮询2ms。
- 实机保存 **4,007/4,007** 条连续 trace，`produced=consumed=4007`、`lost=0`，完整Send/Enqueue/Deliver/Receive/Admission及固定seed故障模型配对通过，没有再次发生 `protocol trace overflow`。
- 保存 **17,582** 条真实事件、**271** 个 commit。正常停止 helper 成功，`nativeRestored=true`、`pinned=true`，**70个不同目标逐槽 before=after**，另有恢复汇总事件。游戏保持同一进程存活；没有强制卸载、重注入或杀进程。
- 恢复 JSON 与原始控制器结果正常落盘，`controllerError=null`、`cleanupErrors=[]`，没有旧 FileNotFoundError。历史真实溢出/缺恢复文件的只读副本由同次入口验证，报告器输出明确缺文件失败并保留 `protocol trace overflow`，该预期失败场景本身检查通过。
- 离线 corpus/生产生命周期 **19,022/19,022 PASS**，17,373 条向量。实机握手、冻结身份、A/B启动意图、每条B准入来自wire、223次接受/486次幂等重复、loss27包/outage60包、dup/delay、B outage tick74..83中性及恢复、3/3 Drop边沿exactly-once均通过。

## 七个未通过项

| 检查 | 实际原因/依据 |
| --- | --- |
| L01.process-and-config | IPC v6成立，但末态`nativeError=7`，不是配置或PE校验失败。 |
| L07.controller | 正常stop已发送，控制器最终返回FAIL/exit1。 |
| L06.wire-ack-matches-every-real-receipt | 271个commit只有270条ACK Send，缺tick271；已发送ACK自身字段没有逐条比较错误。 |
| L06.explicit-state-gaps-match-every-real-commit | 同样只有270条STATE Send，缺tick271；已发送STATE自身字段没有逐条比较错误。 |
| L04.A-and-clock-continue | outage内tick正常继续，但A Resolve为中性，不能证明A实际动作。 |
| L04.A-real-partial-position-change | outage内取得5个A位置样本但值相同；不能把工具成功返回当作游戏动作生效。 |
| L07.no-failure-and-stable-lifecycle | EventKind16在tick271记录detail7，即`Error::Publication`。 |

A物理输入工具记录初始4步及连续64步W/D/S/A均成功（同一PID/创建时间），但原生 **272条PhysicalSample及271条A Resolve全部移动量为0**。当前证据不能确定焦点变化、键位配置或输入采样路径中的具体原因；不伪造A持续动作通过，也不据此自动追加输入实验。

停止命令记录为elapsed4615ms、completed tick269；tick271先记录commit和A/B回执，再记录Publication失败。Shutdown静态顺序为先`world::Close()`，再请求stop；TopExit仍可能处理在途post并要求世界身份与协议publication。**停止期间世界关闭与最后一次发布的交错是静态风险，尚未通过额外实验确认根因**。不删除末尾commit、不放宽逐commit全覆盖以改成PASS。

本次world记录为1 partial、136 gap、135标为complete的capture，backpressure=0、consumer健康关闭；STATE/真实world身份配对270/270通过。仅描述本进程原始观察，不外推跨进程相等、snapshot apply、Steam/socket、双游戏或可玩Replica。

## 复核与复现产物

- [统一结果及全部30项检查](../../../../artifacts/cs-t0/tx-02-01-e2e-01/result.json)、[实机逐项结果](../../../../artifacts/cs-t0/tx-02-01-e2e-01/live-checks.json)。
- [控制器结果](../../../../artifacts/cs-t0/tx-02-01-e2e-01/capture/result.json)、[70槽恢复记录](../../../../artifacts/cs-t0/tx-02-01-e2e-01/capture/restoration.json)、[helper日志](../../../../artifacts/cs-t0/tx-02-01-e2e-01/capture/helper.log)。
- [真实事件](../../../../artifacts/cs-t0/tx-02-01-e2e-01/capture/events.jsonl)、[原始协议轨迹](../../../../artifacts/cs-t0/tx-02-01-e2e-01/capture/protocol.jsonl)、[包配对摘要](../../../../artifacts/cs-t0/tx-02-01-e2e-01/packet-pairing.json)、[stop时点](../../../../artifacts/cs-t0/tx-02-01-e2e-01/capture/operations.jsonl)。
- [实际调用与配置](../../../../artifacts/cs-t0/tx-02-01-e2e-01/invocation.json)、[物理工具请求与返回](../../../../artifacts/cs-t0/tx-02-01-e2e-01/physical-input-tool-result.json)、`failure-report-fixture/live-checks.json`、`sources/`、`build/`、`corpus/`和全部离线原始记录。

最终纯编译：`out/cs-t0/tx-02-01-build-02`，Rust/Native/Helper/Scenario成功；首次纯构建的SDK沙箱访问失败另存`tx-02-01-build`，未执行任何产物。唯一统一命令：

```powershell
python cs-t0/accept-protocol-e2e.py --transaction tx-02-01 --build-dir out/cs-t0/tx-02-01-build-02 --artifact-dir artifacts/cs-t0/tx-02-01-e2e-01 --prior-failure-artifact artifacts/cs-t0/tx-02-00-e2e-03 --authorization "User requested fixing protocol trace overflow and continuing E2E; fresh isolated safe game confirmed in this chat."
```

复现时使用新输出目录，并先准备不含常驻旧DLL的安全进程；上面的命令是本次实际调用记录，不表示已再次执行。本轮结束后只阅读本次保存日志并整理结果，没有追加测试。当时T0-08仍为 **IN_PROGRESS**；后续需要修正停止/post publication顺序并定位A输入零值，按新的实质推进和一次整体入口验收。需要用户操作时先询问。
