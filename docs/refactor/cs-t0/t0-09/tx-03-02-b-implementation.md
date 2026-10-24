# tx-03-02：T0-09-B 离线传输/控制基础实现

日期：2026-10-20（Asia/Shanghai）。状态：**OFFLINE F0 FOUNDATION PASS / 41/41；真实 host+VM P1 NOT_RUN**。
本文描述 T0-09-B 当前实现的边界、运行语义和唯一下一步入口。验收结果另记，不以本文代替运行证据。

## 1. 本轮交付

| 组件 | 实现位置 | 当前事实 |
| --- | --- | --- |
| 独立 endpoint | `native/scenarios/t09_endpoint_main.cpp` | 每个进程只持一个生产 `protocol::Endpoint`；A=Authority、B=Remote。 |
| Wire IPC v1 | `native/include/isaac_cs_t0/t09_wire_ipc.hpp`、`native/src/t09_wire_ipc.cpp`、`src/t09/wire.rs` | 320 字节 header + 两方向各 128 个 1248 字节槽；跨语言 offset/size static assert 与 Rust 常量一致。 |
| UDP relay | `src/t09/relay.rs` | A/B 两个 UDP face、单 datagram 单 packet、原始字节转发、固定 seed 决策入口、handshake 地址缓存和 relay JSONL。 |
| T9C1 control | `src/t09/control.rs` | 有界 TCP JSONL、严格 envelope/body/run/role/seq/ack、READY/ARMED/RUN_START/TICK_HEARTBEAT/STOP/RETIRE。 |
| endpoint controller | `src/t09/endpoint.rs` | 独立 mapping/UDP/control、driver 进程、Wire 复制、日志、正常 stop；不解析/改写 canonical payload。 |
| host orchestrator + F0 | `src/t09/orchestrator.rs` | 启动 relay 与两个 endpoint controller、双 ARMED 后 RUN_START、10,000 tick 目标、heartbeat 转发、STOP 屏障和 artifact。 |
| 统一验收 | `accept-t09-b.py`、`build-t09-b.ps1` | 一次纯编译 + 一次离线 F0 + 一次 analyzer；FAIL 不自动重跑。 |
| analyzer | `report_t09_b.py` | 逐 tick ACK/STATE、INPUT sequence/target/edge、relay raw Counter、control seq/ack/barrier、Wire overwrite 和 stop/retire 检查。 |

## 2. 线程与进程边界

```text
orchestrator process
  ├─ relay process
  │    └─ UDP bytes only; never canonical decode/apply
  ├─ endpoint A process
  │    ├─ controller thread: Wire consumer/producer <=> UDP, T9C1, artifacts
  │    └─ driver A process: production Authority Endpoint; Participant/byte admission/tick
  └─ endpoint B process
       ├─ controller thread: Wire consumer/producer <=> UDP, T9C1, artifacts
       └─ driver B process: production Remote Endpoint; canonical B input source
```

- controller 只做有界复制、UDP、control 和日志；不触碰 Player/device/Lua/world/canonical queue。
- driver 的主线程拥有 Endpoint 解码/commit、`Participant` byte admission 和 Authority tick。
- A driver 每个真实离线 tick 发布一份 ACK 和一份 STATE gap observation；不会等待 B 输入。
- B driver 只在 `RUN_START` 后使用 control heartbeat 与固定 lead 生成 canonical INPUT；
  ACK/STATE 作为独立 lane 观察，不驱动本地 script tick。
- relay 不生成、改写、合并或拆分 packet，也不重算 CRC。

## 3. 当前 manifest / driver 选择

`IsaacCsT0T09 f0` 只接受 `case=F0` 且 fault profile 全零；并且拒绝
`user_preconditions.real_game_authorized=true`，因为真实游戏/Native v7 注入尚未接入本 runner。
当前 `--scenario` 指向独立 C++ endpoint driver。manifest 的 B endpoint 标记为
`host-offline-process`，artifact 子目录仍为 `vm` 以固定分析路径。

这不是设计中的 host+VM 双真实游戏 P1。真实主机/VM、Native 安装、exact pair 和游戏进程
存活必须在后续获得实机授权后单独执行；离线通过不会写成 T0-09 PASS。

## 4. 启动、tick 和停止顺序

1. orchestrator 启动 relay，等待 `relay/ready.json` 得到实际 A/B UDP 端口。
2. orchestrator bind T9C1 listener，启动两个 endpoint controller。
3. controller 创建各自 Wire mapping、启动独立 driver；driver 打开 mapping 后设置 Open。
4. A/B Endpoint 交换 HELLO；A 发首 ACK；两端进入 Established，driver 设置 Ready。
5. controller 发 READY；看到 Established+Ready 后发 ARMED。
6. orchestrator 仅在 A/B 均 ARMED 后向两端发 RUN_START。
7. A driver 从 tick1 连续推进到 manifest `last_tick`；每个 tick 写 ACK+STATE 和 heartbeat。
8. endpoint A controller 只有在 ACK/STATE 两条 wire lane 都已转发到最新 tick 后才发目标
   heartbeat；orchestrator 将该 heartbeat 转给 B controller，B driver 以
   `controller_heartbeat_tick + lead_ticks` 生产 B INPUT。
9. A target_reached 后 orchestrator 保留 400 ms 在途排空窗口，再发 STOP_REQUEST。
10. controller 设置 StopRequested；B 最多等待 500 ms 让最后 ACK/STATE lane 到齐；driver
    正常 retire 并设置 Stopped；controller 排空 Wire/UDP、关闭 mapping、发 STOP_COMPLETE
    和 RETIRE_COMPLETE。
11. orchestrator 等待双端 retire、controller 进程退出，写 relay stop file，等待 relay 正常
    flush/exit，最后写 `orchestrator-result.json`。

## 5. artifact 与失败保留

统一入口写入 host/vm/relay/control/analyzer/source/build 目录。所有 create-new；没有 hash。
离线 PASS 时顶层结论只能是 `OFFLINE_PASS`，并显式保留：

- `t0_09_b = IN_PROGRESS`（真实游戏 Native v7 接入未完成）；
- `realHostVmP1 = NOT_RUN`；
- 本次 scope 仅为 offline two-process transport/control F0。

失败时保留 staging work、controller stdout/stderr、首个错误和 analyzer 失败项；不自动
重跑或换编号重复同一验收。

## 6. 唯一验收入口

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-01 `
  --authorization "user requested T0-09-B offline foundation in this chat"
```

该入口自行完成纯编译、一次离线 F0 runner 和分析。有效执行结果为
`artifacts/cs-t0/tx-03-02-e2e-09`，analyzer **41/41 PASS**，见
[T0-09-B 结果](tx-03-02-b-results.md)。真实游戏、VM、注入和 host+VM P1 仍未执行。
