# tx-03-03：真实 host+VM F0 失败 02

日期：2026-10-21（Asia/Shanghai）。事实记录；artifact 原样保留，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b-live.py `
  --artifact-dir artifacts/cs-t0/tx-03-03-live-02 `
  --authorization "user confirmed second live T0-09-B F0 after both games restarted; ..." `
  --vm-share "D:\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --vm-guest-share "\\tsclient\D\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --await-b-seconds 600
```

用户在 VM 运行生成的第二次 start script 一次。

## 实际结果

- Host A：Native/Helper 注入成功，Wire mapping 创建，driver Open/Handshake/READY，
  未再出现上一轮 tick1 Publication failure。
- Host A 在失败后由正常 helper stop 清理：同游戏 PID 存活、DLL 常驻、
  `restored_catalog_count=51`、`restored_rows=51`、event overwrite=0、native_error=0。
- VM B：start script 到达 helper 注入阶段，helper 返回：
  `FAIL independent T0 helper stage=REMOTE_INITIALIZE native_error=3`
  (`driver.stderr.log` 103 bytes)。B control 未建立，未进入 ARMED/RUN_START。
- 未形成 10,000 tick、ACK/STATE 双端闭环、STOP/RETIRE 双向完成或 live PASS。
- Orchestrator 因缺少 B control 持续等待；人工终止仍存活的 host orchestrator，
  acceptance 记 FAIL。之后停止 relay 并等待 A controller 清理。
- artifact：
  - `artifacts/cs-t0/tx-03-03-live-02/`
  - `cs-t0/vm_share/tx-03-02-F0-live-20261020T182635Z/`

## B native_error=3 的含义

失败构建中的 `IsaacCsT0T09RemoteInitialize` 有两个分支返回 3：

1. approved target image 检查失败；
2. `WireIpc::OpenForRole` 校验失败。

没有更细错误码，因此不能从本次 artifact 唯一区分。为避免猜测，已改为：

- 31 = target image not approved；
- 32 = mapping parameter invalid；
- 33 = Wire mapping open/identity validation failed；
- 34 = Remote Endpoint start/HELLO prepare failed；
- 35 = worker thread creation failed；
- 36 = process/session initialize misuse。

## 已修复/改进

- A controller 最终 control flush 失败时仍写本地 `result.json`，不再因对端提前关闭而丢
  restoration/result。
- Remote initialize 返回码细分，下一次可唯一定位 B 侧失败。
- 第二次失败前的 world-post/publication、51/51 dynamic catalog、UNC append、relay bound
  修复继续有效；host A 已证明能正常初始化并恢复 51/51。
- 最新离线回归：`artifacts/cs-t0/tx-03-03-e2e-03`，**41/41 PASS**。

## 下一步前置

Host 和 VM 游戏均已加载 resident Native DLL（即使 B init 返回错误，helper 仍常驻），
下一次尝试前必须由用户正常重启两个游戏。

同时需要核对 VM 游戏 exe 与 host 是否为同一 approved image。Host 当前运行文件信息：

- size: 9176064 bytes
- last write time: 2026/8/22 18:00:14

项目不使用 hash；只比较文件大小/时间和 approved-image 检查结果。若下一次 helper 返回
`native_error=31`，说明 VM image 不在 approved build；若返回 33，则继续查 Wire mapping
identity/name。
