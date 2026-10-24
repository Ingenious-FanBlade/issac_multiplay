> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this MT-R01–R10 follow-up checkpoint v36

日期：2026-09-06  
状态：`FOLLOWUP_ADJUDICATED / 4 POSITIVE / 16 UNKNOWN / BLOCKED_EXACT_99`

## 1. 结论

本轮继续处理 v34 中 `MT-R01`–`MT-R10` 尚未正向闭合的 18 行。按用户要求，
`MT-R04` 的两行暂时跳过 follow-up 独立测试，因此本轮正式 TEST 范围为 exact16。

- 新增并经作者分离 TEST 接受：`0x0061F2C2`、`0x0068E3DB`。
- 先前已接受：`0x001CBEB5`、`0x00689F63`。
- 前十项当前合计：`4 STATIC_CONFIRMED_NO_OVERLAP / 16 UNKNOWN`。
- v33 当前机器前沿：copy/move `323/405`、memset `225/242`，总计
  `548/647`，remaining `99`。
- `MT-R04` 的 `0x0001AAB0`、`0x0001AEB8` 未产生 follow-up verdict、mutation
  或 promotion，并已独立确认仍保留在 callerHeap、copy/move remaining 与负向审计
  coverage 三个集合中。
- manager-this、whole-image raw v2、formal v7、active/live、runtime mutation、
  05-05、T0-05 总门和 T0-06 全部继续关闭。

## 2. 前十项当前逐任务状态

| 任务 | 正向 / UNKNOWN | 当前结果 |
|---|---:|---|
| `MT-R01` | `1 / 0` | `0x001CBEB5` 沿用 v34 已接受结论。 |
| `MT-R02` | `0 / 2` | allocation 乘法有溢出哨兵，但复制仍为 `4*oldCount`；`oldCount=2,newCount=1` 未被硬边界排除。 |
| `MT-R03` | `0 / 2` | 三处 caller 收敛到 `sub_40ED50`；5000 诊断仍回流，parser count producer 未闭。 |
| `MT-R04` | `0 / 2` | 本轮按用户要求跳过 follow-up TEST；沿用 v34 `UNKNOWN`，两行未移出 remaining。 |
| `MT-R05` | `0 / 1` | 最小未决合取收窄为 `auxiliaryRangeCount>0 && filteredCount==0`；无支配 nonempty guard。 |
| `MT-R06` | `1 / 0` | 四类 InputDevice 的 vtable/constructor/target 集合分区和双缓冲 writer/lifetime fixed point 经修订 v3 与独立重算闭合。 |
| `MT-R07` | `0 / 5` | caller-owned output span、`a3*a4` 与 grow 乘加 no-wrap 仍无全称证明。 |
| `MT-R08` | `2 / 1` | `0x00689F63` 沿用 v34；`0x0068E3DB` 新闭合；`0x00661577` 保留 `Size=0x10001` 反例。 |
| `MT-R09` | `0 / 2` | 两个 source root 已分别收窄；`0x8001` source count 与 producer/lifetime fixed point 仍未闭。 |
| `MT-R10` | `0 / 1` | 22 个直接 writer 中 19 个已局部闭为安全，剩余 3 个值传播 writer 与 28 个 LEA alias 仍开放。 |

## 3. 新正向证据

### `MT-R06 / 0x0061F2C2`

REVERSE v2 曾把扫描集合误写为“81 个 vtable 方法”。独立 TEST 拒绝该精确措辞后，
append-only v3 把集合修正并由 TEST 重算：四张 38-slot vtable 的去重集合为
`75 functions / 18 hits`；另有五个 constructor/path function（Base ctor 贡献两处
写）与目标函数 `sub_A1F280`（三处读），合计 `81/23`。写者只剩 Base ctor、
Init 与 cleanup。Init 先通过动态类型对应的 `+0x14` 填充 16-byte 元素 tuple，
再以同一长度分配两个同时存活的 heap block；模板 size 字段没有 post-init writer。

### `MT-R08 / 0x0068E3DB`

`cap<0` 分支绕过 memmove；其余入口先令 `used=consumed=0`。解析器只在完整
header/body 位于 `used-consumed` 范围内时推进 consumed，恢复搜索也不越过 used，
因此归纳保持 `0<=consumed<=used<=cap`。移位长度为 `used-consumed`，读写两端都
留在同一 live malloc/realloc backing 内；与静态 manager-this slot 不相交。

## 4. 机器证据

| 产物 | SHA-256 |
|---|---|
| corrected REVERSE v3 candidate | `E204077A0A6C922BA1163F8CC4247594079E6116D41C99D2470E5E4FFA5568F8` |
| exact16 independent audit | `064079AB81F4FD18079209444E4718242A04B08C602810133B7FE4212FC8ABDB` |
| v33 frontier | `861BEA2C2B26FA999F4043ACDEE53786759CB4341A6F5EB2B37FEA6645E5B032` |
| v33 independent audit | `176DAD8177015908EC7E49FFF020BE0030E2747DB6AB672183D9CD3A1DFDEB69` |

详细 REVERSE 和 TEST 交接：

- [R02/R03/R05/R09/R10 static successors](../02-copy-move/native-join-manager-this-mt-r02-r03-r05-r09-r10-static-successors-v1-handoff.md)
- [R06/R07/R08 corrected successor v3](../02-copy-move/native-join-manager-this-mt-r06-r07-r08-successor-v3-handoff.md)
- [exact16 independent audit](../02-copy-move/native-join-manager-this-mt-r02-r10-exact16-independent-audit-v1-handoff.md)
- [v33 frontier](../01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v33-handoff.md)
- [v33 independent audit](../02-copy-move/native-join-manager-this-bulk-remaining-frontier-v33-independent-audit-v1-handoff.md)

## 5. 压缩测试与动态边界

每个新 REVERSE successor 只执行 create-new、一次 verify-existing 和核心 mutation；
两层独立 TEST 各只执行 create-new、一次 verify-existing、六个决策分支 mutation 与
wrong-hash fail-before-write。未运行 Cargo、全量 T0 build、重复 self-test 或无关回归。

本轮只使用冻结 PE/IDA 静态读取，没有启动游戏、打开游戏进程、附加 x32dbg、注入、
修改 IDB 或执行运行时 mutation。保留 `UNKNOWN` 不需要动态场景。若以后单独复现
`0x00661577` 的负向路径，需要长度精确为 `0x10001` 的畸形联机/流载荷；该实验不是
当前正向闭包所必需，普通游戏样本也不能替代全称静态证明，因此本轮未请求该场景。
