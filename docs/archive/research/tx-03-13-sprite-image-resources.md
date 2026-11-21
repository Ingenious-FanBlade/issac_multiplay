# tx-03-13：Sprite 图像资源、引用与图形后端

日期：2026-10-22（Asia/Shanghai）。类型：静态逆向。承接 [tx-03-12](tx-03-12-derived-render-sprite-lifecycle.md)，本轮目标是解释 layer 图像 pair 的 control ABI、两个图像缓存、加载失败标志及正常释放到图形后端的路径。按用户要求使用 GPT-6 Luna / max 子智能体研究 control，主智能体独立取证复核，并研究 loader、缓存回调和 PNG 后端。见 [Luna 报告](tx-03-13-luna-image-control.md)与 [主审记录](tx-03-13-primary-review.md)。

## 结论与范围

**独立 Sprite 的图像资源仍与进程内缓存和图形上下文相连。** PNG 的正常加载会直接创建／上传 GL 纹理，最后一个强引用释放会直接到图像删除析构并调用 GL 删除纹理。不能把加载、换图或退休交给任意网络线程，也不能把独立 Sprite 理解为资源完全私有。

本轮闭合了 ImageBase control 的六槽表、24 字节布局、正常强／弱计数分支，以及 `0xC7163C → 0xA12240 → 0xA121B0` 的缓存退休路径；还区分了 Sprite、layer、Image 和后端上传的成功语义。**实际 renderer owner 线程／当前 GL context、并发关闭、原生 EH／OOM 和其它格式完整解码仍 UNKNOWN。** 没有实现或启用 Replica／展示 adapter，没有执行资源加载实验。

## 证据口径

活动 IDB：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，记录的 input path 为同目录 `isaac-ng.exe`，imagebase=`0x400000`。本文 VA 使用该 IDB 地址，RVA=`VA-0x400000`；对象字段和虚槽均明确以 `+0x...` 表示。未比较磁盘文件内容，未计算或校验任何哈希。

`STATIC_CONFIRMED` 指当前函数／表项／原始指令直接可见；正常引用语义、所有权和适配影响属于 `STATIC_DERIVED`，不代替运行时观察。下文没有任何 `RUNTIME_OBSERVED` 结论。完整指令、bytes、函数 xrefs 和元数据保存在 [原始导出目录](../../../artifacts/cs-t0/tx-03-13-static/)。

## 从 layer 到实际图像

主审证据：[Sprite graphics 与辅助函数](../../../artifacts/cs-t0/tx-03-13-static/primary-image-initial.json)、[路径与图像 loader](../../../artifacts/cs-t0/tx-03-13-static/primary-image-loader.json)。

| 入口 VA（RVA） | 可见数据流与限制 |
| --- | --- |
| `0x40C000`（`0xC000`） | 遍历 Sprite+`0x7C/+0x80` 的 layer，stride=`0xA0`。跳过 layer+`0x30!=0`；从 layer 路径取图像，检查 image 非空及 image 虚槽 `+0x00` 的 bool，维护缓存与旧图像，再将 pair 关联到 layer+`0x98/+0x9C`。 |
| `0x9588A0`（`0x5588A0`） | 两个栈词为输出 pair 地址和路径 C 字符串，`ret 8`。构造原生字符串，经 `0x8F5AD0` 解析资源／Mod 路径，再调用 `0xA118D0`。所读路径存在多个搜索分支，未宣称全部 Mod 兼容。 |
| `0xA118D0`（`0x6118D0`） | 路径解析／存在性 helper `0xA17180` 无结果时返回空 image pair；先查询图形资源缓存，命中则复制引用；未命中按扩展名分配 `0x60` 字节 Image 对象、选择派生表，清理旧图形状态、打开 stream 并经虚槽加载。 |
| `0x408640`（`0x8640`） | ECX=layer，栈上是 8 字节按值传递的 `{image,control}`，`ret 8`；先复制非空 image 的 sampler 字段到 layer，再对 layer+`0x98` 调 `0x40C3B0`，最后销毁栈 pair。不能按伪代码把它声明成两个无所有权的裸指针参数。 |

`0xA118D0` 的原始 `ret 0x0C` 与调用点的三个压栈词均保留；已识别输出和路径，第三个栈词在本轮不赋业务语义，不提供可直接调用的完整 ABI 声明。`0x40C000` 也不是只有地址就能直接启用的 adapter 接口。

loader 的明确派生分支为 PNG、JPG／JPEG、BMP、PCX 及空扩展名的 ImagePlatform；未知扩展名没有创建对应 image。调用顺序为 image 虚槽 `+0x3C` 清理、stream、`+0x38` 加载；加载正常返回 false 时清理路径／stream并重置为空 image pair，true 时经 `+0x40` 完成 loaded 标记／sampler，再加入图形资源缓存。本轮深入 PNG 路线，不由共享表项推断其它格式的全部失败清理相同。

空 image pair 可能仍带一个控制空 image 的 control；不能把“image 为零”等同于“两字段均为零”，也不能仅检查 control 非空当成资源成功。

### 加载与绘制前取图像都可能调用图形后端

`0x408590(layer,outPair)` 会比较 layer+`0x20..0x2C` 与 image+`0x30..0x3C`，必要时调用 image 虚槽 `+0x44/+0x48` 更新 sampler，再复制 pair。它不是无副作用 getter。PNG 中两槽分别到 `0xA69A50/0xA69AB0`，直接调用 `epoxy_glBindTexture`、`epoxy_glTexParameteri`；不同 layer 共享 image 时也共享这些 image 字段和纹理对象。证据见 [后端函数](../../../artifacts/cs-t0/tx-03-13-static/primary-image-gpu.json)。

## ImageBase 引用控制块

主审独立导出 [control 函数和表](../../../artifacts/cs-t0/tx-03-13-static/primary-image-control-review.json)，与 Luna 的 [control 原始证据](../../../artifacts/cs-t0/tx-03-13-static/luna-control-core.json)分别取证。`0x40C7F0` 分配 24 字节，调用 `0x40CC10`，再写 `ReferenceCount<KAGE::Graphics::ImageBase>` 表及 image 字段。该 control 与自有 virtual device 的回收策略／实现不是同一个契约。

| control 相对位置 | 正常路径含义 |
| --- | --- |
| `+0x00` | vptr=`0xB1A6E0`，RVA=`0x71A6E0`。 |
| `+0x04` / `+0x06` | 两个 16 位计数；构造以 `0x00010001` 同时初始化为 1。依其销毁行为称为强计数／弱计数；弱计数初始的 1 维持强引用期间 control 的存在。 |
| `+0x08..+0x13` | 内嵌 12 字节 `KAGE::Mutex`；vptr=`0xB81C0C`，其相对 `+0x04` 为 flags、`+0x08` 为分配出的同步状态指针，即 control+`0x10`。 |
| `+0x14` | 所管理的 ImageBase 指针。强计数从 1 变 0 时调用其虚槽 `+0x34(1)`，正常返回后清零。 |

| control 虚槽 | 目标 VA（RVA）／表项 bytes | 原始正常行为 |
| --- | --- | --- |
| `+0x00` | `0x40CB50`（`0xCB50`）／`50 CB 40 00` | 删除同步状态；按栈上 flag 位决定是否释放 control 自身，`ret 4`。不在此槽替代 Image 删除。 |
| `+0x04` | `0x40C6B0`（`0xC6B0`）／`B0 C6 40 00` | 锁内检查强计数；为零则返回 AL=0；非零则解锁后调用 `+0x08` 并返回 AL=1。 |
| `+0x08` | `0x40C690`（`0xC690`）／`90 C6 40 00` | 加锁、递增强计数、解锁；不将其 EAX 当作资源成功 bool。 |
| `+0x0C` | `0x40C630`（`0xC630`）／`30 C6 40 00` | 若强计数非零则减一。原来为零或减后仍非零：解锁并 AL=1；仅 1→0 进入 Image 删除，再尾调 control `+0x14`，返回后者的 AL。 |
| `+0x10` | `0x40C610`（`0xC610`）／`10 C6 40 00` | 锁内递增弱计数。 |
| `+0x14` | `0x40C5C0`（`0xC5C0`）／`C0 C5 40 00` | 非零弱计数减一；1→0 解锁后调用 control `+0x00(1)` 并返回 AL=0；其它分支 AL=1。 |

除删除槽的栈上 flag 外，上述计数槽 ECX=control、无显式栈参。`+0x0C` 的 AL 是 control 是否仍存活的正常路径返回约定，**不是 image 是否已销毁**：强计数归零但还有弱引用时，image 已删而 AL 仍为 1。

计数改动虽有锁，但 `+0x04` 的检查和递增分两次加锁，未证明并发原子 retain-if-alive；16 位增量没有本轮已见的溢出拒绝分支。不得据“有锁”宣称跨线程对象寿命安全。Mutex 的无限等待路径进入 `EnterCriticalSection`，还使用额外 busy byte；本轮不将其描述成可任意重入的普通锁，也不证明 manager teardown 与正在引用者并发安全。

关键指令：`0x40CC42`（RVA `0xCC42`）`C7 46 04 01 00 01 00` 初始化计数；`0x40C649` 的 `66 89 46 04` 写回 16 位强计数；`0x40C666` 的 `FF 50 34` 调 Image 删除；`0x40C5F1` 的 `FF 10` 调 control 删除槽。这些均已由主审重读原始指令。

## 两个图像缓存与释放回调

原始证据：[cache 回调与增删](../../../artifacts/cs-t0/tx-03-13-static/primary-image-release.json)、[缓存摘除与初始化／关闭上下文](../../../artifacts/cs-t0/tx-03-13-static/primary-image-retire-context.json)、[节点删除](../../../artifacts/cs-t0/tx-03-13-static/primary-image-gpu.json)。共享 ANM 缓存仍是 tx-03-12 的另一层，不与下列图像缓存混称一个计数。

| 层次 | 本轮直接观察 |
| --- | --- |
| Sprite 图像索引 `0xC78EE8`（RVA `0x878EE8`） | `0x40E520` 把 image/control pair 加入索引。`0x40E740` 按路径查找，锁 control 读取强计数；大于 2 则保留，至多 2 时把节点 pair 重置为空 image，再删除节点。阈值是该函数的规则，不能当作任意时刻 count=2 就安全删除。 |
| 图形资源缓存 | `0xA11780` 查找、`0xA120F0` 插入；缓存节点自身持有 pair 强引用，使用全局 `0xC37990`（RVA `0x837990`）同步对象保护。key／路径维护见 `0xA12C00`，不能使用网络传来的原生 key 或指针。 |
| 全局函数指针 `0xC7163C`（RVA `0x87163C`） | 多个初始化点写入 `0xA12240`，例如 `0xA11712`、`0xA13036`；关闭点写零，例如 `0xA11741`、`0xA131A1`。当前 IDB 符号可能显示为 `n2_2`，本文以地址识别。静态数据区的 FF 字节不是正在运行的函数指针值。 |
| 回调 `0xA12240`（RVA `0x612240`） | 接收 pair 地址；control 非空时加锁读取 u16(control+4)、解锁。仅 residual strong==1 时调用 `0xA121B0(image)`。它不直接执行 Image／control delete。 |
| `0xA121B0`（RVA `0x6121B0`） | 锁图形资源缓存，按 image+`0x48` 查找并确认节点的 image 指针一致，再调用 `0x414D60` 摘除。 |
| `0x414D60`（RVA `0x14D60`） | 摘树节点并销毁 node+`0x14` pair；调用 control `+0x0C`，AL 为真且全局回调非空才再调用回调；最后释放节点内存。 |

**STATIC_DERIVED，正常无并发引用路径**：layer／临时 pair 减引用后，若只剩图形缓存的一个强引用，回调移除该缓存 pair；这次减引用可使 strong 从 1 到 0，进入 Image 删除。若还有 Sprite 索引或其它持有者，计数与回调条件不同。缓存清理也可能在换图／Sprite reset／其它资源释放时同步触发；不能把 `C7163C` 称为固定的帧末或 worker 释放队列。

初始化和关闭证据只解释函数指针何时赋值／清零，没有运行 manager。关闭后继续使用旧 pair、持锁回调／重入、全部别名及多线程最后释放的安全性均未证明。上轮“图像 worker／GPU 并发未知”现已细化为可见的同步调用链，实际线程归属仍未知。

## PNG 到 OpenGL 的具体路径

[Image 派生表与构造](../../../artifacts/cs-t0/tx-03-13-static/primary-image-tables.json)、[PNG 加载／标志](../../../artifacts/cs-t0/tx-03-13-static/primary-image-png-lifecycle.json)、[GPU 与删除](../../../artifacts/cs-t0/tx-03-13-static/primary-image-gpu.json)。PNG 表 VA=`0xB9FF84`（RVA `0x79FF84`），它与 control 表 `0xB1A6E0` 是不同 receiver 的表。

| ImagePng 虚槽 | 目标 VA（RVA） | 已读行为 |
| --- | --- | --- |
| `+0x00` | `0xA123B0`（`0x6123B0`） | 返回 Image+`0x08` bit0，即 Image 自身 loaded。 |
| `+0x34` | `0xA649D0`（`0x6649D0`） | 删除析构：对 Image+`0x58` 纹理名调用 `epoxy_glDeleteTextures`，调用基础清理，再按 flag 释放 `0x60` 字节对象。 |
| `+0x38` | `0xA64A50`（`0x664A50`） | PNG 签名／libpng 解码、尺寸／通道处理、texel buffer；调用本 Image 的 `+0x58`，调色板分支还调用 `+0x0C`。 |
| `+0x3C` | `0xA69A30`（`0x669A30`） | 直接 `epoxy_glDeleteTextures(1,&Image+0x58)`，再到 `0xA12AD0` 清路径、palette 引用并清 loaded bit。 |
| `+0x40` | `0xA12B80`（`0x612B80`） | 先设置 loaded bit，再同步 sampler／调用 `+0x5C`；PNG 的 `+0x5C` 是空函数。 |
| `+0x44/+0x48` | `0xA69A50/0xA69AB0` | 纹理绑定及 filter／wrap 参数，直接进入 GL。 |
| `+0x58` | `0xA69B30`（`0x669B30`） | ECX=Image，一个栈参为 texel 指针地址，`ret 4`；生成纹理名至 Image+`0x58`，bind、`glTexImage2D` 上传，释放 texel buffer，末尾固定 AL=1。 |

上传的原始调用点：`0xA69B94`（RVA `0x669B94`）`FF 15 78 16 C1 00` 调 `epoxy_glGenTextures`；`0xA69BC5` 的 `FF 15 FC 1E C1 00` 调 `epoxy_glTexImage2D`。析构删除点 `0xA64A08`（RVA `0x664A08`）与清理点 `0xA69A39` 均为 `FF 15 D4 1D C1 00`。这里的导入调用可能经过库分派，但在已读函数中没有把操作投递至另一个线程的队列。

PNG 解码存在明确的 null stream、签名不符、非 8-bit component、set_size、texel 分配／通道、libpng longjmp 等 false 路线。已读 setjmp 的错误分支会清部分解码资源并返回 0；**未证明所有早退、分配失败、SEH／C++ EH 和 libpng 状态都完整清理**。不以日志文本或这几个分支冒充完整错误恢复。

### 四种“成功”不能合并

| 状态 | 能说明什么 | 不能说明什么 |
| --- | --- | --- |
| Sprite+`0x109`／`IsLoaded` | tx-03-12 已确认 Load 正常到达尾部即置 1。 | 不证明 layer、图像或纹理完整。 |
| layer+`0x30` | `0x40C000` 用 image 非空且 image `+0x00` 为真设置；对应写点 `0x40C146`，随后清 `+0x31`。 | 不证明其它 layer 成功或 GL 驱动上传成功；true 时当前函数跳过该 layer，不能当无条件重载。`+0x31` 的完整协议未恢复。 |
| Image+`0x08` bit0 | loader 接受 `+0x38` 的返回后，经 `+0x40` 设置的逻辑标志。 | 不证明当前 context、纹理完整性或驱动接受上传。 |
| PNG `+0x58` 返回 AL=1 | 上传 helper 正常执行到末尾。 | 所读函数没有依据 GL 错误返回改为 false；`0xA69BD6` 的 `B0 01` 是固定成功值。 |

## 对展示适配的影响与下一项工作

以下是设计推导，尚未实现：在 renderer 仍有效、调用线程／context 已解释的原生时段创建／加载、换图和销毁 Sprite；网络侧只传本地可映射的资源身份、动画／layer 展示结果，不传 Image/control/纹理指针、原生 cache key。用明确的支持资源集合建立本地 owner，记录各层实际失败，资源未满足时停止发布该对象的绘制。不能以单一 IsLoaded 或存在 control 代替这些行为。

此次剩余的具体研究为：renderer owner 与 context 的初始化／帧内／退出调用边界；ReplaceSpritesheet、Reload／LoadGraphics 的替换与缓存重载关系；支持视觉集合的尺寸／动画／layer 最小应用字段；失败或代际切换时对象的停止绘制和退休顺序。Player 身体／装备组合、NPC 特殊几何和所有图像格式仍各有范围，不能因 PNG 资源链完成而宣称已覆盖。

全部编辑后仅做一次 [整体静态文档审阅](tx-03-13-primary-review.md)。本轮没有改生产代码／IDB，没有启动或附着游戏／调试器，没有注入、游戏输入、构建、单元测试、E2E 或其它运行实验。静态导出脚本可用新输出路径重复取证，但不能将其当作实机验收。
