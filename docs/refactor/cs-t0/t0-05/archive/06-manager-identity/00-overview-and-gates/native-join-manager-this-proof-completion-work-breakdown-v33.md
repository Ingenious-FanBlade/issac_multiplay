> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this 同代身份完整证明任务分解 v33

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

日期：2026-09-05  
状态：`PLANNED / REVERSE_PAUSED / V30_NEGATIVE_FRONTIER_ACCEPTED`  
当前检查点：[pause checkpoint v32](native-join-manager-this-pause-checkpoint-v32.md)

## 1. 完成定义

本任务的完成目标是 **manager-this 子证明**，不是把整个 CS-T0-05-04 提升为 PASS。完成时必须由正式 v7 inspector/verifier 共同证明：

```text
managerReceiverFormulaStatus = STATIC_CONFIRMED
managerThisIdentityStatus = STATIC_CONFIRMED
managerContextStatus = UNKNOWN
status = BLOCKED_REMAINING_MANAGER_CONTEXT_PREMISES
localChecksPassed = true
allRequiredChecksExecuted = true
overallAccepted = false
exitCode = 1
```

其余八个 manager-context premise 保持 `UNKNOWN`，14 项权限全部为 false，不发布 plan/progress、矩阵或 05-05/T0-05/T0-06 解锁。这个限定出口来自 [v7 独立验收计划](native-join-manager-this-v7-independent-test-plan-v1.md) 与当前 `admission_v7` 决策代码，不能把 manager-this 子证明完成改写成主动实机或 T0-05-04 总完成。

## 2. 当前权威基线

[bulk frontier v30](../01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v30-handoff.md)及其[独立审计](../01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v30-independent-audit-v1-handoff.md)是当前机器前沿：

| 类别 | 正向接受 | 总数 | remaining | 独立负向覆盖 |
| --- | ---: | ---: | ---: | ---: |
| copy/move | 319 | 405 | 86 | 86/86 |
| memset | 225 | 242 | 17 | 17/17 |
| 合计 | 544 | 647 | 103 | 103/103 |

- v30 JSON SHA-256：`12EC336104A9C049EE15E83E634977F2C8CAE3E11AD4AD3E9711DEBCC3D4F814`。
- v30 independent audit SHA-256：`E9331E974ACD5B6DA0F32A3607A89678B9CA3619A768915CCD23C7EC092695D6`。
- `103/103` 负向覆盖只表示全部候选已失败安全地保持 `UNKNOWN`；正向新增闭合为 0。
- [local epoch v2](native-join-manager-this-local-epoch-handoff-v2.md)只在同步 local causal cone 下条件闭合；全线程 wall-clock alias/ABA 仍依赖 whole-image bulk closure。
- production reader 已实现，Debug/Release harness 各 410 assertions 的独立 pre-seal 结果已通过；`runtimeObserved=false`、`processInspected=false` 必须保持。
- [v7 pre-seal 测试](native-join-manager-this-v7-independent-test-results-v2.md)通过 2 个正向 schema、63 个 contract mutation 与 37 个 relationship mutation，但正式 raw v2/contract 尚不存在，不能提升状态。

## 3. 每个正向行证书的统一门

下面每个 `MT-Rxx` 都由 REVERSE 角色生成 create-new candidate 与 handoff；对应 `MT-Rxx-AUDIT` 必须由未参与 candidate 编写的 TEST 角色独立验收。主线只合成已通过 audit 的行，不能用作者自测或既有负向审计代替。

每一行只有同时满足下列条件才可从 `UNKNOWN` 提升：

1. 目标 PE、callsite、helper target、原字节和所属函数精确绑定；
2. destination reaching definition、实际长度、整数 no-wrap 与写区间完整；
3. allocation/object owner、backing extent、constructor/copy/move/assignment/swap/adoption/destructor 生命周期闭合；
4. direct、indirect、address-taken、tail transfer、split/EH、stack spill/reload、return/liveout、callback/reentry 的可达生产者固定点闭合；
5. 写区间与 GameRoot slot `[RVA 0x00871678, 0x0087167C)` 可证明不相交；
6. `fullyAdjudicatedNoOverlap=true`（或 schema 中严格等价字段）、正向 disposition 为 `STATIC_CONFIRMED_NO_OVERLAP`、`unresolved=[]`，并明确消解当前最小 countermodel；
7. 独立审计从冻结 PE 与公开机器输入重算 exact row set、writer/caller universe、区间和 mutation，而不是接受 candidate 自报字段。

### 3.1 REPENTOGON 外部搜索种子边界

本任务分解允许把 REPENTOGON 的逆向成果用于缩小搜索域，但固定为
`EXTERNAL_CANDIDATE_ONLY / NOT_TARGET_BOUND / NO_PROMOTION`：

- 来源固定为 `TeamREPENTOGON/REPENTOGON` checkout commit
  `421c91d082db9be0ae02ac49801b60e3c5a0ad0c`、REPENTOGON `1.1.2`；其英文文档绑定的
  游戏版本为 Repentance+ `v1.9.7.12.J273`。
- 只允许使用 `.zhl` 中的函数签名、类型名、字段候选和 Hook 前后的调用语义作为当前 PE 的
  定位/分类种子。每个种子必须重新完成当前固定 PE 的唯一匹配、指令头与 ABI 核对、caller/xref
  穷尽、目标字节绑定和 owner/lifetime 数据流证明，才能进入 candidate。
- 已观察到结构稳定与漂移并存：REPENTOGON 的 `Game+0x1BAA8=PlayerManager`、
  `Game+0x1A740=ItemPool` 与当前目标关系吻合；但 REPENTOGON 的 Player controller/index
  为 `+0x160C/+0x1610`，当前目标为 `+0x1618/+0x161C`，且 `GameStatePlayer` 大小分别为
  `0x5BC` 与当前 record stride `0x5DC`。因此禁止把 REPENTOGON offset、元素类型、函数地址或
  signature 命中直接写成当前目标事实。
- 禁止运行 REPENTOGON `offline_scanner`、注入/加载 REPENTOGON 或以其 Hook 后行为采集
  T0 干净证据；该 scanner 会注入 `libzhl.dll`，而成就、存档、GameState 和 controller 补丁会
  改变待证明的 vanilla 路径。
- 本节及下方条目不改变 `103 UNKNOWN`、暂停状态、权限、promotion 或作者分离审计要求。

## 4. exact103 正向逆向微任务

全部任务当前均为 `PAUSED_BY_USER`。exact set 以 v30 JSON 为唯一集合真值；“组减去例外”仍是 hash-bound 精确集合，不允许临时增删行。

### 4.1 callerHeap exact17

| ID | 精确范围 | 当前首缺口／正向接口 |
| --- | --- | --- |
| `MT-R01` | `{0x001CBEB5}` | save-record v24 的 167 seeds 已解析 165；从 record-base stack liveout、`0x007B3C41 -> 0x00753620` 及后续 kill 继续，闭合 begin/end/capacity 全生产者。附加搜索种子：REPENTOGON 的 `GameStatePlayer::Init`、`Entity_Player::{StoreGameState, RestoreGameState, RestoreGameState_PostLevelInit}`；按 §4.7.1 重定位，不导入其 `0x5BC` 布局或未命名的 `+0x500`。v31 工作观察不是证据。 |
| `MT-R02` | `{0x00008A62, 0x00008B2A}` | 证明两个 incoming object pair 的 `oldCount <= newCount`，或给出更强的 allocation/span 不变量。 |
| `MT-R03` | `{0x0001513A, 0x000155A4}` | 对全部 parser/interpolation 路径证明输入元素数不超过固定 5000-element backing；现有 limit 只记录日志。 |
| `MT-R04` | `{0x0001AAB0, 0x0001AEB8}` | 分别闭合 177 个与 5 个 destination producer，证明 capacity 始终覆盖 source count。 |
| `MT-R05` | `{0x0033570B}` | 闭合 filtered-vector backing/owner；当前不存在 dominating nonempty guard，必须以真实 owner extent 排除 slot countermodel，不能伪造非空前提。 |
| `MT-R06` | `{0x0061F2C2}` | 闭合 double-buffer `this[48]/this[49]` constructor 与全部 pointer/capacity writer。 |
| `MT-R07` | `{0x00657E0A, 0x00657E94, 0x00658338, 0x006583C2, 0x00658487}` | 闭合 BufferedStream vtable dispatch、indirect `a2` producer、begin/used/capacity constructor/replacement writer universe。 |
| `MT-R08` | `{0x00661577, 0x00689F63, 0x0068E3DB}` | 分别闭合第二静态 ring size、caller output pointer/capacity、nested realloc object `+176/+180/+184` 全 writer。 |

### 4.2 localHard exact7

| ID | 精确范围 | 当前首缺口／正向接口 |
| --- | --- | --- |
| `MT-R09` | `{0x0001B072, 0x0001B3E3}` | EntityList arena 的 parent object lifetime、backing extent 与 already-offset alias writer fixed point。附加搜索种子：REPENTOGON 的 `EntityList+0x70=_tempEL` 与 `EntityList_EL::_size+0x0C` 精确组成当前 scratch count `EntityList+0x7C`；按 §4.7.2 闭合 sublist/shared-backing/`Untie` 生命周期。 |
| `MT-R10` | `{0x002EE04F}` | 闭合 `source+0x2094 -> this+0x5C0` 的 22 direct writer／28 LEA alias 固定点，并证明 `1<=n4<=9`。 |
| `MT-R11` | `{0x004FCD78}` | 闭合唯一非递归 caller 传入 tuple 的全部 producer 与 backing ownership。 |
| `MT-R12` | `{0x0061D0B4}` | 闭合 persistent `+0x2C/+0x30/+0x34` tuple 的 class copy/move、split/address-taken、callback 与生命周期 writer。 |
| `MT-R13` | `{0x00663CFC, 0x006936B7}` | 闭合 PNG descriptor width/bitDepth/channels 来源、row allocation product/add no-wrap，以及 Adam7 pixel offset/loop no-wrap。 |

### 4.3 vectorGrow exact23

| ID | 精确范围 | 当前首缺口／正向接口 |
| --- | --- | --- |
| `MT-R14` | v30 `vectorGrow` 减去 `0x0062C746` 的 exact22 | 闭合 incoming destination **和 source** tuple 的所有 writer、owner lifetime 与 grow/reallocation backing。 |
| `MT-R15` | `{0x0062C746}` | 闭合 caller destination tuple writer fixed point；不得由其他 22 行的共同结论自动覆盖。 |

### 4.4 vectorShift exact25

| ID | 精确范围 | 当前首缺口／正向接口 |
| --- | --- | --- |
| `MT-R16` | v30 `vectorShift` 减去 `0x003DE616, 0x004352BA, 0x00673CD4` 的 exact22 | 闭合 incoming destination/source tuple alias writer fixed point。 |
| `MT-R17` | `{0x003DE616, 0x004352BA}` | 额外闭合 direct-caller branch feasibility，再闭合 incoming tuple writer。 |
| `MT-R18` | `{0x00673CD4}` | 额外闭合 owner address-taken/indirect transfer universe 与 tuple writer。 |

### 4.5 string exact14

| ID | 精确范围 | 当前首缺口／正向接口 |
| --- | --- | --- |
| `MT-R19` | `{0x000310F4, 0x00031154}` | adoption/move 与 incoming string identity/capacity provenance。 |
| `MT-R20` | `{0x000995C0, 0x000995F6, 0x00099608, 0x00099684, 0x00099690, 0x000996A7, 0x00099828, 0x00251B8E, 0x00251B9D, 0x00251BB5}` | borrowed external pointer、SSO union tag、capacity 与 owner lifetime。 |
| `MT-R21` | `{0x00284EB5, 0x00284F19}` | incoming string object identity、SSO union 与 capacity provenance。 |

### 4.6 memset exact17

| ID | 精确范围 | 当前首缺口／正向接口 |
| --- | --- | --- |
| `MT-R22` | `{0x000148AA}` | dual-root already-offset alias successor fixed point：`n298+0x4C614` 与 `owner+0x123C`。 |
| `MT-R23` | `{0x005CF2B3}` | fixed-member receiver producer universe 与真实 object backing。 |
| `MT-R24` | `{0x0062C316}` | 证明 parser output `<=0xFE`，或证明 `EBP-0x118` 独立 0x200-byte backing extent。 |
| `MT-R25` | `{0x006EC167}` | 闭合成功 `AEA9B0` alternate backing producer及 allocator metadata/header capacity。 |
| `MT-R26` | `{0x00308B5A}` | 闭合两个 stack-reloaded parent 的 split/EH successor、owning predecessor 与 constructor/alias state。 |
| `MT-R27` | `{0x003271E2, 0x003272FC}` | 闭合 n298 selected/root tuple 的 split destructor、stack liveout、already-offset `+0x0C` 与 root pre-offset seed fixed point。 |
| `MT-R28` | v30 `standardVectorB` exact5 | 按五行分别闭合 `+0x30`、`+0x24`、address-taken `sub_755470`、GameRoot `+0x1A740/+0x794`、object `+0x1E3CC` 的 owner/alias/writer fixed point。第四行附加搜索种子：REPENTOGON 将 `Game+0x1A740` 标为 `ItemPool`、其 `+0x794` 标为 `_trinketPoolItems`；按 §4.7.3 从 `ItemPool::Init/StoreGameState/RestoreGameState` 重建当前 PE 生命周期，不能覆盖其余四行。 |
| `MT-R29` | v30 `standardVectorC` exact5 | 按五行分别闭合 record `+0x1D4`、owner `+0x1FD4`、object `+0x1E79C`、record `+0x518`、address-taken receiver `+0x2C` 的 parent/already-offset/indirect lifetime fixed point。第一行附加搜索种子为 REPENTOGON `GameStatePlayer::_movingBoxContents+0x1D4`；第四行则是显式漂移警报：REPENTOGON `+0x518` 为标量 `_controllerIndex`，与当前 stride-16 tuple 冲突。按 §4.7.4 分别处理，不向其余三行外推。 |

计数校验：

```text
callerHeap 17 + localHard 7 + vectorGrow 23 + vectorShift 25 + string 14
+ memset A 3 + B 5 + C 5 + nonVector 4 = 103
```

### 4.7 REPENTOGON 辅助执行细化

以下步骤属于对应 `MT-Rxx` 的预定位和 fixed-point 输入，不是额外任务、行证书或验收结果。

#### 4.7.1 `MT-R01` save-record

- 在当前 PE 中分别唯一重定位 REPENTOGON `libzhl/functions/GameStatePlayer.zhl` 的
  `GameStatePlayer::Init`，以及 `libzhl/functions/EntityPlayer.zhl` 的
  `StoreGameState/RestoreGameState/RestoreGameState_PostLevelInit` 签名；零匹配、多匹配、ABI 或
  函数边界不一致时只记录 `NO_USABLE_SEED`。
- 从这些当前 PE 函数反向追踪 `recordBase = objectBase + 0x19D7C + index*0x5DC`、
  `record+0x500/+0x504/+0x508` 与 v24 未闭合的 `[EBP-0x298]` stack liveout，枚举 constructor、
  clear、save、restore、copy/move、手工插入和 online-specific caller。
- 把 REPENTOGON `EntitySaveStateManagement.cpp` 的“存在手工构造/插入”和“online play 使用点未处理”
  说明作为反例清单：不得假定所有写入都经过单一 constructor、`GameStatePlayer::Init` 或
  `Entity_Player::StoreGameState`。
- REPENTOGON record 大小 `0x5BC` 与当前 `0x5DC` 不同，且没有给出当前 `record+0x500` 的可转移
  字段名；禁止据此命名该 tuple。完成条件仍是 v24 缺口的全生产者闭合与独立审计。

#### 4.7.2 `MT-R09` EntityList arena

- 以 REPENTOGON `EntityList.zhl` 的 `_tempEL+0x70` 和 `EntityList_EL.zhl` 的
  `{_sublist+0, _data+4, _capacity+8, _size+0xC}` 为候选类型，先在当前 PE 证明
  `EntityList+0x7C` 的读写确实属于该四字段对象，而不是数值碰撞。
- 重定位并分类当前 PE 的 `EntityList::Reset/QueryType/QueryRadius/QueryCapsule`、
  `EntityList_EL::{constructor,Destroy,Untie}` 候选；沿 `_sublist` 分支枚举共享 `_data`、detach、
  copy/move、reset、free 和 already-offset `EntityList_EL*` liveout 的全部 writer。
- 将 `Untie` 的“sublist 与其它列表共享 backing，随后转为独立 backing”语义加入 alias
  countermodel，不能只扫描 parent 上 `+0x70/+0x74/+0x78/+0x7C` 的直接 store。
- 该语义映射不消解当前 `sourceCount<=0x8000` 硬上界缺口；还必须独立闭合 source-list/query
  push writer universe、arena `0x20000` extent 和两行 slot non-overlap。

#### 4.7.3 `MT-R28` standardVectorB 第四行

- 对 RVA `0x003350C6` 单独建立候选 owner：当前已知 receiver 为
  `GameRoot+0x1A740+0x794`；REPENTOGON 候选类型链为
  `Game::_itemPool -> ItemPool::_trinketPoolItems`。
- 在当前 PE 中唯一重定位 `ItemPool::Init/StoreGameState/RestoreGameState` 及可见的 trinket-pool
  consumers，再从七个已冻结 caller 交叉验证 receiver、元素 stride、tuple offset 和对象边界。
- 对 `ItemPool+0x794/+0x798/+0x79C` 穷尽 constructor、grow、copy/move、assignment、swap、
  adoption、save/restore、reset 与 `sub_426980` destructor escape；同时追踪任何已经偏移好的 tuple
  参数、返回值、stack liveout、indirect/address-taken producer。
- 只有第四行可以使用该候选类型；`+0x30`、`+0x24`、`sub_755470` 和 `object+0x1E3CC` 四行仍按
  各自原首缺口独立闭合，禁止以 ItemPool 共同结论批量提升 exact5。

#### 4.7.4 `MT-R29` standardVectorC 第一、第四行

- 对 RVA `0x003DC995`，以 REPENTOGON `GameStatePlayer::_movingBoxContents+0x1D4` 为 parent
  identity 候选；在当前 PE 重定位 `GameStatePlayer::Init` 与 Player save/restore 系列，并验证
  当前字段确为 begin/end/capacity tuple、元素 stride 与 grow `sub_737F40` 一致，再闭合 parent
  pre-offset/already-offset alias、copy/move、clear 和 save/restore writer。
- 对 RVA `0x005D2C27`，先固定负向版本差异：REPENTOGON 在 `GameStatePlayer+0x518` 声明标量
  `_controllerIndex`，当前 PE 证据却是 `+0x518/+4/+8`、stride 16 的 vector tuple。该冲突只用于
  拒绝布局迁移并提示在当前版本寻找新增/移动字段，不能把当前 tuple 命名为 controller index。
- 第四行必须从当前 PE 的 `0x5DC` record constructor/grow/copy/restore caller 重新确定真实元素类型、
  parent offset、字段 owner 与全 writer fixed point；若 REP signature 命中旧语义或不唯一，标记
  `LAYOUT_DRIFT_SEED_REJECTED` 后继续原生搜索。
- `owner+0x1FD4`、`object+0x1E79C`、address-taken receiver `+0x2C` 三行没有可接受的 REPENTOGON
  类型映射，保持原任务边界和 `UNKNOWN`，不得借第一/第四行外推。

## 5. 审计、合成与版本规则

### `MT-T01`：逐候选作者分离审计

每个 `MT-Rxx` 完成后创建对应独立 audit；TEST 角色不得 import candidate analyzer、修改 candidate 或以 candidate decision 为真值。允许把多个互不重叠的小 candidate 合并在同一次 audit 中，但报告必须逐行给出 verdict，任一失败行保持 UNKNOWN，不能全批次“多数通过”。

### `MT-I01`：增量 positive frontier

主线只合成已独立接受的正向行，create-new 发布 v31+ successor。每版必须：绑定前一 frontier 与新 audit；精确更新 closed/remaining/RVA 集；拒绝重复、遗漏、跨 helper 和未审计行；保留失败行 UNKNOWN；不覆盖 v30。

### `MT-T02`：frontier 独立审计

与 `MT-I01` 作者分离，重算前后差集、行证书、总数、bindings、permissions 和 promotion。中间版不得宣称 whole-image 或 manager-this complete。

### `MT-I02`：zero frontier

最后一个 positive successor 只有达到以下精确状态才可进入 whole-image：

```text
copyMove.closed = 405
copyMove.remaining = 0
memset.closed = 242
memset.remaining = 0
frontier.remainingTotal = 0
647/647 fullyAdjudicatedNoOverlap = true
```

`MT-I02` 仍需独立 `MT-T02` PASS。负向审计覆盖字段不能替代 647/647 正向 closure。

## 6. whole-image 与 v7 发布链

以下顺序不可交换。

| ID | 角色 | 依赖与产物 | 通过条件 |
| --- | --- | --- | --- |
| `MT-W01` | REVERSE/聚合 | 依赖独立接受的 zero frontier；create-new `out/cs-t0/native-join-manager-this-whole-image-alias-v2.json` | 绑定 executable-range component v2、local epoch、direct/literal/split/cross-thread 和 bulk 647/647；八个 raw frontier 全部字面 `STATIC_CONFIRMED`；`boundedWholeImageClosureAchieved=true`、`wholeImagePromotionAllowed=true`、`requiredDisposition=PROMOTE_MANAGER_THIS_IDENTITY`、`unresolved=[]`。 |
| `MT-W02` | TEST | whole-image 独立 audit | 从冻结 PE/组件输入重算 exact647 join、八 frontier、binding DAG 与 mutation；不能只检查 W01 自报字段。 |
| `MT-P01` | MAIN | 运行 `out/cs-t0/build-manager-this-v7-artifacts-v1.py` | create-new 生成 reader contract、whole-image certificate、manager-this sidecar、admission contract v7 四件套；四件当前均不存在。 |
| `MT-P02` | MAIN | contract hash/publication seal | 把 `cs-t0/admission_v7/inspect_admission.py` 中 `CONTRACT_SHA256="__PENDING__"` 替换为实际 contract SHA；创建无环 publication manifest，绑定 inspector、两个公开 PS1 wrapper、verifier、contract 与 builder，消除 checker 漂移未绑定问题。 |
| `MT-P03` | TEST | 正式 inspector/verifier create-new 报告 | 11 项全 PASS：contractPinned、diskInspector、frozenV6FormalInputs 523/523、readerBindingClosure、fourFrontierClosure、sameLiveGenerationAndPublication、Debug/Release 410、zeroAuthority、remainingPremisesPreserved、63+37 contract/relationship cases。 |

`MT-P03` 的唯一正确结论是第 1 节的 `managerThisIdentityStatus=STATIC_CONFIRMED / BLOCKED_REMAINING_MANAGER_CONTEXT_PREMISES / exit 1`。任何 local check、frontier、binary rehash 或 publication binding 失败都必须保持 `FAILED_MANAGER_THIS_IDENTITY_PREMISE` 或 local acceptance failure。

## 7. 恢复后的建议波次

受四个并发槽和作者隔离约束，每波最多安排两个 REVERSE candidate 与一个 TEST 准备/验收角色；主线保留集成职责。

1. **Wave A：已有最深上下文** — `MT-R01` save-record、`MT-R09` EntityList；完成后立即分别独立审计。
2. **Wave B：可共享 fixed-point 机制** — `MT-R22/R26/R27/R28/R29` 的 split/EH、stack-liveout、already-offset owner；先统一证书接口，再按 exact rows 发布。
3. **Wave C：容器 tuple 家族** — `MT-R06/R11/R12/R14/R15/R16/R17/R18`。
4. **Wave D：typed stream/string** — `MT-R07/R19/R20/R21`。
5. **Wave E：数值与外部输入边界** — `MT-R02/R03/R04/R05/R08/R10/R13/R23/R24/R25`。
6. 每波通过 `MT-T01 -> MT-I01 -> MT-T02` 后才进入下一机器 frontier；全部清零后执行 `MT-W01 -> MT-W02 -> MT-P01 -> MT-P02 -> MT-P03`。

若某个共享证明能覆盖多组，它仍必须输出每行 exact certificate 和独立 audit；不得用“同类看起来相同”批量提升。

## 8. 当前暂停边界

本文件只是任务分解与完成性审计。用户尚未恢复逆向，因此 `MT-R01..R29`、`MT-W01` 均不得启动；也不能提前生成 promotable raw v2、四件 v7 正式产物或 formal PASS 报告。

暂停期间可做的工作仅限 schema/validator/publication-manifest 设计和只读一致性核对，它们不会减少 `103 UNKNOWN`。本轮没有调用 IDA、x32dbg、游戏或目标进程，没有生成正向 candidate，没有修改生产、测试、plan/progress、matrix、历史证据或权限。
