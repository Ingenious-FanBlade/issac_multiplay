# tx-03-03：T0-09-B 真实 host+VM P1/F0 接入

日期：2026-10-20（Asia/Shanghai）。状态：**IMPLEMENTED；首次 LIVE F0 FAIL 已修复；最新离线回归 41/41；等待双端重启后新 live run**。
本文件在实机操作前登记接入边界和操作前提，不代表 P1 PASS。

## 1. 用户确认的环境与授权

- Host：本机 Windows，网络地址 `192.168.137.1`。
- VM：`15468`，地址 `192.168.137.148`。
- VM 共享 host 目录：`D:\projects\isaac_hook_reserach\cs-t0\vm_share`。
- VM 内共享路径：`\\tsclient\D\projects\isaac_hook_reserach\cs-t0\vm_share`。
- RDP 会话在整个实机窗口保持连接、不锁屏、不切换。
- Host 防火墙允许 T9 UDP relay/TCP control；VM 由同一 RDP 控制台操作。
- 两端同一 approved build；角色以撒；seed `PY1JDB20`；初始房间。
- 用户已确认 Native/Helper 注入、owned virtual B、UDP relay/TCP control、只读采样和正常
  stop/restore 的实机授权。
- 只能在用户收到实机开始通知并确认两游戏已准备好后执行；不自动运行。

## 2. Endpoint/Native 接入

### 2.1 Authority A

新增：

- `SharedV7`：完整 v5 event/script/input-config 前缀 + `T09AuthorityConfig`。
- `AuthorityWireLink`：Authority `protocol::Endpoint` + 独立 Wire IPC v1。
- `t09_authority_link.cpp` 接入既有 coop-active 主线程：
  - `PumpInputs` 在主线程 drain Wire INPUT，进入既有 `Participant::AcceptInput` bytes 路径；
  - `Publish` 在每个成功 Authority commit 后发送 ACK + STATE gap；
  - 首次发布确定绝对 `run_start_tick`，`run_ticks=10,000`，目标 tick 为绝对 end tick；
  - 超过 end tick 的 B trailing input 不进入 admission；
  - 既有 70 槽 Hook 恢复、DLL 常驻和 game PID 存活继续由 coop-active/helper 管理。
- 本地 B script 在 T09 Authority 中禁用；ordinal1 输入只来自 Wire。

### 2.2 Remote B

新增 resident export：

- `IsaacCsT0T09RemoteInitialize/Shutdown`。
- B 游戏进程内独立 worker 只持 Remote Endpoint + Wire IPC：
  - HELLO/首 ACK handshake；
  - 从 Wire header `controller_heartbeat_tick` 与固定 lead=8 生成 canonical B input；
  - 不触碰 Player/Lua/device/world，不应用 ACK/STATE；
  - sequence 1..N、edge 在 sequence100 仅一次；
  - StopRequest 后 retire、设置 stopped、DLL 常驻；
  - approved target image 检查。
- Helper 操作：`--inject-t09-remote`、`--stop-t09-remote`；resident stop 不 FreeLibrary。

### 2.3 Controller/runner

- `IsaacCsT0T09 endpoint` 增加 real game driver 模式：
  - A：创建 SharedV7 + companion loop mapping，注入 coop-active，drain events/loop events，
    执行一次 `--stop-coop-active`，检查 70 槽恢复、DLL 常驻和同游戏 PID。
  - B：创建 Wire mapping，注入 remote export，执行一次 `--stop-t09-remote`，检查游戏 PID
    与 DLL 常驻。
- `IsaacCsT0T09 live-f0`：
  - relay A=loopback、B=host VM address；T9C1 control 绑定 host VM address；
  - host 启动 Authority controller；
  - 在共享目录 create-new 生成 B 端 manifest、T09 runner、Native/Helper、
    `start-vm-endpoint.ps1`；
  - 等待 VM controller 连接，双 READY/ARMED 后 RUN_START；
  - target heartbeat 后 STOP，等待双 STOP/RETIRE 和 VM artifact 回流；
  - 将回流 `vm/` 与 host/relay/control 合并为同一 artifact。
- `accept-t09-b-live.py` 是实机唯一入口；build、run、analyzer 一次执行，不自动重跑。

## 3. 实机前必须由用户确认

1. Host 和 VM 两个游戏当前都在运行，offline/local，seed=`PY1JDB20`、角色以撒、
   初始房间，且未进入 Online/Isaac Sync。
2. Host/VM 各恰好一个 `isaac-ng.exe`。
3. RDP 会话保持连接，VM 共享路径可写。
4. 实机窗口开始后不操作两个游戏、不暂停、不换房、不关闭弹窗、不手动注入/卸载。
5. 用户收到开始通知后，按提示在 VM 内一次运行生成的 `start-vm-endpoint.ps1`。

## 4. 离线回归

在真实游戏接入后，使用新的 build 和相同离线入口执行了一次离线整体回归：
`artifacts/cs-t0/tx-03-03-e2e-01/`，**41/41 PASS**，build/F0/analyzer exit 0，staging 清理。
该回归只覆盖离线 Wire/relay/control，不触碰游戏，也不替代 live P1。

## 5. 当前事实与未完成

- 当前完成 pure compilation 和一次离线回归，未启动任何游戏、未注入、未创建真实
  SharedV7/Wire mapping。
- 离线 e2e-09 的 41/41 PASS 仅是该接入前的离线基础证据，不能替代本轮实机 P1。
- 已执行首次 `accept-t09-b-live.py`，原始失败见
  [tx-03-03-live-failure-01](tx-03-03-live-failure-01.md)：A tick1 world-post publication
  failure、51/51 实际恢复 catalog 与 70 槽硬编码差异、VM UNC append os87、relay 等待窗口。
  已修复并完成离线回归 `artifacts/cs-t0/tx-03-03-e2e-02` 41/41。
- 下一次 live F0 前必须重启 host/VM 两个游戏，清除 resident DLL/session；等待用户重新确认。
- 即使 live F0 PASS，也只证明 P1/F0；F1–F5、observer state comparison、C0/C1/C3 和完整
  T0-09 矩阵仍属于后续工作。
