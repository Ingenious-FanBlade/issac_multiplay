> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 正常功能路径静态确认 v15

日期：2026-10-13  
推进：`tx-01-33`；W01–W05 属于同一完整推进。  
入口设计：[下一步工作设计 v14](virtual-device-owned-abi-static-confirmation-next-work-design-v14.md)  
证据：[IDA/source 静态证据 v15](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v15.md)

**结果：** 本轮闭合了两个 Popup 的实际 callback/payload writer、普通 B Variant 0 可用的 Main member erase ABI、B 自有设备从共享 InputManager 精确移除到 `UNREGISTERED` 的独立路径、可调用的主线程边界、Join 第二次实际创建的合法 no-create continuation，以及 A/B 共用 S2 的一套内部一致工作候选。删除 B 后，Main vector 的下一轮 Player reader 只枚举 A；`9BA000` 随后调`9A8620`，其满足局部guard的分支才会遍历剩余 Main vector，并以Player+0x1618调用`9BAAA0`。正常功能的静态闭合仍有一条有限条件边：A 保留的 K 到下一次 native physical query 的具体选取，取决于独立 context vector 中该 K 的可见对象及 `9BAAA0` 的映射结果；静态对象关系不能代替现场成员/查询结果。S2、V2、retire coordinator 与 Join attempt owner 仍为 `DESIGN_ONLY`，未采纳、未实现；contract/schema/source 均未改。

## 范围与证据口径

正常输入前提：有效分配/对象/缓冲，主线程串行；Offline/local co-op，Online roster 为空；B 是唯一正 K；创建 B 前仅有 A。Online roster、`n298+0x4B3D8` context vector、Main Player vector 和共享 InputManager pair vector 是不同容器，不能互推。沿用户接受的可靠 native Join create/publish 后置假设，不重做完整 Player Init、global manager-this 或 05-06 全网络证明。A=ordinal 0、B=ordinal 1；C/S ClientId 在 session 内冻结并与 K、Player*、device pointer 分开。

保持 device 38 槽、六个覆写、control 六槽、S2、`NEVER`；base ctor 前 pin DLL，ctor 后写自有 vptr；进入 `BASE_CONSTRUCTED` 后不析构或释放对象/control/有效 base 状态，DLL 常驻；每 session 至多一次自有设备创建，失败不重试。A 物理 Keyboard/Gamepad 与 B 自有对象均路由到同一 S2 读取接口，但仍有各自 receiver、participant TLS 和 generation。候选不依赖 Online Manager、NetInputDevice 或 FrameStore。

证据等级：`IDA-DIRECT` 为本轮只读 IDB 的实际函数/raw/xref；`SOURCE-DIRECT` 为当前 source 阅读；`STATIC-INHERITED` 为 v8/v10/v13/v14 已有静态事实，只在明确位置继承；`CONTRACT` 为 contract v2；`USER-ASSUMPTION` 为题设正常前提；`DESIGN_ONLY` 为未采纳候选。没有把任何静态读取写成 live witness。无哈希校验、测试、构建、harness、probe、smoke、canary、verifier、链接检查脚本或 executable 状态模型；未改旧文档、source、contract/schema 或 IDB。

## Q01–Q07 复核结论

| 项 | v15 结论 |
| --- | --- |
| Q01 | `9BC120` 自有 wrapper 要求 target+0x2C 非零、Main count>1、从 index 1 找 target；`9BA000` callee 分支另有 `==2` 特殊 list、`==1` 专用清理，其他值（普通 B Variant 0）仍到共享 Main erase。不能把 wrapper guard 写成 callee 限制。 |
| Q02 | `9BA000` 为 `__fastcall`，ECX 是 Main vector receiver，EDX 形式参数在当前函数体内被覆盖/不参与结果；栈第一 dword 为 target，第二 dword 在当前 body 没有读，尾部 `9BA8D7 retn 8`。Player vtable+0x28→`6ACB00` 是对象清理，不是 deleting dtor。 |
| Q03 | B 自有 device 必须单独走共享 InputManager `A20EB0` exact pair remove 和成员缺席复核，才可记 `UNREGISTERED`。A 原生 physical pair 与 B 自有 pair 区分；不能保留 B 为 REGISTERED 并让 poll 继续非零来代替退休。 |
| Q04 | A 的原生键盘/手柄 query 和 B 自有 `+0x30/+0x34/+0x38` 都调用同一 per-participant S2 reader；A 物理 query 保持原函数恰调一次，B 自有 slot 不虚构或调用不存在的 base original。唯一 writer 候选是 Authority-pre current/neutral、normal-post previous=current/close；retire 只失效 generation/visible，不另写 previous。与 contract v2 §4 不同，未采纳。 |
| Q05 | case2 的 Join call `9551BD` 先于独立 Room byte/bit guards；`6FADAC` 是 `9BAC40` 返回后续，`6FA540` 自身 RET 在 `6FADB2`；`9BB393/9BB3C2` 是参数准备，实际 action query 分别在 `9BB39A/9BB3C9`。 |
| Q06 | `n15/state12` 是 menu-state holder 的 `+0x40`，不是旧 Popup。`9BB415` create 返回后 `9BB424` 无 null guard 调 setter，因此第二次 create 必须在 call 前绕过 native create 并走合法 no-create label，不能伪造 null。 |
| Q07 | 当前 source 未实现、现场未观察、候选未采纳分别报告；均不伪装成 native 缺边。全映像旧 pointer alias 不列为本轮必做。 |

## W01 — 两个具体 Popup 与 A 的五个后续场景

`9C25E0` 清零 descriptor callback 字段 `+0x7C` 与 payload 字段 `+0x80`；caller 参数写入才可覆盖。`95B310` 两个实际调用点 `95B476` 和 `95B720` 都以两个零值作 callback/payload。`9C01D0` 把 descriptor `+0x7C/+0x80` 原样复制到 Popup `+0xA0/+0xA4`。descriptor 的 `+0x6C/+0x70` 默认也为零，复制到 Popup `+0x90/+0x94`。所以这两条实际 Popup 在 action-15/timeout close 时，`9C0960` 只会在非空 callback 时触发的分支不会调用 callback；随后 state 3/关闭动画及 `9C34F0` manager removal 仍运行。Popup 文本追加 `9C3B70` 不覆盖这些 writer。`n15+0x40==12` 是菜单 holder state，与 Popup 对象 state/address 分离。

`954CD0` 顺序仍是 954F7D poll、954F84 selector scan、954F8F Popup manager update，随后 case2 9551A9 busy 检查。busy 只跳过 9551B4/9551BD Join precheck/call；Room byte guard 9551C2 与 `test [n298+0x4ABBC],1` at 9551CF 独立。bit 0 非零从 9551D6 跳 955238；正常 `6FADC0` 只在 byte==0 且 bit0==0 时于 955231 调用。

| A 后续场景 | 有限 native 路径与可证明结果 | 尚不能声称 |
| --- | --- | --- |
| Popup 正在显示/忙 | `9C3990` 看仍登记 Popup 的 bit2；忙时只挡 case2 Join precheck。Room 两 guard 另判。 | Popup busy 不等于 A 的 Room update 一定停，也不改变 A 的 K。 |
| 点击 action15 或正常 timeout 关闭 | `9C0960` state2 写 close sentinel；本轮两 Popup callback 均空，不做 callback side-effect；state3 等 `9C14C0` 动画完成后由 `9C34F0` 移除。之后该 Popup 不再令 busy reader 命中。 | 关闭 Popup 本身不证明 A 已绑定或切到 K。 |
| B 自有设备移除后重新选择 | selector scan/Popup 的实际创建和 close 仍是上方的 null callback 路径。context empty 分支 `95B87F→7A6450(Player,-1,1)` 只在 v91 指向 A 时把 A Player key 设为 -1；context 非空分支 `95B867→90AF30(context,-1)` 是独立 association 路径。 | Online roster empty 不推出 context empty；`-1` 不是 K，也不代表新 B device 注册。 |
| A 原生 Keyboard/Gamepad 正向改绑 | positive K 由独立的 SetControllerId/selection 路径更新 Player+0x1618。候选只增加 A 的 binding generation 并重置 A 的 physical projection history；ClientId/ordinal 与 B 状态不变。 | 两 Popup writer 都为空，不会从 Popup close callback 自动完成此改绑；现场选中设备未知。 |
| B Main member 删除后 A 再更新 | `[A,B]` 删除 B 后 Main vector 为 `[A]`；4186C0 下次枚举只调 A 的 Player `+0x0C` update，A ordinal0/K 不变。`9BA000→9A8620` 会对剩余 Player 和 `+0x1618` 执行 slot/cache 重建。 | `9BAAA0` 映射依赖 context vector 和实际 keyed device lookup，不能静态指定下一次 physical query 的 live receiver。 |

## W02 — 普通 B 的准确删除、selector/context/listener、Manager remove 与主线程点

普通 Join 的现有构造证据沿用 v14 证据 JSON/文本记录：9BB415→9B9CD0 的 ordinary Main Player constructor 为 Variant 0，Player+0x2C=0。`9BC120` 的非零 target field guard 因而不满足。`9BA000` raw 直接检查 target+0x2C：`==2` 去不同 list 并回 shared tail；`==1` 走专有清理；其余值跳到 9BA459 shared Main path。故可由 coordinator 在有效 `[A,B]` 主线程场景直调 callee，而无需伪称 wrapper `9BC120` 已接受 Variant0。

| 边 | IDA-DIRECT 事实 | 普通 `[A,B]` 中的效果/边界 |
| --- | --- | --- |
| `9BC120→9BA000` | 9BC120 有 `target+0x2C !=0`、Main count>1、index1 起 exact pointer scan；命中后 at 9BC1FF call。 | 普通 B=0 被 wrapper guard 挡住，不能复用为入口。 |
| Direct `9BA000` ABI | `ECX=Main vector descriptor`，`[esp+4]=target B`，`[esp+8]=Main vector begin`（按已读native caller栈形状）；函数当前body不读第二栈dword。EDX在主体路径内被重置；direct bridge可置0，但不据此声称现有caller传0。尾部9BA8D7 `ret 8`。 | 调用前只在vector恰`[A,B]`、B exact pointer位于index1且有效Player对象时开放。若precondition不符，保持`RETIRING`，不盲调。 |
| Main exact erase | 9BA5D2→`757AC0(target,1)`，9BA5DB–5F9 exact search/memmove；9BA5FE end−4；9BA609 target vtable+0x28；9BA60C target+0x161C=-1；9BA61D–629 重写剩余 ordinal。 | 只删除 B，A仍 index0/ordinal0；不调用 `SetControllerId(-1)`，不删除 B InputManager pair。 |
| Player cleanup | `+0x28→6ACB00` 设置 flags，调 866060；若 Player+0x408 child 存在，Lua unref/free该 child并清指针。 | 这是 native Player cleanup slot，不是 deleting destructor；不可据名称推导整对象释放。 |
| cache / relationship / context | 9BA642–6B8 对 Main manager descriptor 的 +0x0C,+0x10,+0x14,+0x18,+0x20,+0x24,+0x28,+0x2C,+0x34,+0x38,+0x3C,+0x40,+0x48,+0x4C,+0x50,+0x54 写 sentinel/0。9BA6C2→83BC50 清/压缩两行40-byte关系表；9BA6D0–750 对 `n60+0x5AB8` 的两行 `0xCC` context row 搜索并压缩；9BA752→9A8620 做实体关系清理/缓存重建。 | `9BA000` 是有明显副作用的 Player cleanup，不是纯 vector erase。原始字段值的语义名称不超出已验证范围。 |
| InputManager exact remove | `A20EB0(this,device,control)` 对 shared manager 调 vtable `+0x7C/+0x80` 通知，再在 `this[2..3]` 的8-byte pair vector按第一 dword exact device pointer删除；必要时对 control调用 `n79_0`；函数不返成功bool。真实 pair 调用形状见 `906020→A20EB0` at 906073。 | B 自有对象移除与 Player erase 是两条 native container 操作。移除后遍历确认 exact B pointer不存在，确认前仍是 `RETIRING`。 |
| listener/selection | `913760` callback 对 NetInputDevice 有自己的第二 vector remove分支；非 NetInputDevice路径按 source pair的 key处理 reset。`90AF30(context,-1)` 是 Popup选路，不是 Manager remove。 | 不把 context、NetInputDevice secondary vector、Main Player vector、InputManager pair合并。 |

本轮选定的 coordinator 候选调用点是已有 `loop_runtime.cpp::UpdateHook` 的 TopUpdateExit observation：`kTargetRvas[1]=0x005AB6D0` 指向VA `0x9AB6D0`；source 1002–1018 先调用完整 `g_update()`，再回调TopUpdateExit。IDA静态链为`_main→9AB6D0`，9AB6D0在9AB6F2调用954CD0；954CD0正常Room继续到6FADC0，再进入802980/9BB5D0及Player dispatch；4186C0在其Main vector loop `418D86–418E4D` 调每个Player+0x0C。TopUpdateExit位于这些原生栈都返回之后，且hook已有主线程检查。它也为pause/loading/noRoom后的下一次已完成top update提供串行处理点；这些分支不是runtime被观察结果。

**DESIGN_ONLY 候选顺序：** 先关闭 B Join/input admission、令 slot/binding generation 失效并读为 neutral（不写 previous）；然后在 TopUpdateExit 用 `A20EB0(shared InputManager,B-device,B-control)` 触发原生通知并移除 B pair；静态复读 pair vector exact pointer，不存在后，仅按ref-class bookkeeping释放一次 `ManagerVectorStrong` 并标记 device `UNREGISTERED`，继续持有规定的root strong、object/control/base state及pinned DLL；不得双重消费原生通知与coordinator的manager strong责任。接着以 ABI 上述形状直调 `9BA000(MainVector,B,MainBegin)`，再确认 Main vector恰 `[A]`、A仍ordinal0/key不变；最后按单一A participant继续下一Authority/input/update。Manager通知放在Player清理前是为了callback执行时仍有正常B Player/key关联；该次序是候选而非原生端到端运行事实。删除状态只能在Manager vector中exact B pair确认缺席后到 `UNREGISTERED`。不得调用已有 `VirtualDeviceLifecycleCoordinator::RemoveAndDestroy` 全流程：source `virtual_device_lifecycle.cpp:360–406` 在 `UNREGISTERED` 后release manager-vector ref并调用 `FinalRelease`，与本路线 `NEVER` 冲突。

有限剩余 native 条件：`9BA000` 会重置上述 Main-manager cache fields，随后 `9A8620` 重扫剩余 A并以 Player+0x1618 调 `9BAAA0`；`9BAAA0` 的 keyed route仍读独立 context vector，并通过 `90B010` 查其设备。Main Player下一 reader确定只见 A、InputManager删除只针对B；但 context成员、keyed设备返回对象及实际 physical query receiver为现场值，未观察。唯一待缩小的静态细边是 A 的 retained K 在该具体 context分支中是否被 reader映回同一 physical device，不影响已证 `[A]` dispatcher结果。

## W03 — A 物理与 B 自有 receiver 的同一 S2、TLS、original 和唯一 writer

`SOURCE-DIRECT`：`active_runtime.cpp:489–550` 只认物理 Keyboard/Gamepad vtable及对象+0x0C key，读取两次稳定后采样；`:584–629` 只在 physical owner key与 current Player key匹配时 SubmitPhysical。`:1394–1433` 在原生 Player update 前读 identity并建Player scope后恰调用原函数；`:1035–1065` consumer wrapper先开scope/捕获before witness、调consumer original一次、捕获after并关闭；`:937–1033` logical query hooks先调用所保存native original一次，随后再替换 query返回值。现代码只有 ordinal0/current identity/一个 dispatcher+adapter，且 owner switch会清掉这一个人的generation状态，不能据此直接轮换A/B。

`STATIC-INHERITED` query ABI：device `+0x30` Pressed 与 `+0x34` Triggered 都是 `__thiscall(device*,uint32 action)`，AL/bool 返回、`ret 4`；`+0x38` Scalar 用同一 ECX/stack action、float 在 x87 `ST(0)` 返回、`ret 4`。外围 callback/consumer scalar 层另有输出指针/AL/XMM0，不得合并成 device slot ABI。现行物理 Keyboard/Gamepad wrapper保留其saved original恰一次；owned B槽按这三个自有签名直接实现，不虚构base original。

| query入口 | receiver身份 | TLS/结果 | 原函数与返回通道 | 边界 |
| --- | --- | --- | --- | --- |
| A Keyboard/Gamepad logical hook | `self` 是本帧 native physical Keyboard/Gamepad，+0x0C key与A的Player+0x1618、binding generation一致。 | Player/consumer TLS→冻结 ClientId映射ordinal0→A current/previous；stale/unbound中性，不fallback B。 | saved native query恰调用一次，再以S2覆盖bool/scalar返回。 | 不把A物理device误判成B自有receiver。 |
| B 自有设备 `+0x30/+0x34/+0x38` | `self` 与session self-owned B device exact pointer/vptr一致，slot/binding generation与TLS ordinal1相同。 | 同一 `QueryS2` API读B publication；retiring/stale/unbound中性。 | 自有覆写直接实现`AL/bool ret4`或`x87 ST(0)/float ret4`签名，不调用虚构Base original。 | 不经physical sampler或A的binding key。 |
| UI / outside gameplay scope | 缺Player/consumer TLS时没有participant owner。 | 物理query保留native结果；B自有slot中性，不借用A。 | 物理原生hook仍调用saved original一次后passthrough；B槽无base original。 | 不提交physical snapshot，不触发C/S Resolve或Join命令；source UI调用分布仍未观察。 |
| Player update / consumer | Main-vector Player exact identity及slot generation。 | Player TLS在Player original前；consumer TLS+witness围住consumer original；A/B顺序交换只换/恢复TLS。 | Player/consumer original各调用一次；query返回层在native consumer造成世界效果前生效。 | 当前source尚无A/B participant TLS。 |

唯一候选writer：Authority-pre对每个active participant恰一次发布immutable current/state-or-explicit-neutral、tick、generation及visible；normal Authority-post恰一次置previous=current并关visible。重复physical poll仅按V1既有raw→canonical→sequence规则提交；重复Query/+0x78不Resolve、不写current/previous、不消费edge。`+0x78`只返回native manager所需活动gate、不采硬件/不阻塞/不分配。Retire只关闭admission并失效该slot generation/visible；neutral query立即生效，不另写previous，最终normal post仍为唯一previous writer。共享tick无全员barrier；两个participant各有queue、sequence/replay/dedupe history、planner、pending，分别Resolve至多一次。

| 纸面步 | queue/Resolve（共享authority tick N，updateEpoch e） | current/previous/visible与TLS/query结果 |
| --- | --- | --- |
| A后B，或B后A | `DrainBeforeResolve_A/B`各自执行；`Resolve(A,N)`与`Resolve(B,N)`各恰一次。 | AuthorityPre各写自己的current/state或neutral，保留各自previous并打开visible。Player/consumer TLS跟随具体Player，Query只读同ordinal；交换调用顺序不清queue/generation。normal AuthorityPost各自做`previous=current`并关visible。 |
| 本tick只有A或只有B有input | 有输入的queue消费它的N目标；另一queue独立Resolve一次。 | 有输入slot发布自己的state，另一slot发布全零neutral；不复制，不等另一人。 |
| B命令target=N+1 | `Resolve(B,N)`保留future command/pending；`Resolve(A,N)`独立。N+1再Resolve B。 | N时B current为neutral、previous保持；N+1命中后B current更新。`updateEpoch`不代替`authorityTick`。 |
| 重复poll/query/+0x78 | 同tick/source canonical、edge=0且SamePersistentState样本在sequence分配前去重；重复query不调用Resolve。 | query重复读同publication；Triggered true可被重复读取。`+0x78`只回native poll gate，不写current/previous；不能推成native side effect exactly-once。 |
| 第一轮noRoom，但physical poll先发生 | `PhysicalExit`可先Submit并改变A planner/pending；Room未进入AuthorityPre，所以本epoch没有Resolve。 | current/previous仍保持上次commit；B queue独立。noRoom不等于输入未提交或authority tick发生。 |
| 第一轮完整TopUpdate没有StandardRoom Authority | 若本update前的physical poll有A样本，它已进入A queue/planner；两participant都没有AuthorityPre/Resolve。本轮source `PauseGapTracker`把missing计数从0增至1，低于阈值2。 | 前一normal post已关visible，current/previous不写；outside-scope query neutral。第一帧正常alternating gap不触发PauseEnter。 |
| 第二个连续完整TopUpdate仍无Authority，且不在loading recovery | 不调用Resolve；在第二个TopUpdateExit，source tracker计数到阈值2并进入PauseEnter。候选提升shared apply generation、关闭A/B visible并清各自旧generation queue/planner/history；冻结ClientId map与bindings不变。 | query中性；旧pending/edge不跨pause重放；current/previous仍只有AuthorityPre/normal-post各自原writer。 |
| pause后重新观察Authority | 若PostLoadingAuthorityGate不阻止，source AuthorityObserved检测PauseExit并清gap tracker；候选按新apply generation为各参与者重新接样本。 | 先前publication仍不可见；新AuthorityPre分别写current，normal post再唯一写previous。 |
| loading recovery gate | `LoadingExited`后等待首个Authority；source `BlocksInput`期间TopUpdateExit不把缺Authority计作pause gap；首个Authority清gate并复位tracker。 | recovery query neutral；不做PauseEnter/Exit伪转换，不复用loading前edge；恢复后按新AuthorityPre发布。 |
| B Join pending / B retire | B未Bind前不启用B queue；retire仅清B queue/pending并使B slot generation失效，A queue不动。 | B unbound/retired查询neutral且visible=false，不写previous；A仍用ordinal0。Join身份预留不等于B gameplay active。 |
| normal AuthorityPost后又query | 两队列不再Resolve该tick。 | visible=false，query neutral；不推进previous、不读取前epoch edge。 |

native consumer action许可沿 `STATIC-INHERITED v8 E03`：Movement 0–3/11；Shooting 4–7/0–3/11；Bomb 8；Item 9/10/11；Pill/Card 10/9/11；Drop 11。候选不得沿用当前 strict-family adapter gate过滤native跨family调用。与contract v2 §4“+0x78唯一 latch writer”冲突，明确为未采纳 DESIGN_ONLY，不改contract/schema/source。

## W04 — 继承去重与 held V2 的本轮接线

已证V1只抑制 `edge==0 && same completed tick && same source && SamePersistentState` 的样本，抑制发生在 sequence++前；Canonicalize在前，因此raw轴变化量化后相同也能被抑制。BuildRawSample给Shoot保留aim-level，action8–11只编码0↔nonzero rising edge，无falling，且V1不编码action9–11 held。逐步轨迹继承v14 E05，不重复做完整去重路径。

`DESIGN_ONLY V2`继续保留24-byte state、version=2、held valid mask `0x0F`：bit0 Shoot、bit1 Item9、bit2 Card/Pill10、bit3 Drop11；edge mask `0x0F`仍对应action8–11，Bomb8仅edge。Validate/Encode/Decode/Queue/Planner、SamePersistentState、ActionMagnitude/Triggered全用同版V2，source/schema尚未更改。

| action9/10/11同key输入轨迹 | V2 state与候选行为 |
| --- | --- |
| 无history的首个样本已按住 | held level置相应bit；不造rising edge。 |
| 保持按住重复poll | held不变、无新edge；同tick/source canonical重复样本可在分配sequence前去重；多个query读取同值。 |
| 0→1按下 | held置位且edge对应位出现一次，Submit有新sequence。 |
| 1→1持续按住 | held保持；后续无rising。 |
| 1→0释放 | 清held、history记录falling但不编码falling edge；因持久state变了，同tick不再误当相同state。 |
| 0→1重新按下 | held再次置位并生成新的rising edge/sequence。 |
| source/key/generation改变 | 重置对应projection history；首个样本只发布当前held level，不跨participant造edge。 |

Query Triggered重复可见与每tick破坏性Resolve是两个环节：V2不会让query消费edge。候选需要同时更新V2 codec/state、active runtime projection和每participant queue；不是当前source功能。

## W05 — Join 第二次 actual create 的准确阻止与合法 continuation

`IDA-DIRECT` action14准备于9BB393，真正query call于9BB39A；action15准备于9BB3C2，query call于9BB3C9。stored confirm满足后9BB3DB清byte；selected type准备后9BB40E push type、9BB40F设置ECX Main Player manager，9BB415才是实际`call 9B9CD0`。callee结束于9B9F5D `retn 4`，也就是它会弹掉caller的单个selected-type stack dword。回到caller之后，9BB41F把EAX作为Player receiver，9BB421保存Player*，9BB424无null guard调用`7A6450(Player,K,1)`；9BB429仅是setter return。`6FA540`外层在6FADA7调9BAC40，6FADAC是它的正常返回后续，函数ret在6FADB2。

| owner/状态 | 唯一规则 | 失败与结束 |
| --- | --- | --- |
| `OwnedDeviceCreateOwner[session]` | 与B Player attempt完全分离；Base ctor前pin、ctor后自有vptr；自有设备每session一次。 | 一旦BASE_CONSTRUCTED不释放对象/control/base state；失败不重试。 |
| `BCreateAttemptOwner[session]` | 唯一BeginAttempt点放在9BB415实际call之前，token绑定session generation、B ClientId、目标正K、updateEpoch与Join pass token。涵盖stored confirm和同一native call路径，不以重复14 query计数。 | 没有到达9BB415的cancel/navigation/guard fail/no pass仅close command，不消耗attempt；一旦开始后任何bind/publish失败为FailedFinal，不重试。 |
| Native绑定/发布观察 | 只在实际native return、9BB424 setter与9BB429返回之后观察Player/K/vector；按用户接受的可靠create/publish后置，在6FADAC正常pass close且全部后置成立才Succeeded。 | setter return或Append本身不等于发布成功；Retire与pending publish互斥。 |

`JoinCommandLane`按冻结ClientId接收B的有界command；event19开/选pass并固定target K，event20/21导航改变selector并使旧K token失效，action14是confirm query、action15取消并走native no-create分支。query重复读取不消费或延长pass；每个pass token只允许匹配自身updateEpoch/ClientId/K。非lane physical input、native stored confirm、另一K或同update第二pass即使穿过native guards到同一9BB415，也在此Admission校验被挡；检查放在所有已列的同一Join create call路径，而不是只数C/S query。

**精确拒绝点：** 在caller到达9BB415、单个selected type仍在栈顶时做Admission。允许首次有效B lane则调用原native call一次，callee `retn 4` 后继续9BB41A–9BB429。若不允许（AlreadyAttempted、token/client/K不匹配、cancel/navigation、wrong pass），跳过`call 9B9CD0`，先`add esp,4`补偿callee原本的`ret 4`，再跳到native no-create/cancel continuation `9BB561`；该块写`*ebx=-1`，9BB567才增加selector pointer并减少loop count。raw `9BB3CB test al,al; 9BB3CD jz 9BB567; 9BB3D3 jmp 9BB561` 表明action15 query返回AL非零才进入`9BB561`，AL为零直接到`9BB567`；不得把`9BB561`误称作action15 false分支。deny路径必须保留`EBX` lane pointer、`EBP` frame及`var_1C` loop count、`EDI`循环上下文和callee-saved寄存器，恢复原caller ESP后仅弹掉selected-type dword；目标`9BB561–9BB56E`在该分支不读取create返回的EAX。允许路径恢复原寄存器/栈并恰调用callee一次。不得让wrapper返回null：9BB424没有null check，会把空值当Player receiver。

| 纸面场景 | call edge | attempt状态 |
| --- | --- | --- |
| 首次有效action14、K唯一、Join pass有效 | admission允许，9BB415一次、setter到9BB429、6FADAC后置检查 | Bind→Publish→Succeeded；自有设备owner不因此重复创建。 |
| action20/21导航或action15取消 | 不进9BB415；action15 query的AL非零边在9BB3D3进入`9BB561`写-1，AL为零从9BB3CD到`9BB567` | command在pass close过期，attempt仍NotAttempted。 |
| Popup忙、Join/Room外层guard不进入Join，或本update无pass | 无9BB415；TopUpdateExit/JoinPassCloser关闭该epoch command | attempt仍NotAttempted。 |
| 非lane physical input或native stored confirm | 即使原生caller走到同一9BB415，ClientId/PassToken不匹配 | 走`add esp,4→9BB561`，不创B，也不消耗B attempt。 |
| 同update第二pass/另一K | token或K不匹配 | 走`add esp,4→9BB561`，不创Player。 |
| 已Started/FailedFinal/Succeeded后再confirm | 禁止第二次native call | 走同一合法continuation；不返回假null。 |
| actual call已开始但后置失败 | 不做自动rollback/retry | FailedFinal；OOM/EH异常恢复不在范围。 |
| B发布pending时Retire | generation失效，pending不publish | RetiredFinal/FailedFinal，中性且无retry。 |

> 本节只设计在已读native调用点的local Admission，不声称当前production source安装了此控制点，也不扩展为05-06全网络证明。

## P01–P06 功能主题及完成边界

这里“native缺边”仅指已定位而尚未闭合的原生调用/字段/分支关系；运行值、候选未采纳和新source未实现分别放在独立列，不作为native缺边。

| 主题 | native静态已证 | 已证native缺边/有限待缩小项 | DESIGN_ONLY候选 / 未采纳 | 现场未观察 | production未实现 | 下一有限验收 |
| --- | --- | --- | --- | --- | --- | --- |
| **P01 Register/enumerate** | active Manager pair、context、Main Player vectors分离；继承native insert/keyed lookup；Main reader下一轮只枚举留下的A。 | 未证明native缺少所需insert/reader边；有限条件链为`9BA000→9A8620→9BAAA0→90B010`。`9BAAA0`按A retained K做context keyed查询，命中/返回对象受本地guard控制。 | frozen ClientId map及C/S participant queues。 | roster/context/manager现场成员以及该key查询实际返回对象。 | 自有ABI登记、participant路由与reader接线。 | 定位A的context row/key，经`90B010`核对匹配分支及vtable `+0x14`目标；不扩成全alias调查。 |
| **P02 Type/name/binding** | `9BB415→9B9CD0`普通B构造与后续`7A6450` Player+0x1618 setter分开；普通Variant0可由`9BA000` direct Main erase。 | 未证明native构造/绑定缺边；需要的具体native writer为`7A6450`，其调用条件不由两个Popup的null callback提供。 | 38槽/六覆写/6-slot control/NEVER属contract；slot layout与自有vptr安排是候选。 | A/B真实K、对象vptr和generation。 | 自有vptr注册、K绑定和ClientId query bridge。 | 对已选A正K路径核对`7A6450`命中和Player+0x1618写入；不把现场K预设为已知。 |
| **P03 Offline Join/Popup** | 两个具体Popup的callback/payload都是0，`9C0960` null-guard不调用；Room byte/bit guards独立于较早Join；`9BB415`后无null guard setter、拒绝时有native no-create continuation。 | 已证的native缺边是这两个Popup descriptor没有side-effect callback入口；不能靠关闭它们调用project owner。 | command/pass/attempt owner及本地Admission是候选。 | Popup/selector状态、query/pass出现次数和所选K。 | Join lane、pass token与attempt admission。 | 只追两个callsite `95B476/95B720` 的descriptor至`9C0960`关闭场景，并在`9BB415`拒绝路径核对`add esp,4→9BB561`。 |
| **P04 Key/B retirement** | `9BC120` wrapper的Variant/count/index1 guard与`9BA000` callee分支分开；direct ABI可删普通B；`A20EB0` exact device pair remove/通知；TopUpdateExit在完整update返回后。 | 一条有限native静态待缩小项：`A20EB0:0xA20F63–0xA20F66/0xA20F8F–0xA20F95` 条件调用全局`n79_0@0xC7E814`，其引用/副作用与project `ManagerVectorStrong` accounting的对应关系尚未证明。 | `RetireParticipant`先移B pair、确认exact absence，再只释放一次manager accounting并调`9BA000`；neutral generation不写previous。 | 实际manager pair缺席和通知回调执行值。 | B retirement coordinator与UNREGISTERED owner。 | 若复用回调会影响strong accounting，限定核对`n79_0`的真实目标及ref effect；实现时独立枚举active pair exact pointer后提交状态。普通Leave按钮不是前置条件。 |
| **P05 Snapshot/input S2** | native物理query、device slots、consumer/query顺序；source现有A-only original恰一次及consumer witness边界。 | 本轮没有证明原生physical query函数缺失；新B receiver不属于native原函数的既有A身份scope。 | 统一participant S2、Authority-pre唯一current writer/Authority-post唯一previous writer及V2 held语义；candidate与contract v2 §4 poll writer冲突，尚未采纳。 | live current/previous、TLS和具体query receiver。 | 双participant publisher/reader、slot adapter、V2 integration。 | 按现有consumer/query hook边界验证每个participant独立S2，先解决contract writer冲突再进入生产实现。 |
| **P06 Pause/cache/lifecycle** | Main cache字段、B context/listener相关cleanup、下一Main reader的A dispatch有有限静态链；TopUpdateExit不在Player loop或consumer original中途。 | A retained K能否经`9BAAA0`的特定context/`90B010`分支返回同一physical query receiver仍待缩小；这是有限条件链，不是“全生命周期未知”。 | pause/loading下的双participant generation close、`NEVER` retirement与Hook恢复流程为候选。 | cache/context实值、pause/loading/retire顺序与Hook现场状态。 | 满足NEVER的coordinator；现有`RemoveAndDestroy`会在UNREGISTERED后`FinalRelease`，不可整条复用。 | 追`9BAAA0` context selector分支及`90B010`调用返回；保持暂停/加载运行结果为现场验收项。 |

## 可开始的实现与剩余静态边

可以按本文开始实现同一participant S2 reader/API、A/B receiver/TLS分离、V2 held变更候选、TopUpdateExit串行`RetireParticipant` coordinator和9BB415 Admission continuation；这些是新source工作，不是native事实。开始生产实现前仍应处理contract v2 §4 poll唯一writer与候选的差异并完成所需设计采纳；不得把本轮候选写回contract当作已批准。

有限待观察/静态化事项为：A retained K经过`9BA000→9A8620→9BAAA0`时，具体`n298+0x4B3D8` context vector中对应设备是否满足`90B010`及其vtable `+0x14`分支，以及之后 native physical query receiver是否还是该Keyboard/Gamepad。它依赖现场成员，不是本轮全alias调查；B own Manager remove后准确absence必须由新实现枚举验证。其余正常候选链有具体入口、ABI、guards、sequence与下一reader，不再归成“全部未知”。

本轮没有运行任何测试/构建/运行模型，没有读写进程或游戏输入；未更改IDB/source/contract/schema、旧报告证据和工作设计。文档准确性、正常功能静态完整程度、候选一致性/采纳、现场未观察及production未实现分别陈述于上方。tx-01-33唯一整体人工静态文档审阅：**通过（文档准确性与声明边界一致性）**；对读工作设计v14、v15报告/证据、相关source/contract及topic README，事实来源和分类口径一致。正常功能静态完整程度仍为部分完成，P01/P04/P06列出的有限条件边、现场未观察和production未实现继续保留；S2候选与contract v2 §4的`+0x78` sole-writer要求冲突，未采纳。审阅后仅回填实际结果，没有修补或重审。
