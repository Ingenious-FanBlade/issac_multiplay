# tx-03-02：T0-09-B 统一验收失败 06

日期：2026-10-20（Asia/Shanghai）。事实记录；完整 artifact 原样保留，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-06 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

在后台 job 中一次执行，未使用前台 120 秒窗口；完整跑到两 driver 正常 retire、两 controller
exit 0、relay exit 0。

## 结果

- 纯编译 exit 0；F0 orchestrator exit 1；analyzer **39/41 PASS**。
- 失败项：
  1. `orchestrator.offline-pass`：orchestrator 把已 Retired endpoint 正常关闭/重置 TCP 连接
     读成 `os error 10054`，在已经收到两端 RETIRE_COMPLETE 后仍把总状态置 FAIL。
  2. `driver.B-normal-retire`：analyzer 将字段 `acks`（最后 ACK tick）错误期望为 ACK 包数量
     10001；实际值为 10000，B driver 本身已正常 retire。
- endpoint A/B result 均 PASS；Wire overwrite=0；relay F0 drop/duplicate/reject/sendError=0；
  10,000 连续 ACK/STATE、10,000 次 control heartbeat、B canonical input/edge exactly-once、
  stop/retire 证据均通过其余检查。
- 未启动游戏、未注入、未操作 Original Online/Isaac Sync。

artifact：`artifacts/cs-t0/tx-03-02-e2e-06/`。

## 根因与修正

1. orchestrator 的 control reader 在 endpoint 进程关闭 socket 时可能先投递 read error，即使
   peer 已完成 RETIRE_COMPLETE。改为：peer state 已 Retired 时把连接关闭记录为
   `ignoredAfterRetire`，不改变已完成 retirement；其他连接错误仍 fail closed。
2. analyzer 的 `driver.B-normal-retire` 将 `acks` 语义写错。改为期望最后 ACK 的
   `server_tick == 10000`，仍同时核对 `states == 10000` 和
   `controllerHeartbeat == 10000`。

这是一次真实功能验收失败。修正后需要使用新的 create-new artifact 根执行新的完整 F0；
本目录不得拼接或改写为 PASS。
