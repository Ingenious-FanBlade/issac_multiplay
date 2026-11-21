# tx-03-11 Luna：render 子树与 EntityList 绘制虚调用

日期：2026-10-22。范围：对 tx-03-10 的更新/展示分派分析作有限静态跟进，重点复核 `0x80EA80`、`0x80F200`、`0x832850`、EntityList 绘制 dispatch 及条件 Lua 路径。本文只记录本子智能体的新增证据；主智能体会对本轮文档与相关证据作一次整体静态审阅。

## 结果

保留原版 render 会进入一条条件 Lua callback 路径：render child `sub_6FBC10` 于 VA `0x6FC0A6`（RVA `0x2FC0A6`）调用 `sub_863680`；已有 tx-03-10 静态反编译确认其条件分支在 `0x8636DB` 进入 `sub_872980`，后者用 registry reference 取得 Lua 函数并于 `0x8729CB` 调 `lua_pcallk`。这是“render 可能执行由 registry 引用选出的 Lua callback”的静态确证。callback 身份、运行时条件是否满足、callback 是否改写 gameplay/world，以及它能否被称作不受限的“任意 Lua”均为 **UNKNOWN**；不能从 call 到 `lua_pcallk` 推出其世界副作用。

EntityList 绘制环中的 Entity vtable byte offset `+0x18` 已追到一个确切基类实现：`??_7Entity@IsaacRepentancePlus@@6B@` 表位于 VA `0xB6730C` / RVA `0x76730C`，表项 VA `0xB67324` 编码目标 VA `0x6B2000` / RVA `0x2B2000`。在此实现中，直接可见的是按 Entity 坐标计算屏幕坐标、读取 Entity 的 Sprite 相关字段、复制/恢复 Sprite 状态字段、短暂置位资源对象 byte `+0x74`、调用 `sub_40A140` 绘制并清除该 byte。此函数体没有直接写 Entity `+0x33C/+0x340`、HP、Room 或 Grid 字段，也没有直接 Lua call；但资源/graphics callees 和派生 vtable override 未闭包，因此这不是全函数树“纯展示”的证明。

render 路径中仍有未解析的间接调用。特别是 `sub_80F200` 内有多组对象 vtable `+0x0C` 调用、至少一个对象 `+0x14` 调用，以及其他资源对象的 `+0x14/+0x18/+0x1C` 和寄存器间接 call。静态代码没有给出全部接收者的动态类型与目标，不能根据偏移本身推断它们是展示或 gameplay。扫描到的 `sub_80F200`、`sub_832850` 直接 call 目标中没有 Lua/pcall 命名函数；这不能排除未解析的动态目标或它们的深层调用。

## IDB 与方法

只读 IDA MCP 针对活动 IDB 查询函数、原始指令、bytes、表项与 xrefs。IDB 路径为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，input path 为同目录 `isaac-ng.exe`，imagebase `0x400000`。路径和 imagebase 与 tx-03-10 记录一致；本轮没有验证磁盘输入文件与 IDB 映射完全相同。地址转换为 `RVA = VA - 0x400000`。数据表地址是 IDA 静态 VA，不可直接视为 ASLR 下的运行时地址。

未运行游戏、测试、probe、构建或实验；未使用 debugger；未调用 `survey_binary`，未计算或验证哈希；未 rename、patch、set_type、保存 IDB。原始 MCP 返回中保留了一次只读名称扫描脚本的 NameError，后续修正查询成功；失败输出仍在证据 bundle 中。

可重复只读导出脚本为 [luna-export-tx0311.py](../../../artifacts/cs-t0/tx-03-11-static/luna-export-tx0311.py)。子智能体初稿建议将含函数定义的脚本直接提交给 `py_eval`，但该工具有 globals／locals 分离限制。主智能体静态审阅后将入口修正为 `py_exec_file` 的单一 namespace，使用 create-new 写证据 JSON，不改 IDB；指定 `TX0311_LUNA_OUTPUT` 可选择新的输出路径。原始分次 MCP 返回和 NameError 仍保留。

## render child 与三个关注函数

`sub_6FBC10`（VA `0x6FBC10` / RVA `0x2FBC10`）的已记录直接控制流为：

| call site VA / RVA | 原始 bytes | ECX 与参数 | 目标 / 直接含义 |
|---|---|---|---|
| `0x6FBC71 / 0x2FBC71` | 见 tx-03-10 `primary-followup-evidence.json` | ECX 为 Room | `sub_80EA80` Room 绘制 helper |
| `0x6FC094 / 0x2FC094` | `E8 67 31 11 00` | ECX = `*(Game+0x18300)`；当前窗口没有显式 stack push。IDA 对 callee 的恢复 ABI 含 `EBP` 参数，具体稳定语义未确认 | `sub_80F200` |
| `0x6FC0A1 / 0x2FC0A1` | `E8 AA 67 13 00` | ECX = `Game+0x1B83C` | `sub_832850` 状态/转场展示分支 |
| `0x6FC0A6 / 0x2FC0A6` | `E8 D5 75 16 00` | 继前分支之后的直接调用 | `sub_863680` 条件 Lua dispatcher |

IDA code xrefs 显示 `sub_80EA80` 还有 `sub_830950`、`sub_8318A0` 的调用；`sub_80F200` 也有来自这两个函数的 code xref。tx-03-10 将其定位在状态/FSM 路径。这证明这些 helper 被 renderer 与更新/FSM 路径复用，函数地址本身不是阶段边界；调用方向不能反推为“render 会调用 update owner”。本轮没有证据证明保留 `sub_6FBC10` 会直接调到 `sub_6FADC0`、`sub_6FD3F0` 或其它已知模拟 owner。

### `sub_80EA80` → EntityList draw dispatch

VA `0x80EAE6–0x80EAF3` 的指令为：

| VA | bytes | 指令 |
|---|---|---|
| `0x80EAE6` | `8D 87 04 12 00 00` | `lea eax, [edi+0x1204]` |
| `0x80EAEC` | `50` | `push eax` |
| `0x80EAED` | `8D 8F 18 12 00 00` | `lea ecx, [edi+0x1218]` |
| `0x80EAF3` | `E8 58 B0 C0 FF` | `call sub_419B50` |

按当前 Room 结构推导，`ECX = Room+0x1218`（EntityList），栈上传入 `Room+0x1204` 坐标结构地址。IDA 函数反编译显示 `sub_419B50(this, float *a2)` 遍历 EntityList 的当前对象指针；通过若干可见/类型条件后，对当前 Entity 的 `vptr+0x18` 发出方法调用，再用该返回 byte 决定是否继续后续显示步骤。具体调用机器码：

`sub_80EA80` 自身也有直接 store：反编译中先有 `j = i`（`i` 是 Room receiver），随后写 `j[7095]`、`j[7096]`、`j+7098`、`((_QWORD *)j)+3553`、`j+7102`、`j[7108]` 以及 byte `j+4598`；也写全局 `xmmword_C79900`。这些是直接写入事实，但字段语义未知，不能据此命名为 gameplay 更新或纯显示状态。对应过滤后的反编译赋值在 [luna-direct-write-excerpts.json](../../../artifacts/cs-t0/tx-03-11-static/luna-direct-write-excerpts.json)；该筛选 bundle 没有保留每条伪代码语句的独立指令 VA，因此不把伪代码序号冒充机器指令地址。

| VA | bytes | 指令 / 状态 |
|---|---|---|
| `0x419C61` | `8B 01` | 从当前 `ECX=Entity` 读 vptr |
| `0x419C63` | `56` | 将 `ESI`（反编译为传入 `a2` 的坐标 float*）压栈 |
| `0x419C64` | `8B 40 18` | 加载 Entity vtable byte offset `+0x18` |
| `0x419C67` | `FF D0` | `call eax`；`ECX` 保持为当前 Entity，返回值在 `AL`，其后 `test al,al` |

基类表的原始字节见 [luna-entity-render-vslot-mcp.json](../../../artifacts/cs-t0/tx-03-11-static/luna-entity-render-vslot-mcp.json)；对应 entry bytes `00 20 6B 00` 解为 IDA 静态 VA `0x006B2000`。表名来自 IDB 的 RTTI 符号 `??_7Entity@IsaacRepentancePlus@@6B@`。该槽在派生实体上可能被覆盖；本轮只确认基类项 `Entity+0x18 → sub_6B2000`，不外推到每一种 Entity 或每一层主绘制。

`sub_6B2000` 的 IDA 原型为 `char __thiscall(this, float *a2)`，尾部为 `retn 4`：ECX=this，1 个 stack float*。其直接代码读取 Entity 的 `+0x33C/+0x340` 与 `+0x160/+0x164` 参与坐标计算；通过 `sub_40B220` 查出 Sprite/资源对象；暂时设置返回对象 `+0x74=1`；在 Sprite 状态对象与临时状态之间复制多个字节；调用 `sub_40A140`（ECX=`Entity+0x48`，stack 中含 Sprite id 与计算坐标指针）；之后恢复保存的 Sprite 字段、将 `+0x74` 复位为 0 并返回成功。直接函数体中未观察到对 Entity position、HP、Room 或 Grid 的 store；`sub_40B220`、`sub_407170`、`sub_40A140` 及动态派生方法的完整副作用仍 **UNKNOWN**。因此“该基类表项的直接写入是 Sprite/render-local”是有限推导；“整个调用树纯展示”不是本轮结论。

### `sub_80F200`：Room 对象列表和间接槽

`sub_80F200` 入口在当前 IDB 为 VA `0x80F200` / RVA `0x40F200`，函数大小约 `0x36EE`，共 2,798 条 IDA instruction item。render owner 在 `0x6FC08E` 将 `*(Game+0x18300)` 装入 ECX，并在 `0x6FC094` 调入它。IDA 恢复为非标准 `__usercall`（ECX 加 EBP 位置参数）；调用窗口没有显示显式 push，不能将恢复类型当完整 ABI 契约。

只读指令扫描显示多组基于 Room/对象数组的循环和动态调用。例如：

- VA `0x80FD69`、`0x80FE61`、`0x81001E`、`0x8100B5` 的原始 bytes 为 `FF 50 0C`，调用当时 `ECX` 是数组取出的对象；其中个别分支先检查对象 `+4` 的原始值。方法目标取决于该对象的实际 vptr。
- VA `0x8104F7` bytes `FF 50 0C`：前指令将 `EDX` 装入 `ECX` 作为当前对象，并压入 `Room+0x1204`；该分支先比较对象 `+4 == 0x10`。这里记录原始条件，不命名对象类别。
- VA `0x810D0D` bytes `FF 50 14`：从 `Game+0x1BB78` 指向的额外对象 vector 取对象作为 ECX，栈上传入 EDI 后调用 byte offset `+0x14`。此处无足够证据确定 vector 对象类型、具体 override 或其返回/世界副作用。
- 另有 `+0x14` 间接调用使用 ECX=`dword_C7984C`、`dword_C797E4`、`dword_C797D4`、`dword_C797FC`、`dword_C79854` 等全局接口指针；这些不是由当前代码证明的 Entity receiver。还可见 `+0x18`、`+0x1C` 与 `call esi` 等其它间接分派。
- 该函数的直接 call 指令列表里没有 IDA 名称包含 `lua`/`pcall` 的目标；调用仍包括资源/动画辅助函数以及未解析的虚调用，因此不能以此排除深层 Lua 或 gameplay side effect。

该函数的反编译赋值筛选还显示直接写全局 `dword_C79A4C`、`byte_C7977F` 与 `xmmword_C79900`；重复向后者装入不同 SIMD/local 值。指针相对写入包括 `i_1+0x7144`、`i_1+0x12B8/+0x12BC`、`i_4+0x6FF8` 起的一组字段及其它局部地址。这里 `i_1` 初值来自 ECX，但函数中会重绑到 `i_2`；仅凭反编译变量名无法判定这些相对写入的稳定对象归属和语义，均不标记为 gameplay 或 presentation 字段。候选赋值列表见 [luna-direct-write-excerpts.json](../../../artifacts/cs-t0/tx-03-11-static/luna-direct-write-excerpts.json)，列表含 SIMD 临时量等伪赋值，按候选而非完整写集使用。

调用上下文、bytes、参数寄存器与完整扫描结果见 [luna-call-targets-80f200-832850.json](../../../artifacts/cs-t0/tx-03-11-static/luna-call-targets-80f200-832850.json)、[luna-indirect-dispatch-contexts-mcp.json](../../../artifacts/cs-t0/tx-03-11-static/luna-indirect-dispatch-contexts-mcp.json) 与 [luna-render-indirect-contexts.json](../../../artifacts/cs-t0/tx-03-11-static/luna-render-indirect-contexts.json)。在 `80F200` 内还直接调用 `sub_4199D0`（VA `0x80FD0C`）及其它绘制/渲染 helper；该子树不在本轮全量闭包范围。

### `sub_832850`：状态/转场绘制对象

`sub_832850` 仅在 `sub_6FBC10` 的当前 code xref `0x6FC0A1` 被找到。调用时 ECX=`Game+0x1B83C`。直接代码只接受对象首 dword 状态值 2 或 3；读取转场/图层参数，构造变换与混色局部结构，按状态分支将若干 per-item 数据写入由 `sub_833010` 返回的记录；也直接写全局 render configuration `xmmword_C79900`。若进入绘制分支，会以 ECX=从对象 `+0x28` 或 `+0x30` 取出的资源对象调用 vtable byte offset `+0x1C`（VA `0x832E50`、`0x832E65`、`0x832EDF`），stack 参数是本地 112-byte 描述结构和变换/颜色数据。基于接收者来源和字段数据流，这些是 render/transition resource 对象；其动态目标与深层行为未闭合，不能仅凭状态机字段或槽偏移断言不存在 gameplay 副作用。

该函数的直接 call inventory 没有 Lua/pcall 命名目标。与 `80F200` 一样，实际 callsite 是间接 `[vtable+0x1C]`；无证据把它等同于 Entity 方法或 Lua callback。`xmmword_C79900` 和 `sub_833010` 返回记录中的字段赋值是直接写入；记录对象字段语义及动态 `+0x1C` 目标仍 **UNKNOWN**。所以目前只确认 render child 上那一条条件 Lua 路径，不能证明 `+0x14/+0x1C` 派生 render dispatch 不会再进入 Lua。

## 未决项和结论边界

- render child 对 Lua 的条件调用是直接证据；回调 registry 引用由谁设置、实机条件是否成立、回调是否执行 Entity/Room/Grid/输入或任意 gameplay 写入均 **UNKNOWN**。
- Entity 基类 `+0x18` 目标的直接 Sprite/坐标行为已观察；全部派生 Entity vtable override、全部实际动态 receiver 以及资源 helper 的 transitive effects 未闭合。
- `80F200` 多个 `+0x0C/+0x14/+0x18/+0x1C` 槽和寄存器间接目标未完全映射；特别是 `Game+0x1BB78` vector 的动态类型未知。静态扫描没有直接 Lua 命名 target，不等于深层调用不存在 Lua。
- `832850` 的 render/transition resource `+0x1C` dispatch 只闭合到接收对象来源、参数和本地状态读写；目标与 transitive effects **UNKNOWN**。
- 旧 tx-03-10 已给出同一个 `sub_80EA80/sub_80F200` 被更新/FSM 函数调用的 code xrefs。该复用说明 helper 地址不是独立的 phase proof；它不能单独证明 render owner 会调用 gameplay update owner，也不能证明全进程没有其它模拟入口。
- 本文不是整个 renderer、Replica side-effect 或原版 Online/Isaac Sync 隔离闭包。本轮没有改变任何源码或运行状态。

