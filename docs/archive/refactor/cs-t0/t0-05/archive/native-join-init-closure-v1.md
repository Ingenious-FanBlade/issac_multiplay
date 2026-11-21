> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 普通 Player 初始化条件闭合交接 v1

- 日期：2026-08-31；create-new 静态逆向交接。
- 整体：**BLOCKED_BY_EVIDENCE**。已列明分支的条件证明可标为 **CONDITIONAL_READY / STATIC_CONDITION_SUPPORTED**；完整 Init 无持久写、prejoin 时间稳定性与 active 资格仍未闭合。
- 本轮没有运行时观察，也不作独立测试验收。所有 active/live/unlock/arm/claim/injection/inputOverride 均为 false。
- 本轮只读 IDA 数据库和项目资料；只创建本文及对应 JSON，未启动、枚举、打开、附着或操控游戏进程，未用断点、补丁、Hook、注入或输入，也未修改 IDB、目标 PE、旧 v1、plan v7 或生产/验收文件。
- 核心进展：把 tail 的九处 AddCollectible、三处 callback20、cache 的六处 achievement setter、13 处 callback8、两个直接 cache 递归点和 P0 更新展开成具体条件。**更多 achievement caller 不等于新增不可排除的污染路径**：已证 native readonly guard 可共同挡住这些 setter；它不能挡住不查 readonly 的直接写。

## 1. 目标、绑定与证据等级

所有地址均为 RVA，preferred image base 为 `0x00400000`，x86 machine 为 `0x014C`，image size 为 `0x008FE000`。IDA survey 的 approved-unpacked 标识与前轮相同；绝对用户路径不写入交接。

| 对象 | SHA-256 |
| --- | --- |
| canonical target | `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30` |
| approved-unpacked target / IDA metadata | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| target equivalence | `8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138` |
| plan v7 | `2BFD017329754E89EF4337248DB37468E292C75CD1490FDA30F4BE3A9FB6DCE3` |
| static v1 | `CCB9F206F0E08067724DCF4D0B7297E1D0537FE612F2B2BE9B5024CCEFB67DEB` |
| reverse v1 | `71041323B3BD446CF006F19971E0C3543423A85770FCB992C95B2FE122E4EE28` |
| followup v1 | `02EDCA945ED56A51584ECA62805C3FD6FD88913A498725F2A47A2B5C1366B555` |
| 本轮机器证据 JSON | `B5C5D48337F8B066FACBD0DE665254DB785B67EB8C197EA388A981FD6EC7BC94` |

机器证据为 [coop-native-join-init-closure-v1.json](../../../../../../cs-t0/coop-native-join-init-closure-v1.json)。它包含 83 个完整指令窗口/数据锚点，另绑定 catalog v1 和 static graph v1。工具是 `survey_binary`、只读 `py_eval`、`decompile`；伪代码用于数据流，ABI、分支和阈值再核原始指令。

`STATIC_CONFIRMED` 表示当前构建的原始指令或明确前提下的路径证明；`STATIC_CONDITION_SUPPORTED` 不表示前提已在运行时成立。`conditionalReady=true` 的范围仅为每项列明的分支，不能解释成整个 active 路线可启用。本文没有新增 `RUNTIME_OBSERVED`。

## 2. 正常原生候选场景及最小共同保护

候选是正常 UI 建立的新 ordinary Isaac（type0、Variant0）run，P0 使用 Keyboard，只有一个未分配的 concrete XInput B，在初始房间、不移动、不使用道具、不进入菜单/重启/换房的声明窗口中走两次自然 manager pass。独立 persistence-window 逆向已找到普通、非 special 的显式 seed 输入成功路径，原生 New Run 将 readonly 置一；其完整锚点和窗口约束由另一份交接负责，本文不提前冻结另一份尚未发布的文件。

本候选不要求改写 ModeLatch，也不要求系统没有与实验无关的 Steam 背景活动。它禁止本实验新增持久污染，不能把 known Steam backend 存在本身当作禁止条件，也不能把背景例外升级成允许持久写。

最小共同保护是 onlineRoot `+0x15`，即 persistent `+1` 的 native readonly。已冻结的 achievement setter `0x529A20` 在任何该 setter 的持久内存写或 Steam 提交之前检查它。因此以下已知 setter 不需要再额外证明所有 threshold 不可达：

- cache 六处 ID 144、19、58、370、355、361。
- P0 resources updater 的 ID 2、583。
- 本轮顺带定位的生命值 helper `0x3588A0`（ID1，max hearts threshold）和 `0x358D00`（ID80，soul health threshold）。
- AddCollectible 已知经过的 achievement setter；其其他 direct write 仍须单独排除。

这是**条件组合**：正常建立 readonly 已有正向链，完整 Init/tick/停止期间不被临时清零仍属于窗口稳定性证明。不能为了得到安全状态而直接写 readonly，不能由 readonly 推出“所有统计、cache、磁盘与 Cloud 均不写”。

对不查 readonly 的直接写，本轮进一步采用原生可读、精确数值的限制：

| 候选输入 | 原因 |
| --- | --- |
| root `+0x26548` bit0x10 清、`+0x26584=0`、`+0x26630=0` | 原已证 ordinary selector 和尾部分支 |
| root `+0x26589=0`、`+0x2663C=0` | 排除 DLC/模式额外初始道具和整组 kit switch |
| persistent `+0x55=0` | 普通 Isaac 的存档解锁会添加 item105；空 UI 装备不能替代它 |
| persistent `+0x484` 低16位为零，roster 三指针 canonical zero | 排除已识别的计数消费、道具添加和直接写回 |
| config0 初始 item 向量为空，card/pill/trinket/pocket-active 为零；config coins=0；modeConfig `+0x74=0` | 排除九处 tail AddCollectible 和随机 card/pill 等来源 |
| callback registry 的 `0x100/0x200/0x100000` 位均为零 | 分别覆盖 cache8、PlayerInit9、callback20 |
| 无 special seed effect 33/35/36/79 | 分别排除额外69金币、scale种子修正及衍生 implicit item；不能根据 seed 名字猜测 |
| P0 无 twin/related child/owned familiar；有效物品、trinket、effects 来源满足第5节 | 排除额外实体分支；它不是“视觉上没道具”的同义词 |

这些条件不保证每份现有存档可用。任何不满足者必须拒绝，不得改存档或清字段。普通数值 seed 与 special seed effect 是两个不同来源，正常 seed 路径本身不能代替 effect 树读取。

## 3. Init/tail 的新闭合

### 3.1 callback20 的 ABI 和三处使用

`0x465340` 是五个栈 dword 的 stdcall helper，末尾 `ret 0x14`，EAX 返回 integer；其第3至5参数传给 dispatch 时按 char 使用。callback ID 固定20，filter为-1，registry global 是 `0x871690`，实际位测试为 `**registry & 0x100000`。

mask 为零时，不进入 Lua dispatch、不读取替换结果，返回原第二参数。mask 非零时，派发并可用 Lua integer 替换该值，再 unref。这里不是输入驱动 callback `0x6209E0` 的 ABI；55 槽驱动及其原始 shared-control 引用计数仍完整沿用 reverse v1。

| RVA | 原始字节 / 意义 |
| --- | --- |
| `0x465374` | `C7 45 D8 14 00 00 00`，ID20 |
| `0x46538D` | `F7 00 00 00 10 00`，mask0x100000 |
| `0x465393` | `0F 84 D9 00 00 00`，mask清跳到返回区 `0x465472` |
| `0x465472` | `8B C3`，EAX取原参数 |
| `0x465485` | `C2 14 00` |
| `0x3BC9F1/0x3BDE08/0x3BE5E7` | 三处 CALL rel32，完整字节及 callee 已入 JSON |

原 followup 中该 helper 的“未解明回调”现在已有局部语义与 guard；旧文不改判。mask20 的排除与 callback8/9 独立，不能仅要求“无 player-init callback”。

### 3.2 九处 AddCollectible 的充分排除条件

tail 的 player config 从 onlineRoot `+0x2A678/+0x2A67C` 向量按 type 取值，stride `0x148`。本轮使用的 config 字段包括：

`+0xB0` red hearts、`+0xB4` soul、`+0xB8` black、`+0xBC` coins、`+0xC0` bombs、`+0xC4` keys、`+0xC8` card、`+0xCC` pill、`+0xD4/+0xD8/+0xDC` starting-item vector、`+0xE0` trinket、`+0xEC` bone hearts、`+0xF0` pocket active。

mode config helper `0x306940` 在 root `+0x26630` 非零时使用 embedded config；否则使用 root `+0x26584` 索引 onlineRoot `+0x2A664/+0x2A668`、stride `0xA4` 的表。合法可读 type0/config0 仍须进入最终 reader 契约。

| 调用点 | 所在来源 | 充分排除条件 |
| --- | --- | --- |
| `0x3BD843` | copied starting-item list，加角色解锁/模式 kit | 原向量空；type0；persistent+0x55=0；root+0x26589=0；ordinary mode；root+0x2663C=0 |
| `0x3BDFEB/0x3BE01D` | config type9/30 随机初始道具 | config.type0 |
| `0x3BE05B/0x3BE08B` | mode config+0x74 添加238/239 | modeConfig+0x74=0 |
| `0x3BE32A` | persistent+0x484 计数消费 | ordinary、空 roster、计数低16=0 |
| `0x3BE434/0x3BE48B` | alternative mode 的继承/转移道具 | root+0x26584=0、+0x26630=0，走 normal branch 到 `0x3BE602` |
| `0x3BE9F2` | challenge31 初始道具 | root+0x26584=0 |

`0x3BCC80` 的 kit switch 先取 root+0x2663C 再减一；零成为无符号超界值而走 default，**不是**执行 kit0。普通 Isaac 添加 D6 的条件除了 persistent+0x55，还含 onlineRoot+8==2 且 root+0x26630>0 或 root+0x26589非零；上述条件一并排除。

tail 最前面 effects667 分支可提前 cache/P0 update 并 return；无 effects667 排除它。tail 的 type19/35 子玩家分支、其他特殊角色效果分支由 type0 排除。ordinary、空 roster 时 `0x50AC70` 返回 null，caller `0x3BE3AD` 跳到 `0x3BE602`，仍沿用 followup 的独立限域在线依赖结论。

### 3.3 不能把任何 AddCollectible 当作天然无污染

本轮在 `0x35F3D4 → 0x52A2D0` 确认通用收藏登记，早于 item-specific switch。readonly 为零、item ID合法且未记录时，它写：

- persistent `+0xAE8+item` byte=1；
- persistent dirty byte=1；
- 再调 `0x52A350`。

`0x52A2F4` 的写指令为 `C6 84 30 E8 0A 00 00 01`，`0x52A2FC=C6 06 01`，`0x52A2FF=E8 4C 00 00 00`。此 sink 自身有 readonly guard，但 AddCollectible 还有不由它保护的 item-specific 分支，例如 item381 对计数的直接修改。因此候选选择排除上表全部九处调用，不以“105只是D6”或“曾经见过该道具”证明安全。

## 4. cache 六 setter、13 callback 和递归分支

### 4.1 六 setter 的具体条件

以下数据条件是可独立检查的局部充分条件，也提供不依赖 readonly 的窄备选；最小持久化合同可以直接组合已证 readonly setter guard。它们不要求把 cache mask 写零，也不假设初始 mask 永远为零。

| setter | 直接 call RVA | 实际聚合 / 排除条件 |
| --- | --- | --- |
| 144 | `0x36E0C5` | `(Count73 + Effects73 + Player+0x153C)/4 >=1`；非负总和<4可排除 |
| 19 | `0x36E1E3` | `(Count207 + Effects207 + Player+0x1540)/4 >=1`；同上 |
| 58、370 | `0x36E5A8/0x36E5BF` | 全玩家238/239计数配对聚合，paired byte非零才调用；所有相关玩家的238或所有239均为0可排除 |
| 355 | `0x36F2E0` | item1..732中满足history predicate且config.type4的 Count+Effects 合计>=5；这些有效计数为0时无需再要求history空 |
| 361 | `0x36FBA1` | scale重算及callback后，`min(P+0x14F0,P+0x14F4)>=3.0` |

前五处位于 cache flag0x200、Variant0、initialized 的相应分支；scale位于flag0x800、Variant0分支。单纯“不调用cache”不是所选路线：tail确实在 `0x3BE60A` 调用它。

scale getter `0xC780` 明确取两个 float 的较小值。比较 `0x36FB86` 是 `0F 2F 05 FC A6 BA 00`，绑定数据RVA `0x7AA6FC = 00 00 40 40`（3.0）。有限值 min<3排除进入setter；不要忽略NaN行为。Init由数据 `0x83793C/0x837940 = 00 00 80 3F` 建立两个1.0。

cache在比较前会重算：pill-history32/33差值、Player+0x18D4/+0x18D8、物品71/438/12/302/340/598/120/121、effects、special seed35/36及room type45均可能影响结果。全零数值源时 exponent为0，经 `0x105B40→0xE4690` 转到正常导入的 pow，结果为1，再由 `0x6104E0` 向量乘法及 `0x3DD3B0` 两dword赋值写入scale。此普通CRT数学语义不能被解读成允许未知替换的import。**只读prejoin scale=1不足以单独证明重算后的比较值。** JSON将每个float比较/加载指令与对应constant anchor显式关联，不把普通绝对内存引用伪装成 IMAGE_POINTER edge。

### 4.2 callback8 与额外实体

13处cache callback均到 `0x463E90`，ID8/mask0x100，完整callsite保存在JSON；mask清即不派发。其结果不是“整游戏无任何回调”，也不证明其他实体初始化自动无回调。

两个直接cache递归点：

- `0x36F07B`：创建子实体后重算。相关 desired count 来自 trinket180（存在时max(1-effectsCount180,0)）以及另一组 collectible667。对应有效计数为0且既有owned child集合空时，不创建/移除这些子实体；第二组也不会递归 `0x3BC740`。
- `0x37034B`：需要 cache flag `(flags & 0x80000010)==0x10`，且 `0x17820` 返回 twin `Player+0x1E68` 非零。twin为0排除它；constructor `0x354653` 明确写该字段为0，P0也需准入保证。

cache还广泛调用 familiar reconcile `0x363370`。它将负desired归零，遍历匹配familiar；超过desired会virtual-remove，不足会创建并virtual-init。**desired=0且没有匹配既有familiar** 可局部排除这些操作。不能只看两个直接递归call，就忽略这条更普通的实体初始化来源。下一步只需把所选普通场景下各 desired-count 输入的来源闭合，不是逆向全游戏所有实体。

## 5. “有效空装备”需要哪些真实来源

`0x370CA0`（count）、`0x3706E0`（has）、`0x3CB6E0`（trinket）并不只读背包主向量。JSON `stateSourceLayout` 给出逐字段宽度与偏移；以下是对污染相关条件最重要的来源：

| 来源 | Player相对布局 / 数据流 |
| --- | --- |
| raw collectible counts | +0x16C8/+0x16CC，dword项 |
| active items | +0x1580/+0x15A0/+0x15C0/+0x15E0，stride0x20 |
| temporary add/remove | ID+0x2EF4、trinket+0x2EF8；方向byte+0x2EF0。constructor的方向byte为1，不能误记为0 |
| effects | embedded+0x1508，begin+4/end+8，entry stride16；getter仅扫描匹配类型/ID；空向量返回0 |
| implicit grants | type、+0x2028 flags、特殊seed79衍生helper、related player等；type0的 `0x371200` 固有角色grant为false |
| extra item sources | +0x1F80/+0x1F84 map、+0x1FB8单item、+0x1FD4/+0x1FD8 byte vector |
| trinkets | +0x16C0/+0x16C4 slots、+0x1748/+0x174C smelted pairs、+0x1FE0/+0x1FE4 byte vector、temporary trinket、递归21/64/75 grants等 |
| related players | twin+0x1E68、related player+0x1E6C，不得允许隐式额外owner |
| per-cache carry | +0x153C/+0x1540；coin-heart prior/pending words+0x1544/+0x1546 |

constructor/Init已经给出有用的正向零来源：twin、temporary IDs、effects容器、raw vectors、cache carries、相关byte vectors被初始化/清零；Init把type从创建参数写入。type0、无special seed79可排除getters的普通角色grant和特殊衍生grant。无items/effects时 `0x371410` 不进入其隐式道具加入分支，而走costume检查；`0x35D850` 在costume vector为空、related player为空时没有erase/recursive操作。

但这些事实与“完整effective-empty已建立”仍有差别：B尚未创建时不能读取B；cache前已执行health/config/初始物品、资源同步及reconcile等调用。需要把这些已知零来源和分支guard连接成同一条、有限的前序状态传播证明。未来在创建后读到空值不能作为事前准入证明。

## 6. P0 更新不能遗漏

`0x3597E0` 在ordinal合法、root+0x26614<2时遍历主player集合，通过 `0x3596A0` 复制共享coins/keys/bombs等。调用名或调用owner是P0，不表示它只更新P0。复制本身是预期world-state变化，不归为persistent写。

其额外分支可以进一步局部排除：

- target initialized时，Item109会加cache flag并重算；有效Item109=0排除。
- 还会调用 `0x3CE420`；有效Item501和Trinket156均为0、prior/pending hearts words为0、正常非负health时，`0x3CE390` 计算0，不发生coin-heart回转，只剩局部重入flag等状态处理。
- 另两个资源更新helper `0x3D2B70/0x3D2DF0` 的特殊角色body由type0排除。
- coins>=55可调用ID2；coins<=0与root+0x2654C bit0x800等条件可调用ID583。0<coins<55，或coins=0且该flag清，分别是充分局部排除；共同readonly亦可直接挡住setter。

`0x3BBBD0` 在同步之后还会追加config coins；special effect33在 `0x3BC63E` 调 `0x359400(B,69)`。原始seed lookup链从 `0x3BC610` 写ID33开始，在root+0x1BBD8的树中查找。这是“prejoin P0低coins”不够的具体反例，候选必须排除该membership或依靠已证共同setter guard，不可省略来源分析。

## 7. prejoin 的静态布局与真正未冻结的契约

followup v1 的独立限域结论继续有效：稳定owner、单P0、唯一B、已知backend、roster三零、onlineFrame零、无NetInputDevice和合法ordinary selector，静态支撑所选正向local路径不依赖OriginalOnline session/ready/channel。这里不需要未来passive `46→49→51` 见证。

已知owner和布局：

- GameRoot slot RVA0x871678；onlineRoot slot RVA0x87169C。active准入必须要求onlineRoot有效非null，不能直接沿用某些passive读法接受null。
- PlayerManager是GameRoot+0x1BAA8 embedded，主vector+0/+4/+8、stride4；selector+0xC、4项、stride0x14，字段controller/type/index/scale/auto。
- InputManager是RVA0x857B18静态对象，不是指针slot；vtable0x7827F4，pair vector+8/+C/+10、stride8，device controller key+0xC。原lookup锁mutex+0x14并取首个匹配，不能替代唯一性扫描。
- Keyboard vtable0x7A1678；Gamepad0x782C80；backend slot0x879A58、允许的known vtable0x7A2440。
- onlineRoot roster+0x4B3D8/DC/E0必须canonical zero；frame+0x4B3E4=0。
- secondary vector+0xD0/+0xD4已有读取证据；完整capacity及对象生命周期的reader合同仍须冻结。

剩余事项分成两类，不能混淆：

1. **静态数据流**：B在publication前不存在，必须证明选定constructor/Init前序链建立有效count和desired-entity前提；以及不调度窗口中的pending transition/save。不允许以事后采样补前置证明。
2. **实现/未来运行验证**：把已知字段固化成完整有界reader，明确所有宽度、capacity、canonical-zero、读失败及未知backend拒绝。generation是adapter自有计数，不存在一个可猜测的“native generation offset”；必须定义owner/vector/device替换和ABA的invalidator，以及两pass之间的采样/漂移策略。后者缺runtime不意味着局部分支静态分析还需无限扩展。

## 8. 最小剩余静态工作与停止边界

不再把“所有持久化类别未查完”作为工作项。本轮剩余两项均有具体入口、owner和输出：

| ID | 精确范围 | 所需证明 |
| --- | --- | --- |
| INIT_EFFECTIVE_STATE_PROPAGATION_NOT_FROZEN | 353DA0/355470 → 3BBBD0 → 3BC740，frontier 3706E0/370CA0/3CB6E0/371410/363370；fresh B及P0 | 将已证构造零值、config限制和callback guard连成一条ordinary type0路径，得到cache有效计数及desired-familiar前提。已知setter统一用readonly，不逐个追加health阈值研究 |
| INIT_PENDING_LIFECYCLE_WRITE_EXCLUSION_NOT_FROZEN | 同上并含363570，GameRoot/onlineRoot | 与独立window交接组合，证明选定初始化不调度GameRoot+0x1B83C/+0x1BA78、onlineRoot+0x4B130/+0x4B288，不临时清readonly+0x15；不能只搜索顶层文字偏移当作别名/传递callee证明 |

window实例已定位pending room-transition路径：GameRoot+0x1B83C非零时自然GameUpdate会进入换房，后续有不受readonly保护的dirty/save链。这是需排除的明确状态流，并不表示它在本候选中已经发生。adapter“停止Hook”也不得默认为执行任何原生退出/重启入口。

若这两项被新的有限证据闭合，剩余字段reader/generation实现和独立验证应转入对应阶段；**不应因没有runtimeObserved而继续无界静态研究**。当前它们还不是仅缺运行读回，故不能发布完整无污染合同，也不能用55槽恢复或文件hash一致覆盖它们。

## 9. 交给主线和独立测试

本文不增加槽、不改变driver、callback ABI或refcount路线；55槽仍沿用前轮精确两PUSH callback wrapper，禁止拆成spawn/assignment/HUD三个直接调用。原static v1、reverse v1、followup v1和plan v7字节保持不变；原七个缺口不被本文自动删除。

83个anchor采用统一格式：`rva/expectedBytes/kind/evidenceLabel/decodeStartRva/decodeEndRva`，完整instruction rows，direct edge使用CALL_REL32/JCC_REL32/JMP_REL32/JCC_REL8/JMP_REL8；浮点数据为EXACT_DWORD，不使用错误的IMAGE_POINTER边。每个窗口的contextBasis记录已知functionEntryRva和从该入口线性解码到窗口边界的依据；独立测试应重新解码，不采信布尔标识本身。机器JSON的memoryReferences将阈值加载的disp32与constant anchor关联。

独立测试可只读校验目标hash、全部字节、边界、relative target、float常量和predecessor binding；这些结果只算新增静态证据复核，不是prejoin/主动触发/无污染实机验收。生产preflight仍绑定static v1，本轮交接的conditional-ready分支不构成可触发权限。

