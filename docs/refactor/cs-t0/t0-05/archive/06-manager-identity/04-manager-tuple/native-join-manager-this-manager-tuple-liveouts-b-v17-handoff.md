> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this：manager tuple live-out Group B v17 switchfix 逆向交接

## 结论

本轮仅关闭接受版 v15 `remainingV11Frontier.blockLiveOutAliases` 中指定的 Group B：16 个函数、每函数 2 个上下文，共 **32/32**。所有输入行均从 v15 原始数组提取并反向断言；没有修改或重排上游证据。

函数内 CFG 固定点以 `(basicBlockVa, register/stack abstract alias state)` 合流，共访问 243 个状态并得到 72 个终点：

- 28 个上下文存在对 manager tuple `end@+4` 的只读 consumer 分支，共 14 个唯一读点；
- 4 个上下文把 `ECX=manager tuple this` 跨块转发给已经由 helpers-a v13 证明为只读的 helper，共 2 个新发现 source callsite；
- 36 个终点在寄存器或栈抽象状态中杀死别名；
- 4 个终点进入 `_invalid_parameter_noinfo_noreturn`，别名既未作为 `ECX` 也未作为栈参数转发；
- tuple `begin/end/capacityEnd` 写终点为 0，新 helper target 为 0，新 writer 为 0，最小未决 frontier 为 0。

因此本轮决定为 `STATIC_CONFIRMED_GROUP_B_EXACT32_REDUCTION_ONLY_GROUP_A40_AND_C16_REMAIN_UNKNOWN`。Group A 的精确 40 行和 Group C 的精确 16 行仍原样保持 `UNKNOWN`；不得据此宣称 v11 全局 writer universe、backing-pointer writer closure 或 manager-this promotion 已完成。

## switch successor 修正审计

首版 v17 引擎把任何无 direct target 的 `jmp` 都先当成 tail/forwarder 终点，遗漏了 IDA 已经为 switch materialize successor 的情况。全 CFG 可达审计在函数 `0x00800500` 的两个 seed 上均命中：

- `0x00800885`，successors=`0x0080088C,0x008009DE,0x008010FD`；
- `0x008009C6`，successors=`0x008009CD,0x00800B49,0x00800B92,0x00800C32,0x00800CC0,0x008010FD`。

因此旧机器证据 `native-join-manager-this-manager-tuple-liveouts-b-v17.json` 不再作为证明输入。修正版先读取基本块 successor；只要 `jmp` 有 CFG successor（包括 indirect switch），就继续 fixed-point，而不是形成 `FORWARDED_JUMP`。修正后 exact32 全量重跑仍得到 243 states / 72 terminals；专门审计覆盖 709 个可达 `(function,block)`，观察到 4 个 `(seed,switch)` 命中、2 个唯一 switch 指令，而 probe 的 switch 误分类 terminal 为 **0**。

## 精确新 source

| source callsite | source function | target | 继承分类 | tuple 效果 |
| --- | --- | --- | --- | --- |
| `0x007FBA95` | `0x007FB250` | `0x009B92C0` | v13 `READ_ONLY_MANAGER_TUPLE` | `begin/end=READ`，`capacityEnd=UNTOUCHED` |
| `0x00956408` | `0x009563B0` | `0x009B9310` | v13 `READ_ONLY_MANAGER_TUPLE` | `begin/end=READ`，`capacityEnd=UNTOUCHED` |

`0x007FBB2C -> 0x00813520` 不是新的 tuple source。精确指令链为：

1. `0x007FBB17 push ecx` 临时把 tuple alias 放到栈顶；
2. `0x007FBB18 mov [esp], 0` 在调用前覆盖该栈槽；
3. `0x007FBB29 mov ecx, esi` 杀死 ECX tuple alias；
4. 因此 `0x007FBB2C` 的 `sub_813520` 不接收 tuple alias。

探针已显式建模 `[esp+disp]` 覆盖及函数内 direct jump，防止把该路径误判成第三个 source。

## 唯一静态锚点

14 个 `end@+4` 读点：

`0x00800596`、`0x00802186`、`0x00817063`、`0x00833B90`、`0x008342A0`、`0x0083BAF8`、`0x0084C1C5`、`0x008D19F6`、`0x0090D410`、`0x00944AF0`、`0x0098DC20`、`0x009A87C7`、`0x009A8B20`、`0x009ACD20`。

两个不返回 sink 为 `0x00833C07` 与 `0x00834327`，均调用导入槽 `0x00B18894` 的 `_invalid_parameter_noinfo_noreturn`。

连续字节验证覆盖：目标 SHA-256、image base、16 个函数的完整边界与完整函数字节 SHA-256，以及上述 consumer/source/noreturn/栈覆盖链共 21 个语义签名。任一漂移都会 fail closed。

## 产物与摘要

- create-new 机器证据：`out/cs-t0/native-join-manager-this-manager-tuple-liveouts-b-v17-switchfix.json`
  - SHA-256：`B0BC798C723DC8D4C539C9793391019BA6021B85DDA9F6BE0E98E6D4AC3952B3`
- IDA create-new reproducer：`out/cs-t0/analyze-manager-this-manager-tuple-liveouts-b-v17.py`
  - SHA-256：`BA06C5D3E4E9EE89F31454C72135DCB6AE71A84771FD0F3D8570BE2DBB6AED92`
- CFG fixed-point engine：`out/cs-t0/probe-manager-this-manager-tuple-liveouts-b-v17.py`
  - SHA-256：`6002C011CC2A5B49FC773E3A115548E0156BEAA2042A1310FC0F1F0F2F750878`
- switch successor 审计：`out/cs-t0/audit-manager-this-manager-tuple-liveouts-b-v17-indirect-jumps.py`
  - SHA-256：`51F9CE49952924AE0A2760DF15DB44695AB8A282FE6D7745F38F7CF7B057A82B`
- 接受的 v15 输入：`out/cs-t0/native-join-manager-this-manager-tuple-helpers-c-v15.json`
  - SHA-256：`B07501A2D6DD6492E6834ECC64A5B23FF3B125DB415643B87F04DE4FB53EDCAD`

上游直接绑定：

- v11 primary：`D7BBF8A0EF3B7C46AFB7D04A831E400329BE8F3711D2B98D16E732E80C733716`
- helpers-a v13：`322FDC72EDF3C23B05636420F7FB65D48962CF4940E5B834834B07F93EC27C88`
- helpers-b v14：`A08B6D6E286E1177D3CFF40BB4AA4C02FFEC8EF207913C072AA985E71031F969`

失败的 helpers-a v12 没有被读取或绑定。

## 重放

在已经打开、冻结的 approved-unpacked `isaac-ng.exe` IDA 数据库中运行：

```text
mcp__ida_pro_mcp__py_exec_file(
  file_path="D:\projects\isaac_hook_reserach\out\cs-t0\analyze-manager-this-manager-tuple-liveouts-b-v17.py"
)
```

首次输出：

```text
PASS created exact_contexts=32 resolved=32 unresolved=0 states=243 terminals=72 tuple_writers=0 new_sources=2 remaining_A=40 remaining_C=16 mutations=30
```

同一文件第二次连续重放输出：

```text
PASS verified exact_contexts=32 resolved=32 unresolved=0 states=243 terminals=72 tuple_writers=0 new_sources=2 remaining_A=40 remaining_C=16 mutations=30
```

随后重放 switch 审计：

```text
mcp__ida_pro_mcp__py_exec_file(
  file_path="D:\projects\isaac_hook_reserach\out\cs-t0\audit-manager-this-manager-tuple-liveouts-b-v17-indirect-jumps.py"
)
```

输出：

```text
PASS exact_contexts=32 cfg_reachable_function_block_pairs=709 switch_context_hits=4 unique_switch_instructions=2 misclassified_terminal_hits=0 successor_first=1
```

语义 mutation gate 为 30/30 fail closed。逆向子任务未自行作最终测试验收；仍需由独立测试职责重放并核对字节摘要、集合分区、source 关系与 mutation 拒绝。

## 权限与边界

本轮没有启动游戏、没有打开或读取目标进程、没有附着调试器、没有修改 IDB、生产代码、测试、历史证据、索引或状态文档。机器证据中全部 permissions 为 `false`，`managerThisPromotionAllowed=false`，也不授予 runtime mutation、active 实机或任何下游能力。
