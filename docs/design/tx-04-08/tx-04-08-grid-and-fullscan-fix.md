# tx-04-08：Grid 种类与全量核对性能修复

> **后续运行更新**：用户另行要求启动e2e-07，已实测Grid分类无UNKNOWN_CLASS、FULL_INDEXED核对28,672 tick/480Ack匹配错误0，最终自动report正常生成。整体因Effect读取依据NOT_READY而超时，第二进程未启动；见[结果记录](tx-04-08-e2e-results.md)。下文“未运行”的构建阶段状态保留其时点。

2026-10-29，Asia/Singapore。用户要求先补Grid种类，再修复全量核对性能；逆向按指定GPT-6 Luna/max执行，主智能体独立复读后实现。目标是消除e2e-06的Grid分类遗漏和逐tick重复全表扫描，保留全量检查与所有原判据。当前完成实现、静态审阅和纯编译，没有部署、运行游戏、replay/compare、局部验收或单元测试。

## Grid 扩表

[主审报告](../../research/tx-04-08-grid-kinds-static-confirmation.md)核定18类Grid（根+17派生），新增16个exact RTTI条目，总清单26类。41个未知对象按运行imagebase/vptr映射确认为Wall，清单补入Wall及其他已核定Grid派生TD；所有派生仍使用公共Grid profile。0–27枚举到Factory实现类的映射另存为可读元数据，不参与按数值猜分类。

完整sizeof未知与公共可读前缀、operator new请求分开记录；Door旧SDK2EC/实际分配请求2F0差异保留。未知TD/不符继承图仍拒绝，初始化byte、槽关联、线程与完整窗口规则继续适用，未改变输入或游戏调用。

## 全量索引

新增`tools/harness/src/tx08/trace_index.rs`，使用BTreeMap/BTreeSet，一次遍历已解析的每个进程trace建索引，然后按完整键查询。索引借用原始Event，避免复制大型JSON；每个进程独立，不混合地址/身份。正常数据规模的关联从逐条全表扫描O(T×E)改为建表和查询约O(E log E + T log E)，输出证据的固有体积仍保留。

| 原重复扫描 | 新索引与保留判据 |
| --- | --- |
| 每tick查找Accepted Submit | ordinal/session/input generation/sequence/target完整键，保留同键Vec；仍要求唯一、先于tick、四项canonical值相同；input_sequence=0仍只允许中性 |
| 每Ack查找既有A tick | session generation/tick的最早A序号；保持原有“至少一条更早A tick”规则，不改为新唯一性规则 |
| 每持久调用查返回与父进入 | call分组整组返回与最早父进入；返回仍唯一、正常且顺序正确，重复不被覆盖 |
| 每窗口查close、每loading查begin及窗口内输入 | id/episode/session分组，tick/submit按原始seq作开区间查询；保留原find首项与边界语义 |
| 每完整capture查pre/commit/标准发布及Player见证 | 同tick/loop/scope计数，Player按ordinal/tick/session/input generation索引；重复阶段继续导致count!=1 |
| L6每相对tick扫描完整录制/trace | 两边完整capture按tick分组、消费按generation/tick/ordinal分组；仍要求唯一完整capture、唯一真实消费及原始Submit匹配 |
| Factory复用逐条查native body | call对应正常native返回集合；仅改变存在性查找 |

全部tick、Ack、阶段、生命周期与样本仍进入核对；没有随机采样。summary明确`mode=FULL_INDEXED,sampled=false`并保存源事件/分组规模。缺第二进程仍不通过，并将已完成进程结果提前写入`evidence/session-processes.json`，避免后续缺材料导致已核对事实只留在内存。

## 静态语义审阅

没有运行修改前/后的比较脚本。主智能体逐分支回读原条件与索引查询，重点检查：

- 同键重复Submit、tick、capture和关闭事件不被单值覆盖；唯一性与存在性判据各按原用途保留。
- Option数值键保留原先对缺字段的比较语义；不以缺值补0，不把早到/晚到/旧代次命令配给当前tick。
- trace解析原本保证seq唯一、非零并排序，范围索引依赖这一原有条件；窗口开区间与空/逆向区间保持一致。
- stage必须同tick/loop/scope且恰好一次；native Player依据仍要求同代次且native reached/commit，不因索引命中放宽字段资格。
- L6的完整capture、ordinal、session/input generation、同脚本偏移、实际canonical输入和run/room范围检查均保留。

原始JSON、重复组及失败序号继续可回查；2,000/256完整数、odd-only例外、V1–V12判据没有改为统计估计。

## 构建与边界

Rust Release纯编译成功（19.92s），日志`artifacts/repentogon/tx-04-08-defg-build/rust-build-full-trace-index.log`；完整DLL生成/编译/链接成功，日志`dll-build-grid-kinds.log`。out/repentogon的harness、DLL及生成清单已更新。外部SDK/ASMPatcher既有警告按构建日志保留，没有修改第三方代码。

本次尚未测量索引版本耗时、与旧归约做运行对照或验证新版Grid完整采集。以后实机整体验收按现行规范只在完整目标结束后统一执行，并逐次确认启动准备；不自动重跑已停止的e2e-06，也不改写旧自动材料或把源码审阅当运行PASS。
