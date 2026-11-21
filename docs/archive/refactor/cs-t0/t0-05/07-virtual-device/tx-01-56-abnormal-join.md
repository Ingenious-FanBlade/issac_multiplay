# tx-01-56：异常 Join 分支选择

日期：2026-10-17。目标：修复 tx-01-51 中自有设备已关联 Main Player、但未经过受控普通创建点而无法接纳的 Join。分析、逆向和修改阶段不测试；完成后通过一个统一入口执行一次构建、注入、实际操作、正常恢复和报告，不自动重跑。

## 失败方式与验收安排（实现前记录）

- 原生 Baby 模式为真时仍直接调用 `5BD3B0→5B98E0`，绕过普通工厂 owner；或者只扩大绑定条件，把 Baby/任意同 K 对象误收为完整 B。
- 仅覆盖第二处模式查询，第一处健康条件或第三处 selector 检查仍使 Join 无法闭合。
- 改写全局 run flags、影响物理设备、把 possessor 的 Pressed 8/10 当作 Triggered，或者重复工厂/原生调用。
- naked wrapper 破坏 ESI/EBX/EDX/栈，调用约定错误；未知身份仍写世界；新 Hook 没纳入同一安装/恢复事务。
- 创建成功但 A 身份/健康改变、B Variant 非 0、精确 pair 不匹配、无法消费同 scheduler 输入、退休或恢复不完整。
- 场景没有实际进入原生 Baby 模式，只有普通 Join 成功：记 NOT_OBSERVED，不能用于关闭本问题。

唯一场景：新进程，隔离 Offline，初始仅 A（阿萨泻勒、键盘），注入前穿过一个普通门并停在安全房间；不人为写 run flags。注入后记录原生三处 Baby 判断为真、自有 B 定向选择普通路径、至多两 pass 内一次原生普通工厂创建、B Type=1/Variant=0 和精确身份、A 健康不变；短暂移动/攻击和 B 脚本；回主菜单后正常 stop。缺少任何实际覆盖记 NOT_OBSERVED；错误记 FAIL。保留源快照、IDA 静态资料、操作清单、fixture、原始事件、逐项结果和恢复字节。

## 静态依据与设计

IDA 数据库目标为本机 `isaac-ng.exe`、image base `00400000`。`5BA980` 只读 `Game+26548` 的 bit `10`；`33FC80` ChangeRoom 在 `33FD00` 设置该 bit。Join `5BAC40` 的三处调用是 `5BAE7E`（ESI=当前 K、健康检查）、`5BB081`（ESI=当前 K、立即 Baby 创建）、`5BB288`（EBX=selector selected-index，K=[EBX-8]、跳过普通确认）。第二处 true 调用 `5BD3B0`，其中 `5BD49F→5B98E0` 直接生成并加入 Main 的 Co-op Baby，并转移宿主健康；普通完整 Player 则由 `5BB415→5B9CD0` 创建。

tx-01-51 #114/#115 两次 action19 true、确认14未调用、Main出现相同自有 K/pair、g_attempt=0，与 Baby 路径吻合；旧捕获没有模式标记，因此原失败的具体来源仍是静态推断，不能改成已运行时确认。

仅自有 B 的健康受控 Join scope 选择完整 Player 路线：三处 wrapper 各调用原 predicate 恰好一次，保存原生结果；ESI/selector K 与当前唯一 exact pair、同一 Game/manager/A、主线程匹配时返回 false。其他设备返回原值；不改全局 flags。自有 Join 内所有 gameplay 查询中性，只有两 pass 的既定 Triggered 命令参与控制，特别是 possessor 8/10 Pressed 为零。普通工厂仍是唯一创建 owner，不追加直接创建、重绑或重试；B 还必须为 Type=1/Variant=0。

三处新 callsite 纳入统一 Hook 事务，48→51 槽；原生命周期、输入队列、NEVER root/DLL 常驻与退休保持。predicate 原始字节分别为 `E8 FD FA FF FF`、`E8 FA F8 FF FF`、`E8 F3 F6 FF FF`，都指向 `5BA980`。naked wrapper 保存通用寄存器、flags 和 XMM0–7，仅替换返回 EAX；特别保留第一处仍活跃的 EDX。第三处 K 在 ECX，并用 EBX 的合法 selector row 再核对。

## 实现和复现入口

生产修改在 `cs-t0/native/src/coop_active_runtime.cpp`，控制器恢复计数同步为51。Join scope 开始前记录唯一 Game/manager/A，之后的自有输入、模式选择和普通工厂均核对该上下文与唯一 exact pair；工厂发生后本 pass 的自有查询立即中性。普通工厂仍只允许一次；未经 owner 的 Main 增员明确失败，不放宽身份接纳。

| 事件 | 字段含义 |
| --- | --- |
| JoinRoute=28 | command=pass；target=真实线程ID；action=callsite RVA；query=原生bool；decision=选路bool；detail bit0=owned、bit1=定向选择；boolean=观察/身份健康；before=K/manager/原flags/A；after=返回后flags/device/control/original调用次数1 |
| JoinOutcome=29 | command=pass、action=phase；query=原Main数量1、decision=返回后数量；detail=B Variant或无B时UINT_MAX；before/after=A最大红心、红心、魂心、骨心原字段；boolean=同一Game/A且健康不变 |
| CreateAttempt=11 / Joined=12 | 前者detail=普通工厂callsite，后者after[2:4]=实际Type/Variant；都带pass，可关联单次创建和精确pair |
| JoinQuery=10 | 额外记录原生实际调用的 possessor Pressed8/10=0；没调用的查询不伪造结果 |

统一入口：[`accept-abnormal-join-e2e.ps1`](../../../../../../cs-t0/accept-abnormal-join-e2e.ps1)。fixture/操作清单为 `cs-t0/testdata/tx-01-56-abnormal-join-{script,operations}.json`；报告 `cs-t0/report-abnormal-join-e2e.py` 只分析本次捕获，保留基础完整报告。静态反编译、原始字节/指令见 [`tx-01-56-join-static.json`](tx-01-56-join-static.json)。

```powershell
pwsh -NoProfile -File .\cs-t0\accept-abnormal-join-e2e.ps1 -GameExe '<本机isaac-ng.exe>' -ArtifactDirectory '.\artifacts\cs-t0\tx-01-56-live-01' -Authorization '<实际隔离场景和操作授权>'
```

控制器启动后先观察 Join，需要新鲜 B 操作时发送一次 `script`。用户确认完整B、A健康/输入正常并回主菜单后，在同一次捕获内写 `manual-join-observations.json`，然后 `stop`。报告核对实际三个 Baby=true 的自有选路、外部设备保留原值、健康/身份、scheduler/native消费者、去重/旧generation拒绝和51槽恢复。缺少Baby=true不判通过。

## 唯一整体验收结果

用户确认已完全重启、隔离Offline仅A并跨普通房间后，执行统一入口一次。Rust Release、Native/helper v143 Win32 构建及注入成功；PID47288、creation time `134366977215579574`。目录 [`artifacts/cs-t0/tx-01-56-live-01`](../../../../../../artifacts/cs-t0/tx-01-56-live-01)；专项 [`result.json`](../../../../../../artifacts/cs-t0/tx-01-56-live-01/result.json) 为 **21/21 PASS**，入口退出0。435,907事件、7,777 Authority tick；nativeError=0、transportFailure=null，重复Resolve/commit/consumer/accept及不一致查询均为false。未运行分析期测试或自动重跑。

| 实际证据 | 序号与结果 |
| --- | --- |
| 自有pair单次注册 | #50，K=1，唯一device/control |
| 原生Baby为真、自有B选择普通路线 | #60 `5BAE7E`、#62 `5BB081`、#63 `5BB288`，native=1、selected=0、flags前后均16；第二pass #109/#110继续同样策略 |
| 物理K=0原生路线保持 | #57/#108，native=selected=1，flags保持16；全部7次模式观察均为同一主线程31332，原predicate各调用一次 |
| 两pass内唯一普通工厂与完整B | #56/68第一pass、#107/117第二pass；#114普通工厂 `5BB415`，#122唯一绑定，#123 Joined，实际Type=1/Variant=0、Subtype=0，K/pair及A/B身份匹配 |
| 宿主健康未转移 | #69/#119，每个Join前后最大红心/红心/魂心/骨心均 `[0,0,6,0]`；Main数量1→1、1→2 |
| possessor输入中性 | #59原生实际Pressed8=false；Pressed10因原生短路没调用，不伪造覆盖 |
| A/B输入消费 | 新鲜命令经共同scheduler被Resolve，Movement/Shooting原生world witness观察到变化；重复包和旧generation包被拒绝 |
| 正常结束 | #435850/#435851菜单teardown，#435855 pair移除/root=1，#435856–#435906共51目标原字节恢复，#435907汇总；pinned=true，同一游戏进程存活 |

用户独立确认“完整 B、A 血量和操作均正常，已回主菜单”，已在stop前保存为 [`manual-join-observations.json`](../../../../../../artifacts/cs-t0/tx-01-56-live-01/manual-join-observations.json)。原始事件、源快照、静态反编译、fixture、操作/调用记录和恢复字节均在同目录。完整基础报告 [`full-feature-result.json`](../../../../../../artifacts/cs-t0/tx-01-56-live-01/full-feature-result.json) 保留 NOT_OBSERVED、controllerExit=1，因为此次没有执行完整T0-05用例；专项入口仅对本目标给出PASS，不覆盖该报告。

已实机解决：原生跨房Baby条件下，自有B可以通过现有owner完成一次完整Player Join，不扣A健康。tx-01-51旧异常与该路径吻合，但旧捕获没有原predicate字段，历史具体来源继续标记为静态推断；不改写旧失败。没有主动触发真正possessor创建、OOM/EH或worker竞争，也不据本次样本声称任意Join分支、跨版本或整个T0-05均完成。恢复后的DLL仍常驻，下次实机需新进程。
