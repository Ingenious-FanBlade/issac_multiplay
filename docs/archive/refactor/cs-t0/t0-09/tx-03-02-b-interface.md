# tx-03-02：T0-09-B 接口、线程边界与 F0 runner

日期：2026-10-20（Asia/Shanghai）。状态：**OFFLINE F0 41/41 PASS；真实 host+VM P1 NOT_RUN**。本文把
[tx-03-01 设计](tx-03-01-design.md)中 T0-09-B 的草案冻结为实现接口；流程以
[现行项目规范](../../project-conventions.md)为准。本文先于生产代码登记，不运行游戏、构建或测试。

## 1. 本轮目标与非目标

### 1.1 目标

1. 一个进程只持有一个独立 `Endpoint` 角色：A=Authority、B=Remote；不得再复用
   `ProtocolLink` 中“同进程同时持有 A/B + logical transport”的拓扑。
2. 新增 Wire IPC v1：Native/端point driver 与同 OS 控制器之间只搬运原样 ICT8 packet。
3. 新增 host UDP relay：单 datagram 单 packet、原字节转发、固定 seed 故障决策、独立 relay log。
4. 新增 T9C1 TCP JSONL control plane：有界消息、严格 run/role/sequence/ack 校验、
   READY/ARMED/RUN_START/heartbeat/STOP 屏障。
5. 新增 host orchestrator 与可执行 F0 runner：启动 relay、两端独立 endpoint controller、
   屏障、10,000 tick 目标、正常 stop、artifact 汇总。
6. 新增双进程离线 endpoint 场景，实际使用两个 OS 进程、两个 Wire IPC mapping、两个 UDP
   端点和一个 relay，以验证 B 的传输/控制基础；它不冒充真实游戏/VM P1 实机结果。

### 1.2 非目标

- 不实现 F1–F5 故障矩阵、observer-simulator、双端最小状态比较或 first-divergence；这些属于
  T0-09-C。
- 不实现 C0/C1/C3、多 seed、Steam、Original Online、Isaac Sync、snapshot/rollback/Replica。
- 没有用户实际操作授权时不启动/注入真实游戏，不自动重跑，不把离线场景通过写成 host+VM PASS。
- 不计算或校验构建、文件、状态、artifact 或 world hash。

## 2. 组件和所有权

```text
t09 f0 orchestrator (host)
  ├─ relay process           UDP A/B faces, immutable packet bytes, relay/ logs
  ├─ endpoint A process      Wire IPC A <-> UDP A, T9C1 client, controller logs
  │    └─ driver A           independent Authority Endpoint, canonical admission/tick
  └─ endpoint B process      Wire IPC B <-> UDP B, T9C1 client, controller logs
       └─ driver B           independent Remote Endpoint, B canonical input producer
```

- **driver** 才拥有 `protocol::Endpoint`、canonical encode/admission、tick 和 packet commit。
- **endpoint controller** 只拥有 mapping 视图、UDP socket、wire 复制、control client 和日志；
  绝不写 Player/device/Lua/world/canonical queue。
- **relay** 不创建 session、不生成/合并/修改 packet、不重算 CRC；只按长度边界和已配置决策
  原样转发。
- **orchestrator** 不持有游戏对象，不解析世界状态；只做进程、端口、T9C1 屏障、停止和 artifact。

## 3. T9M1 run manifest v1

严格 JSON；未知字段拒绝。端口 `0` 表示由 relay 选择临时端口，实际值写入
`relay/ready.json`。公共 manifest 不写用户名、SteamID、绝对 Steam 路径或原始 handle。

```json
{
  "magic": "T9M1",
  "version": 1,
  "run_id": "tx-03-02-F0-offline-01",
  "case": "F0",
  "repeat": 1,
  "build_label": "t0-09-b-local-build",
  "wire": {
    "version": 2,
    "session": "32hex",
    "run_nonce": "32hex",
    "authority_nonce": "32hex",
    "remote_nonce": "32hex",
    "clients": [1, 2]
  },
  "endpoints": {
    "a": {"os": "host", "role": "authority", "artifact_subdir": "host"},
    "b": {"os": "host", "role": "remote", "artifact_subdir": "vm"}
  },
  "script": {
    "id": "t09b-f0-minimal-v1",
    "first_tick": 0,
    "last_tick": 10000,
    "lead_ticks": 3,
    "tick_interval_ms": 2
  },
  "relay": {
    "a": {"bind": "127.0.0.1:0"},
    "b": {"bind": "127.0.0.1:0"},
    "faults": {
      "version": 1,
      "seed": 131080,
      "base_delay_ms": 0,
      "jitter_ms": 0,
      "loss_ppm": 0,
      "duplicate_ppm": 0,
      "reorder_ppm": 0,
      "reorder_extra_ms": 20,
      "outage_after_ms": 0,
      "outage_ms": 0
    }
  },
  "control": {"bind": "127.0.0.1:0"},
  "artifact_root": "artifacts/cs-t0/tx-03-02-e2e-01",
  "user_preconditions": {
    "safe_run": true,
    "only_a": true,
    "no_original_online": true,
    "real_game_authorized": false
  }
}
```

- `session`、A/B nonce 均为非零、互异、本次 run 新鲜；`clients[0]!=clients[1]`。
- 本 runner 对 `case` 仅接受 `F0`。故障字段必须合法；B 离线验收使用全零 profile。
- `user_preconditions.real_game_authorized=false` 时 orchestrator 只允许离线 driver，不得启动游戏。

## 4. Wire IPC v1

### 4.1 mapping 名和尺寸

- 名称：`Local\IsaacCsT0.Wire.<run_nonce_hex>.<A|B>`。
- `run_nonce` 是 manifest 的 16 字节；A/B 各自独立 mapping，role 必须匹配。
- header 320 字节；随后两个方向各 128 槽，槽 1248 字节。
- mapping 总尺寸：`320 + 2 * 128 * 1248 = 319808` 字节。

### 4.2 header

所有整数小端。标记为 atomic 的 64 位字段按 Windows x86/x64 对齐，使用
`Interlocked*` 或等价的 acquire/release 操作。

| offset | bytes | 字段 |
| ---: | ---: | --- |
| 0 | 4 | magic `T9WI` (`0x49575439`) |
| 4 | 2 | version=1 |
| 6 | 2 | header_size=320 |
| 8 | 4 | mapping_size=319808 |
| 12 | 4 | role：A=1、B=2 |
| 16 | 16 | run_nonce |
| 32 | 16 | session |
| 48 | 16 | authority_nonce |
| 64 | 16 | remote_nonce |
| 80 | 16 | clients[2] |
| 96 | 32 | build label |
| 128 | 2 | input schema=2 |
| 130 | 2 | state schema=1 |
| 132 | 2 | wire version=2 |
| 134 | 2 | reserved=0 |
| 136 | 4 | run_state：Created=0、Bootstrap=1、Running=2、StopRequested=3、Stopped=4 |
| 140 | 4 | driver_state bitmask |
| 144 | 4 | driver_error |
| 148 | 4 | controller_error |
| 152 | 16 | produced[2] |
| 168 | 16 | consumed[2] |
| 184 | 16 | overwritten[2] |
| 200 | 8 | heartbeat_tick |
| 208 | 8 | run_start_tick |
| 216 | 8 | stop_tick |
| 224 | 8 | local_generation |
| 232 | 8 | target_tick |
| 240 | 4 | driver_pid |
| 244 | 4 | driver_flags |
| 248 | 8 | driver_heartbeat_qpc |
| 256 | 4 | closed |
| 260 | 4 | controller_state bitmask |
| 264 | 8 | controller_heartbeat_tick（orchestrator→B controller 的最新 TICK_HEARTBEAT） |
| 272 | 48 | reserved，必须为 0 |

`driver_state`：Open=1、Handshake=2、Established=4、Ready=8、Running=16、Stopped=32、Error=64。
`driver_flags`：TargetReached=1、ScriptComplete=2、RestoreComplete=4。
`controller_state`：MappingCreated=1、UdpReady=2、DriverSpawned=4、ReadySent=8、
ArmedSent=16、StopReceived=32、Stopped=64、Error=128。

### 4.3 slot

| offset | bytes | 字段 |
| ---: | ---: | --- |
| 0 | 8 | published（0 或本方向下一个 token） |
| 8 | 8 | token，从 1 单调递增 |
| 16 | 8 | generation |
| 24 | 8 | tick |
| 32 | 4 | direction：ToAuthority=0、ToRemote=1，必须与 ring 一致 |
| 36 | 4 | length，必须 64..1200 |
| 40 | 1200 | packet 原字节 |
| 1240 | 8 | reserved，必须为 0 |

### 4.4 ring 访问

- ring index 直接使用 protocol direction。A 只生产 ToRemote、只消费 ToAuthority；
  B 只生产 ToAuthority、只消费 ToRemote。access violation 一律 fail closed。
- producer：先检查 `produced-consumed<128`；满则增加 `overwritten`、设置本地 error/closed，
  不发布、不假装成功。然后清 `published=0`，写 metadata/packet，release fence，
  release-store `published=next_token`，再 release-store `produced=next_token`。
- consumer：读取稳定 `produced/consumed`；取 `expected=consumed+1` 与对应槽；
  acquire-load `published` 必须等于 expected，否则判定覆盖/损坏并 fail closed。复制并校验
  length/direction/reserved 后才 release-store `consumed=expected`。
- 空环返回 idle，不推进任何序号；close 后仍可按上述规则排空已发布数据，但不得接受新业务输入。
- controller/network worker 只做 `Read/Write` 的有界复制；Native/driver 主线程才解析 packet
  并触碰 canonical queue、scheduler、Player/Lua/world。

## 5. T9C1 control plane v1

TCP，每行一个 UTF-8 JSON object，单行最多 65536 字节（必须含换行；第 65537 字节出现即失败）。
未知 envelope/body 字段、未知 role/type、旧/重复 seq、错误 run/ack、超长 body 均拒绝；断连不重连。

```json
{"magic":"T9C1","version":1,"run_id":"...","role":"A","seq":1,"ack":0,"type":"READY","body":{}}
```

- `role` 只允许 `A`/`B`；每个 run 每个 role 只允许一条连接。
- `seq` 从 1 严格递增；`ack` 只能确认已经收到的对端 seq，且不得回退。
- allowed types：`READY`、`ARMED`、`RUN_START`、`TICK_HEARTBEAT`、`ARM_FAULTS`、
  `FAULT_TRIGGER`、`STOP_REQUEST`、`STOP_COMPLETE`、`RETIRE_COMPLETE`、`ERROR`。
  `ARMED` 来自 tx-03-01 §5.2 的启动屏障，附录 B 的允许列表遗漏该消息，本实现按主文冻结。
- `READY.body`：`phase`、`script_id`、`endpoint_state`、`artifact_writable`、
  `game_pid`、`creation_time_100ns`、`native_state`、`pair_ready`、`observer_ready`。
- `ARMED.body`：`endpoint_established`、`pair_ready`、`observer_ready`。
- `RUN_START.body`：`run_start_id`、`tick0`、`script_id`、`fault_profile`、`last_tick`。
- `TICK_HEARTBEAT.body`：`authority_tick`、`accepted`、`processed`、`target_reached`。
- `STOP_REQUEST.body`：`last_tick`、`reason`。
- `STOP_COMPLETE.body`：`status`、`endpoint_retired`、`driver_exited`、`ring_overwritten`。
- `RETIRE_COMPLETE.body`：`status`、`artifact_closed`、`process_alive`。
- `ERROR.body`：`code`、`message`、`tick`。
- control plane 不承载 canonical input 原文、世界状态、Player/device/Lua 指针、Steam identity。

## 6. UDP relay v1

- relay 分别 bind A/B face，学习对端最近 source address；在双方都出现前允许一个方向的少量
  handshake 缓存，按 FIFO 在地址已知后立即原样转发。
- 单 datagram 必须恰好 64..1200 字节；不拼接、不拆分、不改写。超界/空包记录拒绝且不转发。
- 每个方向独立 xorshift64* 决策流；seed 由 manifest seed 与方向常量派生。每个原始包只在
  receive 时做一次 loss/duplicate/reorder 决策，后续调度不重掷骰子。
- F0 profile 必须全零；B 的可执行 F0 验收只使用全零。非零 profile 的 delay/jitter/loss/
  duplicate/reorder 调度代码保留给 C，但 B 结果不声明 F1–F5 通过。
- relay log 记录方向、QPC receive/due/forward/drop、ordinal、decision、length、ICT8 前缀身份
  和 raw packet hex；不记录无关进程/身份信息。关闭时 flush 并写 stopped summary。

## 7. endpoint controller / driver 边界

- controller 创建 Wire mapping（create-new name）、初始化 manifest identity、启动 driver，
  然后打开已 `connect` 的 UDP socket 和 T9C1 连接。
- driver 只通过 mapping 交换 packet；controller 网络 worker 不解析 canonical 内容，也不
  写 queue/world。
- driver 在打开/握手/Established/Ready 时设置 driver_state；controller 只在所有本地前置
  条件满足后发 READY，Established 后发 ARMED。
- controller 收到 `RUN_START` 才原子设置 `run_state=Running` 与 `run_start_tick`；driver
  在此之前只做 handshake，不产生 script tick/input。
- controller 收到 `STOP_REQUEST` 后设置 `run_state=StopRequested`，等待 driver 设置
  Stopped 并退出；随后停止 UDP worker、排空 ring/日志、关闭 mapping，发送 STOP_COMPLETE
  与 RETIRE_COMPLETE。不得自动重启 driver、重注入或重跑 run。
- 离线 driver 是独立 OS 进程，使用生产 `Endpoint`/canonical codec/Participant；真实游戏
  driver 必须由后续 Native v7 接入，且只有实际执行后才可计入 host+VM P1。

## 8. F0 runner 语义

- 目标：连续 10,000 个 Authority gameplay tick；F0 relay 无配置 delay/loss/duplicate/reorder。
- A driver 仅在 RUN_START 后从 tick1 开始逐 tick：
  1. 主线程 drain Wire inbound packets，经 Authority Endpoint 解码；
  2. INPUT 记录进入 common canonical byte admission/Participant；
  3. 推进 Authority Participant/tick；
  4. 每个真实 tick 发布一个 ACK 和一个 STATE packet，更新 heartbeat_tick。
- B driver 在 handshake 完成且 RUN_START 后，依据 host orchestrator 经 B controller 写入
  Wire header 的 `controller_heartbeat_tick` 与固定 lead 生产 B canonical INPUT；ACK/STATE
  仅作独立 lane 观察，不决定 B script tick。sequence/target 由本地源单调生成，不补发过期
  edge、不滑动 target。
- orchestrator 只在 A/B 均 ARMED 后发 RUN_START；收到 A heartbeat 达到 last_tick 后发
  STOP_REQUEST；等待两端 STOP_COMPLETE/RETIRE_COMPLETE，再关闭 relay。
- 离线 F0 验收必须核对：10,000 tick 连续、A/B packet 方向与 identity、INPUT sequence/target
  唯一、relay 无未知消失/凭空产生、Wire ring overwritten=0、正常 stop/close。
- 离线通过只能标记 `offline_transport_p1`；本轮结果见
  [T0-09-B 结果](tx-03-02-b-results.md)：`artifacts/cs-t0/tx-03-02-e2e-09` 41/41 PASS。
  真实 host+VM 双游戏 P1 仍为 NOT_RUN，直到实际运行。
- 当前离线 driver 是独立 C++ 进程，使用生产 `protocol::Endpoint`、`Participant`、Wire IPC v1
  和 canonical byte admission；它不是游戏 Native v7，也不冒充 host+VM P1。控制面 heartbeat
  路径已在离线 B driver 中实际使用。

## 9. artifact

```text
artifacts/cs-t0/tx-03-02-e2e-xx/
  manifest.json
  source/ build/
  host/{endpoint.jsonl,wire.jsonl,udp.jsonl,control.jsonl,driver-events.jsonl,driver.*,result.json}
  vm/{...同 host...}
  relay/{ready.json,relay.jsonl,stopped.json}
  control/{control.jsonl,ready.json,run-start.json,stop.json}
  analyzer/{checks.jsonl,first-failure.json,result.json}
```

- 所有文件 create-new；失败保留到首个失败点和收尾状态；不覆盖历史 artifact。
- raw packet/事件使用有界逐行 flush；不得因空间不足静默丢证据。runner 启动前做保守磁盘预算，
  不足即 FAIL。公共 artifact 只保留 role/PID/creation time/build label 等去身份化字段。
- artifact 不保存完整 SteamID、用户名、绝对 Steam 路径、系统密钥、peer identity 或无关 handle。
