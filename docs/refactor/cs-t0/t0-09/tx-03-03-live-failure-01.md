# tx-03-03：真实 host+VM F0 失败 01

日期：2026-10-20/21（Asia/Shanghai）。事实记录；artifact 原样保留，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b-live.py `
  --artifact-dir artifacts/cs-t0/tx-03-03-live-01 `
  --authorization "user confirmed live T0-09-B F0 start; host+VM Native/Helper injection, owned virtual B, UDP relay/TCP control, read-only sampling, normal stop/restore authorized" `
  --vm-share "D:\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --vm-guest-share "\\tsclient\D\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --await-b-seconds 600
```

用户随后在 VM 运行生成的 start script 一次。该次未形成 P1/F0 PASS。

## 实际结果

- 纯编译 exit 0。
- Host A 游戏成功加载 Native/Helper；Wire mapping 创建，HELLO 发出。
- A Native 在首个 gameplay tick 以 `native_error=7`（Publication）停止，未进入 ARMED/RUN_START。
- A 正常执行一次 helper stop：同游戏 PID 存活、DLL 常驻、restored 51/51 个实际安装 target，
  event overwrite=0；但 recovery 不是原先错误假设的 70 槽（本 P1 未安装 world-record 的额外
  19 槽）。
- VM B controller 在 helper 注入后追加 driver helper 输出时失败：
  `append \\tsclient\...\artifact\vm\driver.stderr.log: os error 87`。B control 未建立。
- Relay 在等待 B 期间先触发旧的 300 秒 run bound；orchestrator 仍在等待。
- 未进入 RUN_START，未形成 10,000 Authority tick、ACK/STATE、STOP/RETIRE 双端闭环或 live PASS。
- 后续人工终止仍存活的 orchestrator 进程；acceptance 记录 FAIL。未启动第二个测试 run。

## Artifact

- `artifacts/cs-t0/tx-03-03-live-01/`
- VM 回流目录（不完整）：`cs-t0/vm_share/tx-03-02-F0-live-20261020T180002Z/`
- A restoration：`host/restoration.json`，51 rows/catalog count 51。
- A Native failure 触发点可由 `host/events.jsonl` 的 tick1 Authority/Tick 事件和
  `host/endpoint.jsonl` 的 driver error 0x7 定位。

## 根因与修复（作为后续推进）

1. **A Publication false**
   - T09 P1 使用最小 STATE gap，不创建 world-record mapping。
   - `world::PublishPostState` 在没有 world mapping 时返回 false，被误当作整体 publication
     failure。
   - 修复：T09 路径不调用/不依赖 world post-state 发布；ACK/STATE Wire 发布与 event publish
     仍必须成功。
2. **恢复槽数**
   - 本次实际安装并恢复 51 槽；T0-08 的 70 槽包含本次 P1 未创建的 world-record 附加槽。
   - 修复：不硬编码 70；记录 catalog count 与 restored rows，要求二者一致且 catalog count>0。
   - 本次 51/51 恢复事实保留。
3. **VM UNC append 失败**
   - RDP redirected drive 对 `FILE_APPEND_DATA` 打开方式返回 os error 87。
   - 修复：helper 输出改为普通 write handle + seek-to-end，再写入。
4. **Relay 等待窗口**
   - relay 旧 300 秒 bound 小于 live 等待 VM 的窗口。
   - 修复：上限提升到 7200 秒；startup controller 提前退出时 orchestrator 立即停止 relay，
     不空等。
5. **Link pre-handshake publication**
   - 修复 world-post 后进一步静态发现：AuthorityWireLink 在 B handshake 尚未 Established、
     RUN_START 尚未到达时会把正常 game tick 的 `Publish` 当成失败。
   - 修复：pre-Established/pre-RUN_START tick 明确视为“不发布 C/S packet”的成功 pipeline tick；
     只有 initialized/healthy/open 状态错误才失败。首个 RUN_START 后 tick 才建立绝对 run window
     并发送 ACK/STATE。
6. **失败后进程/对象**
   - Host A 与 VM B 游戏进程都已加载 resident Native DLL；同一游戏进程不得再次 T09 初始化。
   - 下一次实机尝试前必须由用户正常退出并重新启动 host/VM 两个游戏，重新进入约定基线。

修复后已完成一次纯编译和离线整体回归
`artifacts/cs-t0/tx-03-03-e2e-02`：**41/41 PASS**。下一次 live F0 必须使用新的 create-new
artifact 根并在用户重新确认双端重启与基线后执行；不得重跑本目录或拼接部分结果。
