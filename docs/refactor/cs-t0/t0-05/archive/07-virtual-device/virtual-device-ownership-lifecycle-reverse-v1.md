> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device ownership/lifecycle 静态逆向交接 v1

## Scope 与证据边界

- 角色：独立 `REVERSE`；范围仅限 `VIRTUAL_DEVICE_OBJECT_OWNER`、
  `VIRTUAL_DEVICE_CREATION_DESTRUCTION`、`VIRTUAL_DEVICE_REFERENCE_COUNTING`。
- 目标：固定 Win32/x86 `isaac-ng.exe`；IDA image base `0x00400000`；
  `VA = RVA + 0x00400000`。
- 目标 SHA-256：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- IDB：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`；
  IDA health 为 `ok`，x86、auto-analysis 与 Hex-Rays ready，目标 hash 与计划一致。
- 只读静态分析：未启动/打开/枚举游戏进程，未使用 x32dbg，未注入、未创建 IPC、
  未修改 IDB、未实现代码、未运行测试。
- `NetInputDevice` 仅作为 OriginalOnline 原生生命周期参照；禁止把它、Online Manager、
  ready gate 或其他 OriginalOnline 状态提升为 local C/S 正向组件。
- 前序绑定：`virtual-device-static-reverse-v1.md` SHA-256
  `DCD5581CE7901DB6812248834A7CD7F249B81086BECF5EA69002531C18F6AE11`；
  `coop-virtual-device-decision-v1.json` SHA-256
  `1F95C4E5EB3D538C4EF0C33BB51706453CDF9C30E162DA1AB27AE9D42ACD16DE`。

## 原生创建路径：STATIC_CONFIRMED

### NetInputDevice 构造入口穷尽

- ctor A：RVA `0x501930`，ABI `NetInputDevice* __thiscall(NetInputDevice*, int)`，`ret 4`；
  唯一 code xref 为 RVA `0x2F664C`。
- ctor B：RVA `0x5015B0`，同为单参数 `__thiscall`、`ret 4`；唯一 code xref 为 RVA `0x2F6704`。
- 两个 ctor 均先调用 InputDeviceBase ctor RVA `0x61E050`，再写 Net vtable RVA `0x778030`。
- Net vtable 的直接 data xref 仅见 ctor A、ctor B 与 deleting dtor RVA `0x5017C0`。
- 原生 local 分支 RVA `0x2F6631`：
  bytes `68 F8 03 00 00 E8 85 8E 31 00 ... E8 DF B2 20 00`，即分配 `0x3F8`
  后在 RVA `0x2F664C` 调 ctor A；RVA `0x2F666A` bytes `E8 F1 F8 20 00`
  调 wrapper RVA `0x505F60`。
- 原生 remote 分支 RVA `0x2F66E7`：
  bytes `68 F8 03 00 00 E8 CF 8D 31 00 ... E8 A7 AE 20 00`，即分配 `0x3F8`
  后在 RVA `0x2F6704` 调 ctor B；RVA `0x2F6717` bytes `E8 44 F8 20 00`
  调同一 wrapper。
- 两个 ctor 均只有上述 OriginalOnline 函数 RVA `0x2F6500` 一个 caller；
  wrapper 也仅有该函数内两个 callsites。

### 物理设备对照

- InputDeviceBase deleting dtor：RVA `0x61E1B0`，delete flag bit 0 决定是否释放 `0x370`。
- Keyboard ctor 在 RVA `0x621A41` 调 base ctor，control block 在 RVA `0x621AB0`
  调公共 refcount ctor；Keyboard deleting dtor RVA `0x66C390` 释放 `0x370`。
- Gamepad ctor 在 RVA `0x621DF2` 调 base ctor，control block 在 RVA `0x621EBE`
  调公共 refcount ctor；Gamepad deleting dtor RVA `0x6220F0` 释放 `0x4F0`。
- Keyboard/Gamepad 与 Net wrapper 均采用下述相同六槽 refcount 机制；这只能证明原生模式，
  不能证明未来独立 virtual InputDevice 的 factory、owner 或线程契约。

## wrapper 与 manager ownership：STATIC_CONFIRMED 子链

- wrapper RVA `0x505F60`，ABI `void/int __stdcall(InputDeviceBase*)`，`ret 4`。
- RVA `0x505F8B` 起始 bytes：
  `6A 18 C7 07 00 00 00 00 C7 47 04 00 00 00 00 E8 21 95 10 00`；
  创建 8-byte 临时 shared pair，并分配 `0x18`-byte control block。
- RVA `0x505FB7` 调 refcount base ctor RVA `0x0000CC10`；RVA `0x505FBC`
  写 vtable RVA `0x778014`，RVA `0x505FC2` 写 `control+0x14 = object`。
- RVA `0x505FF1..0x506000` bytes
  `B9 18 7B C5 00 89 77 04 89 1F E8 F0 AD 11 00`：把 pair 交给
  InputManager RVA `0x857B18` 的 insert RVA `0x620DF0`。
- insert 在 manager `+0x14` mutex 下把 8-byte pair copy 到 vector
  `+0x08/+0x0C/+0x10`；非增长路径调用 pair copy RVA `0x502D60`，增长路径 RVA
  `0x621630` 对新旧元素逐一 copy，再 release 旧元素并释放旧 buffer。
- pair copy RVA `0x502D60` 先调用 control vtable slot `+0x04` 获取 strong ref；
  因而 insert 后 vector 持有一个 strong ref，按值临时 pair 在函数退出时 release。
- insert 解锁后以另一个 shared pair copy 调 manager vtable `+0x78` notification。
- manager remove RVA `0x620EB0` 的 callers 为 RVA `0x506020`、`0x50A8A0`、`0x621F40`；
  它先以 shared pair copy 调 vtable `+0x7C/+0x80` callbacks，再按 object pointer 查找元素。
- 找到后，RVA `0x620F38` 调 shift helper RVA `0x621570`；该 helper 用 pair copy/acquire、
  pointer/control-block swap 与临时 release 左移元素。随后 remove 对旧 tail 调 slot `+0x0C`
  release，并令 vector end 减 `8`（RVA `0x620F6B`）。
- Net 对照 remove wrapper RVA `0x506020`：按 `device+0x0C` key 查询 shared pair，copy 后调
  RVA `0x620EB0`；Gamepad disconnect RVA `0x621F40` 经 RTTI 与设备身份匹配后调用同一 remove。
- manager stop RVA `0x61FAD0` 在 mutex 下调用 RVA `0x621800` release `[begin,end)` 全部 pair，
  置 `end=begin` 后解锁。
- manager deleting dtor RVA `0x61F950` 调 RVA `0x61F980`；后者最终调用 vector destructor
  RVA `0x6214F0`，再次 release `[begin,end)`、释放 buffer 并清零 begin/end/capacity。

## 0x18 ReferenceCount ABI：STATIC_CONFIRMED 子链

### Layout 与初值

| Offset | 静态含义 |
| --- | --- |
| `+0x00` | control-block vptr |
| `+0x04` | `uint16 strong` |
| `+0x06` | `uint16 weak` |
| `+0x08` | embedded mutex vptr |
| `+0x0C` | mutex state/flag 区域 |
| `+0x10` | allocated `CRITICAL_SECTION*` |
| `+0x14` | managed `InputDeviceBase*` |

- ctor RVA `0x0000CC10` 在 RVA `0x0000CC42` 写 dword `0x00010001`，即
  `strong=1, weak=1`，初始化 embedded mutex/critical section。
- Base/Net、Keyboard、Gamepad specialization vtable 分别位于 RVA
  `0x778014`、`0x782918`、`0x782934`，三者 24-byte 内容相同：
  `50 CB 40 00 B0 C6 40 00 90 C6 40 00 B0 58 43 00 10 C6 40 00 C0 C5 40 00`。

### 六个 vtable slots

| Slot | RVA | ABI 与静态行为 |
| --- | --- | --- |
| `+0x00` | `0x0000CB50` | `void* __thiscall(ctrl*, uint8 deleteFlag)`；销毁 critical section/mutex；flag bit 0 时释放 `0x18` control block；`ret 4` |
| `+0x04` | `0x0000C6B0` | `bool __thiscall(ctrl*)`；锁定并检查 strong；非零时转调 `+0x08` acquire；`AL` 返回 |
| `+0x08` | `0x0000C690` | `void/int __thiscall(ctrl*)`；mutex 下 `++uint16 strong` |
| `+0x0C` | `0x000358B0` | `bool __thiscall(ctrl*)`；mutex 下 `--strong`；变零时 inline 调 `object->vtable[0](../../07-virtual-device/1)`、清 `+0x14`，再转调 `+0x14` weak release |
| `+0x10` | `0x0000C610` | `void/int __thiscall(ctrl*)`；mutex 下 `++uint16 weak` |
| `+0x14` | `0x0000C5C0` | `bool __thiscall(ctrl*)`；mutex 下 `--weak`；变零时调 `ctrl->vtable[0](../../07-virtual-device/1)` 释放 control block |

- strong acquire bytes（RVA `0x0000C690`）：
  `56 8B F1 57 6A FF 8B 46 08 8D 4E 08 FF 50 0C 66 FF 46 04 ...`。
- last-strong bytes（RVA `0x000358B0`）包含：
  `66 FF/DEC [ctrl+04]` 的编码序列、`8B 4E 14 ... 6A 01 FF 10` 对象 deleting dtor，
  以及 `C7 46 14 00 00 00 00` 清 managed pointer。
- weak acquire 使用 `66 FF 46 06`；last-weak 路径使用 `6A 01 FF 10` 调 control deleting dtor。
- Net deleting dtor RVA `0x5017C0`，ABI
  `NetInputDevice* __thiscall(NetInputDevice*, uint8 deleteFlag)`，`ret 4`；
  RVA `0x5018C3` 起 bytes
  `8B CF C7 87 70 03 00 00 20 2E B8 00 C7 07 58 27 B8 00 ...`，
  先退化 UserProfile/Base vtables 并调用 base cleanup；delete flag bit 0 时在 RVA `0x5018E8`
  以 size `0x3F8` 释放对象。

## 三项 verdict 与首缺口

### VIRTUAL_DEVICE_OBJECT_OWNER = UNKNOWN

- 已闭合：原生 wrapper 把初始 strong ref 交给 InputManager vector；copy/grow/remove/stop/destructor
  均能落到同一 refcount ABI，manager vector 是一个明确的长期 strong owner。
- 最小反例/首缺口：insert 的 vtable `+0x78` notification 与 remove 的 `+0x7C/+0x80`
  callbacks 均接收 shared pair copy；现有静态路径没有证明 callback 不持久化该 strong ref。
  因此不能证明 InputManager 是唯一长期 owner，也不能证明谁拥有最终 remove/final-release authority。
- future requirement：独立 virtual factory 必须声明唯一 lifecycle coordinator、manager/callback 的 owner
  cardinality、remove authority、reader drain 顺序；不得借用 Online Manager/raw-pointer vector 作为 owner。

### VIRTUAL_DEVICE_CREATION_DESTRUCTION = UNKNOWN

- 已闭合：原生两条 Net allocation→ctor→wrapper→manager insert 路径、manager remove、last-strong
  deleting dtor 及 manager stop/destructor 的正常释放子链均已定位；ctor/xref 范围也已穷尽。
- 最小反例/首缺口：wrapper 的 control-block allocation 若返回 null，CFG 从 RVA `0x505FB1`
  走 `0x505FC7`，仍在 `0x505FF6/0x505FF9` 构造 `{object, null-control}` 并调用 insert；
  静态证据没有提供该 allocator “只抛异常、绝不返回 null”的契约，也没有在此分支释放 object。
- 两个 ctor 虽有 SEH/unwind metadata 与分阶段状态，但当前静态 CFG 未闭合每个成员分配失败后的
  partial-destruction、原始 `0x3F8` allocation 回收、wrapper/insert/grow/callback 异常 rollback。
- future requirement：独立 virtual factory 必须另行定义 create/insert/assign 的事务边界、所有失败点
  rollback、reentrancy 和 exactly-once destroy；原生 ctor/dtor 形状不能替代该合同。

### VIRTUAL_DEVICE_REFERENCE_COUNTING = UNKNOWN

- 已闭合：control-block layout/初值、六槽 ABI、strong/weak acquire/release、last strong inline 删除对象、
  last weak 删除 control block、vector copy/grow/erase/stop/destructor 的 refcount 调用链均为静态确认。
- 最小反例/首缺口：slot `+0x0C` 在执行最后一次 strong release 的调用线程上直接调用对象 deleting
  dtor，没有线程转发或 owner-thread dispatch；静态证据未证明所有可能 final release 均发生在允许线程/
  main-loop phase。因此 destruction-thread contract 未闭合。
- 同时未见 `uint16` strong/weak 的 overflow/underflow guard；独立 virtual 方案必须约束计数上限与
  acquire/release 配对，并与 exception rollback、callback-retained refs、reader drain 一起证明。
- 所以“机械 refcount ABI 已闭合”不等于 decision item 完成；该 item 要求的 last-release thread 与
  failure/rollback 全称性质仍是 `UNKNOWN`。

## 结论与安全边界

- 三项都不能 promotion 为 `STATIC_CONFIRMED`；decision 中应继续列为 `UNKNOWN`。
- `necessityAssessment` 仍是 `REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`；
  `formalDecision = null`；`workPackageStatus = BLOCKED_BY_EVIDENCE`。
- 本交接不改变既有 decision、验证器或权限；`csT00506Unlocked=false`，所有 runtime/active/mutation/
  process-access/game-launch/injection/IPC/virtual-device-create 权限保持 `false`。
- 没有实现授权，也没有 passive/active canary 授权。
