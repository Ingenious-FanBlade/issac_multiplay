# tx-03-03：真实 host+VM F0 失败 03

日期：2026-10-21（Asia/Shanghai）。事实记录；artifact 原样保留，不自动重跑。

## 实际调用

用户重装双端游戏后确认第三次 live F0：

```powershell
python cs-t0/accept-t09-b-live.py `
  --artifact-dir artifacts/cs-t0/tx-03-03-live-03 `
  --authorization "user confirmed third live T0-09-B F0 after reinstalling both games; ..." `
  --vm-share "D:\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --vm-guest-share "\\tsclient\D\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --await-b-seconds 900
```

## 实际结果

- Host orchestrator 生成 VM launch script 后启动 host A controller。
- A helper 调用 coop-active initialize 返回 `native_error=2`。
- `coop_active::Error=2` 是 **Image**：host 当前 `isaac-ng.exe` 不属于项目
  `target_image::IsApproved` 的 approved build。
- 因此 A controller 在 control 连接前退出；orchestrator 检测到 A child exit，
  正常停止 relay 并返回非0。
- 用户**没有**运行本次 VM start script；B 未被注入或连接。
- 未进入 READY/ARMED/RUN_START，未形成任何 F0 tick/packet。
- acceptance FAIL，artifact：
  - `artifacts/cs-t0/tx-03-03-live-03/`
  - `cs-t0/vm_share/tx-03-02-F0-live-20261021T051546Z/`（仅生成启动 material）

## 直接证据

失败前 known-good host 文件：

- size `9176064`
- last write `2026/8/22 18:00:14`

重装后当前 host 文件：

- size `9362440`
- last write `2026/10/21 12:30:13`

`target_image::IsApproved` 拒绝当前新 build；这不是现场操作错误，也不是允许通过
修改 checker 绕过的问题。新 build 的 RVA、Hook target、ABI 和 virtual device 基线尚未在
T0-01 中批准，不能用于 T0-09-B。

## 当前状态

- Host helper 即使 initialize 返回 Image，resident 注入仍已把 Native DLL 加载进 host 游戏；
  下一次尝试前 host 游戏必须正常重启。
- VM 游戏本次未运行 live script，未注入；但仍建议重新启动以保持两端同 build/同基线。
- 没有 T09 controller/relay 进程残留（orchestrator 的 early-exit cleanup 生效）。
- 最新离线回归仍是 `artifacts/cs-t0/tx-03-03-e2e-03`，41/41 PASS；该结果与当前游戏 build
  mismatch 无关。

## 继续条件

1. Host 和 VM 都必须恢复到先前 approved build：
   `isaac-ng.exe` size `9176064`、last write `2026/8/22 18:00:14`（以 Native
   `target_image::IsApproved` 实际结果为准）。
2. 正常重启双端游戏到 seed `PY1JDB20`、以撒、初始房间。
3. 用户重新确认后使用新的 create-new artifact 根执行；不得重跑 live-01/02/03。
4. 若无法恢复 approved build，则 T0-09-B 实机 P1 保持 NOT_RUN，不能切换到未批准 build
   继续同类测试。
