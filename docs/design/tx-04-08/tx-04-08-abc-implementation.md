# tx-04-08 A/B/C 工程续作

日期：2026-10-27。仅实现与静态资料分析；本轮不运行测试，A–G 完成后的唯一整体 E2E 保留。

当前交付：A 的读取/撤销接线、B 的受限入口/真实失败分支/统一编排及归约、C 的可核定场景参数和 episode 索引已编写。**Player Replace 的完整自然触发资格仍 UNKNOWN，默认禁用；复用、重复发布及特殊保留对象只能按实际观察登记，不能宣称覆盖。** Rust harness 与完整 DLL 已完成纯编译链接，未部署、启动游戏、运行 Lua、replay/compare 或任何测试。D–G 和整体功能验收尚未完成。

## B 的失败语义（编写入口前登记）

控制口专用 `cs world_test <token> <generation> <mode> <value>`，要求配置 `world_record.e2e_token` 非空、来源为本地 port、当前录制 generation 相同且仍在 Recording。每份录制只接受一次故障注入；reader 恢复单独使用 `resume 1`。无任意地址、路径或 UDP 接口。

- `reader_pause 250`：raw reader 在本 DLL 内暂停最多 250 ms，生产线程照常申请槽；真实无 FREE 时生成背压 gap。stop/fault 取消暂停，超时自动恢复，不补旧世界。
- `objects 1` / `memberships 1` / `relations 1`：降低当前录制的自有枚举预算，真实计数超限产生不完整样本；不删除固定字段。relations 预算计入实际 Player 节点，默认 A/B 两节点足以触发。
- `read 1`：准备线程建立 PAGE_NOACCESS 自有页，下一个合格字段读取经同一 SEH MemoryReader 读取该页并失败；不能描述成游戏坏指针。
- `source 1` / `root 1`：只更改采集器保存的比较期望值，真实前后比较拒绝；记录原值与注入值，不改变游戏 root/descriptor。
- `reference 1`：首个非空引用在自有解析判定点标为 Ambiguous，保留 raw 地址与原解析状态，不写 EntityPtr；没有非空引用时未触发，不能 PASS。
- `events 1`：reader 暂停后，向真实事件环投递明确测试诊断事件直到环满，保留实际成功次数与容量失败；不伪造构造/析构。
- `thread 1`：自有后台线程仅调用 observer 的外线程检查，传空对象，不执行 Hook/original、不访问身份表。故障仍由主线程桥收取。
- `file 1`：writer 关闭自有 captures 句柄并走真实写入/flush 错误，保留前缀；禁止补造成功终态。

参数错误、配置关闭、token/代次不符、重复注入、录制未就绪、线程创建/自有页分配失败必须明确拒绝。账本含请求、录制、generation、mode/value、生产 attempt/事件 watermark、实际故障及收口、Host tick/Ack 前后。超时未 Closed 不新建录制，不释放在途 owner；其余故障 NOT_RUN。

实际实现还限定：暂停参数 100–500 ms，其余模式只接受 1；`fault-events` 在准备时将本录制真实事件容量降为64并写 manifest，等待 reader 的原子暂停确认后，主线程最多投递65个测试诊断事件。没有主线程等待。事件使用空对象的 `GapOrUnknown`、result=`0x54455354`，对应录制有独立 TEST_INJECTED 诊断，不生成假的 Factory/Init/出生。线程通知不调用 Hook，且不增加 Hook record 的 original 调用计数；准备 worker 在标 Closed 前等待该通知线程真实退出。

不可读页在后台准备，按 VirtualAlloc 的64 KiB保留范围记入同一 ledger；其非拥有指针随录制存活，reader/writer和通知线程真实退出后才释放。预算失败不提交不完整来源的成员退出，避免低测试预算伪造物理退休。writer 的 captures 句柄关闭后，实际 WriteFile(INVALID_HANDLE_VALUE,...) / flush 失败保留 Win32 错误，`files_flushed=false`、CAPTURE_ERROR，不补成功终态。

主线程只修改采集器状态或有界原子开关，后台只持有 owned values。`source/root` 的游戏原值和修改后的自有期望值分别保留，`reference` 保留实际 raw 指针与原解析状态；它们均不写游戏。首因仍不覆盖；`test_injection.rejection` 另外记录实际 Incomplete 分支，避免先发生的 FIRST_OBSERVED/NOT_READY 遮蔽本次拒绝证据。

统一入口在正常 primary 关闭后，依次新建 `fault-reader_pause/objects/memberships/relations/read/source/root/reference/events/thread/file` 11份独立录制；不重试。每项保存本次 generation、admission、实际触发快照、Closed快照、Host completed 与 remote tx/Ack 前后；缺非空引用或没有真实背压时归约失败。全部真实 Closed 后保留健康 S9 录制。文件故障的错误收口与健康 stop 分开；未 Closed 时不开始下一项，失败清理按当前真实 label 归属，不能覆盖 primary 的终态。

`fault-recordings.json` 与 `evidence/faults.json` 是未来统一 E2E 的产物；本次没有生成运行账本。V8 已有依据实际触发、作用域、输入推进和收口的正向代码路径；V9 增加这些子证据，但完整 V9 仍需 E 项的直接退出协调。不得因 B 接线完成把完整 V9 改为 PASS。

## A/C 静态与实现记录

字段与 Player 路线补充逆向由用户指定的 GPT-6 Luna / max 执行，主智能体复读原始资料后登记最终采用范围。具体参数静态可达不等于游戏运行可用；Lua 视图为实际回调阶段的独立证据，不补造 native episode。

### A：字段依据与撤销

按当前78字段清单，唯一 `InitUnknown` 是 `0x000A removed_by_factory`，Entity+0x177，宽1字节。代理发现基 Init 的跨字段宽写，主审直接复读目标 IDA 的指令与字节：VA `0x6A4BD6` / RVA `0x2A4BD6`，`66 c7 86 76 01 00 00 00 00`，`mov word ptr [esi+176h],0`。这覆盖 +176/+177；因此 layout 修正为 InitConfirmed。旧 ST3 漏项作为历史保留，见[代理笔记](../../research/tx-04-08-abc-agent-notes.md)、[原始指令摘录](../../../artifacts/repentogon/tx-04-08-abc-static/entity-base-init-removed-by-factory.json)和[主审复读](../../../artifacts/repentogon/tx-04-08-abc-static/main-review-init.md)。Factory 临时置1/清0不是恒零保证。

| 层次 | 当前处理 |
| --- | --- |
| 初始化历史 | 只确认 base Init 中的零写；没有观察过的对象仍不补 Init/出生历史，构造与所有写入方闭包仍未知 |
| 当前资格 | 用户本轮明确采纳 `USER_ACCEPTED_CURRENT_ENTITY_BYTE`；Identity.current_public_ready 独立于通用 readiness，创建 owner 完成或有效 exact-pair PlayerUpdate 才建立 |
| 撤销 | acquire/Init/Morph/Restore、Player 重绑/生命周期失效和物理退休撤销；Morph 正常 owner 返回或新一轮有效 PlayerUpdate 可重建 |
| 复制尾部 | source/root 前后复核拒绝时，撤销本批相关资格并将 +177 字段标 NOT_READY，整批不完整；不以读取成功绕过来源条件 |
| 出生与清单 | FIRST_OBSERVED 不补 birth；固定78字段、宽度、profile不变，manifest字段证据说明初始化与用户规则；不填零、不删字段 |

其余 InitDefault 字段仍按原证据区分构造默认与当前值：Entity valid/位置/速度/spawner/type/variant、Player player_index/twin、Grid variable_seed。通用HP/maxHP的历史语义假设保留；Grid collision_class 的 Init 虚调后态仍由当前原值观察，不由零写推恒零。它们不是新发现的 InitUnknown，也没有改成“全生命周期初始化已确认”。

### C：参数和自然路线

动作使用现有单一 F8 队列，发生于 MC_POST_RENDER；每次 Spawn 都写显式 seed、位置、速度和 spawner_ordinal=0，先确认 A 仍有效、位置在实际房间且碰撞为空地。没有自动找替代坐标、删占用对象或重试。具体参数来自[静态候选资料](../../research/tx-04-08-abc-agent-notes.md)和本地枚举/资源，**其运行可用性尚未观察**。

| 场景对象 | API与具体参数 | 前/后态与观察 |
| --- | --- | --- |
| Player A/B | A=原生新局；B=现有 Host/Join，普通Isaac ordinal1 | 不用 Game:Spawn 或 SpawnCoPlayer 创建 Player；当前字段、动画、unique binding分开 |
| tear | Game:Spawn(2,0,(160,200),(0,0),A,0,400801) | 新对象→独立Lua视图→Remove；下次状态复读 |
| projectile | 9/0/0，seed400802，(160,200)，其余同上 | 新对象→视图→Remove |
| familiar | 3/1/0 Brother Bobby，seed400803，(480,200) | 以A为spawner；视图后Remove；本表不保证不会参与原生AI |
| pickup | 5/10/1，seed400804，(160,360) | Morph(5,10,2,true,true,true)，然后Remove；显式保价/seed/ignoreModifiers |
| NPC | 13/0/0 Fly，seed400805，(480,360) | Morph(14,0,0,-1) 到资源中的Pooter；之后Remove；不把void返回当成功 |
| effect | 1000/15/0 Poof01，seed400806，(160,200) | 新对象与视图；寿命由游戏推进，早退/退休如实失败或缺观察 |
| 短命/复用候选 | 同类effect，seed400807/400808，两个独立标签 | 同动作创建再Remove；必须看真实birth/clear/destroy/use事件，第二次分配不保证复用 |
| Grid | index32，ROCK=2/variant0，seed400809，varData0 | 先确认实际空格、原始GetGridPath=0且无door alias；SpawnGridEntity必须返回true |
| Grid替换 | 同格同类，seed400810，varData0 | 直接调用SpawnGridEntity真实覆盖owner，不先Remove；保存原生前后槽 |
| Grid移除 | 自建rock标签/房间代次/seed/type/variant匹配；PathTrail0，keepDecoration=false | 原始该格路径值必须0、无door alias；观察实际清槽与物理退休各自事件，不能把标签/seed当native身份 |
| 引用故障来源 | 换层后3/1/0 familiar，seed400811，(160,200)，spawner A | 为后续reference测试提供真实非空引用候选；没有实际引用仍不能通过 |

主审复读 `0x813BD0` 确认 PathTrail 原值写 `Room+0x76C+4*index`、清槽再调析构包装；传0有路径影响，仅限制于上述初始路径为0的自建普通rock。门不直接生成：通过实际新局/切房后的 GetDoor/格子映射只读视图，与采集来源11/12交叉观察；八EL、roster、Esau、Player关系按capture实际成员记录，空来源仍如实为空，reserved13/14仍NOT_APPLICABLE。

受限 Player Restore 继续使用真实菜单保存/Continue：外层Game恢复、Manager/Player恢复、控制器重分配、之后的unique绑定与native-update依据分开。Esau Jr.703 的静态caller存在，但 public use flags/实际slot到最终替换owner的完整资格未确认；Flip711条件也未知。`player_esau` 默认禁用且没有猜ABI执行入口。不能自然触发的特殊替换/保留对象、重复发布与未发生的池复用保留未覆盖，不由“尚未观察”改成PASS。

### C：原始事件与episode

LifecycleEvent 由104扩为208字节，环预算继续按sizeof计入256 MiB ledger，编译期预算断言保持成立。新增mutation、candidate tick、实际线程、对象domain/lifecycle/readiness/ordinal、session/participant generation/device/key、原始实参及Grid前后槽；只进入diagnostics，不改跨进程key。出生事件关联真实创建owner call；成员事件保留前后mask；Player撤销在清ordinal/anchor前记录旧代次，重建事件记录本次真实candidate tick，不借最近completed。

`tx08/episodes.rs` 在最终归约读取实际action_id/step、checkpoint事件watermark、参数匹配的call/parent树与local/use/birth/revision，保存前一份及动作区间capture和Player上下文；Continue另保存真实Restore/撤销/重建事件。输出 `evidence/episodes.json`，含完整原始native序号、Lua实际witness/回调阶段和关联限制。INDEXED仅表示已建立索引；时间窗/地址候选不等于因果证明，capture缺席不等于销毁，短命链须有真实正常body/owner事件。完整V4/V5/V6最终判定仍由F项补齐，不在此提前PASS。

## 构建与剩余范围

纯编译记录见[ABC构建说明](../../../artifacts/repentogon/tx-04-08-abc-build/README.md)。没有新增单元测试、运行局部测试或启动任何游戏进程。实际2,000/256次、所有故障触发、Lua参数可用性、完整episode覆盖、旧格式导入及最终V1–V12结果均留给A–G完成后的单一整体入口。
