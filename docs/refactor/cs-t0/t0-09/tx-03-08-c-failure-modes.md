# tx-03-08：T0-09-C 实施前失败方式

日期：2026-10-21。状态：**实施前设计资料，未实现、未执行用例。** 适用 [C 工作设计](tx-03-08-c-design.md)；补充并保留 [T0-09 原失败方式](tx-03-01-failure-modes.md)。本轮仅文档，以下不是已运行的测试或新增 unit tests。

每项都应能在同一个最终 E2E 的真实证据或明确的实现边界中判断；不要求为了逐项覆盖而提前运行独立负向测试。无法实际覆盖的故障写清未覆盖，不伪报通过。

## A. 脚本、输入与时标

1. Host A 仍提交物理输入，或 VM A/B 使用了与 Host 不同的脚本/24-byte canonical bytes。
2. 把 10,000 tick 表塞进旧 256-step 数组并截断，或静默丢弃尾部。
3. 静默被实现为 neutral 命令；恢复补发旧 edge，或 heartbeat 跳号使源 sequence/target 重新分配。
4. wire target 使用 VM 绝对 tick；比较时又强求 Host/VM 本地 command 的绝对 target bytes 相同。
5. accepted 高水位、generation、局部 capture/id/use 跨端原数值不同被误判为世界分歧；反过来，仅这些数字相同就伪报位置相等。
6. RUN_START 前已消耗非中性脚本，或 r0 不是有效普通完整帧；第一帧时序差被隐藏。
7. 仅按每次 heartbeat 生成一条命令，跳过目标但没有 SEND_DEADLINE_MISSED 或中性 receipt。
8. 用 render、墙钟、UDP 到达或 ACK 数推进 Authority；为了等 B/origin 停止 Host 日常 tick。
9. 为让故障通过动态调 lead、future window、缺输入年龄、target 或采集配对偏移。
10. VM 本地 B 和网络副本共用可变 Participant/游标，导致本地重复 apply；VM A 也被发往 Host。
11. ACK/STATE 被写到 VM 世界，或 Host 直接读取共享文件夹的 B 脚本绕过 wire。
12. Drop 作用于实际持有物，移动穿门或触发拾取/死亡，仍继续以同 room/generation 比较；事后文件未变被当作整个窗口无副作用证明。

## B. VM owner、采样与比较

13. 旧 Remote worker 的 Ready 仍被当作完整 observer Ready；VM 实际没有本地 B/scheduler。
14. 同一 VM 注入两套独立 Hook owner，重复安装/恢复；worker 直接碰游戏对象或 Lua。
15. P1 的 `g_t09_enabled` world-publication 绕过仍保留，普通采样实际没有发布。
16. controller 用最新 world batch 配当前 receipt，tick/token/capture 并不属于同次复制。
17. 为取两个位置在 worker 重新解引用 Player，或在 TopExit 修改/渲染后采样。
18. current-player witness 绑定失效后仍可读，或把 USER_ACCEPTED witness 伪写成已观察的构造/Init。
19. 上游完整采样失败，却仅挑出可读位置而把 frame 标为 complete_for_subset。
20. odd-only 没有原生 kind+reason+连续来源证明；普通读失败/背压/缺号也被划为 odd。
21. 两端同 r phase 不同被向前后搜索配对；全 gap/零普通配对仍得到 PASS。
22. 把 T0-06 W07 的 10,000 普通采集数当成 T0-09 的 10,000 gameplay tick，或跳过 odd 后重编号。
23. 原始 pointer/entity id 被当成全局 Player identity，或虚构未确认的角色字段。
24. 用 epsilon/四舍五入、删除字段或单一 checksum 隐藏 F0 位置不一致；非有限值被比较为相等。
25. wire PartialValues 被写成完整世界；reason=0、entity/use=0、values 未排序导致严格 decoder 拒绝。
26. F1–F5 所有分歧都笼统归为网络/RNG，没有输入/字段因果；第一次解释后豁免所有后续状态。
27. 只写 mismatch，无首次 r、ordinal/field/raw bits、前后窗口和 packet/apply 引用。

## C. 重复、乱序与真实网络故障

28. 接收端仍将合法旧序 duplicate/reorder 返回 Sequence 并终止整场；或为避免失败忽略所有协议错误。
29. Guard 在严格解码、CRC、peer/协商身份检查前丢包，把错误会话或同 identity 不同 bytes 隐藏为 stale。
30. bounded 历史外包被宣称 exact duplicate；同 lane 高水位回退；ACK/STATE 共用错误 lane。
31. 将 ReceiveGuard 丢弃计作 relay loss，或把 scheduler Stale/Expired 当成 decoder reject。
32. Relay 文档写 xorshift64*，实际仍是 xorshift(13,7,17)；未记录条件 PRNG 消费，seed 无法解释决策。
33. 延迟/丢包只在日志生效，实际 datagram 原样立即 forward；副本被重新编码或分配新 edge identity。
34. bootstrap 已承受故障，或等待操作员启动 VM 的时间提前耗尽 F5 outage。
35. F5 从 relay 启动毫秒计时；control 重复 trigger 导致第二次断网；触发晚到却伪报精准 tick2000 开始。
36. outage 只丢新到包，旧 scheduled 包仍在 2 s 内 forward；或已排程包被无日志清空/延后。
37. 只中断一个方向却宣称双向 outage；outage 前/后不足 1,000 tick 仍通过。
38. 将不同 OS 的 QPC 直接相减推导单向延迟；VM/NAT/宿主排程被混同于配置 impairment。
39. scheduled heap/worker 队列/日志无界，系统过载丢证据却算成预期网络丢包。

## D. 六场编排、恢复与报告

40. 未等待双方到 N 就 STOP，VM 尾部普通帧消失；Host 等 STOP 时又多发 ACK/STATE 或继续 admission。
41. 后续场复用旧进程/常驻 DLL/旧 nonce/旧准备确认；只比较 PID 漏掉 PID 复用。
42. 代理自动 kill/restart 游戏或强制卸载 DLL；同一场失败自动重试，或六场被拆成六次提前验收。
43. 只核验历史 Host 51 槽，不核验 VM 实际 catalog 与新增 world 槽；恢复错误被 staging 清理错误覆盖。
44. 正常退休释放 BASE_CONSTRUCTED 对象/root 或 control block；删除仍被游戏映射的 DLL 再机械报功能失败。
45. 共享文件尚未关闭就分析，或旧 case 文件混入当前 case；日志序号缺失却汇总通过。
46. 所有输入/状态全部写盘导致空间耗尽，或者滚掉因果所需原 bytes；失去原始错误/首次失败窗口。
47. 单 seed 的 C P2 被写成完整 T0-09 PASS；D 的 C0/C1/完整多 seed 工作被抹除，或恢复已删除的 C2。
48. 为文档任务运行构建/游戏/测试；为实施探讨运行 smoke/probe；恢复旧 gate/哈希/作者分离流程。

出现实际失败时保留 case/run、tick、双端来源、首个原因、已恢复项和未执行项。同一 suite 不自动重跑；后续实质修复再说明新推进。
