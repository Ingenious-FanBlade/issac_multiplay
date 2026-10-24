> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device 自有 ABI 补充静态确认 v3（IDA-MCP）

日期：2026-10-12
推进编号：`tx-00-06`（执行 [`virtual-device-owned-abi-static-confirmation-next-work-design-v2.md`](virtual-device-owned-abi-static-confirmation-next-work-design-v2.md) 的 K01–K07）
状态：补充静态确认主体完成；唯一一次整体静态审阅见 §12
证据索引：[`out/cs-t0/virtual-device-owned-abi-static-evidence-v3.md`](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v3.md)
v2 基线：[静态确认 v2](virtual-device-owned-abi-static-confirmation-v2.md)；v2 证据索引与主题证据：`out/cs-t0/virtual-device-owned-abi-static-evidence-v2.md`、`...-w0607-draft.md`、`...-w08-draft.md`、`...-w09-draft.md`

先读[现行项目规范](../../../../../project-conventions.md)。本推进不设 gate、不校验哈希、不使用旧 decision/解锁字段/作者分离/矩阵完成率作为前置条件。范围仅包含资料阅读、只读 IDB/磁盘 PE 分析、静态控制流/数据流推导与文档编辑；未启动游戏、未附加调试器、未读进程内存、未注入/Hook/IPC、未做故障注入、未运行构建/harness/verify/旧 builder，未修改或保存 IDB。

证据标签：`IDA_STATIC_CONFIRMED`（本轮直接读指令/字节/元数据）、`STATIC_DERIVED`（由直接事实推导）、`DESIGN_ONLY`（设计建议，未实现/未采纳）、`UNKNOWN`。继承 v2 的内容标“引用 v2，未在本轮重读”。

---

## 1. 执行范围与目标

本轮目标：修正 v2 中注册回调读写方向、分配整数回绕和 binding 容量的事实/约束错误；补充 cleanup 生命周期、异常 unwind 与 shadow/mirror 一致性分析；形成可按明确条件使用的 v3，并保留 v2 的已支持结论、原始记录与历史审阅结果。

执行编号 `tx-00-06`；K01–K07 只是内容编号，不是多个验收。整个目标完成后只执行 §12 一次整体静态文档审阅。

实际分析对象（本轮只读读取）：

| 项 | 值 | 读取方式 |
| --- | --- | --- |
| input PE / IDB | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` / `isaac-ng.exe.i64` | `ida_nalt.get_input_file_path()` / `ida_loader.get_path` |
| IDB imagebase / 段 | `0x400000`；`.text 0x401000-0xB18000` 等（引用 v2 §2.1） | `ida_nalt.get_imagebase()`；未校验哈希 |
| 地址规则 | IDB VA = RVA + `0x400000`；旧资料 RVA 逐处标注 | 与工作设计 v2 §3 一致 |

## 2. v2 修正表（K07 交付项 1）

| # | v2 位置 | v2 原结论 | v3 直接事实 | 修正原因/依据 | 受影响项 |
| --- | --- | --- | --- | --- | --- |
| C1 | §4.5；W06/W07 草稿 callback 副作用 | “命中已有条目时从设备 binding vector 固定复制 0x200 字节”；未提同名写回 | 新条目 helper `sub_835BA0`：**device binding → record**，每 action 读 dword0/dword1，共 0x100 字节；同名分支 `0x835E58..0x835ED7`：**record → device binding**，每 action 写 dword0/dword1（`0x835EB7`），覆盖 32 个 action | 直接反汇编/数据流，见证据 v3 K01 | O-NOTIFY、O-BUF、O-INSERT-ABI；F6/F7 |
| C2 | §3.1/§3.3；W02 | “≥4GiB 请求被静默截断为 0 → malloc(4)”；“约 4GiB 必然失败” | `sub_A648B0`：`hi!=0` 时把 64-bit size 清零后 `malloc(4)`；`hi==0` 时 `malloc((lo+4) mod 2^32)`，**`size=0xFFFFFFFF → malloc(3)`**；`mul` 的 OF=高半非零；Init 溢出路径构造 `(hi=0,lo=0xFFFFFFFF)` | 74 条指令反汇编，见证据 v3 K02 | O-ALLOC、O-CTOR；F1/F4/F5 |
| C3 | §4.5、§10.6、§11.4 | 注册需要 n≥32，Join 需要 n>19 或覆写 slot21；两处分开表述 | 统一条件：**n≥32 且 32 个 binding 元素与两份 32-float 缓冲完整初始化**；slot21 覆写只保护 Join flush，**不能保护注册 helper 的直接 binding 读/写**；n=20 或 n=0+slot21 覆写均有反例 | K01 真实访问集合 + 消费者表，见证据 v3 K03 | O-BUF、O-ABI-QUERY、O-JOIN、O-ACTION；F4/F5/F6 |
| C4 | §5.1、§5.4、§11.2 | “对象/control 不释放，所以基类状态有效”“不主动调用 cleanup/Init” | `NEVER` 只保证内存不释放，不保证缓冲/vector/vptr 状态；cleanup/Init/Base dtor 有直接调用者（派生 dtor 等）与未隔离的 vtable 间接调用者；Base dtor 无 code xref，但 vtable slot0 路径必须靠自有覆写拦住 | 直接调用者普查 + 资源表，见证据 v3 K04 | O-CTOR、O-BUF、O-REMOVE、O-THREAD；L2/L3 |
| C5 | §8.3、§8.7、§12 | EH 状态“未逐分支展开” | 已解析 4 个 FuncInfo（insert `0xBD21A8`、remove `0xBD21DC`、grow `0xBD22F4`、`sub_A21880` `0xBD2354`）、unwind/try/handler 与 funclet：insert/remove 无 unlock action；grow 有 catch-all `0xA217CE` 清理部分新 range 与新 buffer 后重抛；部分 state 用 `__std_terminate` | EH 元数据直接读取，见证据 v3 K05 | O-INSERT-ABI、O-REMOVE、O-NOTIFY、O-CTOR；§8 |
| C6 | §6.5 | “镜像可能短暂过时” | 有限交错反例显示可**永久**不一致；在“任意线程、不依赖原生 mutex、无阻塞”的现有文字下无法用仅原子 shadow+普通 mirror 证明一致性；需契约修订（候选：自有望短锁 + 32-bit 打包 mirror store，或改用原生 embedded Mutex） | 手工静态推导，见证据 v3 K06 | O-CB、O-RC-INLINE、O-THREAD；协调器契约 v2 §5 |
| C7 | §4.1 正文；证据索引状态；§13 | “slot0/30 为 purecall”（编号错误）；索引写“W04–W10 待补”；旧“全部通过”被当作当前结论 | slot0 = deleting dtor `0xA1E1B0`；**slot3 与 slot30** = `_purecall 0xAF05D3`；索引状态与报告一致；v2 §13 保留为历史记录，`tx-00-04` 核查发现的错误由本 §2 记录，本轮审阅见 §12 | v2 四张 vtable 原始内容 + 会话核查摘要 | 全文一致性；O-ABI-0C、O-ABI-78 |

## 3. K01：注册回调读写方向与数据流

调用链：`insert sub_A20DF0` → manager vtable `+0x78` handler `sub_A211D0` → 回调 `sub_95B290` → `sub_835C90(gameList, key)`。

### 3.1 两条分支的真实方向

| 分支 | 触发 | source → destination | index/stride | 实际宽度 | 越界行为 |
| --- | --- | --- | --- | --- | --- |
| 新条目 `sub_835BA0` | 名称未出现在 game list | device `+0x10` binding → record | action 0..31、binding stride 16B、record stride 8B | 每 action 2 dword，共 0x100 字节 | `action>=n` 只记日志，仍越界读 |
| 同名 `0x835E58..0x835ED7` | 名称已存在 | record → device `+0x10` binding | action 0..31、record 8B/action | 每 action 2 dword，共 0x100 字节写入 | `action>=n` 只记日志，仍越界写 |
| 列表 append | 有空位/满位 | record 拷贝进 list | 0x104 字节/record | `qmemcpy` 或 `sub_8367E0` 增长 | 增长失败见 K05 |

- record 布局：`+0x00` 名称 `char*`；`+0x04` 起 32 action × 8B（两个 primitive dword）；新条目时调用者先 `memset(record+4,0xFF,0x100)`，未覆盖的 dword 保持 `0xFFFFFFFF`。
- `+0xB00/+0xC4`（状态缓冲）在本路径完全不出现；v2 把它与 binding vector 混用属错误，见 §2 C1。
- 同名写回会在首次 insert 时同步覆盖自有 binding 的 dword0/dword1（若存在同名旧 record）；dword2/3 保留。
- UI 路径：`sub_95AF30 → sub_A243C0(device+0x10, action, out)` 只读 binding；`sub_835820` 从全局默认 binding 填游戏侧配置，不写设备 binding。未发现 UI/配置修改设备 binding 元素/长度/缓冲。

### 3.2 影响

- F6 之前必须准备好 32 个有效 binding 元素（K03）；slot3 名称必须稳定非空，且同名 record 写回要求名称唯一性策略。
- 注册回调无 `+0xC0/+0xC4` 访问，因此“状态缓冲中性”与“binding 可被注册路径改写”必须分开描述。
- 注册 helper 的越界读/写在 n<32 时是直接内存破坏（不只读），O-NOTIFY/O-BUF 的结论按此修订。

## 4. K02：分配器位宽、回绕与 Init 失败 CFG

### 4.1 精确公式

设 64-bit 请求 `(hi:lo)`。`sub_A648B0` mode0：

| 输入 | malloc 参数 | 头 `[ptr]` | 返回 payload | 备注 |
| --- | --- | --- | --- | --- |
| `hi==0, lo<=0xFFFFFFFB` | `lo+4` | `lo+4` | `lo` | 正常 |
| `hi==0, lo==0xFFFFFFFC..0xFFFFFFFF` | `lo+4` 回绕：0/1/2/**3** | 同 malloc 参数 | 0 | `0xFFFFFFFF` 是明确反例 |
| `hi!=0` | 4（size 被清零后 +4） | 4 | 0 | 不是 4GiB 失败路径 |

- `malloc` 只接收 `edi`（32 位）；64 位统计在 `[esi]/[esi+4]`，与实际请求分离。
- malloc 失败 → `sub_A23200(0x7FCB9DD6)`（Memory Manager handler 链）后返回 0；F1 的“无副作用”需修订：诊断/handler 副作用未展开。
- free 契约：`sub_A0F500(ptr)` → `sub_A648B0(mode=1,a2=ptr)` → `free(ptr-4)`；不能 free 返回值本身。
- `malloc(0)` 返回行为在当前只读输入内无法确认：`UNKNOWN`。

### 4.2 Init `sub_A1E210`

```text
eax = (end-begin)>>4 ; n
mul edi(4) ; EDX:EAX = n*4（无符号）
push 0 ; 高 dword = 0
seto cl ; OF/CF = (EDX!=0)
neg ecx ; ECX = EDX!=0 ? 0xFFFFFFFF : 0
or ecx,eax ; 溢出时低 dword = 0xFFFFFFFF
push ecx ; 64-bit size=(hi=0, lo)
```

- 溢出时 allocator 收到 `(hi=0,lo=0xFFFFFFFF)` → malloc(3)，随后按 `4*n` 低 32 位 `memset` → 堆越界。
- 正常对象的 n 由真实 binding 元素数给出；只有畸形/损坏 vector 才可能让 `4*n` 溢出（正常 32 元素对象不可达）。
- n=0：`malloc(4)` 也可能失败；失败时函数仍返回 1，`+0xC0/+0xC4` 为 0。
- 失败表（修订）：两次分配失败、header 回绕、长度不一致、slot5 grow、CS 分配、异常、重复 Init 全部保留；返回后 null 检查不能覆盖函数内部崩溃。

## 5. K03：统一 binding 容量与 F4–F6/Join 条件

### 5.1 消费者与统一条件

| 消费者 | 索引范围 | 所需 n/缓冲 | 方向 |
| --- | --- | --- | --- |
| 注册新条目 `sub_835BA0` | action 0..31，dword0/1 | n≥32；binding | 读 |
| 注册同名 `0x835E58..` | action 0..31，dword0/1 | n≥32；binding | 写 |
| Join flush `sub_A1EE10` | action 19 + `+0xC0[19]`/`+0xC4[19]` | n≥20 且两份 float 缓冲≥20；或覆写 slot21 | 读写 |
| query 12/13/14 | `a<n` | n 与两份 float 缓冲一致 | 读 |
| common `sub_A1F280` | 当前 n | 两份 float 缓冲各≥n | 读写 |
| UI `sub_A243C0` | 传入 action | n≥action；binding | 读 |

统一条件（保留原生注册路径的前提）：

```text
n >= 32
32 个已初始化 16B binding 元素（4 dword/元素；-1 = unbound）
+0xC0: 32 个 float current 缓冲（非空、清零）
+0xC4: 32 个 float previous 缓冲（非空、清零）
+0xC8=-1, +0xCC=4；flags bit1=1, bit0/bit3=0
+0x20..+0x3C=0；+0xE8/+0x368=0；扩展区清零
slot3 首次 insert 前返回稳定非空字符串
```

- 反例：n=20 满足 Join action19，但注册 helper 访问 action20..31 → 越界；n=0+slot21 覆写仍不能保护注册。slot21 覆写只能处理 Join flush 一条路径。
- 仅把 `end` 改成 `begin+0x200` 不等于初始化 32 个元素/缓冲。
- 若必须小 n/空 vector：需拦截/替换 notification 路径或覆写触达 binding 的全部槽，属 DESIGN_ONLY；当前六槽基线不包含该能力。
- 同名写回会覆盖 binding dword0/dword1；名称唯一性策略与容量条件必须同时满足。

## 6. K04：cleanup、重复 Init、dtor 与有效基类状态

### 6.1 直接调用者

| 函数 | 直接 code callers | 含义 |
| --- | --- | --- |
| cleanup `0xA1E2C0` | `sub_9017C0`(Net dtor)、`sub_A1E1B0`(Base dtor)、`sub_B11862`(Base dtor funclet)、`sub_A220F0`(Gamepad dtor ×2)、`sub_A22430`(Gamepad slot2)、`sub_A6C390`(Keyboard dtor ×2) | 均作用于对应派生/基类对象；vtable slot2 在四张表中都存在 |
| Init `0xA1E210` | `sub_A22190`(Gamepad Init)、`sub_A6C3E0`(Keyboard Init) | 只在构造路径直接调用；vtable slot1 可被外部间接调用 |
| Base dtor `0xA1E1B0` | 无 code caller | 仅 Base vtable slot0；自有 slot0 覆写可拦截 vptr 分派 |
| vector dtor `0x4B3950` | 68 条（含四个设备 dtor 与其它 vector 用户） | 对自有对象只经上述 dtor 到达 |
| slot5 `0xA1E430` | 无 code caller | Init 内虚调或外部 vtable+0x14 分派 |

### 6.2 资源不变量与反例

- 反例 1：cleanup 后 `+0xC0/+0xC4` 为 0；common 在 n>0 时 `memcpy/memset` 空指针 → 崩溃/越界。**“对象常驻”不能推出“缓冲有效”。**
- 反例 2：重复 Init 重新分配两缓冲（旧缓冲泄漏），slot5 还可能改写/扩容 `+0x10` vector；长度与外部缓存可能不一致。
- 反例 3：Base dtor 若被调用（当前无直接 caller；vtable slot0 被自有覆写拦截），会写回 Base vptr、释放缓冲/vector、可能 free 对象，6 槽覆写全部丢失。
- L2 修订：`NEVER` 只保护对象/control 内存；L2 需要“无原生路径对自有对象调用 Init/cleanup/Base dtor/slot5”作为附加条件。
- L3 修订：退役后覆写槽中性仍成立；继承槽需要缓冲有效；slot30 的存活信号与 slot3 名称不适用“中性=0”（见 §8.2）。

可选处置（DESIGN_ONLY，需契约修订）：覆写 slot1（拒绝/受控重复 Init）、slot2（不释放自有缓冲）、slot5（限制默认模板重拷贝）；保持对象不导出并限制原生 caller 集合。未隔离的 vtable 间接调用者保留 `UNKNOWN`。

## 7. K05：insert/grow 的 EH 状态与锁恢复

### 7.1 FuncInfo 解析

| 函数 | FuncInfo | maxState | unwind | try/handler |
| --- | --- | --- | --- | --- |
| insert `sub_A20DF0` | `0xBD21A8` | 2 | [0] `0xB11650` 析构按值 pair；[1] `__std_terminate` | 无 |
| remove `sub_A20EB0` | `0xBD21DC` | 3 | [0] `0xB11680` 析构 pair；[1] toState=0+terminate；[2] terminate | 无 |
| grow `sub_A21630` | `0xBD22F4` | 3 | [0] 无；[1] toState=0 `0xB11780` 析构局部；[2] 无 | try states 0..1，catch-all `0xA217CE` |
| `sub_A21880` | `0xBD2354` | 1 | [0] `0xB117B0` 析构局部 | 无 |

- grow catch-all `0xA217CE`：`sub_A21550(var_2C,var_14)` → `sub_A21800` 逐元素 ReleaseStrong（释放已构造的部分新 range）；`sub_426940(var_24,var_20)` 释放新 buffer；`_CxxThrowException(0,0)` 重抛。
- `sub_4242B0(pair)`：control 非空且 `vtable+0xC` 为真 → `n79_0(pair)`（pair 析构）。
- insert/remove 的 unwind map 中**没有 manager mutex unlock action**；lock/unlock 都是显式 vtable 调用，不是 RAII。

### 7.2 退出路径状态

| 出口 | 锁状态 | pair/buffer | 通知/副作用 | 可恢复性 |
| --- | --- | --- | --- | --- |
| 正常 | 已解锁 | 已发布 | 已执行 | 可继续 |
| append/grow 中 C++ 异常（长度/分配异常） | **锁内；元数据无 unlock → 异常逃出后 mutex 保持锁定**（STATIC_DERIVED） | pair 由 unwind 析构；grow catch-all 清理部分新 range/buffer；vector 未发布 | 未执行 | 不能安全重试 insert/remove |
| 小分配失败（`sub_40CF00` 返回 0 未检查） | 锁内 | 空指针写 AV（非 C++ unwind） | 未执行 | 进程级不可恢复 |
| 大分配失败 `invalid_parameter_noinfo_noreturn` | 锁内 | 不返回 | 未执行 | 进程终止 |
| TryAddStrong 失败 | 锁内 | `sub_902D60` 静默 `{0,0}`；grow 继续并释放旧 buffer，可能丢元素 | 未执行 | 无回滚 |
| 通知/回调中异常 | 已解锁 | unwind/terminate 处理 pair | 已发生的列表/binding 修改不回滚 | 需按 K01 状态处置 |

## 8. K06：shadow/mirror/object 指针一致性

### 8.1 反例

```text
初始 shadow=1, mirror=1
T1 AddStrong: shadow 1->2 （暂停，未写 mirror）
T2 AddStrong: shadow 2->3；写 mirror=3
T1 恢复: 写 mirror=2
最终 shadow=3, mirror=2；无后续操作时永久不一致
```

仅原子 shadow + 普通 mirror store 无法证明一致；原生 inline reader（若存在）会读到错误镜像。

### 8.2 候选算法 A1（DESIGN_ONLY）

```text
slot(mode):
  acquire 自有望短锁（扩展区 32-bit 原子，bounded CAS，不取原生 mutex、不回调）
  s = load(shadow strong/weak)          // 线性化点
  new = f(s, mode)                      // 饱和 0xFFFF；下溢/超限 -> PINNED
  store(shadow, new)
  store32(ctrl+4, (new.weak_sat<<16)|new.strong_sat)   // 一次对齐 32-bit store 同时写 +4/+6
  if last strong: store(ctrl+0x14, 0)
  release 自有望短锁
  return 与原生可观察语义对齐的值
```

- 线性化点：shadow 更新与镜像 store 都在同一短锁内；锁释放后 shadow 与 mirror 一致；32-bit store 保证原生 uint16 读取不会看到强/弱镜像的中间组合。
- 与 manager 锁无反向获取；不阻塞等待主线程；不在槽内回调/析构/释放。
- 对原生 inline reader/writer：若存在且不取同一锁，仍不能保证互通；O-RC-INLINE 未发现此类路径（有限范围），保留 UNKNOWN。
- 方案 B：直接使用原生 embedded Mutex 作为槽锁，最接近原生语义，但与契约 v2“自有槽不依赖它”冲突，需修订；方案 C（镜像仅 advisory）不满足当前契约文字。
- **结论**：当前“任意线程、不依赖原生 mutex、无阻塞”的文字下不存在可证明一致的纯原子算法；至少需要一处契约修订（A1 或 B）。

## 9. 原 16 项 O-* 引用范围

| 义务 | v3 处理 |
| --- | --- |
| O-ALLOC、O-CTOR | §4（K02）修正分配回绕与 Init 失败；其余引用 v2 §3 的范围 |
| O-CB、O-RC-INLINE | §8（K06）补充一致性；原生六槽事实引用 v2 §6，本轮未重读 |
| O-INSERT-ABI、O-REMOVE、O-NOTIFY | §3（K01）、§7（K05）、§6（K04）；其余引用 v2 §8 与 W06/W07 草稿 |
| O-ABI-0C | 引用 v2 §4：slot3 是 Base 的 `_purecall` 槽，自有实现必须覆写为稳定非空名称；退休不返回 0 |
| O-ABI-78 | 引用 v2 §4：slot30 是存活信号，基类为 purecall；**slot3 与 slot30 是 purecall，slot0 是 deleting dtor**（修正编号） |
| O-ABI-QUERY | §5（K03）：query 需 n 与缓冲一致；越界语义引用 v2 |
| O-BUF | §3、§5：注册路径读写 binding（非 `+0xC0`）；n≥32 统一条件 |
| O-RTTI | 引用 v2 §9 与 W08 草稿（本轮未重读） |
| O-KEY | 引用 v2 §7：计数器非原子、lookup 首匹配、miss 分配 `{0,new-control}`；注册同名写回是新增的 key 相关影响面 |
| O-JOIN、O-ACTION | §5：Join `+0x54(19)` 条件；Join ID/选择链引用 v2 §10 与 W09 草稿（本轮未重读） |
| O-THREAD | §7、§8：EH 锁恢复与计数并发；线程事实引用 v2 §8.5/§8.7 |

## 10. 协调器契约修订草案（DESIGN_ONLY，不覆盖契约 v2）

| # | 契约 v2 旧条款 | 建议条款/差异 | 依据 | 未决 |
| --- | --- | --- | --- | --- |
| D1 | §4 六槽覆写；越界 action 行为与 Base 一致 | 增加“F4 必须提供 n≥32 的 32 个有效 binding 元素 + 两份 32-float 缓冲”；或若采用小 n，必须同时处理注册 notification 路径（非仅 slot21） | K01/K03 直接数据流 | 运行期默认 binding 值；是否拦截 notification 属额外设计 |
| D2 | §4/§5 槽内无阻塞、不依赖原生 mutex | 允许自有望短锁（A1）或允许使用原生 embedded Mutex（B），明确锁范围与线性化点；镜像用对齐 32-bit store 同时写 +4/+6 | K06 反例 | 是否存在原生 inline reader |
| D3 | §4 六槽基线 | 评估新增 slot1/slot2/slot5 的受控覆写（拒绝重复 Init、不释放缓冲、限制 slot5 重拷贝），或明确接受间接调用风险 | K04 调用者/反例 | vtable 间接 caller 未隔离 |
| D4 | §6 失败处置 | 明确 Init 分配失败/回绕不可恢复；insert/grow 锁内 C++ 异常无 unlock action，不能重试；小分配失败是 AV、非 C++ unwind | K02/K05 | 运行期 handler 行为、AV 分类 |
| D5 | §4 slot30 返回非 0；退休输出中性 | 明确“slot30 存活信号必须非 0（退休也不能用 0 表达）”；slot3 名称保持；只有 slot12/13/14 在退休后返回中性（false/0.0） | v2 §4 + 本轮编号修正 | 退休后 poll 的实际调用窗口 |
| D6 | §5 计数所有槽可在任意线程 | 保留；补充“镜像仅对取同一锁者一致；原生 inline 路径未证明” | K06 | 同上 |

## 11. 残余问题与静态边界

- 运行期未观察：默认 binding 实际 count/内容、malloc(0)/handler 行为、worker/main 实际交错、现场 UI/日志/持久化、现场 COL/typeid。
- 可继续静态展开但本轮未做：其它 `sub_A21630` unwind state 的逐指令映射、`sub_B11862`/`sub_A22430` 等派生 cleanup 覆盖的完整路径、vtable slot1/2/5 的全部间接 caller、原生 inline 计数旁路的 points-to。
- 设计未采纳/未实现：D1–D6 的契约修订、slot1/2/5 覆写、计数同步方式 A1/B。
- 原范围外残余：未解析计算地址、未知模块、完整 points-to、其它 Net/teardown caller（引用 v2 §12）。
- key 非原子/F7 竞争、注册与 assignment 非原子、ordinal 1、generation、重复 poll 与 Join/confirm exactly-once 继续引用 v2 的限定结论；K01 新增“同名写回可覆盖 binding”面。

## 12. 唯一一次整体静态文档审阅

本节按工作设计 v2 §7 执行一次整体静态审阅（读取 v3 主报告、证据 v3、契约草案与导航更新），不运行程序/校验脚本。

1. **注册两条分支**：新条目 device→record、同名 record→device binding；`+0x10` binding 与 `+0xC0/+0xC4` 状态缓冲已分列；stride 16B/8B、元素数 32、实际宽度 0x100（不是 0x200 连续复制）。通过。
2. **allocator/Init 整数**：malloc 32-bit 请求、`lo+4` 回绕（`0xFFFFFFFF→3`）、`hi!=0` 清零、无符号 `mul`/OF、正常 32 元素对象不可达溢出分支均已说明；无“溢出一定失败”外推。通过。
3. **容量一致**：F4–F6、注册、Join、query/common 共用 n≥32 条件表；slot21 覆写明确不能替代注册 32 项访问；n=20/n=0 反例保留。通过。
4. **生命周期**：对象/control 常驻与缓冲/vector/vptr 有效分开；cleanup/Init/Base dtor 的直接调用者和未隔离间接调用者分列；L2/L3 改为有条件。通过。
5. **EH 与计数**：EH state/funclet/资源/锁按实际元数据列表；未展开项明确；计数给出候选算法、线性化点与永久不一致反例；不把不释放当一致性证明。通过。
6. **16 项 O-*、F1–F8、L1–L4、覆写建议与契约差异**：§9/§10 引用范围与修正一致；设计建议、继承证据、本轮新事实标签正确；purecall 编号已修正。通过。
7. **状态与历史**：主报告与证据 v3 状态一致；v1/v2、失败记录与 v2 §13 历史保留；无 gate、哈希校验、作者分离或多次验收安排。通过。

### 12.1 本次工作实际完成范围

- K01–K06 均形成直接事实或明确的设计推导：K01/K02/K03/K04/K05 为 `IDA_STATIC_CONFIRMED` 或 `STATIC_DERIVED`；K06 为 `DESIGN_ONLY` 候选算法 + 静态反例；未实现、未采纳契约修订。
- 产出：本 v3 报告、`out/cs-t0/virtual-device-owned-abi-static-evidence-v3.md`、07-virtual-device 导航更新（见 §13）。

### 12.2 纠正的错误与仍未完成

- 纠正：注册回调方向/字段/宽度（C1）、分配回绕（C2）、容量条件不统一（C3）、NEVER/lifetime 推理（C4）、EH 状态（C5）、shadow/mirror 不一致（C6）、purecall 编号与索引状态（C7）。
- 仍未完成：运行期默认 binding 值、malloc(0)/handler、vtable slot1/2/5 间接 caller、原生 inline 计数路径、EH 未展开 state、契约修订是否采纳、A1/B 计数方案选择、Join/线程/持久化等 v2 残余（§11）。

## 13. 07-virtual-device 导航更新

`docs/refactor/cs-t0/t0-05/07-virtual-device/README.md` 的“当前自有 ABI 静态工作（2026-10-12）”已更新：入口指向本 v3 与证据 v3，说明 `tx-00-06` 已执行、v2 的错误与修正、以及仍未解决的问题；历史设计 v1/v2、契约 v2 与原证据入口保留。

---

附：本报告不生成 promotion、解锁、实现资格或 gate 字段；全部结论按上述范围与标签使用。
