# tx-03-17 Luna：Sprite 正常 owner 释放与失败退休路径

日期：2026-10-23（Asia/Shanghai）。本报告负责 tx-03-17 中 Reset、析构、Lua GC、ANM 引用释放、部分资源准备失败后的可清理状态，以及可见的 native owner 调用顺序。分析范围是完整构造后、主线程正常返回的静态路径；不扩展到全引擎 owner 闭包。

## 结论

`Sprite::Reset` `0x407F10` 是可复用的资源／展示状态重置操作，不只是动画帧复位。它按当前 filename 查找并减少一次 ANM cache 引用，重置两个动画 state，销毁并释放实例 layer 数组，清空 ANM 借用字段和字符串，并恢复部分实例展示字段。它不析构 Sprite 的原生字符串对象，也不释放分配 Sprite 的 wrapper/raw allocation。

`0x408170` 是独立 Sprite 的原生析构函数：先 Reset，再析构两个 animation state 和两个原生字符串；它本身不释放 raw object。Lua `Sprite()` 实例的 GC 路径也不调用 `0x408170`。Sprite Lua metatable 的 `__gc` closure `0x8C31F0` 会走 LuaBridge `UserdataValue<ANM2>` vtable slot 0 到 `0x8C4CE0`。该 wrapper 析构函数对 payload 执行等价的子对象清理，GC 调用时 deleting flag 为 0，因此 wrapper 内不调用 allocation-release helper；之后由 Lua allocator 回收这块 userdata。

已完整构造且尚未发布的候选 Sprite，在一次正常返回的 `Load`／`LoadGraphics` 或姿态 apply 失败后，可以按有界设计路径先退出展示 owner，再在已知的主线程／renderer resource context 调一次 Reset，释放本地 layer、图像控制关联和路径，并清掉借用的 ANM 指针。随后移除最后的 Lua 引用，等待 Lua GC 调 wrapper finalizer。这个路径不证明部分 C++ 构造、OOM、EH、异步图形任务或缓存拓扑变化时也能安全恢复；如果经过 `Reload`，一个 Reset 也不能抵消多次额外 ANM 引用。

## 证据与地址

当前 IDB 的 input path 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，imagebase=`0x400000`，`RVA = VA - 0x400000`。字段偏移按对象基址计算，不是 RVA。这里仅读 IDB 的反汇编、伪代码、raw bytes 和静态 xrefs，没有执行 native helper、游戏、构建、测试或调试器，也没有修改／保存 IDB 或校验哈希。

完整函数 raw bytes、指令、伪代码、VA/RVA、局部 xrefs 和关键注册点上下文见 [Luna 静态导出](../../artifacts/cs-t0/tx-03-17-static/luna-retirement-static.json)；只读 create-new 导出脚本见 [luna-export-retirement.py](../../artifacts/cs-t0/tx-03-17-static/luna-export-retirement.py)。

| 角色 | VA / RVA | 本轮确认 |
| --- | --- | --- |
| Sprite payload constructor | `0x4074C0 / 0x74C0` | 初始化大小为 `0x114` 的 Sprite payload，随后调用 Reset |
| Reset | `0x407F10 / 0x7F10` | 减引用、清资源／动画／展示字段；保留可复用对象壳 |
| 独立 Sprite native dtor | `0x408170 / 0x8170` | Reset、两个 state dtor、两个 string dtor；不 free raw object |
| animation state dtor | `0x4087A0 / 0x87A0` | 释放 state 的两组索引数组并清指针 |
| Sprite layer dtor | `0x407A60 / 0x7A60` | 按 image control 与全局 callback 条件释放关联，再析构路径 string |
| `Load` / `LoadGraphics` | `0x40BD50 / 0xBD50`; `0x40C000 / 0xC000` | ANM 绑定、引用和逐层 graphics 准备；具体失败态沿用 tx-03-15 |
| ANM ref increment / decrement / sweep | `0x40E110 / 0xE110`; `0x40E2B0 / 0xE2B0`; `0x40E470 / 0xE470` | 按 cache key 查找；sweep 只清零引用节点 |
| Sprite metatable setup | `0x8A71B0 / 0x4A71B0` | 安装 Sprite `__gc` closure `0x8C31F0` |
| Lua `Sprite()` userdata constructor | `0x8BB720 / 0x4BB720` | 分配 userdata、设 wrapper/metatable，再构造 native payload |
| Lua `__gc` closure / extractor | `0x8C31F0 / 0x4C31F0`; `0x85C3E0 / 0x45C3E0` | 取出 Sprite userdata wrapper 后调用其 vtable slot 0 |
| LuaBridge `UserdataValue<ANM2>` vtable slot 0 | vtable `0xB73228 / 0x773228` → dtor `0x8C4CE0 / 0x4C4CE0` | deleting flag 控制是否调用 `sub_AEF15C` allocation-release helper；Lua GC 传 0 |

## Lua userdata 的构造与 GC ABI

注册入口 `0x866960` 在 `0x867218` 左右以 `sub_8A71B0("Sprite", ...)` 建立 Lua Sprite 类型。该 helper 注册的 `__gc` closure 为 `0x8C31F0`，并将供 userdata 设置 metatable 的 registry key `C8298F` 指向相关 Sprite metatable。

`0x8BB720` 的静态顺序是：

1. `lua_newuserdata(L, 284)` 分配 `0x11C` bytes。
2. userdata 起始处写 `UserdataValue<ANM2>` vptr `0xB73228`；wrapper `+4` 写入 payload 指针 `wrapper+8`。
3. `lua_rawgetp(L, LUA_REGISTRYINDEX, &C8298F)` 并 `lua_setmetatable(L, -2)`。
4. 最后才调用 `0x4074C0(wrapper+8)` 构造 `0x114` bytes Sprite payload，返回一个 Lua 值。

GC handler `0x8C31F0` 调 `0x85C3E0(&C8298F)` 取 wrapper，然后执行 `(**v0)(v0, 0)`。`0xB73228` 的 slot 0 是 `0x8C4CE0`。该 destructor 的主要顺序为：

```text
this->vptr = UserdataValue<ANM2>::vftable
Reset(this + 8)                       // this + 8 是 Sprite payload
state destructor(this + 88)           // payload + 0x50
state destructor(this + 56)           // payload + 0x30
string destructor(this + 32)          // payload + 0x18
string destructor(this + 8)           // payload + 0x00
this->vptr = Userdata::vftable
if (a2 & 1) sub_AEF15C(this)            // allocation-release helper
```

GC handler 明确传 `a2=0`，所以这一路不会在 wrapper dtor 内调用 `sub_AEF15C`；该 helper 经 `0xA0F500 → 0xA648B0` 进入原生分配器封装，主审补充的 raw 见 [primary-allocation-wrapper.json](../../artifacts/cs-t0/tx-03-17-static/primary-allocation-wrapper.json)。userdata 原始块由 Lua 的 userdata allocator/GC 生命周期负责。这里闭合的是 Sprite() 返回的 owned wrapper；其它来源包装需按其实际 vtable 另解释。`0x8C4CE0` 没有调用 `0x408170`，而是对 wrapper 内的 payload 字段直接执行相应子对象析构。不要对 `lua_newuserdata` 返回块再手动 `delete` 或手动 free。

`0x408170` 是另一种 owner 语境下的 Sprite native dtor：`Reset(this)`、state dtor `this+0x50`、state dtor `this+0x30`、string dtor `this+0x18`、string dtor `this+0x00`，然后返回。该函数不执行分配器释放；raw native Sprite 的 allocation owner 与匹配释放点不在本轮闭合。IDA 当前静态图中还能看到多处 tail `jmp 0x408170` destructor/unwind thunks，但它们没有证明这些调用对象由何处分配或何处释放。

metatable 在 userdata payload 构造前安装，所以 `0x4074C0` 若未能正常完成时，是否以及何时会对部分初始化 payload 调 `__gc` 没有静态保证。当前正常析构结论只适用于 ctor 已正常返回的对象。

## Reset、析构和资源 owner 的分工

### Reset 的可见操作

`0x407F10` 按以下顺序清理：

1. 若 Sprite filename string 长度非零，调用 `0x40E2B0` 按当前 filename 查找并减少一次 cache 引用。
2. 清 filename 的长度与首字符；以空 descriptor 重置 main/overlay 两个 animation state；清 default animation string。
3. 若 Sprite layer array 非空，依据其 array header count 对 `0xA0` stride 的 layer 逐项调用 `0x407A60`，再释放 array allocation。
4. 清 Sprite layer array 指针/数量、ANM cache 的借用数组与数量，并恢复部分 Sprite 实例字段。

Reset 不析构 Sprite 的两个原生 string 对象，也不清 Sprite 的 wrapper/raw allocation；完整 native dtor 或 LuaBridge wrapper dtor 会再析构 string 和 animation state。Reset 完成后旧的 layer、descriptor 或动画表指针不能继续被绘制方使用。

部分 Reset 默认值直接来自进程全局：Sprite `+0x94/+0x98` 写 `n3_2` 与 `C7B644`；`+0x9C/+0xA0` 写 `C3793C` 与 `C37940`。这些是 Offset/Scale 的全局来源，当前值未知，不能称为常量零或一。其余本次确认的直接写入为：`+0xA4` rotation=0；Color `+0xA8..+0xB4` 四个 f32=1.0（DWORD 位模式 `0x3F800000`），`+0xB8..+0xD0` 七个 DWORD=0；Flip `+0x100` word=0；speed `+0x104`=1.0；`+0x108` word=0（其中 `+0x109` 是 IsLoaded byte）；draw order `+0x70`=0；shadow index `+0x10C`=-1；special flags `+0x110`=0。全局取值不由该静态分析推成游戏运行时常量。

### Layer、ANM cache 与图像控制

`0x407A60` 处理一个 Sprite 实例 layer：layer+`0x98` 是 image，layer+`0x9C` 才是 control。control 非空时先调用其虚槽 `+0x0C` 做自身减引用；结果为真且全局 `0xC7163C` callback 非空时，把该 layer 的 image/control pair 另传给 callback；随后析构 layer+`0x08` 路径 string。该 dtor 不减少 Sprite 的 ANM cache 引用。control 的正常六槽和 optional callback 已由 [tx-03-13](tx-03-13-sprite-image-resources.md)解释；本轮主审另导出 [primary-control-retirement.json](../../artifacts/cs-t0/tx-03-17-static/primary-control-retirement.json)重读。自身减引用不以 callback 存在为前提；最后 strong 正常释放可先在 control 内调用 image 析构。实际回调线程、在途／GPU 与异常寿命仍未闭合。

ANM 引用是独立层：`0x40E110` 对可查到的 cache node+`0x50` 加一，`0x40E2B0` 对查到的节点减一，`0x40E470` 只清理引用为零的节点，并调用 `0x40D850` 释放其共享 payload 后摘除／释放节点。Reset 只做一次 decrement，不触发 sweep；静态可见 sweep 调用点在 `0x68FD57`、`0x8F8C6F`、`0x9B248F`。不能把一个 Sprite Reset、layer image release 和 ANM cache sweep 合并成同一“释放 Sprite 资源”。

Reset 的 ANM decrement 是按当前 filename 重新查 cache key，没有与该 Sprite 绑定的 reference token。正常、非重入、同一主线程的稳定 cache 窗口内，单次成功 Load 后 Reset 可对应其当前路径引用；若 failure 和 Reset 之间另一条路径创建/替换了同 key cache node，按 filename 扣减可能改变别的 owner 的计数。该时序在此仅列为未知边界，不能声称跨并发/重入安全。

## 部分资源准备失败后的可清理状态

下述是有前提的静态推导，不是所有 Load/OOM/EH 状态都可恢复的保证。

**已构造的干净候选 + 普通 LoadGraphics 失败返回。** `LoadGraphics 0x40C000` 逐层处理，资源 resolver 可正常返回空 pair；调用路径仍会更新该层路径与 pair/status，且不保证前层成功状态回滚。完整构造的候选若尚未发布到 draw owner，可先退出发布状态，再在正确 graphics/context owner 调 Reset：它按本地 layer array 逐项析构 control/path，随后清 ANM 借用指针和本地 layer array。已成功取得的一次 ANM ref 按当前 filename 减一次；cache node 由稍后的零引用 sweep 才回收。IsLoaded `0x420160` 只读 Sprite+`0x109`，而 Load 会设置该位且不据资源 resolver 返回值统一失败；所以它不能作为“所有资源已准备成功”的证明。

**tx-03-15 的条件性 `Load(F) → Load(G)` 失败分支。** 在旧 F 有效、该 Sprite 是 F 最后一个引用、G 不同且找不到节点、资源 open 失败、查找拓扑稳定且正常返回的限定条件下，Load 先按 F 减 ref，再把 filename 改成 G；G 绑定失败时保留一部分旧 layer／ANM 借用指针，随后仍清动画 state 并置 `+0x109=1`。若旧 F 后续零引用 sweep，旧借用数组会失效，但调用 Reset 的可见路径按当前 filename 查 G、重置 state，销毁本地 layer array，并把旧 borrowed fields 清零；它不遍历被借用的旧 F descriptor/track 表。若 G 不存在，decrement helper 会记录找不到 G 的警告。

这只说明该限定分支上 Reset 有一条不依赖访问旧 ANM 数组的本地清理路径，不证明立即 render、所有失效关系、worker/GPU callback 或 EH 状态安全。由于 decrement 按当前名字重新查找，若 G cache 节点在失败与 Reset 之间因重入/并发出现，Reset 可能扣到那个节点；要求的正常方案因此限定同一主线程、无资源调用重入、cache topology 不变的 bounded window，越出该条件即 UNKNOWN。

**Reload 的额外 ref。** 正常 `Reload 0x40BEA0` 在 filename 非空时重新绑定并调用 `0x40E110`，但不先释放旧 ref；tx-03-15 记录的稳定节点静态推导为 k 次 Reload 多 k 次 ref，之后一次 Reset 只减一次。失败候选如执行过多次 Reload，仍可清本地 arrays 和图像控制，但不应声称 ANM cache ref 已配平；后续实现应避免用 Reload 做资源阶段的循环重试。

## bounded 正常 owner 退休顺序候选

这是对有界正常 owner 的设计建议，不是运行时已验证行为：

1. 在 Lua/game 主线程、已建立 graphics/resource context 的正常阶段创建一个全新 Sprite userdata，等待 `0x4074C0` 正常返回。候选保持未发布，避免它被 renderer 或其它调用方并发借用。
2. 对候选做一次受支持的 `Load`/资源准备和姿态 apply。该阶段任何失败或不支持的结果都不把候选挂入展示 owner；不以 `IsLoaded` 单独判成功，也不对同一对象循环 Reload。
3. 失败时先撤下候选可能已经登记的本地展示引用，然后在同一已知 owner/context 对该完整构造的候选调用一次 Reset，清理本地 layer/image control/path 和当前 cache-key 引用。若失败发生在 Lua userdata ctor 尚未正常返回之前，停在 UNKNOWN，不能使用此方案。
4. 成功时只有完整 apply 后才让一个 owner 发布候选指针。替换时先完成 owner 的新指针切换，再对不再被任何 renderer/client 借用的旧对象 Reset。
5. Reset 后 userdata wrapper 仍由 Lua 持有。取消项目／registry 对它的引用，让 Lua GC 之后调用 `0x8C31F0 → 0x8C4CE0(a2=0)`；不手动 free Lua userdata。显式 Reset 控制资源状态退休，GC 的时间仍由 Lua 决定。

在这个顺序中，Reset 的 layer/control 自身减引用与条件性的 `0xC7163C` 后置 callback 分开；可进入图形资源释放，需处于已理解的 context owner。tx-03-14 提供调度约束，但没有证明所有 driver/worker 异常。原生 cache sweep 的调用时机也仍是另一 owner 路径。若显式 Reset 已正常清空本对象资源、之后未再加载或使用，后续 GC 再 Reset 的空路径不再减少该对象原 ANM 引用或逐层释放原 image；不能据此外推任意非空 userdata 的 GC 都与 context 无关。

## 主审修订记录

初步消息曾因 xref 读取方式而称 `408170` 没有直接代码引用，后续 raw 已发现 tail jmp／unwind stubs；撤回该否定，不用它证明 owner。默认值消息中的 Color 零值尾部范围由 B8..C8 修正为 B8..D0。主审还将终稿的 layer+98 control 改为 +9C、四个 DWORD=1 改为 f32=1.0，并区分 control 自身减引用和 optional callback，避免覆盖 tx-03-13 的既有正常路径结论。原始导出与历史消息保留。

## 未知与本轮边界

- constructor 分配/初始化过程中抛出、部分构造 payload 上运行 `__gc` 的行为未知；不把正常析构顺序外推到部分构造对象。
- `Load`、layer array 分配、图像 control 创建的 OOM/原生 EH 与回滚闭包未知；`LoadGraphics` 多层操作不是事务。
- ImageBase control 正常实现沿用 tx-03-13；全局 release callback 的实际线程、GL/context guard 和 GPU worker 最终退休仍未闭合。
- ANM 引用按 key 再查而不是对象 token；并发/重入改变 cache 拓扑时不能保证引用归属。
- 单次 Reset 对多次 Reload 不配平额外 ANM ref。
- Lua finalizer 负责的是 Lua-owned userdata wrapper；不能用 `0x408170` 代替它，也不能从任意 tail thunk 推出 Sprite raw allocation 的 allocator/free owner。
- 没有对资源成功率、图像、渲染结果、heap 生命周期或失败恢复做 runtime 观察。

## 相关研究

- [tx-03-12：derived render 与 Sprite 生命周期](tx-03-12-derived-render-sprite-lifecycle.md)记录构造、Reset、native dtor、cache 及字段基础证据。
- [tx-03-15：Sprite replacement、Reload 与 ANM 引用](tx-03-15-sprite-replacement-reload.md)记录重复 Reload 和条件性失败借用指针状态。
- [tx-03-16：独立 Sprite 展示字段](tx-03-16-independent-sprite-display-fields.md)限制固定姿态、特殊分支及最小支持集合。
- [tx-03-14：renderer context 与生命周期](tx-03-14-renderer-context-lifecycle.md)说明本地 graphics/context 调度背景。

本轮只完成受限的静态研究与报告；tx-03-17 整体静态文档审阅由主智能体在目标完成后统一执行一次。
