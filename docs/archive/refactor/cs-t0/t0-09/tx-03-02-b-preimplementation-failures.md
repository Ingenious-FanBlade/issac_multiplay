# tx-03-02：T0-09-B 实施前失败方式

日期：2026-10-20（Asia/Shanghai）。本文件先于 T0-09-B 代码登记。总体失败方式继续以
[tx-03-01-failure-modes.md](tx-03-01-failure-modes.md) 的 60 项为准；下面列出 B 的
接口/实现特有失败方式。出现任一项时保留原始错误和 artifact，不自动重跑，不把离线场景
通过写成 host+VM P1 PASS。

## A. Endpoint 与 packet

1. 一个进程仍同时持有 Authority 和 Remote endpoint，或复用同进程 logical transport，
   导致网络/进程边界被短路。
2. controller 解码并改写 canonical input、重算 CRC、拼接/拆分 datagram，或绕过 driver
   的 Endpoint commit/receive 顺序。
3. HELLO/首 ACK 使用非 exact pair 的 identity、错误 role/nonce/session，仍进入 Established。
4. Established 被直接当成游戏侧 Active/observer ready，提前产生 script tick。
5. 同一 packet 被 controller 和 driver 双重 apply，或 endpoint commit 前写入 outbound ring。

## B. Wire IPC v1

6. C++ 与 Rust 的 header/slot offset、size、magic 或方向 index 不一致，仍继续运行。
7. producer 在 ring 满时覆盖旧槽、只增加计数并假装成功，或 consumer 在复制前推进 consumed。
8. slot metadata/packet 在 published 前后发生撕裂；consumer 未 acquire-load published。
9. 非法 direction、slot length、reserved、role 或 session 被接受。
10. controller 网络线程触碰 canonical queue、scheduler、Player/Lua/world。
11. mapping 名称使用旧 run nonce、共享 A/B 同一 mapping，或关闭后的旧 slot 复活。
12. driver/controller 一方异常退出时，另一方无限等待或继续发送已结束 session 的 packet。

## C. Control plane

13. 未知字段/type/role、超长行、旧 seq、重复 seq、回退 ack 或错误 run_id 被接受。
14. `RUN_START` 在两端 READY+ARMED 之前发出；controller 在收到前推进 tick/script。
15. 控制连接断开后自动重连并继续旧 run，或两个 role 复用一条连接。
16. STOP_REQUEST 后仍接受新业务输入，或 stop 失败覆盖最初错误。
17. 公共 control log 写入绝对 Steam 路径、SteamID、用户名、Player/device 指针或 raw world。

## D. Relay / UDP

18. relay 在长度检查后仍修改 payload、重算 CRC、生成/合并 packet，或把多个 datagram
   拼接成一个 packet。
19. 对端地址未知时直接丢弃 HELLO，或缓存无界；地址已知后恢复顺序错误。
20. loss/duplicate/reorder 决策依赖到达交错或非固定 PRNG，无法从 seed/ordinal 重放。
21. 决策只写日志而实际 forward 不遵循；或 duplicate 被 endpoint 当成新 sequence/edge。
22. UDP 超过 1200 字节/分片未被记录，或 relay 在自身故障后继续宣称 healthy。
23. VM/NAT 自身丢包/重排与配置 impairment 混在一起，未保留 source address/time/decision。
24. relay 的 stop-file/stop 状态关闭后仍转发 packet。

## E. Barrier、tick 和 B input

25. A driver 在无 B 输入时停止 tick，或由 UDP 到达/渲染帧/墙钟直接推进 Authority tick。
26. B driver 在每个 ACK 上重复同一 target/sequence，或在 duplicate ACK 上再次 apply edge。
27. B driver 在 RUN_START 之前发送业务 input；A driver 在 RUN_START 之前接受/apply。
28. F0 实际 tick 未连续到 10,000，却因 heartbeat 缺失被跳过或由 runner 补数。
29. Authority ACK/STATE 每个 tick 数量/identity 不配对，analyzer 只检查“至少一个”。
30. A 的 `heartbeat_tick` 回退/跳跃，controller 未 fail closed。
31. lead/future/late 语义被 runner 自动重试或滑动 target 掩盖。

## F. Runner、停止与 artifact

32. offline driver 通过被标记为真实 host+VM 双游戏 P1；或 user_preconditions 未授权仍启动游戏。
33. orchestrator 未实际启动两个独立 OS 进程，却在 manifest 写双进程。
34. 端口竞争/ready 文件半成品导致 controller 连到旧 relay/旧 control connection。
35. artifact 覆盖历史目录，失败时删除证据，或空间不足时静默丢日志。
36. 只有 relay 日志、缺少双端 controller/driver packet 与 control 屏障证据。
37. stop 后没有核对两个 driver 进程退出、mapping closed、overwritten=0 和 artifact flush。
38. analyzer 记录 PASS 但无法从同一 artifact 重建 tick/packet/input 计数。
39. 任一端失败后自动重跑、换编号重复同一验收，或把未执行真实游戏写成 PASS。
40. 使用 hash/gate 字段作为运行前置或通过依据。
