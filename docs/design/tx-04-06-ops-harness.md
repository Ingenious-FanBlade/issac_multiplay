# tx-04-06：3.4 / 3.5 Hook、命令、界面与远端编写

日期：2026-10-25。承接 [game 编写记录](tx-04-06-game.md)，完成[实现方案](tx-04-06-implementation-plan.md)第 3.4、3.5 节的接线与代码编写，仍属同一次 tx-04-06 推进。本轮没有部署、启动游戏、运行网络程序、执行 E2E 或新增单元测试；唯一整体验收尚未执行。下文描述实现，不表示运行时已通过。

## Hook 与补丁

- 复核并沿用清单中的 Join pass、4 处 CALL shim 和 7 处 K 通知/UI 分派；包括第三处 ECX 保留、完整 GP/EFLAGS/FXSAVE 环境与 CALL 拒绝清栈合同。首次 Manager 主线程核对并安装，直到进程结束常驻；失败关闭能力，不恢复字节。
- mode trace 增加 site、original、result 与该次原调用数；创建点记录允许创建的唯一角色参数，完整返回仍由 Join 结果确认。设备记录 allocated、constructed、initialized、registered、retiring、delete，不在日志中把 NEVER 标记当作完整内存安全证明。
- 为 V12 增加 5 个只读观测入口，均在同一 `hooks.json` 注册：官方 `PersistentGameData::TryUnlock=0x503930`、`IncreaseEventCounter=0x503A50`、`SaveLocally=0x503460`、`SaveToSteamCloud=0x502F00`，以及自有 thiscall 定义的 `SaveButtonMaps=0x42AC90`。前四项使用官方 SDK 的定义与 ABI；最后一项的无栈参数、int 返回及入口字节来自既有 ST8 原始导出 `st2-st9-primary-ida.json` 的 `0x42acd5` 项，不新增逆向实验。
- 当前清单为 **33 个入口、8 个设备槽、12 处字节补丁**（Odd 1 + Join/K 11）。5 个持久观测 Hook 也常驻并保持 original exactly-once；不 Hook 原版在线模块，也不修改游戏 vtable。
- `game/persistence.cpp` 只读记录 readonly、dirty、fileLoaded、相应成就位或计数的前后值与正常返回；K 注册/移除、Join、B 移除分别记录 operation，整个进程中的相关入口均保留。没有翻转全局 readonly、回滚状态或抑制原生保存。此处证据仅覆盖 ST8 已定位的入口，不声称穷尽任意第三方代码；首版仍不支持第三方 gameplay mod。

## 命令、配置与诊断

`PlaceholderSession` 已移除。`cs host <port> [character]` 校验并排队，在下一次最外层 Manager 入口调用唯一 `StartHost`；character 默认为 0，非负 int32，原生可选性仍由 Join 的角色表确认。`cs stop` 对 Host/Solo 都调用 `StopSession`，`cs solo start/stop` 保留。待执行的启动请求也参与互斥；stop 可在能力失败时排队清理。

schema=1 的 `host` 增加 `bind_address`（默认 `0.0.0.0`，严格 IPv4 文本）、`peer_timeout_ms`（默认 3000，允许 500–60000 ms，须大于 250 ms 心跳）。样例默认关闭；E2E 部署配置使用 `127.0.0.1`。网络线程只使用配置值与队列，不访问 Lua/游戏对象。

`cs status` 补齐 peer、K、Join、B、players、RTT、网络错误分类、收发字节、副本/stale、队列丢弃、Bye 尝试/成功、线程停止确认和 armed。`players` 读取当前世界，退休后不沿用旧拓扑中的 2。`net_dropped` 汇总 foreign、stale 和双队列丢弃，duplicate 单列且不当错误。`net` trace 仅记录状态变化/退休摘要；`ack` 每 30 次主线程 publication 汇总一次。tick 对 A/B 各输出一条完整汇总；B submit 也带 session_generation，避免跨会话同序号误配。

纯 UI mod 删除占位提示，显示 peer、RTT、B 绑定、Join、会话错误与 terminal。Hosting/Warmup/Active 提供停止按钮，Idle 提供端口与启动按钮；角色仍不在 UI 选择。保留无认证、仅可信网络提示，不增加 gameplay 回调。

## Rust 远端与夹具

`tools/harness/src/remote/` 独立实现四类 ICSN v1 codec、Remote Endpoint 与 UDP 脚本客户端，不共享 C++ codec：

- Hello 每 250 ms 重发原字节，首次发送开始计握手超时；First Ack 专用缓存独立于 Progress，后到副本不回退进度。
- Ack 严格检查身份、协商、reserved、CRC、状态及进度；合法旧 Ack 为 stale，不续命、不交付。Bye 固定同一包原字节尝试 3 次；收到 Bye 后只在本轮有界接收预算内计副本，不回包。
- generation 增加时清空旧样本、脚本、边沿和冗余，保留会话 command sequence；generation=0/Warmup 或 completed 停顿时发送中性心跳。墙钟不生成模拟 tick，脚本只相对 Ack.completed。
- 每次新 completed 采集，target=Ack.completed+lead（默认 2），带最多 3 条旧命令冗余。记录完整收发字节、解码字段、首发/重发/副本/stale、脚本安装点和最终计数。支持主动 Bye 或停止发送制造超时。
- 主线程用有界消息切换脚本，远端线程独占 socket/Endpoint，错误路径 Drop 请求停止并 join；不控制游戏。

测试 Lua 只在带 tx-04-06 临时配置时于真实主菜单执行一次 Host。只读观测扩展到所有 Player，保留 A 旧字段供 tx-04-05 使用；每条 JSON 记录带唯一 token/serial/loop。第一/第二次 F8 继续分别跳更新与切房，不使用原版截图键 F9。

tx-04-05 的 V2 S10 按场景记录的 `s10_key` 判方向，缺方向字段不能通过；旧失败报告原样保留，不重算历史结果。

## E2E 入口与产物

第 13/14 节的 S0–S11 / V1–V12 已编写为一个入口、单次游戏启动、三轮 Host，失败不自动重跑。整体验收前需按现行规范告知用户：本地远端客户端与 harness 会接管键盘数分钟，SkipIntro=1 时不需手动跳动画，启动前停止手动操作。编写本身没有执行该入口。

```powershell
pwsh -NoProfile -File scripts/build.ps1
# 全目标结束并安排实机配合后，唯一整体验收入口：
out/repentogon/harness.exe e2e tx-04-06 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-06-e2e-01
```

纯网络脚本入口（使用已存在的 Host，不启动游戏；不计作另一个授权的验收批次）：

```powershell
out/repentogon/harness.exe remote --host 127.0.0.1:27090 --script tools/harness/scenarios/tx-04-06-remote.json --out artifacts/repentogon/remote-manual-01
```

E2E 产物包括场景与源码快照、部署清单、控制口日志、三轮原始包日志、DLL trace、Lua 观测、步骤、截图、存档快照/逐字节变化、`tick-evidence.json`、`persistent-evidence.json` 与 V1–V12 JSON/Markdown 报告。归约要求实际证据；缺失字段或未完成场景记录 NOT_RUN/FAIL。收发计数要求回环下与 DLL 退休摘要对齐，不能把重复/stale 当协议错误。V12 联合过程记录与既有 D18/D20 存档基线，不以最终 dirty=0 推断安全。

## 构建与限制

Win32/v143/Release DLL 与 i686 Rust harness 纯编译、链接成功，最终日志为 [build-04](../../artifacts/repentogon/tx-04-06-ops/build-04.log)，失败与前序日志也保存在 [3.4/3.5 构建目录](../../artifacts/repentogon/tx-04-06-ops/README.md)。Rust 最终无 warning；全量 DLL 构建的 SDK/RapidJSON/生成 wrapper 既有提示如实保留。构建不运行测试。

运行时补丁冲突、并发异常、特殊角色与完整副作用窗口仍须由 tx-04-06 的一次整体验收观察；本轮没有把这些未验证项改为已通过。tx-04-07 的 relay/故障注入、B 死亡复活、异常 Baby Join、跨房射击等范围没有扩张。
