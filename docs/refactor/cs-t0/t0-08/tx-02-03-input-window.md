# tx-02-03：A 物理输入窗口与 Authority 激活同步

日期：2026-10-20（Asia/Shanghai）。用户要求修复 tx-02-02 中“输入工具成功、前台证据成立，但 A 原生采样移动量全零”的问题，然后继续测试。本轮先记录失败方式，再修改验收编排，最后完成一次统一 E2E。

实际 `tx-02-03-e2e-01` 在离线 19,022/19,022 PASS 和历史失败报告检查 PASS 后，等待外部初始 `physical-input-ready.json` 超时；`live=NOT_RUN`、controller 未启动、游戏未注入或输入，整体 2/2 FAIL。该失败原样保留，未自动重跑；后续由[tx-02-04](tx-02-04-owned-physical-input.md)把输入会话收归验收器并取得 33/33 PASS。

## 现有证据

- tx-02-02 唯一实机结果：`29/31 PASS`，两项失败均为 A 无实际移动：
  - `L04.A-and-clock-continue`
  - `L04.A-real-partial-position-change`
- 物理输入工具记录初始 4 步和连续 64 步均成功，同一 game PID 8604。
- 269/269 个 Native `PhysicalSample` 都显示游戏为前台，但 `move_x/move_y` 全为 0；269 个样本的 state 完全相同。
- `physical-input-ready.json` 记录连续输入结束于 `22:01:17.846`；验收脚本首次记录 `authority-active` 的 `stage.json` 写于 `22:01:20.352`。即当前入口允许连续物理输入在 Authority 真正开始采样前结束，本轮实际记录顺序也如此。
- 历史 tx-01-98 使用延迟启动并让 A 输入覆盖 Authority tick 后，曾得到 391/437 个非零 `PhysicalSample`。因此本轮优先修复验收时序，不先改 Native canonical/queue/apply 语义。

## 必须避免的失败方式

1. 继续在 Authority 激活前跑完连续移动，之后才采样，造成工具成功但 A 全零。
2. 只增加前台 PID 检查，把“窗口在前台”误当成“输入窗口与 Authority 重叠”。
3. 开始 A 输入太晚，错过指定 outage 的 A Resolve/位置采样；或输入在 stop 之后仍继续。
4. 用脚本伪造成功、直接写 A world、设备 cache 或 canonical，绕过共同 codec/queue。
5. 接受旧 run 的 `physical-input-continuous-result.json`，或在没有结果时仍然继续 stop。
6. 不记录实际工具请求、启动/结束时间和结果，只在文档里声称输入成功。
7. 修改 wire v2、IPC v6、Native ABI、Hook 恢复或 B 脚本语义来掩盖编排问题。
8. 自动重跑、覆盖 tx-02-02 失败 artifact、计算哈希或使用 gate。
9. 把外部连续输入请求失败写成 Native PASS；失败必须保留原始错误和未完成项。

## 设计

- 保留 `physical-input-ready.json`：由外部工具在 controller 启动前完成一次短闭合 W/D/S/A，作为前台/工具就绪回执。
- controller 启动后，验收脚本等待首个真实 commit；只有观察到此 commit 后才写 `physical-input-continuous-request.json` 并打印新的 `PROTOCOL_E2E_CONTINUOUS_INPUT_WINDOW`。
- 外部输入工具收到该窗口后执行请求中的连续 64 步有界 W/D/S/A。每步 200ms，无额外间隔，估计 12.8 秒；必须使用既有 `run_key_sequence`（`SendInput` 扫描码 + 前台校验）。
- 验收脚本等待并校验 `physical-input-continuous-result.json`：request id、game PID/creation time、64 步、工具成功、started/finished UTC、请求后开始、Authority 已激活，以及结果观察时已越过 outage 末尾。
- 将协议故障 outage 移到 tick 160..179，使输入启动延迟不会轻易错过指定 outage；B 仍有 240 步脚本和足够恢复区间。A 连续输入持续 12.8 秒，覆盖正常 60Hz 及更慢的回放。
- 在结果中增加 `physical-input-continuous-validation.json`。报告器新增 `L04.continuous-input-window` 检查；若缺失、晚于 outage、步数或身份不一致、工具报错，则整体 FAIL。
- stop 仅在 script target 覆盖完成且连续输入结果有效后发送；若输入结果超时，仍执行一次正常 stop，并保留失败。

## 已实现内容

- `accept-protocol-e2e.py` 保留初始前台/工具就绪窗口；controller 启动后等待首个 `EventKind8` commit，再生成 `physical-input-continuous-request.json` 并打印 `PROTOCOL_E2E_CONTINUOUS_INPUT_WINDOW`。
- 连续请求固定为 64 个单键 W/D/S/A 闭合步骤，每步 200ms、无额外间隔，预计 12.8 秒；与初始 4 步分离。
- 验收器最多等待 45 秒读取 `physical-input-continuous-result.json`，核对 request id、工具成功、64 步、game PID/creation time、UTC 顺序、request tick 早于 outage、结果观察时已越过 outage 末尾。
- 生成 `physical-input-continuous-validation.json`；只有该验证有效且 B 脚本/最后 target 覆盖完成后才发送 stop。超时或无效结果保留为 `ContinuousInputTimeout`/`ContinuousInputFailure`，仍只执行一次正常 stop，不自动重跑。
- `report-protocol-e2e.py` 新增 `L04.continuous-input-window`，并要求 request 在 outage 前、结果在 outage 后、工具启动延迟不超过 5 秒且时长大于零。
- wire v2、IPC v6、Native ABI、B 脚本 240 步、共同 codec/queue/apply 与 70 槽恢复逻辑未改；只把验收协议故障 outage 从 tick64..83 移到 tick160..179，为外部输入启动留出同步余量。

## 纯编译与预定唯一验收

构建目录：`out/cs-t0/tx-02-03-build`。unique acceptance 目录：`artifacts/cs-t0/tx-02-03-e2e-01`。

```powershell
python cs-t0/accept-protocol-e2e.py --transaction tx-02-03 --build-dir out/cs-t0/tx-02-03-build --artifact-dir artifacts/cs-t0/tx-02-03-e2e-01 --prior-failure-artifact artifacts/cs-t0/tx-02-00-e2e-03 --authorization "User requested fixing the A physical-input window and continuing E2E; fresh isolated safe WASD game confirmed in this chat."
```

实机运行时必须在第二个 stdout 窗口出现后，使用现有 `isaac_test.run_key_sequence` 执行请求中的 64 步，并写回实际工具结果。未收到该第二个窗口前不得提前运行连续输入；未写回有效结果时验收器不会发送正常完成 stop。

## 本次验收范围

一次统一入口覆盖原有 corpus、生产 Participant 生命周期、历史缺恢复文件报告场景、真实 Authority、wire v2/IPC v6、固定 seed 故障、B 中性/恢复、逐 commit ACK/STATE、world 身份和 70 槽恢复，并额外要求 A 物理输入窗口与 Authority/outage 真正重叠。失败不自动重跑。实机前必须由用户完整重启为唯一隔离 `isaac-ng.exe`、安全新局、只有 A，并保持游戏前台。
