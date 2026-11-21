> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 原生持久化保护与停止窗口证据 v1

本次新增的正向结论是：**正常菜单输入一个合法的普通种子，可以原生建立 readonly=1，并与普通 Isaac selector 分支兼容。** 普通模式字段 `gameRoot+0x26630=0` 还能排除已定位的两处 `0x2F9F20` 调用；该函数存在本身，不是候选窗口必然发生写入的反证。

完整主动无污染证明仍为 **BLOCKED_BY_EVIDENCE**。剩余缺口是具体的 Init／自然更新状态传播：原生 manager 返回后，同一帧仍可能运行 GameUpdate；已有计时器可以请求转场，随后执行不检查 readonly 的保存。下一 pass 才发现字段变化，不能反向证明先前没有污染。当前不允许转为主动 runtime 实现或申请实机。

机器证据见 [coop-native-join-persistence-window-v1.json](../../../../../../cs-t0/coop-native-join-persistence-window-v1.json)。包含 63 个完整指令窗口、逐条 RVA／长度／助记符、相对控制流边和已知函数入口。`expectedBytes` 采用单空格分隔的大写十六进制；短跳按真实 REL8 分类。指令匹配只核验静态锚点，不证明窗口无污染或任何实机结果。

## 1. 目标、来源和授权

| 字段 | 值 |
| --- | --- |
| 观察 SHA-256 | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| PE Machine | `0x014C` |
| Preferred ImageBase | `0x00400000` |
| SizeOfImage | `0x008FE000` |
| 地址形式 | RVA_ONLY |

仅绑定下列已冻结的前驱，不绑定正在更新的状态文档：

| 文件 | SHA-256 |
| --- | --- |
| `cs-t0/coop-plan-v7.json` | `2BFD017329754E89EF4337248DB37468E292C75CD1490FDA30F4BE3A9FB6DCE3` |
| `cs-t0/coop-native-join-static-v1.json` | `CCB9F206F0E08067724DCF4D0B7297E1D0537FE612F2B2BE9B5024CCEFB67DEB` |
| `docs/archive/refactor/cs-t0/t0-05/native-join-active-followup-v1.md` | `02EDCA945ED56A51584ECA62805C3FD6FD88913A498725F2A47A2B5C1366B555` |

使用 IDA MCP 只读反编译、指令／字节读取、直接 xref 和位移候选搜索。没有进程枚举／打开、游戏启动、调试器附加、输入、Hook、注入或 IDB 修改；没有执行独立测试。独立测试由另一个子智能体负责。未修改历史根、旧哈希、旧生产二进制或生产源码。全部 active/live/arm/claim/PASS/unlock 权限为 false；旧 passive 授权不可沿用。

## 2. 正常可达的候选保护状态

候选固定 P0 Keyboard、唯一未分配的物理 XInput B、普通 Isaac，使用正常菜单建立的新普通 run。种子须被普通解析器接受、被特殊种子识别器拒绝；特殊 seed effect 集还须为空，不保存实际种子文本。窗口从正常初始创建和初始存档都完成、起始房稳定之后开始。

`0x54D069 → 0x5E9DB0` 先识别特殊 seed；其特殊分支可能修改统计和保存，予以排除。返回零时进入 `0x54D408 → 0x5EB6B0` 普通解析；非零有效结果最终在 `0x54D4E6` 将 MenuRoot 内 seed 对象的首字节 `+0x178` 置一。

原生路径把 seed 复制到 onlineRoot pending seed，再进入 `0x2F5320` New Run，复制到 `gameRoot+0x1BB84`。构造 `0x5E9320` 和赋值 `0x5E9430` 都保留对象首字节。机器锚点 `menu-start-seed-copy`、`pending-start-seed-copy`、`new-run-start-seed-copy`、`new-run-seed-owner-copy` 冻结各条调用。

`new-run-readonly-expression` 显示 `0x2F54E8` 检查该 seed 首字节，非零直接选择 true；`0x2F5544` 写入 `onlineRoot+0x15`。Persistent 是 `onlineRoot+0x14` 的内嵌对象，所以这是 `persistent+1 = readonly`，不是需要注入修改的开关。

该 OR 首项不要求 challenge／特殊模式字段非零，因此可与普通 selector 的 `gameRoot+0x26584=0`、`+0x26630=0`、`+0x26548 bit0x10 clear` 同时成立。实际 owner、seed 来源、effect 集和模式尚无 runtime 读回。

Readonly 只保护已经证明经过共同 guard 的写路径：`0x529A2A` 在 achievement byte 写、Steam helper 和通知之前检查 readonly；`0x529B5E` 在统计数组和 dirty 写之前检查 readonly。Init 侧若已经证明 health/cache helper 的持久化副作用仅经过这些 setter，即可组合该 guard，不必继续展开相同支配路径下每个 threshold。直接 `persistent+0x484` 等写不受保护，仍由 Init／tail 证据排除。

没有使用故意失败的存档加载、清计数、写 readonly/save-enabled 或压制不理解的回调来制造安全场景。

## 3. readonly、dirty、save-enabled 与 Steam 的不同边界

| Persistent 字段 | 作用 | 不能推出的结论 |
| --- | --- | --- |
| +0x0，1 byte | dirty | 停止时为零不代表从未保存；flush 会先清零。 |
| +0x1，1 byte | readonly；已知 setter 的前置 guard | 不是直接统计写或 save 的通用 guard。 |
| +0x2，1 byte | Steam achievement enable | 不是 OriginalOnline 状态。 |
| +0xF8C，1 byte | persistent save enable | 不阻止之前的 GameState 序列化及 dirty 写。 |

`0x52644B` 初始化 dirty/readonly 为零，`0x526450/0x526454` 将 Steam enable 和 save-enabled 置一。加载路径 `0x526C8C → 0x526E90` 的解析结果在 `0x526C96` 写 save-enabled；没有证据把解析失败当成正常完全 no-save 场景，且禁止刻意构造失败。

完整 `0x529660` flush 只检查 dirty/save-enabled，不检查 readonly。在 `0x529679` 清 dirty 后，可能调用 backup `0x5292C0`，再按 Steam context／Cloud 配置选择 `0x528EE0` 或 `0x5294F0`。空 roster 允许这些路径，不能把离线当成不持久化。

`0x529790` 自身检查 persistent+2、平台全局位和 Steam context；不再检查 readonly。由 `0x529A20` 进入时可使用共同 readonly guard；未审查的其他调用不能一并排除。不要求所有无关后台 Steam I/O 都消失，也不把背景 Steam 一概归类为 OriginalOnline。

## 4. 已排除的特定 readonly 失效入口

`0x2F9F20` 在 `0x2F9F4B` 清 readonly，执行直接统计和 `0x529B40`，再在 `0x2FA00F` 置一。当前两处直接代码 xref 是：

| caller | 必要 guard | 普通候选 |
| --- | --- | --- |
| Game::End，0x2F9880 | 0x2F97C2：gameRoot+0x26630 > 0 | 字段持续为零时跳过。 |
| input/lifecycle，0x2FA96E | 0x2FA8EC：同一字段 > 0 | 字段持续为零时跳过。 |

这是两处已知 direct-call 的条件排除，不是 readonly writer 的完整闭合。New Run 的 `0x2F5544`、Rerun 的 `0x2F747D` 重算该位；VictoryLap 的 `0x2F7C0E` 置一；native teardown 的 `0x2FA2DB` 清零；persistent 初始化会重置。

Game::End 普通分支仍有 `0x2F9920..0x2F9951` 直接统计加一和 dirty=1。排除 `0x2F9F20` 不表示 ending 安全，本候选必须排除 ending 本身。

## 5. 保存入口与停止边界

GameStateSave `0x558ED0` 要求 onlineRoot+8=2、gameRoot+0x26630=0、空 roster 和有效存活 P0，均可能与候选兼容。函数先捕获／写出 run state，再在 `0x5590BB/0x5590C1` 更新 persistent 数据／dirty，`0x5590C5` 才检查 persistent save-enabled。因此不能用 readonly 或 save-enabled 代替生命周期排除。

| 直接 caller | 所属入口 | 窗口处理 |
| --- | --- | --- |
| 0x2F57A0 | New Run 0x2F5320 | 正常初始保存必须在窗口开始前完成；不归因给 join。 |
| 0x2F76BC | Rerun 0x2F72F0 | 禁止重开并排除 pending request。 |
| 0x2F802F | VictoryLap 0x2F7AC0 | 窗口内排除。 |
| 0x2FA21D | native teardown 0x2FA0C0 | 不属于 adapter Shutdown。 |
| 0x52FC7C | floor transition 0x52F1C0 | 需要稳定 pending／timer 条件。 |

Character-menu action15 导入 `0x54C91E → 0x529660`。`0x5548D0` 还有热键 direct-save 分支，但明确要求 state=1、MenuRoot+0x40=9。稳定 in-game state=2 能排除当前这些分支；主帧仍先运行 pending-new `0x559720`、pending-menu `0x559D00` 和 cutscene，所以没有发送退出键并不证明生命周期稳定。

窗口只包括两个自然 manager pass、action19／selector readback、action14／完整原生 join、连续双 BOUND 和 adapter 禁止触发、drain、退休、55 槽恢复与 terminal 发布。adapter 停止不能执行游戏菜单退出、重开、save 或普通 B native-exit。恢复或 terminal publish 不确定时保持 Unsafe／诊断现场，不强卸载、不重注入、不退出 B 来把失败现场清理成成功。

## 6. 自然更新的具体 writer 与剩余传播

`0x2FADA7 → 0x5BAC40` natural manager 位于 input/lifecycle `0x2FA540` 末尾。主帧 in-game 分支先调用它，然后适用的帧会调用 `0x555231 → 0x2FADC0` GameUpdate。

| 状态 | 已定位请求／写入 | 候选条件及限制 |
| --- | --- | --- |
| gameRoot+0x1BA78，floor phase | 0x2FDC10 在 0x2FDF5E 写1、0x2FE0A4 写2；GameUpdate 在 0x2FB997 调用。 | phase=0；timer +0x67788=0。正计时自增到90会请求转场。 |
| gameRoot+0x1B83C，room phase | 0x2FDB94 取子对象，0x2FDB9B→0x42EE40；0x42F0D7 写 this[0]=1。 | phase=0；timer +0x68D6C=0。正计时减到0且 dimension=2时，0x2FB9F2 请求转场。 |
| +0x265B0／+0x265BC，fade | 0x2F5290、input/lifecycle 和 GameUpdate 都有写入口。 | 有限 progress=0、mode=0，无 ending／输入／cutscene，且必须保持。 |
| onlineRoot+0x4B130，pending new | 0x558AA0/0x558CB0 等设置，0x559720 消费。 | 零值只跳过当前 dispatcher，不证明所有来源不可达。 |
| onlineRoot+0x4B288，pending menu | GameUpdate fade 和主帧 cutscene 等设置。 | 零值及 state=2 必须保持。 |
| readonly | New/Rerun/VictoryLap/teardown/特定统计入口 | 已排除 2F9F20 的两个普通 caller；完整传递 writer 尚未闭合。 |

GameUpdate 在 `0x2FB4E1` 检查 floor phase，非零调用 `0x52F1C0`；在 `0x2FB50D` 检查 room phase，非零调用 `0x4318A0`，经 `0x430950 → 0x2FE2F0 → 0x33FC80` 进入 room-change flush。`0x33FD20/0x33FD4E/0x33FD57` 分别进入 backup／Cloud／local，未用 readonly 保护。

机器证据 `writerInventory.directCallerSets` 冻结了 stage／room request 和 aliased room writer 的当前直接 caller 集；同一节列出 fade/pending writer 位移候选。它们是后续入口，不表示所有 caller 都会在候选运行。位移搜索会漏掉别名和 bulk copy，不能把未搜到直接位移当作没有写入。

进一步候选条件包括：初始无战斗房、玩家存活、无门／陷阱／传送触发、两个 timer=0、无物理 gameplay／菜单输入、无 mods／外部回调。但完整 Init 和后续 entity tick 是否保持这些状态，仍未冻结。B 在 prejoin 尚不存在，不能用未来 B 的读回来代替构造／Init 的静态传播。Init 子智能体未宣称已证明这些字段的全部传递写缺失，本证据也不升级结论。

## 7. 最小剩余证据

实际静态阻断只有两个组合项：

1. 在新普通显式 seed 起始房候选中，闭合完整 Init 与随后自然 tick 到 transition timers／request／fade／pending lifecycle／readonly 的传播。writer 与具体入口已列出，不要求证明任意未来关卡或全游戏永不保存。
2. 与独立 Init／tail 证据组合，排除不经过 readonly setter 的直接 persistent 写，并证明构造／配置／health／cache 源状态满足分支条件。已被共同 readonly 支配的 setter 路径无需重复扩大追踪。

之后才是未来 runtime 条件：owner/source/config/effect 身份和 guard 读回，persistent 干净基线、完整因果链、双玩家绑定、无实验造成的内存待提交／direct／save／Cloud／Steam 污染，以及正常停止恢复。后一次采样发现漂移不能排除前一次采样之后已经发生的 flush；文件哈希或缺环境跳过也不能替代这些条件。

本轮只可独立验收新增静态事实和严格 blocked 判据；不能标记 05-04 PASS、申请或执行主动实机、解锁 05-05、t005Unlocked 或 T0-06。改变结论需 create-new 契约／验证链，不重写历史哈希使旧门通过。

