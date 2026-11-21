> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device 自有 ABI 静态确认 v2（IDA-MCP）

日期：2026-10-12
推进编号：`tx-00-03`（按用户会话语义执行 [`virtual-device-owned-abi-static-confirmation-next-work-design-v1.md`](virtual-device-owned-abi-static-confirmation-next-work-design-v1.md) 的 W01–W10）
状态：静态确认主体完成（W01–W10 已写入；补充子任务证据见主题文件索引；整体静态审阅结果见 §13）
证据索引：[`out/cs-t0/virtual-device-owned-abi-static-evidence-v2.md`](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2.md)

本文先读[现行项目规范](../../../../../project-conventions.md)。不设 gate、不校验哈希、不以旧 decision/解锁字段/矩阵完成率作为推进前置；逆向、实现与最终审阅可由同一智能体完成。本轮范围仅为资料阅读、只读 IDB 分析、只读磁盘 PE 解析与文档编辑，不运行游戏、调试器、注入、Hook、IPC、构建、harness、probe、smoke、canary 或验证脚本。

证据标签使用：`IDA_STATIC_CONFIRMED`（本轮 IDA 直接读指令/数据）、`STATIC_CONFIRMED`（其它只读静态工具）、`STATIC_DERIVED`、`DESIGN_ONLY`、`UNKNOWN`。v1 报告与既有交接保留其原始 `RUNTIME_OBSERVED` 语境，本轮不新增运行期观察。旧哈希、gate、独立角色与多组验收流程只作历史资料，不执行。

---

## 1. 执行范围与实际目标

本轮目标：把[静态确认 v1](virtual-device-owned-abi-static-confirmation-v1.md)补充为可供实现使用的、有明确适用条件的原生接口说明，覆盖[设计修订稿 v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md)全部 16 项 O-* 义务，并重点纠正 Init 失败处理、F7 唯一性、query 语义与继承槽安全性方面的不足。

本轮明确不做：

- 不运行游戏、不附加调试器、不读取进程内存、不注入、不 Hook、不 IPC、不做故障注入；
- 不运行旧 builder/verifier、不自测、不做 mutation/replay/PASS-FAIL 判定、不校验任何哈希；
- 不修改 IDB 的字节、名称、类型、函数定义，不保存数据库；`rename`/`set_type`/`define_func`/`patch` 等写工具未使用；
- 不把静态结论外推为“已实现”“已实机验证”或“全部迟到访问安全”。

完成本文表示所有 W01–W10 问题已被分析并如实交代；允许存在带原因的 `UNKNOWN`。

## 2. 实际分析对象与地址空间（W01）

### 2.1 目标

| 项 | 本轮读取值 | 读取方式 |
| --- | --- | --- |
| 磁盘 PE | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `ida_nalt.get_input_file_path()`、磁盘 `Get-Item` |
| 文件大小 / 修改时间 | 9,176,064 bytes / 2026-08-22 18:00:14 (+08:00) | 磁盘元数据；无 VS 版本资源 |
| IDB | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64` | `ida_loader.get_path(PATH_TYPE_IDB)` |
| PE Machine / ImageBase / SizeOfImage / EntryPoint RVA | `0x014C` / `0x400000` / `0x8FE000` / `0x6EFC46` | 磁盘 PE 头直接解析 |
| IDB imagebase | `0x400000` | `ida_nalt.get_imagebase()` |
| IDB 段 | `.text 0x401000-0xB18000 (rx)`、`.idata 0xB18000-0xB18A28`、`.rdata 0xB18A28-0xBF8000`、`.data 0xBF8000-0xC9D000 (rw)` | `idautils.Segments()` |

说明：`ida_ida.inf_get_baseaddr()` 在本 IDB 返回 `0x0`，与 PE ImageBase/段起点不一致；本文地址换算采用 **IDB VA = `0x400000` + RVA**，与 `ida_nalt.get_imagebase()` 和段布局一致。该差异仅记录。本轮不计算哈希、不进行哈希一致性判断。

### 2.2 地址约定与纠错

- 设计稿 §4.2 的旧数值（如 `0x0061E050`）是 RVA；对应 IDB VA 为 `0x00A1E050`。v1 报告 §0 把旧数值称为“已带 imagebase 的绝对地址”的措辞不再使用；v1 表内 `0xA1E...` 等已是换算后的 IDB VA。
- 设计修订稿附录 A 的四张表数值是 RVA。例如 Net slot12 设计稿 `00501480` 对应 IDB VA `0x901480`；Keyboard slot0 `0066C390` 对应 `0xA6C390`。本轮直接读取了四张 vtable 的真实 IDB VA 内容（§5.1、附录 A）。
- `0xA112C0`（旧稿未标地址种类的种子）本轮定位为函数 `sub_A112C0`，大小 `0x1E2`，是**日志格式化函数**（`OutputDebugStringA` 与可选远程日志），全映像 code 引用 3286 条；它不是 query fallback 函数，而是 query 越界分支调用的日志 callee：`sub_A112C0(16, "ActionId is out of range\n")`。设计稿 O-ABI-QUERY 中的“fallback（0xA112C0 与元素函数指针）”应据此精确化为“越界日志 + 继续读取 binding 元素函数指针”。

### 2.3 种子函数复核

| 对象 | IDB VA | 符号 | 大小 | 证据 |
| --- | --- | --- | ---: | --- |
| Base ctor / Init / cleanup / deleting dtor | `0xA1E050 / 0xA1E210 / 0xA1E2C0 / 0xA1E1B0` | `sub_A1E*` | `0x15E / 0xA1 / 0x8F / 0x34` | IDA_STATIC_CONFIRMED |
| allocator / 下层分配释放 | `0xA0F4C0 / 0xA648B0` | `sub_A0F4C0 / sub_A648B0` | `0x1B / 0xB1` | IDA_STATIC_CONFIRMED |
| refcount ctor / dtor | `0x40CC10 / 0x40CB50` | `sub_40CC10 / sub_40CB50` | `0x88 / 0x58` | IDA_STATIC_CONFIRMED |
| TryAddStrong / AddStrong / ReleaseStrong | `0x40C6B0 / 0x40C690 / 0x4358B0` | 同名 | `0x34 / 0x1E / 0x58` | IDA_STATIC_CONFIRMED |
| AddWeak / ReleaseWeak | `0x40C610 / 0x40C5C0` | 同名 | `0x1E / 0x46` | IDA_STATIC_CONFIRMED |
| query 12/13/14 | `0xA1E8D0 / 0xA1E970 / 0xA1EA20` | 同名 | `0x97 / 0xA8 / 0xAC` | IDA_STATIC_CONFIRMED |
| per-device common / manager update | `0xA1F280 / 0xA1FC00` | 同名 | `0x49F / 0x419` | IDA_STATIC_CONFIRMED |
| keyed lookup / insert / remove | `0xA20390 / 0xA20DF0 / 0xA20EB0` | 同名 | `0xAD / 0xBB / 0xFD` | IDA_STATIC_CONFIRMED |
| vector erase / grow | `0xA21570 / 0xA21630` | 同名 | `0xB7 / 0x1C7` | IDA_STATIC_CONFIRMED |
| SetControllerId | `0x7A6450` | `sub_7A6450` | `0x229` | IDA_STATIC_CONFIRMED |
| manager update 已知 callsite | `0x954F7D` | 位于 `sub_954CD0`（`0x954CD0`，`0x8CC`） | — | IDA_STATIC_CONFIRMED |

### 2.4 v1 结论分类（原文保留；本表只做归类，便于后续引用）

| 类别 | v1 中的内容 | 本轮处理 |
| --- | --- | --- |
| 直接代码/数据 | §1.1–§1.3 ctor/Init/cleanup/dtor 与字段读取；§2.1–§2.3 control 布局、六槽、typed/generic vtable；§2.4 计数扫描；§3.1–§3.3 38 槽目标、调用点、common/manager 锁行为；§4.1 四张 COL/TD/CH 解析；§4.2 cast 站点；§5.1–§5.4 manager/insert/remove/notification 指令读；§5.6 worker 线程锚点；§5.7 SetControllerId | 已在本轮重读的关键项标 `IDA_STATIC_CONFIRMED`；未重读的标“继承原报告，未在本轮重读” |
| 静态推导 | “其余 32 槽无条件转发安全”的摘要；F7 复核“必要但不充分”；L1“强静态支持但非 points-to”；insert/remove 无共同事务；注册/绑定时序窗口 | 本轮对 32 槽改为条件分类（§5.4）；F7/L1 保留但降级为有限范围结论（§7/§6.4/§11.2） |
| 设计建议 | S2 六槽覆写集合；`NEVER`；自建 COL；路线 A/B；DLL pin；影子计数/饱和镜像 | 仍为 `DESIGN_ONLY`；§11.3/§11.4 给条件与修订建议 |
| 运行期未决 | S-1–S-11：raw 计数/内联锁/free、worker 并发、Net/teardown 线程、默认 binding 运行期值、`qword_C7B640` 运行期写入、Init 分配失败、自定义 COL 现场、typeid 比较、control 直接 delete、`+0x04` 未初始化 | 本轮逐项给出更精确范围或保留 `UNKNOWN`（§12） |

### 2.5 v1 纠错记录（保留 v1 原文，只在本节列修订）

1. 地址措辞：旧种子值是 RVA；IDB VA = RVA + `0x400000`。见 §2.1–§2.2。
2. Base ctor 顺序：`sub_A2C980(this+0xE8)` 在 `memset(this+0xE8,0,0x284)` **之前**执行（`0xA1E165`、`0xA1E193`）。`sub_A2C980` 只清零 16×40B + dword，无分配。
3. Init ABI：`sub_A1E210` 末尾 `retn 4`，接受一个被忽略的 dword 栈参；Base 实现不读它。
4. Init 分配原语：不是裸 `malloc`，而是 `sub_A648B0(mode=0, EDX=0, 64-bit size)`；小对象路径 `malloc(size+4)` 并在 `[ptr]` 存总大小，返回 `ptr+4`；配对释放 `sub_A0F500(ptr)`/`sub_AEF15C(ptr)` → `free(ptr-4)`。
5. deleting dtor：`0xA1E1CF push 370h` 存在，但被调 `sub_AEF15C` 忽略该参数，只按 `this` 走释放链。对象大小 `0x370` 由该 push 与分配路径共同确认。
6. `qword_C7B640` 是日志缓冲区 `OutputString`（`0xC7B648`）前 8 字节；日志函数通过 `&qword_C7B640 + strlen + 7` 读 `OutputString` 末字符，不写 `C7B640`。5023 个 `XrefsTo` 全为引用，静态值 0；是否另有直接写入见 §6.4。
7. key 计数器 `0xC7362C` 全量 `XrefsTo` 仅两条 code 引用：`0xA1E07C` 读、`0xA1E09B` 写，均在 `sub_A1E050`，无锁/非原子。
8. v1 把 wrapper `0xA5F230`（`call [*this+0x30](../../../07-virtual-device/design-revision/this)`）与 `0xA5F370`（`call [*this+0x38](../../../07-virtual-device/design-revision/this)`）列为 device query 调用者不成立：Base slot12/14 分别需要 1 个 action 栈参（`ret 4`），这两个 receiver 不可能是 InputDeviceBase。详见 §5.5。
9. v1 §2.3/Net 相关：Net 覆写 query 槽不检查 binding 长度，也不读 `+0xC0/+0xC4`；v1 的“三者都先检查 action < n”只适用于 Base/Gamepad/Keyboard 共用的 Base 实现，不适用于 Net。

## 3. W02：分配、构造与 Init 失败（O-ALLOC / O-CTOR）

### 3.1 分配链与失败语义

- `sub_A0F4C0(size)` → `sub_A648B0(mode=0, a2=0, size)`（`0xA0F4C0` 反汇编）。`sub_A648B0` 直接读取：
  - mode 0：`if (HIDWORD(size)) size=0;`（**≥4GiB 请求被静默截断为 0**，不是失败）；`p = malloc(size+4)`；成功写 `[p]=size+4`、累加计数、`return p+1`（`ptr+4`）；失败调用 `sub_A23200(0x7FCB9DD6)` 后 `return 0`。
  - mode 1：`free(a2-4)` 并做计数扣减。
  - mode 2：计数操作后返回 0。
- 失败回调 `sub_A23200` 遍历 `KAGE::Memory::Manager`（静态对象 `0xC37964`，初始化函数 `sub_406740`）的 handler 链 `0xC37974`，逐个调用 `fn(code, ctx)`；静态路径不 throw、不 abort，**因此分配函数可以返回 0**；运行期 handler 是否长跳/抛出属运行期未决。
- 配对释放：`sub_A0F500(ptr)` = `sub_A648B0(1,ptr,0)` = `free(ptr-4)`；`sub_AEF15C` 尾调 `sub_A0F500`。这是 `F1` raw allocation 回滚与 `RAW_ALLOCATED -> EMPTY` 唯一允许的释放原语。
- vector 分配 `sub_40CF00`：`n==0` 返回 0；`n<0x1000` 直接返回 `sub_A0F4C0(n)`（可能为 0，调用者未查空）；`n>=0x1000` 走 32 字节对齐路径，失败调用 `invalid_parameter_noinfo_noreturn()`。
- 对齐：对象分配得到 `malloc+4`（4 mod 8）；`+0xD4` 的 8 字节 store 在 x86 合法。≥0x1000 的 vector 路径 32 字节对齐。

### 3.2 Base ctor `sub_A1E050` 与字段初始化表

`char *__thiscall(char *this)`，`0x15E`，唯一 `retn`，无失败出口、无 SEH、无可抛出 callee。写入顺序：读 key → 写 vptr `0xB82758` → `word[+0x08]=0` → 写 key `+0x0C` → `+0x10/+0x14/+0x18=0` → 全局 key+1 → `+0x20..+0x3C=0` → `+0xC0/+0xC4=0` → `+0xC8=-1`、`+0xCC=4` → `+0xD0=0x3E4CCCCD (0.2f)`、`+0xD4=0x3E99999A (0.3f)`、`+0xD8=0` → `+0xDC/+0xE0/+0xE4=0` → `sub_A2C980(this+0xE8)` → `word[+0x08]|=2` → `memset(this+0x40,0,0x80)` → `memset(this+0xE8,0,0x284)`。

| 字段 | ctor 是否初始化 | 说明 |
| --- | --- | --- |
| `+0x00` | 是 | Base vptr |
| `+0x04` | **否** | 未观察到 Base vtable 方法读取；原生路径不读不能由本轮证明（UNKNOWN） |
| `+0x08` | 是 | bit0=0；bit1=1（query 前提）；bit3 由运行期置位 |
| `+0x0C` | 是 | 全局 key（Keyboard ctor 之后覆写为 0） |
| `+0x10/+0x14/+0x18` | 是 | binding vector begin/end/cap = 0；Init slot5 填充 |
| `+0x20..+0x3C` | 是 | 当前/上一帧 bit 状态与方向位，common 每帧维护 |
| `+0x40..+0xBF` | 是 | memset 0 |
| `+0xC0/+0xC4` | 是 | 0；Init 重新分配；cleanup 释放并置 0 |
| `+0xC8/+0xCC` | 是 | `-1 / 4`（binding write-back 索引） |
| `+0xD0/+0xD4` | 是 | float 0.2f / 0.3f（死区 + 判定阈值） |
| `+0xD8..+0xE4` | 是 | 0 |
| `+0xE8..+0x36B` | 是 | 16×40B active 记录区 + 计数 `+0x368`（清零后由 slot34 写） |

key 递增由 `mov edx,dword_C7362C` / `mov dword_C7362C,eax` 两条普通指令完成（`0xA1E07C/0xA1E09B`），无锁；主线程创建与 worker Gamepad 创建存在真实竞争窗口。

### 3.3 Init `sub_A1E210` 的失败 CFG

反汇编关键序列（`0xA1E210`，60 条指令）：

1. `call [vptr+0x14]`（**先虚调 slot5**，Base 为 `0xA1E430`，把全局默认 binding 拷入 `+0x10`，必要时 grow）。
2. `n = (this[+0x14] - this[+0x10]) >> 4`。
3. `mul edi(4)` 计算 `4*n`；`seto cl` 捕获有符号溢出；溢出时把 64-bit size 置为 `0x00000000FFFFFFFF`（约 4GiB）；`sub_A648B0(CL=0, EDX=0, size)` → 写 `+0xC0`。
4. 用当前重新计算的 n 对 `+0xC4` 重复一次分配。
5. `memset(+0xC0,0,4*n)`、`memset(+0xC4,0,4*n)`。
6. `mov al,1; retn 4`（**总是返回 1**，忽略栈参）。

| 失败点 | 条件 | 实际出口 | 资源/指针状态 | 实现影响 |
| --- | --- | --- | --- | --- |
| Init 第一次分配 | `sub_A648B0` 返回 0 | 仍执行第二次分配，随后 `memset(0,0,4*n)` | `+0xC0=0`，`+0xC4` 可能非 0 | n>0 时无法 fail-closed，进程崩溃；返回后检查 `+0xC0` 无法覆盖 |
| Init 第二次分配 | 返回 0 | `memset(0,0,4*n)` | `+0xC4=0` | 同上 |
| `4*n` 32-bit 溢出 | OF=1 | size=0xFFFFFFFF → malloc 失败 → 上述崩溃 | — | 不能靠“返回值 1”证明成功 |
| slot5 grow | 分配失败 | `invalid_parameter_noinfo_noreturn`（大对象）或 0 指针 memmove | vector 状态被清零 | 异常/终止在 Init 内发生，不能返回 |
| n=0 | 两次 `malloc(4)+4` | 返回 1 | 两指针非空，memset 长度 0 | 正常 |
| 重复 Init | 无条件 | 覆盖两指针 | 旧缓冲泄漏 | 设计约束：每 session 只调用一次 Base Init |

`sub_A1E2C0` cleanup：`+0xC0`/`+0xC4` 非空则 `sub_A648B0(1,p,0)` → `free(p-4)` 并置 0；幂等。`sub_A1E1B0` deleting dtor：写回 Base vptr → cleanup → `sub_4B3950(this+0x10)`（binding vector 析构，按 32B 对齐/`[-4]` 头释放）→ `flag&1` 时 `sub_AEF15C(this)` 释放对象。

### 3.4 原生创建顺序与 F1–F5 的条件

四条 `sub_A1E050` 调用路径直接反汇编确认：

- Keyboard `sub_A21980`：`sub_A0F4C0(0x370)` → null 检查 → base ctor → 写 Keyboard vptr `0xBA1678` → `+0x0C=0`（固定 key=0）→ 若 slot1 仍为 `sub_A6C3E0` 则直接调 Keyboard Init（`push 0`）。
- Gamepad `sub_A21D40`：`sub_A0F4C0(0x4F0)` → null 检查 → base ctor → 写 Gamepad vptr `0xB82C80` → 初始化 `+0x370..+0x38C` 专有字段（`+0x378="unknown"`）→ 在 `0xA21E7F` 调 Gamepad Init `sub_A22190` → `test al,al; jz` 失败时 `push 1; call [obj->vtable]`（原生 deleting dtor）。
- Net ctor `0x9015B0/0x901930`：base ctor 后写 Net vptr；不调用 Base Init（`sub_A1E210` 的 code 调用者只有 Keyboard/Gamepad 的 Init 覆盖）。
- Init 覆盖内部：Keyboard `0xA6C42B`、Gamepad `0xA22420` 各 `push` 一个参数后直接 `call sub_A1E210`；Base Init 忽略该参数。
- control block：`sub_A0F4C0(0x18)` → `sub_40CC10` → 写 typed vtable → `[ctrl+0x14]=object` → 以按值 pair `{object,ctrl}` 调 insert（Gamepad `0xA21EF8..0xA21F02`）。

对自有实现的静态约束（DESIGN_ONLY 建议，不冒充已采纳实现）：

1. `F1` 必须检查 `sub_A0F4C0(0x370+ext)` 返回非空；`RAW_ALLOCATED -> EMPTY` 只能用 `sub_A0F500(ptr)` 释放。
2. `F2` 调 `sub_A1E050` 后 key 已递增且对象已“逃逸”（无失败出口）；失败回滚不能假设 key 可复用。
3. `F3` 必须在 `F4` 之前写自有 vptr：Base Init 第一步就虚调 slot5，继承 slot5 = `sub_A1E430`，会以 `this+0x10` 为 vector 调 `sub_7DEFC0`。
4. `F4` 可选的更安全实现（DESIGN_ONLY）：自行实现 slot5 等价（拷贝默认 binding 或保持空 vector），然后用自有分配器分配两个 `4*n` 缓冲并写 `+0xC0/+0xC4`，避免 Base Init 的崩溃路径与重复 Init 泄漏；代价是必须精确复刻 Init 对 `+0x10` 的默认 binding 语义。若仍调用 Base Init，则**不能**依赖“返回值 1”或事后 null 检查覆盖内部崩溃。
5. `F5` 必须检查 control 分配与控制块 `+0x10` CS 指针（若采用原生 `sub_40CC10`，`sub_A15770` 在 CS 分配失败时仍返回 1 并留下 `+0x10=0`）；不存在 `{object,null}` 或 `{object, cs=null}` 的发布路径。
6. 原生 Gamepad/Keyboard 的“Init 返回 0 → deleting dtor”模式不能用于自有对象：自有 slot0 不释放，失败只能进入有记录的泄漏/retire 路径。

## 4. W03：六个覆写槽与 query 精确语义（O-ABI-0C / O-ABI-78 / O-ABI-QUERY）

### 4.1 六个覆写槽 ABI

| 槽 | Base 原生目标 | 已确认 ABI（本轮直接读取） | 自有语义要点 |
| --- | --- | --- | --- |
| 0 `+0x00` | `0xA1E1B0` deleting dtor | `void* __thiscall(this, int freeFlag)`，`ret 4`；写回 Base vptr、cleanup、vector 析构、`freeFlag&1` 时释放对象；返回 this | 不调 cleanup/dtor、不释放；标 ZOMBIE/入队并返回 `this`；coordinator 不主动调用 |
| 3 `+0x0C` | `_purecall`（`0xAF05D3`） | `const char* __thiscall(this)`，无栈参；消费者按 C 字符串 `strcmp/strlen/strncpy` | 返回自有静态非空字符串；insert 通知链在 `0x835D1A` 同步调用 |
| 12 `+0x30` | `0xA1E8D0` | `bool __thiscall(this, uint32 action)`，`ret 4`，结果 AL；`0xA20940` 直接调用 | 由快照返回 pressed；`RETIRING` 后 false |
| 13 `+0x34` | `0xA1E970` | `bool __thiscall(this, uint32 action)`，`ret 4`，结果 AL | 由快照返回 triggered/边沿；`RETIRING` 后 false |
| 14 `+0x38` | `0xA1EA20` | `float __thiscall(this, uint32 action)`，`ret 4`，结果 x87 `ST(0)` | 由快照返回 value；`RETIRING` 后 0.0 |
| 30 `+0x78` | `_purecall`（`0xAF05D3`） | `bool __thiscall(this)`，结果 AL | 锁存当前已发布快照（current/previous）；**必须返回非 0**，否则 manager update 在 manager 锁内直接 erase |

slot0/30 Base 为 `_purecall`；slot3 在第一次 insert 的 `+0x78` 通知链中立即被调用，slot30 在每帧 per-device common 中被调用。

### 4.2 slot30 poll 的调用条件与 true/false 语义

per-device common `sub_A1F280`（直接反编译）顺序：

1. `+0x20/+0x24 → +0x28/+0x2C`；`+0x30/+0x34 → +0x38/+0x3C`（保存上一帧）。
2. `memcpy(+0xC4, +0xC0, 4*n)`；清零 `+0x20/+0x24/+0x30/+0x34`；`memset(+0xC0, 0, 4*n)`。
3. 调 `slot30`；**返回 0 立即 return 0**（跳过后续所有槽）。
4. 调 `slot32`；为真则对 index 0/1 调 `slot31` 并经 `sub_A1EED0` 把归一化模拟量写入 `+0x40..+0x5C`，同时按阈值设置 `+0x30/+0x34` 方向位。
5. `+0xCC != 4` 时调 `slot15(+0x3C)(0)`；非 -1 时写入 binding（越界只记日志）。
6. `+8 & 8` → return 1。
7. 震动衰减并调 `slot33`；return 1。

原生 poll 实现：Net `0x709C30` = `mov al,1; ret`；Keyboard `0xA6C650` 无 0 返回路径；Gamepad `0xA22940` 在全局手柄列表中找不到物理设备时返回 0。因此 poll 的语义是“设备仍存在/可更新”，不是“输入中性”。**设计约束：自有 poll 必须返回非 0；退休不能通过让 poll 返回 0 来表达“中性”，否则会触发原生锁内 erase。**

### 4.3 query 三槽分支公式（Base 实现，本轮直接读取）

设 `n = (this[+0x14]-this[+0x10])>>4`，`flags = *(uint16*)(this+8)`，`cur = (float*)*(this+0xC0)`，`prev = (float*)*(this+0xC4)`，`T = *(float*)(this+0xD4)`，`binding[a]` 为 `+0x10` vector 第 a 个 16B 元素（4 个 int key，-1 表示空）。

- `pressed(a)`（slot12 `0xA1E8D0`）：
  ```text
  if (!(flags & 2)) return false;
  if ((uint32)a >= n) return false;
  if (cur[a] >= T) return true;
  for (k in binding[a].keys[0..3]):
      if (k != -1 && slot9(k)) return true;   // slot9 = 0xA1E610: slot11(k) > 0.0f
  return false;
  ```
- `triggered(a)`（slot13 `0xA1E970`）：
  ```text
  if (!(flags & 2)) return false;
  if ((uint32)a >= n) return false;
  if (cur[a] >= T && prev[a] < T) return true;          // 上升沿
  for (k in binding[a].keys[0..3]):
      if (k != -1 && slot10(k)) return true;            // slot10 = 0xA1E640 raw pressed
  return false;
  ```
  比较使用 float `<`/`<=`；NaN 行为未定义（记录为残余）。
- `value(a)`（slot14 `0xA1EA20`）：
  ```text
  if (!(flags & 2)) return 0.0f;
  if ((uint32)a >= n) return 0.0f;
  v = cur[a];
  for (k in binding[a].keys[0..3]):
      if (k != -1) v = fmaxf(v, slot11(k));             // slot11 = 0xA1E790 binding 值
  return v;
  ```
- 越界：入口用 unsigned `a >= n` 返回 false/0；函数内部循环里对 `a >= n` 的分支只调用 `sub_A112C0(16,"ActionId is out of range\n")`，**不 break、不钳制，随后仍按 binding 指针读**；因此运行期若 vector 在检查后被并发修改，存在越界读。本轮未发现 `+0x10` vector 在 Init 之后的 writer（见 §6），但保留并发缺口。
- Net 覆写不检查 n、不读 `+0xC0/+0xC4`：slot12/13 用 `+0x380/+0x384`（当前）与 `+0x388/+0x38C`（上一帧）的 64-bit bitmask；slot14 对 action 0..7 返回 `+0x390..+0x39C` 模拟轴，其它 action 虚调 slot12 返回 1.0/0.0。Net 行为不能直接搬给自有对象；自有实现按快照语义。

### 4.4 slot31/32/33

- slot31：`void __thiscall(this, float out[2], int index)`，`ret 8`，无返回值（out 由调用者提供）；Base 复制 `qword_C7B640`，Gamepad 按 axis key 组合；common 通过 `sub_A1EED0` 使用归一化结果。
- slot32：`bool __thiscall(this)`；Base/Keyboard/Net = 0x709C30 恒 true；Gamepad = 0x5CCC00 恒 false（跳过模拟量处理）。
- slot33：`void __thiscall(this, arg1_32, arg2_32)`，`ret 8`，两个 32-bit 栈项；Base 为空实现。Gamepad `0xA22C80` 可读出 `(int a2, float vibration)`；per-device common 以两个 float 调用（remaining, original）。**Base 空实现无法唯一确定类型；v1 的 `(int,float)` 与 `(float,float)` 不一致成立。** 自有对象继承 Base 中性实现即可，若需严格中性可显式覆写空实现。

### 4.5 slot3 消费者与覆盖条件

insert 通知链：`insert 0xA20DF0` 解锁后同步调 manager `+0x78`（`0xA211D0`）→ 注册 callback `0x95B290` → `sub_835C90(key)`。后者：

- 用 `sub_A20390(key)` 按 key 查设备；查不到直接跳过。
- 对命中的 object 做 `__RTDynamicCast(Base → NetInputDevice)`；**非 Net** 才进入游戏侧 controller 列表逻辑。
- 调用 `device->vtable[+0x0C]` 取名称：`strcmp` 查重、两次 `strlen`、一次 `strncpy`。返回值必须非空且稳定。
- 命中已有条目时，从设备 binding vector 固定复制 `0x200` 字节（32×16B action）；当 `n<32` 时，`v21>=n` 只记录 `ActionId is out of range` 日志，**仍继续越界读取**。因此 **自有对象的 n 必须 ≥ 32**（或该消费者必须被认定为不可达；本轮未发现使其不可达的条件）。

### 4.6 query 调用者与 v1 修正

- 直接调用者（本轮直接反编译）：`sub_A20940`（slot12，`(device*, *a3)`）与 `sub_A209E0`（slot13）；二者后接可选 `n79_0` 回调（`0xC7E814`）。
- v1 §3.2 的 wrapper `0xA5F230`/`0xA5F370` 不是 device query 直接调用者（无 action 栈参，receiver 类不同）。修正见 §2.5 第 8 条。
- slot14 的其它候选调用点（普查命中的 `call [reg+38h]`：`0x6F7BBB`、`0x8EAB1F`、`0x8EACDD`、`0x8EB84F`、`0x8EDC34`、`0x903298`、`0x930D6D`、`0x930DB2`、`0xA60EF2`、`0xA71397`）未逐一完成 receiver 类型判定；其中 `0x903298` 已确认在在线存档/日志函数 `sub_903040` 中，receiver 非 device。其余保留 `UNKNOWN`，不作为 ABI 依据。

### 4.7 重复 poll 与 authority tick

原生 per-device common 在每次调用中把 current 复制为 previous 并清零 current，即“每次 common 调用 = 一个输入帧”。自有 S2 设计若在 poll 中再维护一份私有 current/previous，则同一 authority tick 内多次 poll 会推进两次边沿，与 scheduler exactly-once 不自动一致。**设计约束（DESIGN_ONLY）：快照锁存必须以 authority tick 为键，重复 poll 只返回同一 (current, previous)，不得再次轮转。** 具体 exactly-once 属 05-06，本报告不声称已实现。

## 5. W04：32 个继承槽与双缓冲（O-BUF）

### 5.1 18 处 `+0xC0/+0xC4` 指令命中（本轮重算）

四张 vtable 152 槽去重为 75 个函数；指令级扫描得 18 条非栈 `+0xC0/+0xC4` 命中，分布于 9 个函数：`sub_A1E210`（4）、`sub_A1E2C0`（4）、`sub_A1E4B0`（1）、`sub_A1E590`（2）、`sub_A1E8D0`（1）、`sub_A1E970`（2）、`sub_A1EA20`（1）、`sub_A1ED50`（1）、`sub_A1EE10`（2）。写函数（vtable 群）= `sub_A1E210`、`sub_A1E2C0`；另有 Base ctor 两处显式写 0（`0xA1E0F5/0xA1E0FF`）；per-device common `sub_A1F280` 三处指针读（`0xA1F2C2` memcpy 源/目标、`0xA1F2FB` memset 目标）。

逐函数界条件（本轮直接反编译）：

| 函数/槽 | 读取上下文 | 界条件与退役后行为 |
| --- | --- | --- |
| `sub_A1E4B0` slot7 | `+0x20..+0x34` 全零时取两路模拟量；再按 n 扫 `+0xC0` 找 `>= +0xD4`；否则扫 `+0x368` 条 active 记录 | n=0 跳过缓冲扫描；n>0 时长度由 n 约束；退役后缓冲仍有效但内容不再更新，可能继续返回 1 → 非中性，L3 缺口 |
| `sub_A1E590` slot8 | 按 n 扫 `+0xC0/+0xC4` 的上升沿；否则扫 `+0x368` 条记录 `&1` | 同上；n=0 跳过 |
| `sub_A1ED50` slot19 | `a2<n` 时写 `(+0xC0)[a2]` | 有界；把 float bit pattern 写入当前帧 |
| `sub_A1EE10` slot21 | `a2<n` 时处理 binding 后把 `(+0xC0)[a2]` 复制到 `(+0xC4)[a2]` | 结尾的读在 if 之外，**`a2>=n` 会越界读**；调用受限 |
| `sub_A1E8D0/970/EA20` | query 读 cur/prev | 入口 unsigned 界检查；`n>0` 需缓冲非空（Init 保证）；slot21/cleanup 可改写/释放缓冲，调用受限 |

结论：在只调用一次 Base Init、n 与缓冲一致、不主动调用 cleanup/dtor、不调用 slot34 的前提下，除 slot21 越界读与退役后非中性外，18 处命中对本对象内存安全；`NEVER` 下退役后缓冲不释放，因此不会出现 use-after-free，但会延长旧输入语义。**建议对 slot7/8 增加中性覆写或让 coordinator 保证退休后这些槽不再被调用；slot21 只允许 `a2<n` 的调用。**

### 5.2 binding 长度/容量与缓冲重分配

- Init 两次分配使用同一 n，两缓冲等长；Init 后 `+0xC0/+0xC4` 只有在 cleanup（置 0）或重复 Init（覆盖指针，泄漏旧缓冲）时改变。
- per-device common 每帧按当时的 `n=((+0x14)-(+0x10))>>4` 重算 memcpy/memset 长度；若 `+0x10` vector 在 Init 后扩容而缓冲不同步，普通 common 会越界。本轮直接确认的 writer 只有：Base ctor（置 0/0/0）、Init slot5 `sub_A1E430`（拷贝/扩容）、cleanup/vector dtor（释放/置 0）；**未发现 Init 之后对 `+0x10` vector 的增删 writer**。该“未发现”不等于全程序不可达，列为残余。
- binding 元素 stride 16；`+0xC0/+0xC4` 是 n 个 float，按 binding index 索引；query 的 `slot+0x24/+0x28/+0x2C` 按 `binding[a]` 的 4 个 key 继续取值。

### 5.3 slot34 与 `+0xE8/+0x368`

`sub_A1F740(this, records, count)`：一次性 TLS 初始化后 `qmemcpy(global, this+0xE8, 0x284)`、`memset(this+0xE8,0,0x284)`，再按 count 从 records 抄 40B/条写回 `+0xE8` 并 `++*(uint32*)(this+0x368)`；**count 无上限检查**，count>16 会越界写 `+0x368` 之后的区域。`+0x368` 的 reader 是 slot7/slot8 与 manager update 的 deferred 判定路径。设计上协调器不调用 slot34；游戏若调用且 count>16 会破坏对象，列为调用受限 / UNKNOWN（caller 清单见并行 W04 补充）。

### 5.4 32 个继承槽分类

本轮已把除 6 个覆写槽外的 32 个 Base 槽逐个直接反编译/读取（原始指令与要点见证据索引 W04 补充）。分类如下：

| 槽 | 偏移 | Base VA | 直接读取要点 | 分类 |
| ---: | --- | --- | --- | --- |
| 1 | `+0x04` | `0xA1E210` | Init：虚调 slot5 → 按 n 分配两缓冲；无 null 检查、总返回 1；`retn 4` | 调用受限（仅 F4；失败不可返回） |
| 2 | `+0x08` | `0xA1E2C0` | 两缓冲非空则 `free(p-4)` 并置 0；幂等 | 调用受限（NEVER 下不得主动调用） |
| 4 | `+0x10` | `0xA1E360` | key code → 静态字符串表；无 this 使用 | 可继承 |
| 5 | `+0x14` | `0xA1E430` | 把全局默认 binding memmove 到 `+0x10`；容量不足经 `sub_7DEFC0` grow | 调用受限（Init 依赖；仅 F4 前有效） |
| 6 | `+0x18` | `0xA1E490` | 写 `+0xC8=a2`、`+0xCC=a3` | 可继承（默认 a3=4 时 common 不做写回） |
| 7 | `+0x1C` | `0xA1E4B0` | 读 `+0xC0` 阈值 + active 记录 `&3`；manager update 锁内调用决定 deferred | 建议中性覆写（退休后旧缓冲可继续返回 1） |
| 8 | `+0x20` | `0xA1E590` | 读 `+0xC0/+0xC4` 上升沿 + 记录 `&1` | 建议中性覆写（同上） |
| 9 | `+0x24` | `0xA1E610` | 经 slot11 取值 `>0`；flags bit2 前提 | 可继承（无绑定键时中性） |
| 10 | `+0x28` | `0xA1E640` | 用 `+0x20..+0x3C` 位状态与 slot35/36/37 判定 raw pressed | 可继承（无输入位时 0） |
| 11 | `+0x2C` | `0xA1E790` | 经 slot35/36/37 + `+0x20..+0x3C` 返回 binding 值 | 可继承 |
| 15 | `+0x3C` | `0xA1EAD0` | mode 0..3 反查；无匹配返回 -1 | 可继承（中性 -1；common 仅在 `+0xCC!=4` 调用） |
| 16 | `+0x40` | `0xA1EBE0` | 以 1.0f/0.0f 转发 slot17 | 可继承 |
| 17 | `+0x44` | `0xA1EC20` | 分类 key 后写 `+0x20/+0x24` 或 `+0x30/+0x34` 位与 `+0x40..` float | 可继承 |
| 18 | `+0x48` | `0xA1ED10` | 以 1.0/0.0 转发 slot19 | 可继承 |
| 19 | `+0x4C` | `0xA1ED50` | `a2<n` 时写 `(+0xC0)[a2]=a3` | 调用受限（必须 a2<n） |
| 20 | `+0x50` | `0xA1ED80` | 置 `+0x28/+0x2C` 或 `+0x38/+0x3C` 位 | 可继承 |
| 21 | `+0x54` | `0xA1EE10` | `a2<n` 处理 binding 后复制 `+0xC0[a2]→+0xC4[a2]`；结尾读在 if 外 | 调用受限（a2>=n 越界读） |
| 22 | `+0x58` | `0xA1EE90` | 写 `+0xD0` | 可继承 |
| 23 | `+0x5C` | `0xA1EEB0` | 写 `+0xD4` | 可继承 |
| 24 | `+0x60` | `0xA1EFA0` | 对 action 0..3 调 slot9 返回位掩码 | 可继承 |
| 25 | `+0x64` | `0xA1F0E0` | 按 `+0x368` 计数扫描记录并拷贝 40B | 可继承（记录空时中性） |
| 26 | `+0x68` | `0xA1F160` | 按 `+0x368` 扫描记录并拷贝 | 可继承 |
| 27 | `+0x6C` | `0xA1F1F0` | `memset(out,0,0x284)` 后拷贝匹配记录 | 可继承 |
| 28 | `+0x70` | `0x42C160` | `return 0` | 可继承 |
| 29 | `+0x74` | `0x42C160` | `return 0` | 可继承 |
| 31 | `+0x7C` | `0xA1F720` | 复制 `qword_C7B640` 到 out[2] | 可继承（依赖该全局；可覆写中性） |
| 32 | `+0x80` | `0x709C30` | `return 1` | 可继承 |
| 33 | `+0x84` | `0x42BC90` | `ret 8` 空实现 | 可继承 |
| 34 | `+0x88` | `0xA1F740` | 记录区装载；count 条写回并 `+++0x368` | 调用受限/UNKNOWN（count 无上限；无 code xref） |
| 35 | `+0x8C` | `0xA1F8F0` | `return *key < 0xABAB`（只读） | 可继承 |
| 36 | `+0x90` | `0xA1F910` | `key-0xABAB<=0x1F` 归一化并返回 1 | 可继承 |
| 37 | `+0x94` | `0xA1F930` | `key-0xCDCD<=1` 归一化并返回 1 | 可继承 |

补充条件：

- 上述“可继承”以“只调用一次 Base Init、n 与两缓冲一致、不主动调用 slot2/dtor、不调用 slot34”为前提；退役后继承槽仍会读有效内存，但 slot7/8 的旧缓冲可能继续产生非中性结果，故需要中性覆写或停止调用。
- slot34 只有 4 条 vtable data 引用、无 code xref；经 vtable 的间接调用者未隔离，count>16 的越界写风险保留。
- 75 个 vtable 函数内对 `+0x10/+0x14/+0x18` 的写入扫描只命中栈局部，没有函数写 binding vector 字段；已知 writer 仍只有 ctor（置 0）、slot5（拷贝/grow）与 vector 析构。
- 默认 binding writer `sub_4307C0` 只有 `_main@0x93114C` 一条 code 引用：两套全局 binding 只在启动时初始化一次；`qword_C7B640` 的 5023 条引用中未发现写入者。运行期实际取值与计算地址写入仍为 `UNKNOWN`。
- 未发现调用点只能写“当前范围未发现”，不能据此标记不可达。

## 6. W05：control block 六槽、计数旁路与一致性（O-CB / O-RC-INLINE）

### 6.1 六槽逐返回分支（本轮重读；均为 `__thiscall`）

| 槽 | VA | 直接行为 | 返回载体 |
| --- | --- | --- | --- |
| 0 dtor | `0x40CB50` | 写 ReferenceCountBase vptr；`CS=[+0x10]` 非空则 `DeleteCriticalSection` + `sub_AEF15C(CS)` 置 0；`byte[+0xC]&=~1`；写 MutexBase vptr；`flag&1` 时 `sub_AEF15C(this)`；返回 this | EAX=this |
| 4 TryAddStrong | `0x40C6B0` | Mutex lock(+8 对象的 vtable+0x0C)；`word[+4]!=0` → unlock 后 `call ctrl->vtable+8`（AddStrong），返回 1；`==0` → unlock 返回 0 | AL |
| 8 AddStrong | `0x40C690` | lock；`++word[+4]`；tail unlock（vtable+0x10）；无溢出检查 | EAX=Mutex unlock 结果（非 bool 语义） |
| 12 ReleaseStrong | `0x4358B0` | lock；`word[+4]==0` → unlock 返回 1；非零 → `--word[+4]`；非零 → unlock 返回 1；变零 → unlock，`obj=ctrl[+0x14]`，非空则 `obj->vtable[0](../../../07-virtual-device/design-revision/obj,1)`，`ctrl[+0x14]=0`，tail `ctrl->vtable+0x14`（ReleaseWeak），返回值 = ReleaseWeak 返回值 | AL |
| 16 AddWeak | `0x40C610` | lock；`++word[+6]`；tail unlock；无溢出检查 | EAX=Mutex unlock 结果 |
| 20 ReleaseWeak | `0x40C5C0` | lock；`word[+6]==0` → unlock 返回 1；非零 → `--word[+6]`；非零 → unlock 返回 1；变零 → unlock，`ctrl->vtable[0](../../../07-virtual-device/design-revision/ctrl,1)`，返回 0 | AL |

布局：`+0x00` vptr、`+0x04` uint16 strong、`+0x06` uint16 weak、`+0x08` 内嵌 Mutex（vptr）、`+0x0C` Mutex 状态字节、`+0x10` CRITICAL_SECTION*、`+0x14` object*。`sub_40CC10` 写 `[+4]=0x00010001`（strong=1/weak=1）；`sub_A15770` 分配 0x1C CS 并 `InitializeCriticalSection`；Mutex vtable `0xB81C0C` 的 `+0x0C` = `0xA157F0`（lock）、`+0x10` = `0xA159A0`（unlock，含 "trying to unlock mutex that wasn't locked" 诊断字符串）。

关键失败语义：`sub_A15770` 在 CS 分配失败时仍 `flag|=1`、`[+0x10]=0`、返回 1；后续 lock/unlock 会在空 CS 上操作。**自有 control 构造必须显式检查 `+0x10`，否则不得发布。**

### 6.2 typed/generic 等价与 L1

- `0xB82918`（`ReferenceCount<InputDeviceKeyboard>`）与 `0xB82934`（`ReferenceCount<InputDeviceGamepad>`）的六槽目标与通用 `0xB78014` **逐项相同**：`0x40CB50 / 0x40C6B0 / 0x40C690 / 0x4358B0 / 0x40C610 / 0x40C5C0`；差异只在 `[-1]` COL。
- 六槽 `XrefsTo` 本轮复读：dtor `20 data/0 code`、TryAddStrong `20/0`、AddStrong `20/0`、ReleaseStrong `11/0`、AddWeak `20/0`、ReleaseWeak `20/0`。所有引用来自 vtable 数据。
- `sub_40CC10` XrefsTo：37 个 code 调用者、0 个 data 引用（各创建路径直接调用 ctor）。

### 6.3 pair/reset 族的计数语义

- `sub_902D60` pair copy：只经 `ctrl->vtable+4` 取强引用，释放旧引用只经 `ctrl->vtable+0x0C`；不使用 raw 计数。
- `sub_902CB0` reset-null：新建 0x18 control → `sub_40CC10` → 写通用 vtable → `ctrl+0x14=0`；旧 control 经 `vtable+0x0C` 释放；失败返回 `{0,0}`。
- `sub_902F10` reset-object：新建 control → 写通用 vtable → `ctrl+0x14=object`；旧 control 经 `vtable+0x0C` 释放。
- `sub_A20390` keyed lookup 命中时调 `sub_902D60` 返回 acquire 副本；**未命中时调 `sub_902CB0(out)`，返回 `{0, new-control}`**（strong=weak=1），因此“lookup miss”也会分配一个 control block；调用者必须按 pair 所有权释放。该点对 F7/注册事务和异常清理有直接影响。

### 6.4 旁路普查（O-RC-INLINE）

- 16-bit 内存访问 `+4/+6` 指令级普查（全 `.text`）：共 186 条命中，分布于 83 个函数。已知 InputDeviceBase pair/manager helper 族（`0x902D60`、`0x902CB0`、`0x902F10`、`0xA21570`、`0xA21630`、`0xA21800/0xA21880`、`0xA20DF0`、`0xA20EB0`）**零命中**。
- 命中主要来自其它 `ReferenceCount<T>` 特化函数（`0x40C630`、`0x9146A0`、`0xA23E50` 等，各自 vtable data-only）与无关的 16-bit 字段结构；`sub_902950` 已确认是 `NetInputDevice::broadcast_frame_input`，`sub_900CA0` 是 Net 消息写入器，不属于 pair/control。
- 直接 `operator delete`/free control block：本轮只确认 `sub_40CB50` 自身在 flag bit0 时 `sub_AEF15C(this)`；未发现 pair/helper 族对 control 的 raw free。全程序“计算地址 free”无法用本轮扫描排除，保留 `UNKNOWN`。
- 结论（有限范围）：对 `ReferenceCount<InputDeviceBase>`，计数与释放的已确认路径全部经 control vptr；不存在全程序 points-to 证明；32 位组合写入、计算地址、其它模板混淆仍是残余缺口。

### 6.5 自有无锁影子计数与原生 inline 的并发差异

原生六槽用内嵌 Mutex 串行化；自有实现设计为 32 位影子计数 + uint16 饱和镜像，并在最后释放时只入队不释放。原生 helper 通过 vptr 分派时会进入自有槽，因此与影子计数处于同一实现；但若存在绕过 vptr 的 inline lock/unlock 或 raw 计数（本轮未发现，见 §6.4），自有无锁写入与其不同步。**功能影响：NEVER 下不会二次释放，但镜像可能短暂过时；L1 只应表述为“已确认路径经 vptr”，不能表述为“所有路径经 vptr”。**

## 7. W06：key 分配与 F7 唯一性（O-KEY）

### 7.1 直接事实（本轮）

- key 计数器 `0xC7362C`：全量 `XrefsTo` 只有两条 code 引用——`0xA1E07C` 读、`0xA1E09B` 写，均在 `sub_A1E050`；无锁、非原子，存在 read-modify-write 丢失更新窗口。
- Keyboard ctor 在 `0xA21A4C` 把 `device+0x0C` 覆写为 0（固定 key=0）；计数器为 Keyboard 消耗的键值被丢弃。
- `Player+0x1618`：writer 只有 `sub_7A6450@0x7A6598`（SetControllerId）；本轮在 `0x780000-0x7B0000`、`0x950000-0x960000`、`0x6F0000-0x700000`、`0x420000-0x430000` 范围扫描到 57 条引用，reader 集中在 Player 类函数（`sub_782AF0`、`sub_78B640`、`sub_78CA00`、`sub_7A3D30` 等）及 `sub_9563B0`、`sub_95A7B0`、`sub_95B310`、`sub_6F5850`、`sub_6F6500`。本轮未逐一证明各 reader 对 key 范围/符号/特殊值的假设；`0x42xxxx` 的 6 条同偏移命中属于其它类。
- keyed lookup `sub_A20390`：加 manager `+0x14` 锁；线性遍历 active vector（8B stride），比较 `object+0x0C == key`；命中 `sub_902D60` 返回 acquire 副本；未命中 `sub_902CB0` 构造 `{0,new-control}`（strong=weak=1）；解锁后返回 out。**miss 也分配 control，调用者必须释放。**

### 7.2 F7 拆成三问

1. **设备确实在 vector**：insert 后仍有原生移除路径。Gamepad disconnect `sub_A21F40` 只经 RTTI 匹配 Gamepad，非 Gamepad 不 erase（W08 cast #16）；Net remove `sub_9091A0→sub_906020` 按 key lookup 后 remove，若 key 重复可能移除首个匹配；manager update 在 poll 返回 0 时锁内 erase；`sub_A213B0` 只在 `device+8` bit0 置位时重置 manager `+0x20` entry。自有 poll 必须非 0 且 bit0 不被置位。
2. **同 key 成员唯一**：只有 `sub_A1E050` 的计数器提供键值，但无锁；worker Gamepad 创建与主线程创建并发时可能读到同一值并都写入；Keyboard 固定 0 与计数器空间重叠（正常启动顺序下 0 已被首个设备消耗，但顺序不由 F7 保证）。2^32 回绕在静态上未设保护。
3. **lookup 解析到自有对象**：返回首个匹配；若重复 key 且对方在 vector 更前，lookup 返回对方。一次 lookup 通过不能证明唯一。

静态反例：`vector=[other(K), self(K)]` → lookup 返回 other；`vector=[self(K), other(K)]` → 返回 self 但 key 仍重复。该反例是控制流推导，不运行实验。

### 7.3 竞争窗口与可选设计

- 竞争窗口：worker Gamepad 创建/移除与主线程 create/insert/F7 并发；F7 通过后到 `REGISTERED` 之间、以及 `REGISTERED` 之后，都可能出现新同 key 成员或 vector 变化。F7 复核无法覆盖“之后”。
- 可选方案（DESIGN_ONLY，均未实现）：
  - 原子化 key 分配：需要修改原生 ctor 的内联读改写，或 hook 该指令；静态上不能仅凭调用顺序保证。
  - 持 manager 锁做创建与复核：worker Gamepad 创建不取 manager 锁，无法覆盖该 writer。
  - 主线程创建 + 插入后全 vector 扫描同 key + 再 lookup：缩短但仍有 TOCTOU。
  - 保留特殊 key：必须先证明 `Player+0x1618` 消费者、Keyboard 0 与原生 Net id 不假设连续/范围；当前未证明。
- “多查一次/稍后再查”不作为消除竞争的方案。

### 7.4 W06 主题证据补充（`...-w0607-draft.md`）

- `device+0x0C` writer 只有 2 条：`sub_A1E050@0xA1E08E`（key=counter）与 Keyboard `sub_A21980@0xA21A4C`（强制 0）；reader 至少 11 条（lookup `0xA203F2`、dispatch `0xA21144/0xA210D7`、insert callback `0x95B2C2`、disconnect callback `0x913792`、Net remove `0x90604D`、teardown `0x90AAAC`、NetManager `0x909B31`、Net add-player `0x6F675B/0x6F67A5` 等）。全映像 `+0xC` 命中 19,064 处不属于设备类型判定，保留 UNKNOWN。
- `Player+0x1618` 全 `.text` 指令扫描：普通 writer 恰好 2 条——Player ctor `0x7540BF` 写 `-1` 初始化、`sub_7A6450@0x7A6598` 写 controllerId；直接以该字段为 lookup key 的 callsite 有 `0x42AF89 / 0x6F6A24 / 0x78323F / 0x7B9F9C / 0x7BDBA6 / 0x98DD51 / 0x9B279D`。`-1` 是“无 controller”哨兵；游戏侧另用 `0xDEADBEEF` 作为无效 key 哨兵（`0x934B0B`、`0x913760`），lookup 不识别哨兵。
- lookup 命中但 `TryAddStrong` 失败（control strong==0）时 `sub_902D60` 静默返回 `{0,0}`；因此“key 匹配”不等于“拿到可用 pair”。
- dispatch `sub_A20FB0` ABI 为 `char __thiscall(manager*, int key, bool (*pred)(pair,a4,a5), int a4, int a5, int* outKey)` `retn 14h`；`key!=-1` 时谓词在 manager 锁外执行，`key==-1` 时谓词在 manager 锁内执行；谓词收到的是新强引用，由谓词负责释放。

## 8. W07：insert/remove/notification、异常与线程（O-INSERT-ABI / O-REMOVE / O-NOTIFY / O-THREAD）

### 8.1 insert `sub_A20DF0(manager, pair by value, ctx)` `retn 8`

1. manager `+0x14` Mutex lock（`this[5][3]` = Mutex vtable+0x0C）。
2. `end==capacity` → `sub_A21630(manager+0x08 作为 vector, end, &pair)`；否则 `sub_902D60(end,&pair)` 取 acquire 副本，`end += 8`。
3. unlock（Mutex vtable+0x10）。
4. manager vtable `+0x78` handler；`sub_902D60(&tmp,&pair)` 再取一份临时 pair；同步调用 `handler(manager,tmp.obj,tmp.ctrl)`。
5. 返回/异常帧释放调用者按值传入的 pair；如 ctx 有 vtable+0xC 也释放 ctx。

结论：append 在 manager 锁内；notification 在解锁后、调用者线程同步执行；回调持有 acquire 副本、不得长期保存。调用者补全（W06/W07 并行直接读取）：`sub_A20DF0` 的**直接** code xref 只有 `0x905FFB`（wrapper）与 `0xA21F02`（Gamepad）两处 + vtable cell `0xB82860/0xB829BC`；Keyboard 的 insert 是对 Manager vtable `+0x6C` 的间接调用（`0xA21AFC`），因此直接 xref 计数不代表全部动态调用者。

### 8.2 remove `sub_A20EB0(manager, object, control)` `retn 8`

- `object==0` → 跳过通知/搜索，只做尾部 pair 释放。
- 否则先按顺序对 manager vtable `+0x7C`、`+0x80` 各以 acquire 副本调用一次；两者都在 erase **之前**，且 object 不在 vector 中也会触发。
- 线性搜索 `pair.object == object`；命中 `sub_A21570` 交换式 erase，释放移出的最后 pair，`end-=8`；未命中不 erase。
- **全程不获取 manager 锁**；函数无 bool 返回，返回值不能证明移除；重复/非成员 remove 仍产生通知。

### 8.3 erase / grow / 异常

- `sub_A21570`：逐元素 acquire→swap→释放副本，返回新 end；remove 尾部释放被换出的旧 pair。
- `sub_A21630`：1.5x 扩容；`sub_40CF00(8*newcap)` 分配（大对象失败 noreturn，小对象可能返回 0）；`sub_902D60` 迁移/插入；`sub_A21800`/`sub_A21880` 析构/迁移旧区间；最后释放旧存储。SEH 帧存在，但异常逐分支未反汇编；分配失败在多数路径不可恢复。
- 结论：grow/insert 的分配失败没有可供 coordinator 安全重试的返回路径；“再次调用 remove”不能假定为异常后的合法清理。

### 8.4 notification handlers 与 callback

- `sub_A211D0`（`+0x78`，链表 manager+0x2C）/`sub_A212C0`（`+0x7C`，manager+0x34）：遍历节点；对每个节点用 control vtable+4 构造 acquire 副本，调用 `fn(device_obj, ctrl_or_0, ctx)`，返回后释放副本；循环结束释放传入 pair。同步、调用者线程。
- `sub_A213B0`（`+0x80`，manager+0x44）：若 `device+8 & 1`：在 manager+0x20 数组（计数 `dword_C799F4`）按 object 查找，命中 `sub_902F10(entry,0)` 重置；遍历 +0x44 回调；清 `device+8` bit0；释放传入 pair。
- `sub_95B290`（+0x78 回调）：`sub_835C90(game_list, *(device+0x0C))`——用 device key 维护游戏侧 controller 列表；slot3 名称调用与 32×16B binding 复制见 §4.5/§5.1。
- `sub_913760`（+0x7C 回调）：打印 `Input device (ID=%d) disconnected`；`__RTDynamicCast(device,Base→Net)`；按记录指针或存储 device 的 key 匹配后 `sub_902F10(...,0)`；释放传入 pair。
- 副作用范围：controller 列表、Player/Net 存储 pair、日志；更深层 Lua/UI/持久化未逐条归属，保留 UNKNOWN。

### 8.5 线程与主循环（当前范围）

- v1 §5.6 的 worker 锚点（`0xA21B38` 线程启动、`sub_A220C0` 循环、`sub_A6DAB0→sub_A6D560/sub_A6DA10`、全局 `0xC75DA8/0xC78D7C`）保留为“继承原报告，未在本轮重读”。
- manager update `sub_A1FC00`：v1 的“持 manager 锁遍历；poll 返回 0 时锁内调 `+0x7C/+0x80` 并 erase；否则 slot7/deferred”保留为继承结论；本轮直接确认 callsite `0x954F7D` 位于 `sub_954CD0`。
- 静态可确认：insert 持锁、remove 不持锁、manager update 持锁；worker 与主/游戏线程都可能触发 manager 变更。三者对同一 vector 没有统一锁，实际交错为 UNKNOWN；`sub_954CD0` 的 caller 线程归属未由本轮静态证明。

### 8.6 assignment 事务边界

- `SetControllerId` `sub_7A6450(Player*, int controllerId, char propagate)`：写 `Player+0x1618`（`0x7A6598`）；callees 为日志、`__RTDynamicCast`、PlayerManager 相关（v1 §5.7）；没有 insert/remove、manager lookup 或共同锁。
- `sub_95B310` 的 `0x95B87F`（unassign）与 `0x95BCA0`（reassign）调用点与 remove 调用栈分离；join root `sub_9BAC40` 的 `0x9BB004/0x9BB424` 也直接调用 SetControllerId。
- 保留状态：已注册未绑定、已移除仍持旧 key、意外原生 erase。F7/F8 不能假设 assignment 与注册是同一事务。

### 8.7 W07 主题证据补充（`...-w0607-draft.md`）

- 所有权：insert 的调用者按值 pair 在 append 与通知期间一直存活（实参在调用者栈上），vector 与 handler 各取一份强引用，insert 末尾才释放调用者强引用；remove 不 acquire vector，每个 handler 前各取一份副本；update 在 poll==0 时取副本并调 `+0x7C/+0x80`。
- remove 语义表：`object==0` 不通知/不 erase；成员单个匹配只删第一个；非成员与重复 remove 仍各触发一次通知；multiple match 只删第一个；`control==0` 时 handler 收到 `{object,0}`，仍按 object 扫描/erase。
- grow 失败出口：`count==0x1FFFFFFF` → `_Xlength_error`（C++ 抛出）；newcap 超限 → `bad_array_new_length`；小分配失败（`sub_40CF00` 返回 0 无检查）→ 空指针写 AV；大分配失败 → `invalid_parameter_noinfo_noreturn`；`TryAddStrong` 失败 → 新元素 `{0,0}` 而 grow 继续并释放旧 buffer，可能丢失元素。insert 的 `CxxFrameHandler3` unwind map 只析构按值 pair，不含 manager mutex unlock；若 grow 的 C++ 异常逃出，静态上有 manager mutex 保持锁定的风险（STATIC_DERIVED）。
- 注册点：`+0x78` registrar `0x952C36`（ctx=0，函数 `0x95B290`）；`+0x7C` registrar `0x906E70`（ctx=Net manager，函数 `0x913760`）；两者都在 `sub_952410` 游戏侧 manager 构造路径，不是逐帧注册。`+0x80` 的 `+0x44` 链表在静态上没有 registrar（沿用 JRV2）。
- 线程图（静态调用图）：worker `sub_A220C0→sub_A6DAB0→sub_A6D560/sub_A6DA10→Gamepad ctor insert / disconnect remove`；主链 `_main→sub_9AB6D0`（`SteamAPI_RunCallbacks→sub_A714E0(0)→sub_954CD0(...,a3=1)`）；Net remove `sub_90B150→sub_8FD900/sub_911120→sub_9091A0→sub_906020`，teardown `sub_90A8A0`。实际 thread id、频率与交错仍为 UNKNOWN。
- 额外数据竞争（静态）：worker 在创建/移除时无锁写全局设备数组 `dword_C79BEC/dword_C79BF0`，而 Gamepad poll `0xA22940` 在 main update 中无锁读同一数组；该数组没有 manager mutex 保护。`sub_9091A0` 的 6 个 caller、`sub_90A8A0` 的 13 个 caller 的运行期线程归属未定。

## 9. W08：RTTI/COL（O-RTTI）

本轮已用 IDA MCP 直接完成；完整逐站点证据与查询范围见独立证据文件 [`out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w08-draft.md`](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w08-draft.md)（355 行）。主报告摘要如下。

### 9.1 x86 COL/TD/CH/BCD 原始布局

| 表 | vtable | `[-1]` → COL | pTD | pCH | CH nbase |
| --- | --- | --- | --- | --- | ---: |
| Base | `0xB82758` | `0xBB02F0` | `0xC5D4C0` | `0xBAEF54` | 1 |
| Gamepad | `0xB82C80` | `0xBB0460` | `0xC5D4EC` | `0xBB0474` | 2 |
| Keyboard | `0xBA1678` | `0xBB191C` | `0xC61194` | `0xBB194C` | 2 |
| Net | `0xB78030` | `0xBAEF78` | `0xC5D518` | `0xBAEF44` | 2 |

- COL 为 0x14 字节：`signature/offset/cdOffset=0`、pTD、pCH；**没有 x64 `pSelf`**。每个 COL 的静态 xref 只有对应 vtable 的 `[-1]`；四张设备 vtable 的引用全部是构造写入 `mov [obj], offset vtable`，未发现 `cmp/lea/push` 形态的 vptr 比较。
- TD：`+0` = type_info vptr `0xB1A424`；`+4` = `_UndecoratedName` 缓存槽（静态 0，**不是名称指针**）；`+8` = 内联名称。名称分别为 `.?AVInputDeviceBase@Input@KAGE@@`、`.?AVInputDeviceGamepad@Input@KAGE@@`、`.?AVInputDeviceKeyboard@Input@KAGE@@`、`.?AVNetInputDevice@IsaacRepentancePlus@@`，各只有一份。
- CH 0x10；BCD 0x18：self `numContainedBases=1`（Base BCD `0xBAEEFC` 为 0）、`mdisp=0`、`pdisp=0xFFFFFFFF`、`vdisp=0`、attrs=0x40；Gamepad/Keyboard/Net 的 base array 都是 `[自身 BCD, 0xBAEEFC]`。自建 COL 的现成模板（布局见 W08 草稿 §W08-4.1）。
- control vtable 也各有合法 `[-1]`/COL（通用 `0xB78014→0xBAEF64` 等），未发现原生代码消费 control RTTI；作为布局/常驻要求。

### 9.2 TD 消费与 cast 分类

- 四 TD 全量 xref：Base 19 = 17 code + 2 data；Gamepad 11 = 9+2；Keyboard 2 = 0+2；Net 10 = 8+2。
- 全映像 `__RTDynamicCast` thunk `0xAF08B1` 共 822 个 code xref；与四 TD 相交 17 站：source 全部为 Base，target = Gamepad 9 / Net 8 / Base 或 Keyboard 0。**Keyboard TD 没有任何 code 消费点**。
- null 处理：15 站有直接/后续 `test+jz` 并走安全分支；2 站（`0x934B52`、`0xA2201A`）无即时判定，但前置完全相同 cast 已成功且对象未改写，静态 null 不可达。17 站均无 null 解引用。
- null 分支有可观察副作用的站点：`sub_835C90@0x835CF0`（非 Net 路径，要求 slot+0x0C 已实现且稳定非空）、`sub_913760@0x9137B7`（key 清理）、`sub_8D8B00/0x8D9340`（控制器 UI，调用继承 Base 槽）、`sub_9563B0@0x956446`（assignment/控制器选择，调用 slot18）、`sub_6F6500`（seed/实体块）、`sub_A21F40@0xA21FF5`（非 Gamepad 设备回调）。

### 9.3 typeid 独立消费与方案条件

- 未发现独立 typeid/type_info 比较消费者：`__std_type_info_name` 的 11 个调用全部在 `sub_6F0090`，是静态 entity `typeid(T).name()` 日志；未导入 `__std_type_info_compare`；`0xB1A424` 的 324 个 xref 中 323 个是 TD `+0`、1 个是 `sub_AEF139` 构造写入；TD+4/TD+8 xref 为 0；动态取 TD 链（`8B ?? FC [8B ?? 0C]`）全 `.text` 扫描 0 真命中。**表述为“本轮范围未发现”，不写“不存在”**；内联裸 `strcmp`、运行期懒写 TD+4、vcruntime cast 内部与未来模块消费者仍为 `UNKNOWN`。
- 两方案在 17 个已发现消费点行为相同；差异只在 `typeid(*p)` 的 TD/名称（Base COL 直指报原生 Base TD 且与 `&typeid(Base)` 相等；自建 COL 报自有 TD 且与 Base 不等）。契约 v2 固定自建 COL，本轮不替换。
- 适用条件：单继承、Base 子对象在偏移 0、层次只含 `[自身, InputDeviceBase]`、TD 复用 `0xB1A424` 且名称内联、不得调原生 type_info dtor、不得把 TD+4 当名称指针。两方案都要求 pin 自有 DLL；自建 COL 额外要求 COL/CH/BCD/array/TD 名称常驻。

## 10. W09：原生 Join、action 与身份（O-JOIN / O-ACTION）

本轮已用 IDA MCP 完成离线 Join 选择链的直接读取；完整逐站点证据、action 立即数、路线 B 副作用与残余清单见 [`out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md`](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md)（486 行）。主报告摘要如下。

### 10.1 选择链与帧内位置

| 步骤 | 位置 | 事实 |
| --- | --- | --- |
| in-game frame | `sub_954CD0` | 主状态更新；`0x954F7D` 调 `sub_A1FC00`（Manager update）。 |
| 断连/重选 | `0x954F84` | `sub_95B310`（含 Gamepad RTTI 与两个 SetControllerId 站点），先于 Join pass。 |
| state 2 Join | `0x9551BD` | `sub_6FA540(n298)`；唯一 caller。 |
| Join/selector pass | `0x6FADA7` | `sub_9BAC40(PlayerManager = gameRoot+0x1BAA8)`；唯一 caller。 |
| 设备查询 dispatch | Manager vtable `+0x74` | `sub_A20FB0`：`key!=-1` 走 lookup 后回调；`key==-1` 持锁枚举全部 pair 调谓词。 |
| 查询回调 | `sub_A209E0` / `sub_A20940` | 分别调设备 vtable `+0x34`（triggered）/`+0x30`（pressed）。 |

同一帧相邻查询：action12 经 Manager `+0x14`→`+0x44`（`0x6FA55E/0x6FA56B`）、action31 经 `+0x74`+key `-1`（`0x6FAD85`）；两者语义为 `UNKNOWN`。`sub_954CD0` 的 caller 为 `sub_6F7AC0` 与 `sub_9AB6D0`（后者先 `SteamAPI_RunCallbacks` 再以 `a3=1` 调用）。

**Manager vtable 校正**：静态 `[0xC57B18]=0xB827F4`（ManagerBase）；初始化函数 `sub_402280@0x40233B` 写入运行期 `0xB82950`（Manager）。两套表的 `+0x28/+0x74` 都分别指向 `sub_A20390/sub_A20FB0`；ManagerBase 析构 `sub_A1F980` 再恢复 `0xB827F4`。静态初值不能当作 Join 时的实际 vtable。

### 10.2 选择语义与 key 来源

- 选择顺序 = Manager active pair vector 的物理下标顺序（`Manager+0x08..+0x0C`，8B stride），**没有类型优先级**；`sub_A20240` 在 mutex 下按下标复制 pair。
- key 来源：`0x9BAD2C mov esi,[esi+0Ch]`，即 `device+0x0C`；保存进 selector slot（`item+0`）。
- 冲突扫描：主 Player vector 的 `player+0x1618 == key`、secondary vector、4 个 selector slot；命中即视为已绑定。
- selector 分配 `sub_9BAAA0`：最多 4 slot；同 key 幂等复用；全满返回 -1。若 online roster 非空（`0x9BAAE7`），额外要求 NetInputDevice 映射，找不到返回 -1；**offline 空 roster 跳过该分支**。
- 选择链不依赖 Gamepad/Keyboard RTTI、平台 handle、`+0x370` 专有字段、物理枚举或窗口 handle；相邻 `sub_95B310` 则有 Gamepad RTTI（`0x95B3C6`，读到 `+0x384` 仅当 Gamepad），自有 COL 必须让 `dynamic_cast<Gamepad*>` 返回 null。

### 10.3 Join action ID 与调用槽（全部来自 `sub_9BAC40` 立即数）

| action | 用途 | 查询槽 | 关键地址 |
| ---: | --- | --- | --- |
| 19 | 进入 Join/选择（分配/复用 selector；调 `device+0x54(19)` flush） | `+0x34`（`sub_A209E0`） | `0x9BB05D/0x9BB064` |
| 14 | 确认：创建 Player + SetControllerId | `+0x34` | `0x9BB393/0x9BB39A` |
| 15 | 取消/退选：selector index := -1 | `+0x34` | `0x9BB3C2/0x9BB3C9` |
| 20 | 上一角色 | `+0x34` | `0x9BB2B0/0x9BB2B7` |
| 21 | 下一角色 | `+0x34` | `0x9BB2FE/0x9BB305` |
| 8+10+19 | possessor 备用组合（8/10 用 `+0x30` pressed） | 19 `+0x34`；8/10 `+0x30`（`sub_A20940`） | `0x9BAF71/0x9BAF9D/0x9BAFC9` |

- 普通路径最终绑定：`0x9BB415 sub_9B9CD0(PlayerManager,type)` 创建 Player → `0x9BB424 sub_7A6450(newPlayer, selectorKey, 1)`；possessor 路径 `0x9BAFFA sub_9B9CD0(PlayerManager,-1)` → `0x9BB004 SetControllerId(player,key,1)`。确认/取消后 selector index := -1（`0x9BB561`）。
- `0x9BB0B8 FF 50 54` 无条件调用设备 `+0x54(19)`（`sub_A1EE10`）。该实现只在 `action<binding_count` 时处理循环，但函数结尾仍无条件读写 `(+0xC0)[action]`/`(+0xC4)[action]`；**若 binding count ≤ 19 且不覆写 slot21，这里是明确的越界读写**。路线 A 的静态关键条件之一。
- 设备 vtable `+0x30/+0x34/+0x38` 的实现即本轮 §4.3 的 Base query；Join 只使用 pressed/triggered，未使用 value。

### 10.4 路线 B：`SetControllerId` ABI 与副作用

- ABI：`char __thiscall sub_7A6450(Player* this, uint32 controllerId, char propagate)`，`retn 8`；`0x7A6598 mov [edi+1618h], ebx` 是唯一写点。函数尾没有把返回值归一化，两个已知 caller 都忽略返回值；**不要用 AL 判定成功**。
- `Player+0x1618` 在 Player ctor `0x7540BF` 初始化为 -1；Net add-player 在 insert 之后（`0x6F68B7`）直接用 `device+0x0C` 写该字段，不经 lookup。
- 副作用：日志；`sub_9B94E0` 颜色数组槽（仅在 `PlayerManager+0x5C` 非零时）；遍历实体把 NPC `+0xF1C == old` 改写为 new（`__RTDynamicCast` Entity→NPC）；`propagate!=0` 时递归 `+0x1D98/+0x1E6C/+0x1E68`，并遍历主 Player vector 对 `player+0x3BC == this` 的其他 Player 递归。没有 insert/remove、Manager 锁、selector、HUD 或 ordinal 同步。
- 已知 caller `sub_95B310`：`0x95B87F`（`SetControllerId(player,-1,1)` 解绑）与 `0x95BCA0`（`SetControllerId(player,selectedKey,1)` 重绑）都在 offline roster 为空的断连/重选分支，与 remove 调用栈分离。其余 27 个 caller 未逐一分析。

### 10.5 身份关联与 exactly-once

- 静态可确认：`key ↔ device+0x0C`（ctor 计数器）；`key ↔ Player+0x1618`（SetControllerId）；`ordinal(+0x161C) = 创建时主 Player vector 旧长度`，随后 append；`sub_9B9CD0` 只检查 count+1 ≤ 0x40，native 不强制 ordinal==1。
- native 没有 `(generation,key)` 复合键或 join 时间戳；session generation、临时绑定失效、key 回绕/ABA 只能由自有 session 层维护。
- 多次 poll 语义：action19 反复为真只会反复建立/复用 selector；**action14 反复为真会重复 `sub_9B9CD0` 生成第二个 Player**；action15 会退役 selector。native 无全局 one-shot 锁。scheduler 必须保证 canonical 的 join/confirm 边沿对同一 `(generation, ordinal 1)` 至多返回一次 triggered=true，并且只让 B 对应的 virtual key 参与 Join 窗口。
- spawn（`0x9BB415`）、assignment（`0x9BB424`）、HUD（`0x9BB4FD`）之间没有共同事务；注册/绑定/HUD 可被观察到不同步。

### 10.6 路线 A 的静态前置（汇总）

1. 设备已由 insert 进入 Manager vector；
2. `device+0x0C` key 唯一且非负；
3. slot `+0x34`（以及 possessor 的 `+0x30`）对 Join action 返回正确 boolean；
4. binding count > 19，或覆写 slot21 以避免 `+0x54(19)` 越界（见 §10.3）；
5. slot30 poll 非 0（§4.2）；
6. 自有 COL 对 Gamepad/Keyboard/Net cast 返回 null（§9）；
7. offline 空 roster；online roster 非空时 native 要求 NetInputDevice 映射，virtual key 很可能拿不到 selector（路线 A 在线模式不可用，需另行处理）。
8. exactly-once join/confirm 边沿（§10.5）。

残余：`sub_706940`、`sub_9BD3B0`、LABEL_69 分支语义，action12/18/31 语义，运行期 binding count/阈值，`sub_9B9CD0` 之后的 Player Init/world 副作用（继承 05-04 证据链），其他 27 个 SetControllerId caller。

## 11. W10：统一结论与契约修订建议

### 11.1 16 项 O-* 状态表

| 义务 | 本轮状态 | 依据/缺口 |
| --- | --- | --- |
| O-ALLOC | 已直接确认 | `sub_A0F4C0`/`sub_A648B0` 返回 0 路径、4GiB 截断、配对 free `sub_A0F500`；运行期 handler 行为 UNKNOWN |
| O-CTOR | 已直接确认（失败处置仍受限） | ctor 无失败出口；Init 崩溃路径无法返回；字段表见 §3.2 |
| O-CB | 已直接确认 | 六槽返回分支、布局、typed/generic 等价、CS 分配失败语义 |
| O-RC-INLINE | 有限确认 | 186 命中/83 函数；pair 族零命中；无 points-to，保留缺口 |
| O-INSERT-ABI | 已直接确认 | 锁内 append（copy/TryAddStrong 或 grow）、解锁后同步通知、按值 pair 释放；SEH 逐分支未展开 |
| O-REMOVE | 已直接确认 | 不取 manager 锁；object==0 跳过；通知先于 erase 且未命中也触发；无 bool 返回 |
| O-NOTIFY | 已直接确认（副作用有限） | 两 handler 同步回调（device, ctrl, ctx）；`+0x80` 查 manager+0x20 后 reset；深层副作用 UNKNOWN |
| O-ABI-0C | 已直接确认 | slot3 三实现 + 消费者调用序列；硬前置 |
| O-ABI-78 | 已直接确认 | poll 真/假后的 common/manager 分支；必须非 0 |
| O-ABI-QUERY | 已直接确认 | 完整分支公式、越界日志、Net 差异、slot31/33 参数 |
| O-BUF | 已直接确认 | 18 命中/9 函数 + 32 槽逐项分类；slot7/8 需中性覆写，slot19/21/34 调用受限 |
| O-RTTI | 已直接确认（独立 typeid 消费未发现） | 17 个 cast（Gamepad 9/Net 8），null 处理安全；typeid/内联比较路径按范围保留 UNKNOWN |
| O-KEY | 已直接确认关键路径 | counter 仅 2 条指令且非原子；lookup 首匹配、miss 返回 `{0,new-control}`；F7 三问与竞争窗口见 §7 |
| O-JOIN | 已直接确认（offline 条件） | 选择链 `sub_954CD0→sub_6FA540→sub_9BAC40→Manager+0x74→设备+0x34/+0x30`；Join ID 19/14/15/20/21（possessor 19+8+10）；key 来源 `device+0x0C`；无 RTTI/硬件/窗口依赖；online NetInputDevice 门控；`+0x54(19)` 越界条件 |
| O-THREAD | 部分直接确认 | insert 持锁/remove 无锁/manager update 持锁；worker 链与主链 `_main→sub_9AB6D0→sub_954CD0`；worker 全局设备数组与 main poll 无锁竞争；具体 caller 的线程/频率与 UI 状态 UNKNOWN |
| O-ACTION | 已直接确认（Join）+ 继承（gameplay consumer） | Join ID 立即数直接读取；canonical→native 对应为 DESIGN/STATIC_DERIVED；movement/shooting/bomb/item/pill/drop 的 Player consumer 继承 T0-04 |

### 11.2 L1–L4 与最小反例

- L1（分派）：在已确认的 `ReferenceCount<InputDeviceBase>` 创建/pair/reset 路径中成立；反例缺口是“未解析的计算地址/其它模板/未知 caller”，不能由本轮升级为全称。
- L2（只延长）：自有槽不同步释放，`NEVER` 下进入 `BASE_CONSTRUCTED` 后不释放对象/control；`RAW_ALLOCATED -> EMPTY` 是唯一释放路径。反例检查点：slot0 的直接调用者若绕过 vptr 直接 call `0xA1E1B0`，会释放对象——这是继承槽直接调用旁路的一种，需在实现中避免/不导出。
- L3（迟到访问无害）：覆写槽返回中性；继承槽在 `NEVER` 下对象内存有效，但 `+0xC0/+0xC4` 旧值会让 slot7/8 继续产生非中性结论（§5.1），因此 L3 需要“slot7/8 中性覆写或停止调用”作为条件。
- L4（代码有效）：vtable/COL/slot 代码需 DLL pin；本轮不改变该设计条件。

### 11.3 建议覆写集合与理由

保持 6 槽（slot0/3/12/13/14/30）为基线。W04/W09 结果并入后的补充：

- **必须新增/显式覆写 slot21（`+0x54`），或保证 binding count > 19**：Join 普通路径 `0x9BB0B8` 无条件调用 `device+0x54(19)`；Base slot21 在 `action >= count` 时仍读写 `(+0xC0)[19]`/`(+0xC4)[19]`（`0xA1EE7D/0xA1EE80`），越界。若不覆写且 n≤19，路线 A 会破坏内存。
- slot7/8 建议中性覆写（退休后旧缓冲可能继续返回非中性）。
- slot15 视调用需求决定覆写；slot2 禁止主动调用；slot5 仅 F4 调用；slot19 调用受限（a2<n）；slot34 调用受限（count≤16 且无 code xref）。
- slot3/12/13/14/30 保持覆写；slot0 保持不释放。
- 其余 26 个继承槽按 §5.4 的“可继承”条件使用；未分析的直接调用旁路不得默认安全。

### 11.4 F1–F8 修订建议（当前）

- F1：检查 `sub_A0F4C0` 返回非空；失败直接回 EMPTY，无副作用。
- F2：接受 key 已递增、不可复用；ctor 无失败出口。
- F3：自有 vptr 必须在 Init 前写入。
- F4：不得依赖 Init 返回 1；可选自建 slot5+缓冲分配（DESIGN_ONLY）以消除崩溃路径；否则记录“n>0 分配失败即进程崩溃”的不可恢复性。**binding count 必须覆盖 Join action19（>19），或覆写 slot21**。
- F5：检查 control 分配与控制块 CS 指针；绝不构造 `{object,null}`。
- F6：保持按值 pair；insert 通知会同步调用 slot3，必须先就绪；Keyboard insert 走 Manager vtable `+0x6C`，直接 xref 计数不代表动态调用者。
- F7：必须分别验证“在 vector”“同 key 唯一”“lookup 解析到本对象”；lookup 未命中会分配 `{0,new-control}`，需按 pair 释放；计数器非原子，worker 创建/移除与主线程并发是真实窗口。
- F8：注册与 assignment 无共同事务；保留“已注册未绑定”“已移除仍持旧 key”“已创建 Player 但 HUD/ordinal 未同步”状态。
- Join 专项：canonical 的 action19/action14 边沿必须 exactly-once；否则原生 Join 会重复建立 selector（19）或重复 spawn Player（14）。

## 12. 残余 UNKNOWN 与运行期未决

- Init 内存分配失败发生在函数内部 memset，无法 fail-closed；不进行故障注入，不声称返回后检查可覆盖。
- 运行期默认 binding 数量/内容（`sub_4307C0` 的参数来源）静态不可确定；n<32 时 insert 回调会越界读；n≤19 且不覆写 slot21 时 Join 的 `+0x54(19)` 会越界读写。
- 运行期 Memory Manager handler 是否长跳/抛出。
- `+0x04` 等未初始化字段是否存在未观察 reader。
- 非 vptr/计算地址的 raw 计数、control free、inline lock（无 points-to）。
- insert/grow 的 SEH 异常逐分支未展开；`TryAddStrong` 失败时 `sub_902D60` 静默返回 `{0,0}`，grow 继续并释放旧 buffer 可能丢失元素。
- worker 与主线程的实际交错时序、通知回调是否实际触发副作用；manager update 的具体 caller 线程/频率。
- online roster 非空时 native 要求 NetInputDevice 映射，virtual key 很可能拿不到 selector（路线 A 在线模式不可用）。
- `sub_706940`、`sub_9BD3B0`、LABEL_69 分支与 action12/18/31 的语义。
- W08：内联裸 `strcmp(name,...)` 比较、运行期懒写 TD+4、`__RTDynamicCast` vcruntime 内部行为与未来模块消费者；17 个 cast 的深层 null 分支副作用只追到第一层。
- W09：`sub_9B9CD0` 之后的 Player Init/world/持久化副作用仍由其原证据链承担；ordinal 1 依赖运行时主 vector；session generation 在 native 无字段。
- 同一 authority tick 多次 poll 的 exactly-once（05-06）；Join action19/14 边沿重复会分别导致 selector 重建/重复 spawn。

### 12.1 v1 S-1–S-11 承接表

| v1 项 | 本轮处理 | 静态结果边界 |
| --- | --- | --- |
| S-1 raw 计数/inline lock/free | §6.4/§7.4：六槽 xref 全 data、pair 族 16-bit 普查零命中、lookup/dispatch 只经 vptr | 无 points-to；计算地址/其它模板仍 UNKNOWN |
| S-2 inline lock 路径 | §6.4/§6.5：未发现绕过 vptr 的 lock/unlock；自有槽与原生 inline 是否同步未证明 | 保留缺口，NEVER 下不二次释放 |
| S-3 worker/main vector 并发 | §7.3/§8.5/§8.7：insert/update 持锁、remove 无锁、worker 与主链可并发 | 实际交错/频率/thread id UNKNOWN |
| S-4 Net/teardown 线程 | §8.7：Net remove `sub_9091A0→sub_906020`、teardown `sub_90A8A0` 的静态调用图 | 6/13 个 caller 的线程归属未定 |
| S-5 默认 binding 运行期值 | §5.2/§5.4：writer `sub_4307C0` 只被 `_main` 调用一次；Init 拷贝当时向量 | 运行期实际 count/内容 UNKNOWN；n≤19 触发 `+0x54(19)` 越界 |
| S-6 `qword_C7B640` 可写路径 | §5.4：5023 条引用未发现写入者；与日志缓冲区相邻 | 计算地址写入不能排除；slot31 依赖该全局 |
| S-7 Init 分配失败 | §3.3：分配失败在 `memset(0,0,4n)` 崩溃，无 fail-closed | 未做 fault injection |
| S-8 自定义 COL 现场 | §9：17 个已发现 cast 站点静态安全；自建 COL 布局可行 | typeid/dynamic_cast 现场行为未运行 |
| S-9 typeid/operator== | §9.3：未发现独立 typeid 消费者（范围已列） | 内联裸 strcmp/未来模块 UNKNOWN |
| S-10 control 直接 delete | §6.4：只确认 dtor 自身释放；pair/helper 无 raw free | 计算地址 free 未排除 |
| S-11 未初始化字段 | §3.2：`+0x04` ctor 不写，未观察到 Base 方法读取 | 未证明所有原生路径不读 |

## 13. 唯一一次整体静态文档审阅

本节是按工作设计 §8 执行的**唯一一次整体静态审阅**（读取 v2 主报告、证据索引、W08/W09 主题证据文件与本次改动的导航文档，一次给出整个推进结果）。审阅只做文档/静态一致性检查，不运行程序、不运行校验脚本、不校验哈希。

### 13.1 审阅结果（逐条对照 §8）

1. **16 项 O-\* 覆盖**：§11.1 的 16 行全部有具体回答或带原因的 `UNKNOWN`；O-JOIN/O-ACTION 为“已直接确认（offline 条件）”，O-THREAD 保留运行期线程/频率未决；每条结论后列了读取范围或主题证据文件。通过。
2. **指令、调用者使用与 ABI 表一致**：v2 的 §3–§10 与证据索引/W08/W09 草稿逐条对应；RVA 与 IDB VA 在 §2.2–§2.3 分列，旧稿 RVA 一律经 `+0x400000` 换算；未定类型（slot33 参数、TD+4 语义）和未读分支（SEH 异常、805 个其它 `__RTDynamicCast`、27 个 SetControllerId caller）均显式标注。通过。
3. **关键问题均有明确处理**：
   - Init 崩溃路径：§3.3（分配失败在 `memset` 崩溃、无法 fail-closed）。
   - F7 lookup 反例：§7.2（`[other(K),self(K)]` / `[self(K),other(K)]` 两个静态反例）。
   - query 参数域/边沿：§4.3（pressed/triggered/value 公式、unsigned 越界、NaN 未定义）。
   - cleanup/重复 Init：§3.3（幂等释放；重复 Init 泄漏）。
   - 双缓冲容量：§5.1–§5.2（18 命中/9 函数、n 与 writer 集合、slot21 越界）。
   - worker 同步：§8.5（insert 持锁/remove 无锁/manager update 持锁；实际交错为 UNKNOWN）。
   通过。
4. **覆写建议、F1–F8、L1–L4、NEVER、模块常驻、契约建议的一致性**：§11.3 在 6 槽基线上新增 slot21 必须覆写或 n>19（Join `+0x54(19)`）、slot7/8 中性覆写、slot2/5/19/34 调用受限；§11.4 的 F1–F8 与 §11.2 的 L1–L4 条件一致；`NEVER` 与 pin 的要求在 §9.3/§11.2 重复但一致。未发现“slot0 释放”“模块可卸载”“poll 可返回 0”等矛盾表述。通过。
5. **事实、推导、设计、运行期观察未混用**：本轮新增结论标 `IDA_STATIC_CONFIRMED`；由控制流构造的结论标 `STATIC_DERIVED`（如 F7 反例、null 不可达）；自有实现建议标 `DESIGN_ONLY`；运行期值（默认 binding count、实际分配失败、handler 行为、现场 cast）标 `UNKNOWN`。v1 的 `RUNTIME_OBSERVED` 保留原场景；未把“n 个 cast 站点安全”外推为“所有 RTTI 消费已证明”，未把“主线程创建”外推为“无竞争”。通过。
6. **历史保留与规范一致性**：v1 报告、decision v6、旧交接、失败记录均未改写；修正项在 §2.5 以“保留原文 + 新报告修订”方式列出；全文不含 gate、promotion、解锁字段、哈希校验、角色分离或多次测试流程。通过。

### 13.2 本次静态工作是否按范围完成

- 资料阅读、只读 IDB 分析、只读 PE 解析与文档编辑均在授权范围内执行；未启动游戏/调试器，未注入、Hook、IPC，未写 IDB，未运行任何测试/构建/harness/verify。
- W01–W10 的分析内容全部有落点：W01 §2；W02 §3；W03 §4；W04 §5；W05 §6；W06 §7；W07 §8；W08 §9 + W08 主题证据；W09 §10 + W09 主题证据；W10 §11。
- 产出物：本 v2 主报告、`out/cs-t0/virtual-device-owned-abi-static-evidence-v2.md` 证据索引、`...-w0607-draft.md`、`...-w08-draft.md`、`...-w09-draft.md`。W04 的 32 槽分类、18 命中、slot34 与默认 binding 由主智能体直接读取并写入 §5；未另行产出 W04 主题草稿（并行子任务被中止），不影响本 §13 的审阅结论。
- **结论：本次静态工作按范围完成**；这表示所有 W01–W10 问题已被分析并如实交代，不表示自有设备已实现、已实机验证或全部 ABI 已获无条件证明。

### 13.3 仍未解决的 ABI/实现问题（不生成 promotion/解锁字段）

1. Init 内部内存分配失败不可恢复（崩溃而非 fail-closed）；运行期实际默认 binding count/内容未知。
2. Join 的 `+0x54(19)` 越界风险：n≤19 且不覆写 slot21 时会读写越界；需要契约修订覆写集。
3. F7 唯一性：key 计数器非原子，worker/主线程并发创建的真实交错与可证明的同步方案未定；lookup 首匹配反例。
4. L1 的 points-to 缺口：计算地址/32-bit 组合写入/其它模板的 raw 计数与 control free 无法由本轮排除。
5. insert/grow 的 SEH 异常逐分支、`TryAddStrong` 失败时的 `{0,0}` 静默丢失元素未闭合。
6. W08：内联 `typeid` 名称比较、运行期 TD+4 懒写、vcruntime cast 内部行为、未来模块消费者。
7. W09：online NetInputDevice 门控使路线 A 在线不可用；`sub_9B9CD0` 之后的 Player Init/world/持久化副作用、ordinal 1 的运行时保证、action12/18/31 语义。
8. O-THREAD：具体 caller 的运行期线程/频率、暂停/切场景边界；主线程创建与 worker 变更的实际交错。
9. 同 authority tick 多次 poll 的 exactly-once（05-06）：Join action19 重复只会重建 selector，action14 重复会重复 spawn Player。

审阅发现问题时按约定记录失败与未完成项，不自动重跑；后续有实质修订时按新的完整目标继续。
