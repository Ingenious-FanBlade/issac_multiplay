> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this short-vector exact7 v12 交接

日期：2026-09-02  
状态：`0 VERIFIED / 7 UNKNOWN`  
范围：只裁决 standard-vector-v11 的 short helper exact7；不扩大到其余八行，不推广 whole-image 或 manager-this

## 结论

本轮闭合了这七个 resize helper 自身的 direct-call universe，并绑定了每条 fresh grow/allocation/install 路径：

| 目标 memset RVA | helper | direct calls | fresh allocation / install | 裁决 |
| --- | --- | ---: | --- | --- |
| `0x00308B5A` | `sub_708AE0`，stride 12 | 3 | `sub_709480`：`12*capacity`，安装三元组 | `UNKNOWN` |
| `0x00330FE1` | `sub_730F90`，stride 4，resize 16 | 2 | `sub_732020`：`4*capacity`，安装三元组 | `UNKNOWN` |
| `0x00331054` | `sub_731000`，stride 4，resize 50 | 3 | `sub_732100 -> sub_41D150` | `UNKNOWN` |
| `0x003310C4` | `sub_731070`，stride 4，resize 98 | 3 | `sub_7321A0 -> sub_41D150` | `UNKNOWN` |
| `0x0033112F` | `sub_7310E0`，stride 4，dynamic resize | 18 | `sub_732240 -> sub_41D150` | `UNKNOWN` |
| `0x003DC995` | `sub_7DC940`，stride 8 | 2 | `sub_737F40`：`8*capacity`，安装三元组 | `UNKNOWN` |
| `0x003DCD16` | `sub_7DCCD0`，stride 1 | 15 | `sub_7DEDB0 -> sub_7DF050`：`capacity` bytes | `UNKNOWN` |

目标 `.text` 的 raw E8 扫描与只读 IDA `XrefsTo` 对每个 helper 精确相等；七个 helper 的绝对 VA 在目标原始文件中均为零出现，因此 helper 自身没有未枚举的 address-taken 调用入口。目标 call bytes、memset target、grow-call target、writer install bytes 和共同 zero/free-zero bytes 均由复现器绑定。

这些事实只证明 fresh grow 分支安全。v11 已确认的 in-place 不等式仍是条件式：

```text
0 < requested-old <= capacity-old
start = begin + stride*old
fill  = stride*(requested-old)
start + fill <= capacityEnd
```

当 helper 在旧 tuple 上进入 in-place 分支时，仍需证明该 `begin/end/capacityEnd` 的完整 construction/copy/move/clear/grow writer universe。七行均至少存在一个没有闭合的旧 backing phi，所以不能推广。

## 逐行阻断

### `0x00308B5A`

三个 receiver 分别是 `DWORD[0x00C71678]+0x6774C` 与两个对象 `+0x1E790` member。虽然 `sub_709480` 的 `12*capacity` fresh grow 已闭合，但三个 member 的全生命周期 writer/copy/move/adoption 集未闭合。仍可构造旧 tuple 指向 manager slot 的反例，因此保留 `UNKNOWN`。

### `0x00330FE1`

两处调用都传对象 `+0x44` tuple。`sub_724CB0` 在构造时把 `+0x44/+0x48/+0x4C` 清零；`sub_724F00` 的构造路径会走 grow 而不执行目标 in-place memset，reset 后同尺寸路径也不执行目标。真正的缺口是 `sub_7252A0`：它可从 reset/destructor 的任意旧对象状态进入，完整生命周期 writer 未闭。

### `0x00331054` / `0x003310C4`

两行分别操作 `+0x30` 与 `+0x24` tuple。构造器清零与 `sub_732100/sub_7321A0 -> sub_41D150` fresh install 已确认；但 `sub_7252A0` 的 reset/destructor 入口和 `sub_72CFF0` 的 parser 入口都可带入旧 backing。未证明所有 copy/move/adoption writer，因此两行仍为 `UNKNOWN`。

### `0x0033112F`

18 个 direct call 已逐一记录 receiver。除多个固定 member 外，`sub_725570` 的两处调用从栈槽取 caller-supplied tuple，`sub_9CE720` 与 `sub_9D8580` 直接接受 caller-supplied tuple；`sub_755470` 还是 data-xref/address-taken owner method，内部传 `+0x16C8/+0x16D4`。这些都是明确的未闭 writer/lifetime 入口，故为强 `UNKNOWN`。

### `0x003DC995`

两处调用都传 serializer/deserializer 对象 `+0x1D4` tuple。`sub_737F40` 的 fresh `8*capacity` allocation/install 已闭合，但该 member 在两个方向上的 construction/copy/move/clear 全集未闭；旧 backing phi 保留，裁决 `UNKNOWN`。

### `0x003DCD16`

15 个 call 中，`sub_A2C6E0` 直接接收 caller-supplied `a4` byte-vector tuple；`sub_755470`、`sub_A57C40`、`sub_A57EB0` 与 `sub_A583E0` 存在 data xref/address-taken owner-method 入口。`sub_A58040` 的 fresh BufferedStream constructor 虽把 `+0x0C` tuple 清零，使其构造调用绕过目标 in-place memset，但这不能覆盖其余旧 backing 路径。因此保留 `UNKNOWN`。

## 冻结输入与产物

| 角色 | SHA-256 |
| --- | --- |
| standard-vector exact15 v11 JSON | `6580AB34B5050DEABE1FCE60E1EBDC581531A8F8C3A3498E9E3BADC2F65637F6` |
| bulk memset v3 JSON | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| independent range audit v5 JSON | `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` |
| target `isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| v12 machine JSON | `71E9906ECA9E21C4EBE1D757A349D4A484DFC4393B3F6688904C591C5EBFDD8D` |
| v12 reproducer | `C39D4D175DE5D5187F083642745D6385E3CB6B262DDC4C2602AF9E0E96431784` |

## 复现与 mutation gate

```powershell
python out\cs-t0\analyze-manager-this-fill-vector-short-seven-v12.py --verify-existing
```

结果：

```text
PASS existing=out/cs-t0/native-join-manager-this-fill-vector-short-seven-v12.json sha256=71E9906ECA9E21C4EBE1D757A349D4A484DFC4393B3F6688904C591C5EBFDD8D verified=0 unknown=7 mutations=41
```

41 项 mutation 覆盖 predecessor/target/exact7、summary/decision/permission，以及每行 status、caller、writer completeness、old-backing provenance 和 arithmetic；全部失败关闭。

## 后续最小生命周期家族

若继续这七行，应按独立家族拆分，不再按 helper 混合：

1. `sub_724CB0/sub_725190/sub_7252A0/sub_72CFF0` 对象的 `+0x00/+0x0C/+0x18/+0x24/+0x30/+0x44` 六个 tuple writer 全集；
2. serializer/deserializer 对象的 `+0x194..+0x1D4` 与 `+0x4C4/+0x4DC/+0x4E8/+0x500` member 生命周期；
3. address-taken `sub_755470` owner family；
4. BufferedStream vtable/constructor/destructor 的 `+0x0C` tuple；
5. caller-supplied tuple 路由 `sub_725570/sub_9CE720/sub_9D8580/sub_A2C6E0`。

任何家族只要不能封闭 scalar adoption、copy/move、异常发布与旧 backing phi，就应继续保持对应行 `UNKNOWN`。

## 操作边界

本轮只读取冻结 JSON、目标磁盘 PE 与已经打开的只读 IDA 数据库。没有启动游戏，没有打开或读取目标进程，没有附加调试器，没有修改 IDB；没有修改 production、tests、既有证据、formal artifact、index、status 或 research plan。全部权限为 false。
