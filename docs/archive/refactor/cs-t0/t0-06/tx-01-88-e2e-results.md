# tx-01-88：replay 性能修复与剩余验收完成

2026-10-19（Asia/Shanghai）。唯一离线统一入口执行一次并正常退出 0，**PASS**。W08 全量 replay 和最大 u64/乱序重编码本次实际检查通过；W01.isolation 按用户要求永久从当前清单移除，未改成 PASS。当前 **32/32 PASS、0 FAIL、0 NOT_COVERED**，其中 **30 项沿用 tx-01-87 及其历史出处，2 项 W08 在本次检查**。这是累计检查清单完成，不代表新 Controller 单次重跑全部实机项；tx-01-75 中断和历次 FAIL 均保持原样。

[完整账本](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/result.json)、[收尾](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/finalization.json)、[清单变更](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/scope-change.json)、[完整证明](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/replay-proof.json)。验收约在 **23:26–23:28** 完成，没有游戏进程查询、注入、输入、Native/Mod 修改或哈希校验。

## 修改与性能

旧 `world-replay` 将 serde_json 的细粒度写入直接交给 File；读取、校验和排序并非本次省略的工作。现在用固定各 1 MiB 的读写缓冲聚合文件 I/O，JSONL serializer、每条 128 MiB 限制、parse/validate/canonicalize 均保留。成功报告只在显式 flush 返回成功后发布；新增 io 统计记录实际 File 写调用、字节数、耗时和 flush 状态。该 flush 表示用户态缓冲已交给文件系统，不是物理介质同步保证。

修改位置：`cs-t0/src/world_record.rs` 的 `line`、`ReplayFile` 和 `replay`。共享 `line` 仅将 writer 类型从 File 放宽为 Write，live 和 compare 的调用路径不增加新行为。Native、Helper、世界采集、协议和游戏 Hook 均未修改。直接离线 Release 纯编译成功（2m28s），未运行单元测试或试跑；[构建日志副本](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/sources/out/cs-t0/tx-01-88-build/rust-build.log)。

真实输入为 tx-01-75 primary 的完整 captures.jsonl，**2,297,381,939 bytes**，不是缩短采集、减少字段或截取普通样本。指标包含 CLI 启动到退出的墙钟耗时：

| 版本/范围 | 记录数 | 耗时 | 输入吞吐 | 实际 File 写调用 |
| --- | ---: | ---: | ---: | ---: |
| 旧版、同一输入前缀 4,306,472 bytes | 64 | 3.002 s | 1.368 MiB/s | 旧版无此统计 |
| 新版全量 A | 33,050 | 14.499 s | 151.106 MiB/s | 2,191 |
| 新版全量 B | 33,050 | 13.580 s | 161.336 MiB/s | 2,191 |

相对同轮旧版前缀实测吞吐约 **110.5–117.9 倍**，超过本次预先列出的至少 5 MiB/s 且至少 5 倍目标。旧版没有重新全量运行，前缀和全量的数据分布及缓存条件不同，这个倍数仅描述本次基线比较；不宣称普遍或严格同负载的加速倍数。历史约 0.6 MB/s 观察仍保留，不用新测量替换旧事实。两份新输出各 **2,297,381,939 bytes**，flushed=true；详见 [performance.json](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/performance.json)。

## 正确性与失败行为

- 完整 **33,050** 条记录，独立 Python canonical 值与两次生产 replay 均相同；逐条累计比较 **20,931,447** 个字段。
- 两份 replay 输出逐记录逐字节一致；前 64 条与旧版实际输出的字节也相同。没有用 digest/hash 代替比较，没有把输入直接复制当作 replay。
- 输入 manifest 与 captures.jsonl 和冻结副本直接逐字节相同，输出 manifest 与源值相同；原始文件没有修改。
- 真实样本的 sources/objects/fields/memberships 在自有副本中倒序，生产 CLI 正确恢复 canonical 顺序；`outer_token=18446744073709551615` 原样保留。覆盖排序和超过 2^53 的最大 u64，而非仅解码空记录。
- 非法字段宽度、u64 溢出、额外 schema 字段、有效记录后的截断 JSON 均由生产 CLI 拒绝，退出 1、保存 failure.json，且没有生成成功 result.json。
- 准备阶段零测试；所有生产 CLI、完整比较和上述变体均属于这一统一入口的一次执行，没有自动重跑。实际磁盘写失败未被人为制造，不据正常 flush 成功外推所有存储故障。

逐条证据：[record-comparison.jsonl](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/record-comparison.jsonl)、[全部命令/耗时/退出码](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/commands.json)、[A](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/replay-a/result.json)、[B](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/replay-b/result.json)。当前 source 仍有 UNKNOWN 等原始语义；重编码通过只证明原样保留，不把未知世界状态升级为相等或确定性。

## 清单与复现

当前机器 `OBLIGATIONS`、`MANUAL`、人工检查说明及活动入口的排除列表均不再列 W01.isolation；导入历史账本时只保留当前存在的检查。第 13 条[项目约定](../../project-conventions.md#13-已移除的原版联机实机测试)记录用户决定。W01 默认 51 槽、可选 70 槽和原生调用保持继续存在，不进入/不复用原版联机状态的设计约束不变。历史 33 项结果和旧排除原文不改写。

冻结 [sources](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/sources/)、[二进制出处](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/binary-provenance.json)、[参数](../../../../../artifacts/cs-t0/tx-01-88-replay-performance-e2e-01/invocation.json)和 frozen-source/replay-a/replay-b 均保留在原始目录。新的验收须另选新 artifact-dir，不能覆盖本次材料。入口：

```powershell
& 'C:\Users\114514\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  cs-t0/accept-world-replay-performance.py `
  --controller out/cs-t0/tx-01-88-build/rust/release/IsaacCsT0.exe `
  --baseline artifacts/cs-t0/tx-01-87-final-windows-e2e-01/binaries/IsaacCsT0.exe `
  --world-dir artifacts/cs-t0/tx-01-75-e2e-01/captures/primary/capture/world `
  --prior artifacts/cs-t0/tx-01-87-final-windows-e2e-01 `
  --artifact-dir artifacts/cs-t0/NEW-UNIQUE-DIRECTORY
```

本轮 W08 性能问题及当前测试清单均已完成，没有剩余测试项。该结论不扩大到当前 T0 原型之外的 Replica、预测/rollback 等后续目标。
