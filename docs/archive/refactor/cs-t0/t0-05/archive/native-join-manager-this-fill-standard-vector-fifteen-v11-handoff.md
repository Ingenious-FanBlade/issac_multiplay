> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this standard-vector exact15 v11 交接

日期：2026-09-02  
状态：`0 VERIFIED / 15 UNKNOWN`（局部 resize 范围已确认；tuple backing/writer universe 未闭合）  
范围：只裁决 audit-v5 的 `V4_STANDARD_VECTOR_TUPLE_CLOSURE` exact15，不推广 whole-image 或 manager-this

## 结论

15 个 `memset` callsite 均是标准 vector resize 的 in-place tail-fill 分支：

```text
old_count < requested_count <= capacity_count
start       = begin + stride * old_count
fill_bytes  = stride * (requested_count - old_count)
available   = stride * (capacity_count - old_count)
start + fill_bytes <= capacityEnd
```

每行都在 `memset` 之后才发布新 `end`；相等/零长度分支绕开目标 callsite；超过 capacity 的分支调用该行绑定的 grow helper 并绕开目标 callsite。目标磁盘 PE 的 15 处 call bytes、`memset` direct target、15 个 grow-call target 均已重放。

这只能得到 `STATIC_CONFIRMED_CONDITIONAL_LOCAL_RANGE_ONLY`，不能得到 slot 排除。15 行的完整 direct-caller 集已经由只读 IDA `XrefsTo` 观测，并由目标 `.text` 的独立 raw-E8 target 扫描逐行精确相等验证；所有 15 个 containing function 的绝对 VA 在 PE 原始文件中均为零出现。因此 direct-call/forwarder 这一层已闭合。

仍未闭合的是每个 caller-owned tuple 的全部 `begin/end/capacityEnd` writer、fresh allocation/usable extent、旧 backing phi、scalar pointer adoption、异常生命周期和整数域。故每行都保留以下同构反例：

```text
begin = end = 0x00C71678
capacityEnd = begin + stride * requested_count
old_count = 0
```

各行选择的最小 `requested_count` 使 fill 至少覆盖 `[0x00C71678,0x00C7167C)`；局部 capacity guard 会接受该 tuple。只要完整 producer/writer universe 未排除此赋值，就不能声称目标范围不覆盖 manager slot。

## 逐行状态

| RVA | stride | 最小反例 fill | direct caller universe | 最终状态 | 未闭合对象 |
| --- | ---: | ---: | --- | --- | --- |
| `0x00308B5A` | 12 | 12 | raw/IDA 精确相等，3 calls | UNKNOWN | caller `this` tuple 的全部 writer/backing |
| `0x00325160` | 4 | 76 | 精确相等，2 calls | UNKNOWN | `a1+0x118` tuple 的对象生命周期 |
| `0x003271E2` | 4 | 4 | 精确相等，2 calls | UNKNOWN | collection record `+0x0C` tuple 的 fresh/existing phi |
| `0x003272FC` | 4 | 4 | 精确相等，2 calls | UNKNOWN | caller root tuple 的 writer/backing |
| `0x00330FE1` | 4 | 64 | 精确相等，2 calls | UNKNOWN | caller `this` tuple 的 writer/backing |
| `0x00331054` | 4 | 200 | 精确相等，3 calls | UNKNOWN | caller `this` tuple 的 writer/backing |
| `0x003310C4` | 4 | 392 | 精确相等，3 calls | UNKNOWN | caller `this` tuple 的 writer/backing |
| `0x0033112F` | 4 | 4 | 精确相等，18 calls | UNKNOWN | 18 条入口所传 tuple 的完整 writer/backing |
| `0x003350C6` | 8 | 8 | 精确相等，7 calls | UNKNOWN | receiver `+0x794` member tuple 的对象 extent/writers |
| `0x0033685C` | 4 | 4 | 精确相等，1 call | UNKNOWN | caller argument `a2` tuple 的 producer |
| `0x003DC995` | 8 | 8 | 精确相等，2 calls | UNKNOWN | caller `this` tuple 的 writer/backing |
| `0x003DCD16` | 1 | 4 | 精确相等，15 calls | UNKNOWN | byte-vector tuple 的 15 条入口 producer |
| `0x005CDDEF` | 8 | 8 | 精确相等，3 calls | UNKNOWN | receiver `+0x1E79C` tuple 的对象 extent/writers |
| `0x005D2C27` | 16 | 16 | 精确相等，7 calls | UNKNOWN | receiver `+0x518` tuple 的对象 extent/writers |
| `0x0061AC2C` | 16 | 16 | 精确相等，17 calls | UNKNOWN | member/stack phi 的全部 tuple 分支 |

## 冻结输入与目标

| 角色 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-bulk-memset-v3.json` | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json` | `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` |
| target `isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

复现器还要求：x86、image base `0x00400000`、SizeOfImage `0x008FE000`、可执行 `.text` 的 RVA/raw/flags、exact15 集合与顺序、v3 UNKNOWN/proof-gap、v5 proof-group、call bytes、目标 call、grow call、raw caller universe、逐行 countermodel、决议与全部 false 权限均不漂移。

## 产物与复现

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-fill-standard-vector-fifteen-v11.json` | `6580AB34B5050DEABE1FCE60E1EBDC581531A8F8C3A3498E9E3BADC2F65637F6` |
| `out/cs-t0/analyze-manager-this-fill-standard-vector-fifteen-v11.py` | `B9A4C746B11FAAF22B407B4124EBD30FD7F43D34F217C997EEE8150C9BB054DD` |

```powershell
python out\cs-t0\analyze-manager-this-fill-standard-vector-fifteen-v11.py --verify-existing
```

结果：

```text
PASS existing=out\cs-t0\native-join-manager-this-fill-standard-vector-fifteen-v11.json sha256=6580AB34B5050DEABE1FCE60E1EBDC581531A8F8C3A3498E9E3BADC2F65637F6 verified=0 unknown=15 mutations=70
```

70 项语义变异覆盖 schema/identity/bindings、exact15、summary/decision、每行 status/RVA/producer/countermodel 和权限，全部失败关闭。

## 操作边界

本轮只读取冻结 JSON、目标磁盘 PE 与只读 IDA 数据库。没有启动游戏，没有打开或读取目标进程，没有附加调试器，没有修改 IDB；没有修改 production、tests、既有证据、formal artifact、index、status 或 research plan。该 exact15 artifact 不允许 manager-this、whole-image alias 或任何运行时能力升级。
