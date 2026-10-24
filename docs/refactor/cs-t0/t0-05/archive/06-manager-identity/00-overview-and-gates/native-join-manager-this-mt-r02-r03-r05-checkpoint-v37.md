> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this MT-R02/R03/R05 checkpoint v37

日期：2026-09-06  
状态：`EXACT5_ADJUDICATED / 0 NEW POSITIVE / 5 UNKNOWN / BLOCKED_EXACT_99`

## 1. 当前结论

在 v36 之后继续审计 `MT-R02`、`MT-R03`、`MT-R05` exact5。五行均通过候选前
封印、作者分离的独立 TEST，但没有一行关闭正向晋升所需的全部四项全称义务：

- `MT-R02` 两行：按 `newCount` 分配、按 `oldCount` 复制，缺少
  `oldCount<=newCount` 与两端 backing extent fixed point；
- `MT-R03` 两行：5000 比较只记录并回流，5001 最小路径未被硬控制流排除；
- `MT-R05` 一行：辅助范围非空而过滤结果为空的合取未被支配 guard 排除。

因此前十项仍为 `4 STATIC_CONFIRMED_NO_OVERLAP / 16 UNKNOWN`；v33 仍是当前机器
frontier：copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`。
不创建空 successor，不改变 R04 跳过边界，也不提升 manager-this、whole-image 或
formal v7。

## 2. 精确停止点

| 任务 | RVA | first open obligation | retained countermodel |
|---|---|---|---|
| `MT-R02` | `0x00008A62` | `backingExtent` | `oldCount=2,newCount=1`：分配 4 字节、复制 8 字节。 |
| `MT-R02` | `0x00008B2A` | `backingExtent` | 第二组 pair 的同一独立 countermodel。 |
| `MT-R03` | `0x0001513A` | `backingExtent` | `5001*84=420084 > 420000`。 |
| `MT-R03` | `0x000155A4` | `backingExtent` | `5001*108=540108 > 540000`。 |
| `MT-R05` | `0x0033570B` | `countAndNoWrap` | aux 非空且 filtered 为空时 raw length=`0xFFFFFFFC`。 |

R02/R03 的局部控制流已经提供更强负向证据，继续做普通场景采样不能得到正向全称
证明。R05 若以后做存在性动态复核，必须命中 `sub_734F30` 且同时观测 aux 非空、
filtered 为零；当前没有可可靠映射到该内部合取的特定游戏操作，所以本轮未启动动态
实验，也未要求用户进入猜测场景。

## 3. 证据与验收

- [static follow-up handoff](../02-copy-move/native-join-manager-this-mt-r02-r03-r05-static-followup-v1-handoff.md)
- [sealed TEST preparation](../02-copy-move/native-join-manager-this-mt-r02-r03-r05-exact5-sealed-preparation-v1-handoff.md)
- [sealed independent audit v2](../02-copy-move/native-join-manager-this-mt-r02-r03-r05-exact5-sealed-independent-audit-v2-handoff.md)

| 机器产物 | SHA-256 |
|---|---|
| candidate | `47B1E320B329C5571D2C7F6CD35D48EBDF89D69E5EE6B5BC7C659E509689F694` |
| independent audit v2 | `C17A2589256FEF6152F5878FE0FADDE7219C68D8B5ED5F852CE9704A85E32ECE` |

测试只保留封印 preparation 的 create/verify、candidate audit 的 create/verify、六个
决策性 mutation 和 wrong-hash fail-before-write。两次 harness 预写失败没有生成 audit，
修正版 append-only 保留失败原因且没有修改 contract。未运行 Cargo、全量 build、游戏、
进程访问、调试器或 IDB 修改。

## 4. 关闭状态

`manager-this`、whole-image raw v2、formal v7、active/live、runtime mutation、05-05、
T0-05 总门和 T0-06 继续关闭。v33 frontier 和所有既有权限字段保持不变。
