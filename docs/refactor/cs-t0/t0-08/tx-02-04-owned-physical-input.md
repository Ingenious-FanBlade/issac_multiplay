# tx-02-04：由验收器持有物理输入会话

日期：2026-10-20（Asia/Shanghai）。**唯一整体 E2E 结果：33/33 PASS，见[实际结果](tx-02-04-results.md)。** tx-02-03-e2e-01 在离线部分通过后，因外部编排没有在 90 秒内写入初始 `physical-input-ready.json` 而超时；`live=NOT_RUN`，controller 未启动，游戏未注入或输入。该失败原样保留，不覆盖，也不自动重跑。本次推进把物理输入会话收归统一验收器，消除“第二个窗口需要另一个交互代理即时接力”的脆弱点。

## 实施前失败方式

1. 验收器在后台运行但外部代理未及时看到提示，初始 ready 超时；或看到提示后接力延迟，连续输入错过 outage。
2. 使用不可验证的窗口句柄/进程身份，把键发给错误进程。
3. `SendInput` 结构布局、扫描码、扩展键或 key-up 错误，导致键卡住或输入无效。
4. 在游戏非前台、最小化、失去焦点时继续发送，把 OS 接收成功误当成游戏输入。
5. 初始闭合动作与连续动作混在一起，或在 Authority 激活前完成连续动作。
6. 连续输入期间验收器停止轮询，导致结果观察 tick 仍停留在旧值；或日志/协议环消费被阻塞。
7. 输入线程与 stop/清理并发，stop 后仍按键；异常路径没有释放已按下键。
8. 用直接写世界、canonical、设备 cache 或伪造 Native 事件绕过共同输入链路。
9. 修改 wire、IPC、Native ABI、B 脚本、Hook 恢复或 70 槽逻辑来掩盖编排问题。
10. 自动重跑、覆盖 tx-02-03-e2e-01 失败、计算哈希或新增 gate。

## 设计

- 新增独立 `cs-t0/e2e_physical_input.py`：只在 E2E 内定位唯一游戏 HWND，调用 `ShowWindow(SW_RESTORE)`/`SetForegroundWindow`，在每个键按下、保持和释放前后核对 `GetForegroundWindow` 仍属于游戏 PID；使用与 isaac-test MCP 相同的 Win32 scan-code `SendInput`。任何窗口/前台/发送失败都返回结构化错误，并尝试释放已按键。
- `accept-protocol-e2e.py` 默认使用 owned input：
  - 在 controller 启动前自动执行 4 步短闭合动作，写实际 `physical-input-ready.json`；
  - 启动 controller，等待首个真实 commit；
  - 写 `physical-input-continuous-request.json`，由同一进程立即执行 64 步 W/D/S/A（每步 200ms），期间主循环持续读取事件与 wire 尾部；
  - 写完 `physical-input-continuous-result.json` 后等待/轮询至 `committed > outage_end`，再用现有校验逻辑生成 validation。
- 外部手动输入模式不再作为本轮默认路径；仍保留原有 ready/result artifact 字段，但由验收器自己生成，内容包含实际 pid、creation time、hwnd、每步 time/foreground 结果。
- 输入模块不会写游戏世界、canonical、Player/device；只发送 OS 键盘事件。
- 协议 outage 继续使用 tx-02-03 的 tick160..179；连续输入从首个 commit 后开始，预计 12.8 秒，覆盖正常和较慢 tick 速率。
- 本次为新的推进：构建目录 `out/cs-t0/tx-02-04-build`，唯一验收目录 `artifacts/cs-t0/tx-02-04-e2e-01`。完成后只执行一次统一 E2E，不自动重跑。

## 已实现内容

- 新增 `cs-t0/e2e_physical_input.py`：唯一定位游戏 HWND，恢复/前置窗口，使用 Win32 scan-code `SendInput` 执行单键或多键步骤；按下、保持、释放及步骤结束均记录前台 HWND/PID，失败时尝试释放已按键。
- 验收入口自动执行初始 4 步焦点检查并落盘 `physical-input-ready.json`；controller 启动、首个 commit 出现后，立即自动执行请求的 64 步连续 W/D/S/A（每步 200ms），同时生成 request/result/validation 三份 artifact。
- 连续输入在验收器主循环内同步执行；输入结束后由同一循环读取事件/wire backlog，等到 `committed > outage_end` 才验证并允许 stop。输入失败、tick 未越过 outage、前台样本不一致、释放失败或 request id 不匹配都会使整体 FAIL。
- 报告器新增 `L04.physical-input-initial-focus` 和 `L04.continuous-input-window`，并继续执行原 A Resolve、真实位置变化与前台原生采样检查。
- wire v2、IPC v6、Native/Helper 行为、B 脚本和 70 槽恢复未改；只修改测试编排并新增独立输入 helper。

## 本次验收范围

与 tx-02-03 相同，并新增由验收器实际发送、前台可核对的 A 初始与连续输入记录；失败必须保留原始异常、已验证/未验证项，不外推 A 动作为通过。
