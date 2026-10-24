# tx-01-90：T0-07 操作 2，成功模拟返回后的 Authority tick

2026-10-20（Asia/Shanghai）。本次完整目标：把现有普通 Room / odd-player-only 两条实际模拟路径的 tick 建立/提交收敛到生产 AuthorityTickClock，明确配对、来源、线程、调用窗口、正常返回和终止行为。只完成操作 2；保留操作 1、A/B 统一输入、lead=2、启动例外、立即中性和完整世界采集规则。不得把顶层 loop/render/墙钟当作模拟 tick，不实施操作 3–9。

## 实现前登记的失败方式

1. 在进入模拟、物理采样、render、顶层空转或暂停/FSM 分支直接增加 completed tick；未正常返回却留下“已完成”。
2. 普通 Room 与 room-lifecycle / ordinary-door 的嵌套 Room 入口混淆；odd 特殊 FSM Player caller 冒充正常 `0x2FD60C`。
3. Begin/Return 的 kind、原生调用来源、owner、loop 或主线程身份不配对；返回先于入口、重复返回或同一 loop 重复模拟导致多次计数。
4. 使用 loop_iteration 的差值推进 tick，空闲/暂停跳过若干 loop 后补造模拟步；普通/odd 相邻更新被分成两套计数器。
5. Warmup 或生命周期阻断时仍 Resolve，或非 Active 的无入口返回被误判成功；暖机/原生早退没有输入步却递增。
6. 缺 A/B 命令或 B 单独退休导致时钟等待全员；以 binding/command 存在作为正常完成条件，改变现行 scheduler 中性策略。
7. 已开始的 scope 中 session 退休/失败、未知重入、异常或丢失返回后继续消费输入；下次重用 completed+1，却残留 Participant last_resolved 高水位或旧 edge。
8. 将 room/death generation 变化或 collector gap 直接等同“原生没有模拟”，错误回滚已发生的正常更新；或反过来把正常 tick 提交宣称为完整世界样本。
9. odd helper 未经过正常 Main Player 路径也提交；普通 Room 返回后等待 world collector/文件输出才推进模拟。
10. tick 或未来目标计算溢出；将普通容量耗尽误报重入；从其它位置直接赋写已完成计数。
11. 为处理异常吞掉原生 EH、重复调用 original、从 worker 访问游戏对象，或新增未纳入事务的 Hook。
12. 只运行与生产不同的时钟替身就声称 Native 接入已验证；验收失败后自动重跑，或把离线正常返回场景当作新实机证据。

## 分析和验收安排

用户指定 GPT-6 Luna / max 子智能体只读静态分析，主智能体逐项重新读取源码/既有原始证据后记录采纳、更正和未决点。静态原始事实与设计决定分开，不把旧实机授权扩展成新游戏操作。

先登记以上失败方式与离线生产链场景，再实现时钟和 Native 接入。完成后直接纯编译 Native/离线程序，并仅执行一次统一离线验收，贯通真实 AuthoritySession、tick clock、V2 codec、Participant 与显式 native-return 事件。保存输入、事件、源文件、二进制出处和构建日志；没有中途试跑、单元测试或哈希校验。实机行为不在离线 PASS 的证明范围。

准备阶段尚未运行任何测试。

## 当前实现契约

- `Completed()` 只在生产 Clock::Complete 内写入一次；Begin只预留 `Current=Completed+1`。两条原生模拟路径共用此时钟；loop token仅用于配对及排除同 loop重复，不用它的差值补造tick。
- 普通调用必须为标准 Room 返回 RVA `0x2FBA52`；odd必须为 driver helper返回 `0x555246`，且scope中首次正常 Main A/B Player caller为 `0x2FD60C`。保存首次Player见证，正常post使用同一owner；普通使用同一Room self。临时指针不成为网络身份。
- Active且无现有阻断才Begin；暖机/暂停/加载/menu/原生早退无tick。入队与命令是否存在、B是否暂时失绑或单独退休，不成为日常时钟条件。
- Native记录真实pre，包括合法跳过，post先配对source/kind/owner/loop/thread；未知post、重入、第二scope、不匹配或耗尽停用。Clock的NoScope只表示无候选，不生成已完成步。
- scope中room/death失效继续由Participant撤销绑定/previous；所选原生body确实正常返回且session仍Active时，tick仍提交。完整世界样本继续要求后续owner/Room/tail/outer正常链，odd仍是gap。
- session终止/停用时取消pending，不增计数；outer正常返回仍有pending或未闭合pre则关闭时钟。没有正常post时不能重用已Resolve的候选tick，不能回滚或补执行旧edge。EH越过所有post时不伪造完成，不吞异常；若以后有新Begin，未闭合调用将失败停用。
- worker只读取原子counter用于既有诊断，不变更pending/Closed或访问游戏对象；恢复、Hook槽与原生original调用次数沿用原协调owner。本次无新增Hook。

新增 EventKind 32 `AuthorityTickBoundary`：decision=TickDecision，query=kind（1普通/2odd），target=loop，action=原生return RVA，detail=odd Player caller；before为completed低/高32位和current低/高32位，after为thread/main-thread/本地owner/closed。旧AuthorityCommit仍仅在成功提交发布，world::Commit继续记录同一tick/kind；Shared/Event布局不变。

[子智能体结论与主智能体逐项复核](tx-01-90-static-review.md)保留采纳、补充、旧行号语境和未知范围。场景清单、离线程序和失败定义先于生产时钟实现写入，共30场景174条命令；Native及生产Clock/Session/codec/Participant链准备完成后只执行一次统一验收。

## 纯编译与执行准备

`cs-t0/build-authority-tick.ps1` 已直接纯编译成功：VS 2022/v143、Hostx64→Win32 Release，Native DLL与AuthorityTickScenario均正常链接。输出、两份日志及toolchain.json在 `out/cs-t0/tx-01-90-build`；未执行旧build.ps1、Cargo/Native测试、游戏查询或注入。准备过程零测试；以下唯一统一离线验收结果另记。

执行更新：唯一统一离线验收已完成，30场景、174条命令、175/175检查PASS，退出0；真实Native/游戏未运行。见[实际结果与证据](tx-01-90-results.md)，本段不改变上文准备阶段尚未测试的历史事实。
