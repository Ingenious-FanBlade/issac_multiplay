> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 原生 Join 主动路径逆向交接 v1

- 日期：2026-08-31。
- 状态：**BLOCKED_BY_EVIDENCE**。精确 UI callback、selector 和 ordinary join 路线已静态确认；独立 prejoin Offline 与零持久污染尚未闭合，不能生成 active arm 资格。
- 机器证据：[coop-native-join-static-v1.json](../../../../../cs-t0/coop-native-join-static-v1.json)，identity `IsaacCsT0-CoopNativeJoinStatic-v1`，SHA-256 `CCB9F206F0E08067724DCF4D0B7297E1D0537FE612F2B2BE9B5024CCEFB67DEB`。
- 本文及机器证据为 create-new 交接；不改写旧 graph/catalog/decision、live source、analysis、旧失败结论或授权。后续闭合须新版本，不把本版 UNKNOWN 原地改为 PASS。
- 本次只读取仓库、磁盘 PE 与 IDA 数据库并写这两份新文件；没有进程枚举/打开、游戏输入、注入、Hook、断点、补丁、实机或测试验收。逆向与独立测试仍由不同实例负责。

## 1. 构建、方法和证据等级

IDA survey 与实际磁盘 PE 的 SHA-256 一致：

| 项目 | 值 |
| --- | --- |
| canonical SHA-256 | `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30` |
| approved-unpacked SHA-256 | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| equivalence SHA-256 | `8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138` |
| machine / preferred image base / image size | `0x014C / 0x00400000 / 0x008FE000` |

使用当前可调用的 `mcp__ida_pro_mcp__survey_binary`、`decompile`、`py_eval` 与 `xrefs_to`。Python 只执行 IDA bytes、函数/反汇编、xref 与反编译读取；没有修改 IDB。所有本文地址均为 RVA，机器证据不含目标绝对路径、PID、实时 VA 或设备身份。JSON 的 53 个原始字节锚点来自该数据库；`INSTRUCTION_WINDOW` 截止于完整指令边界，`EXACT_DWORD` 为虚表项或明确的 PUSH imm32 operand。

- `STATIC_CONFIRMED`：目标构建上的字节、ABI、局部数据流及满足列明前提时的条件路径。
- `RUNTIME_OBSERVED`：仅引用现有 live-06 的正常物理输入 join、passive mapping 与 restore；不外推为本次主动、prejoin 或持久化证据。
- `UNKNOWN`：尚未证明的完整资格；不能由名称、纯内存 fixture 或缺少异常日志升级。

前置 decision v3 的 PASS 仅解锁工作包；plan v6 仍无 live/active 权限。历史 passive 非零 backend 决议及 source 均保持原 scope。

## 2. 精确 driver：53＋2 槽，完整自然 manager 路线

原 owner 调用为 `0x2FADA7 → 0x5BAC40`，`ECX=gameRoot+0x1BAA8`，原返回 EAX 保留。主线程资格继承自已冻结 owner 契约；新 driver 只在自然 manager depth=1 中工作，不额外调用 manager。

| 目的 | 指令 RVA | 原始字节 | operand RVA |
| --- | --- | --- | --- |
| 普通加入 action19 | `0x5BB057` | `68 E0 09 A2 00` | `0x5BB058` |
| 普通确认 action14 | `0x5BB38B` | `68 E0 09 A2 00` | `0x5BB38C` |

这两处将同一原 callback `0x6209E0` 入栈，随后通过 InputManager `vtable+0x74` 调用 `0x620FB0`。实际 query call `0x5BB064`、`0x5BB39A` 均为两字节 `FF D0`，不能冒充 REL32 槽。新增 ordinal 53/54 使用各自独立的 `PUSH_IMM32_CALLBACK` 描述，两个 operand 都四字节对齐。总槽数 55，不继承设备重分配专用 56 槽身份。

普通创建链是：

```text
selected action19 callback
 -> native selector creation
 -> next natural manager pass selected action14 callback
 -> 0x5BB415 -> 0x5B9CD0 ordinary Player creation
 -> 0x5BB424 -> 0x3A6450 assignment
 -> 0x5BB4FD -> 0x5A8620 HUD
 -> 0x5BB504 -> 0x3BC740 InitPostLevelInitStats
 -> native state/effect/counter and selector retirement
 -> full manager return
```

不能用 spawn/assignment/HUD 三次直调替代：原 continuation 还设置位置、使用 RNG、调用 InitPostLevelInitStats、写 Player `+0x414=15`、创建出生效果、使 `+0x13BC≥45`，最终把 selector index 置为 `-1`。其中 InitPostLevelInitStats 存在下面第 5 节的持久状态写入反证。

原生函数 ABI：

- `0x5B9CD0`：`Entity_Player* __thiscall(PlayerManager*, int playerType)`，`ret 4`；ordinary type=0，`-1` 是禁止正向使用的 possessor 路线。
- `0x3A6450`：`void __thiscall(Player*, int controllerKey, uint32 propagate)`，`ret 8`；该 caller 的 propagate=1。
- `0x5A8620`：HUD thiscall，无栈参数；HUD owner=`root+0x1DA04`。

既有 pointer patch primitive 可用作实现材料，**不是已通过的 executable-operand 契约**。`hooks.cpp` 的 PointerPatch Apply/Restore 使用 PAGE_READWRITE；新实现必须验证完整 PUSH 指令与上下文、精确 callback target、暂停事务、原页权限恢复、instruction-cache flush、逐槽 rollback/restore。不能只验证四字节“像一个指针”。

## 3. callback ABI、引用生命周期和 selector

### callback 所有权

`0x620FB0` 的具体 controller 分支在 `0x62111C..0x621136` 形成四个栈 dword，调用后 `add esp,0x10`。正确 wrapper ABI 是：

```cpp
uint8_t __cdecl Callback(
    void* device,
    void* sharedControl,
    uint32_t* action,
    uint32_t unusedExtra);
```

当前两个 caller 的 unusedExtra=0；原 `0x6209E0` 未读取第四项，不能赋予它业务含义。返回值在 AL，callee 为普通 RET、不清参数。不能依据反编译器显示的三个参数丢掉实际 caller 的 16 字节形状。

dispatcher 通过 `0x502D60` 复制 device/control 双 dword：调用 control 虚表 `+4` 的 acquire 后填充复制项；原 callback 调 control 虚表 `+0x0C`，根据返回值调用全局 finalizer 槽 `0x87E814`。原 callback 内在 `0x620A17` 调 device `vtable+0x34` triggered query，返回地址为 **`0x620A19`**。Keyboard 与 Gamepad 的该原逻辑目标均为 `0x61E970`。

wrapper 必须先捕获需要的身份，再调用原 callback 恰好一次，保留它的 release/finalizer 副作用；不得额外 retain/release，或在 original 返回后解引用 control。ABI 栈平衡、AL、非易失寄存器、引用调用次数及异常路线仍须独立 harness，静态分析不代替测试。

通用 GamepadTriggered 的返回地址只能定位共同 helper，不能区分多个 manager action19 caller；本方案使用两处独立 callback operand 提供 provenance，不使用猜测栈回溯。

### selector 的只读判据

selector owner 为 PlayerManager，4 项，从 `manager+0x0C` 开始，每项 `0x14` 字节：

| 相对项偏移 | 静态含义 |
| --- | --- |
| `+0x00` | controller key |
| `+0x04` | player type |
| `+0x08` | selection index；退休为 -1 |
| `+0x0C` | scale；普通创建写 1.0f |
| `+0x10` | autoConfirm byte |

`0x5BAAA0` 按 controller 查找或分配 slot。`0x5BB0BB..` 的普通分支设置 type=0、index=1、scale=1.0、autoConfirm=0。必须拒绝 `root+0x26548 & 0x10 != 0`、`root+0x26584 != 0`、`root+0x26630 != 0`；这些数值分支可走 `0x5BD3B0` 或 autoConfirm=1。暂不为这些数值字段推断产品模式名称。

未来 driver 固定两次自然 pass：

1. READY_SINGLE：P0 唯一 Keyboard；B 是唯一未分配 concrete Gamepad；main vector 恰有一项、secondary vector 无额外角色、无其他 pending selector。
2. 精确 action19 callback 在原返回 false 后至多覆盖一次 true；其他 callback 原样返回。该 manager 完整返回后，仍必须单 Player，B selector 唯一且字段符合上表。
3. 下一自然 pass 重新验证 owner/generation/device/selector，精确 action14 callback 在原返回 false 后至多覆盖一次 true。
4. 要求精确 ordinary `46→49→51`、完整 manager 返回、selector 退休及双 Player 连续稳定 mapping。原返回已 true、重复 query、早发确认、部分失败、generation 漂移均不自动重试，不把自然输入结果记为 synthetic 成功。

这是冻结路线的设计，不是允许现时调用：第 4、5 节 UNKNOWN 必须先闭合。

## 4. prejoin Offline：局部 guard 已证，完整独立资格未证

`0x5BAC6D` 从 online root 读取 `+0x4B3D8/+0x4B3DC`，不相等即跳至 `0x5BB574`，跳过整个 local body。这能证明局部选择路径；不能单凭它证明 backend、完整 ModeLatch、ready gate、channel 或持久化静默。

已知只读布局可作为必要 screen：GameRoot slot=`0x871678`、onlineRoot slot=`0x87169C`、backend slot=`0x879A58`，Steam backend vtable=`0x7A2440`、onlineFrame=`onlineRoot+0x4B3E4`。canonical-empty roster 要求 begin/end/capacity 三指针均零，不仅 count=0。对象及这些字段必须在当前 generation 稳定；未知 backend、NetInputDevice、多匹配或读失败都拒绝。

`coop-passive-backend-screen-v1` 对非零已知 Steam backend 的接受依赖**已完成**的同 generation ordinary route。它是被动观察限域决议，不能为主动 join 前的资格提供未来结果。用户批准研究一个新的限域主动判据，不等于已证明该判据。因此本版 `prejoin.independentActiveOfflineClassificationComplete=false`；不将背景已创建 manager/backend 简单等同在线，也不据此自动宣布离线。

## 5. 持久化反证与准确的 UNKNOWN 边界

### A. ordinary Init 可条件到达 achievement 与 dirty 写入

ordinary spawn 使用 Player vtable `+4 → 0x355470`，Variant=0。Init 在 `0x3577CD` 检查 Variant，`0x3577D5` 调 `0x3BBBD0`；后者在 `0x3BC02B` 取得 P0、`0x3BC034` 调 `0x3597E0(P0,-1)`。

当 P0 ordinal≥0、`root+0x26614<2`、P0 `+0x1368≥55`，`0x3598B8` 调 `0x529A20(onlineRoot+0x14,2)`。该 setter：

- `0x529A2A` 检查 persistent `+1`，非零可挡住这个 setter。
- readonly=0 且 achievement 尚未设置时，`0x529A44` 写 `persistent+0x38+achievementId=1`。
- `0x529A77` 写 dirty=1。
- platform-enabled byte `+2` 非零时经 `0x529A6A` 调 `0x529790`，进入 Steam achievement 相关接口调用（虚表 +0x18、+0x24）。

这是有条件的静态可达路径，不是本次实机已发生污染。它足以反驳“只创建 B、没注入 gameplay、roster 空就一定无 achievement 影响”。

### B. join 尾部直接写 persistent counter，未检查 readonly

`0x5BB504 → 0x3BC740`（InitPostLevelInitStats）包含独立直接写：

- `0x3BC83B` 捕获 `root+0x264F8`。
- 普通 `root+0x26584=0`、`root+0x26630=0`、捕获值为 0 时，`0x3BE0D7..` 在空 roster 路线选 `onlineRoot+0x14` 为 persistent owner。
- `0x3BE18E` 读 persistent `+0x484`；空 roster 使用低 16 位，最多取 2。
- 计数非零时进入有 100 次尝试上限的物品选择循环；循环完成或耗尽后，`0x3BE393` 的 AND `0xFFFF0000` 清低 16 位，`0x3BE3A0` 的 `89 B0 84 04 00 00` 直接写回 persistent `+0x484`。

此支不读取 persistent `+1`，所以“readonly=1 可证明所有持久内存无变化”不成立。该计数的产品统计名称仍 UNKNOWN，不用名称代替证据。检查该计数为零可排除此单支，但不证明完整 Init/tail 写集合为空。

### C. 初始化回调、延迟存盘和外部提交仍须封闭

Init 尾 `0x357959 → 0x464050`，后者在 `0x464099` 检查 callback mask `0x200`，非零经 `0x4640B1 → 0x474120` 派发 Lua callback。未证明允许实验场景中该回调不存在或无持久副作用；不能静默屏蔽回调来制造通过。

限定检查发现这些 sink，各自静态存在，但**未证明 ordinary Init/tail、后续正常帧及停止窗口对它们的完整不可达性**：

| RVA | 能力及限制 |
| --- | --- |
| `0x529660` | dirty 与 save-enabled `+0xF8C` 驱动备份、Cloud/磁盘选择；需要覆盖延迟执行窗口。 |
| `0x5292C0` | 空 roster 允许保存备份路径。 |
| `0x528EE0` | PersistentGameData Steam Cloud 写入；入口不以 readonly 阻断。 |
| `0x5294F0` | PersistentGameData 本地文件写入；空 roster 可达写路径，入口不检查 readonly。 |
| `0x558ED0` | active-run 与空 roster 路线可执行 GameState save，并可继续保存 persistent。 |
| `0x529790` | Steam achievement 相关平台调用；磁盘文件哈希不能覆盖该提交面。 |

因此本版不能冻结一个原生、正常可达且持续无写的实验场景；`nativeNonwritingScenarioProven=false`、`zeroPersistenceProven=false`。文件前后 hash、关闭 Cloud、readonly 单字节、默认初始角色或“没打 Boss”都不能独自闭合验收。

## 6. 后续实现和独立测试的准入

机器证据固定四个 missing ID：

- `PREJOIN_INDEPENDENT_ACTIVE_OFFLINE_UNPROVEN`
- `PERSISTENCE_NATIVE_NONWRITING_SCENARIO_UNPROVEN`
- `PERSISTENCE_INIT_TAIL_WRITE_CLOSURE_UNPROVEN`
- `PERSISTENCE_DISK_CLOUD_STEAM_EXCLUSION_UNPROVEN`

本轮允许主线实现**无 mutation 的阻断型 inspection/资格门**，独立测试验证字节、schema、hash、未知拒绝与零进程/arm 路径。即使 55 槽恢复 harness 将来通过，也不能使这四项变为 PASS。

最小补证工作是：找出并证明一个经游戏正常路径建立的实验场景，排除上述条件写、其余相关 Init/tail direct writes 和 callback 副作用；再冻结整个授权窗口、正常帧与 stop 后持久化静默条件，覆盖内存 dirty/stat/unlock、备份/磁盘、Cloud 与 Steam 提交。不得写只读位或清计数来制造证据，也不得将未知权限作为配置开关放行。

证据闭合后另建 static v2、scope/plan/catalog 与完整独立验证。当前没有 arm/token/claim、没有 active live authorization，也未解锁 05-05 或 T0-05 总门。

