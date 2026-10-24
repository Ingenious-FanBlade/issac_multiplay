# tx-03-15 Luna：Sprite 换图与 LoadGraphics

日期：2026-10-23（Asia/Shanghai）。类型：静态逆向子任务。范围限于 `ReplaceSpritesheet`、`LoadGraphics`、layer pair helper、参数 ABI、layer+`0x30/+0x31` 状态和失败后的对象状态。未追 Reload／ANM 重建、完整 draw 可见性、图像格式细节或 GPU／线程闭包。

## 结论

`ReplaceSpritesheet` 是注册给 Sprite 的原生成员回调 `0x40BF30`（RVA `0xBF30`）。它按有效 layer ordinal 改写该 layer 的路径字符串并清 layer+`0x30`，让后续 `LoadGraphics` 能再次处理；它本身不解析路径、不加载图像、不调用 pair helper，也不替换 layer 当前的 image/control pair。路径相同或 ordinal 无效时返回 false；路径不同并完成赋值时返回 true。返回值表示路径是否被接受／更新，不能当成资源加载成功。

`LoadGraphics` 的小型原生 thunk `0x40BFF0`（RVA `0xBFF0`）进入 `0x40C000`（RVA `0xC000`）。后者循环 Sprite 的 layer 表；layer+`0x30` 非零时跳过该层。处理该层后，只有候选 image 非空且 image vtable `+0x00` 返回 true 才把 `+0x30` 设为 1，否则为 0；随后将 `+0x31` 写成 0。`+0x31` 在这条处理路径中未被读取，含义未知。`LoadGraphics` 因此是受逐层状态控制的处理／重试入口，不是无条件强制重载。

如果新路径已写入而 `LoadGraphics` 尚未处理该层，旧 pair 仍留在 layer+`0x98/+0x9C`；不可据此推断当前是否可见。若 `LoadGraphics` 处理了该层，代码先处理原 pair，再关联候选 pair；失败时不会恢复原 pair。失败留下新路径、`+0x30=0`、`+0x31=0`，可由后续调用重试。候选控制块是否被接入由另一个虚函数条件决定，故不能将所有失败分支概括为 pair 两字段皆零。

## 数据与证据口径

活动 IDB 的 input path 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，imagebase=`0x400000`。本报告 RVA 均由 `VA - 0x400000` 计算；Sprite／layer 字段偏移、layer 表项索引不是 RVA。主原始导出含函数完整指令 bytes、伪代码及 IDA xrefs：[luna-sprite-replacement.json](../../artifacts/cs-t0/tx-03-15-static/luna-sprite-replacement.json)；比较 helper 的补充导出为 [luna-sprite-path-compare.json](../../artifacts/cs-t0/tx-03-15-static/luna-sprite-path-compare.json)。可重复的只读导出脚本为 [luna-export-sprite-replacement.py](../../artifacts/cs-t0/tx-03-15-static/luna-export-sprite-replacement.py) 与 [luna-export-sprite-path-compare.py](../../artifacts/cs-t0/tx-03-15-static/luna-export-sprite-path-compare.py)，支持 `TX0315_LUNA_OUTPUT` 并拒绝覆盖已有文件。

`STATIC_CONFIRMED` 表示 IDB 中可见的指令、伪代码、表字段访问与 xref；`STATIC_DERIVED` 表示据此解释的路径／pair 状态；运行时分支、Lua error／异常展开的完整契约及最终像素结果未观察。没有计算或检查哈希，没有改 IDB／目标代码，没有执行原生 helper。

## Sprite 方法注册与 ABI

Sprite 注册函数 `0x866960`（RVA `0x466960`）在 `0x86731A`（RVA `0x46731A`）装入字符串 `ReplaceSpritesheet`，接着在 `0x86731F` 推入回调 `0x40BF30`，并于 `0x867324` 调 `0x89EF10`。紧邻的 `0x86732F`（RVA `0x46732F`）注册 `LoadGraphics`，推入 `0x40BFF0` 并于 `0x867339` 调 `0x89F030`。原始调用点 bytes：

| VA（RVA） | bytes | 直接所见 |
| --- | --- | --- |
| `0x86731A`（`0x46731A`） | `BA E0 04 B7 00` | `EDX` 指向 `ReplaceSpritesheet` 名称 |
| `0x86731F`（`0x46731F`） | `68 30 BF 40 00` | 推入 callback `0x40BF30` |
| `0x867324`（`0x467324`） | `E8 E7 7B 03 00` | 調 `0x89EF10` 注册 helper |
| `0x86732F`（`0x46732F`） | `BA F4 04 B7 00` | `EDX` 指向 `LoadGraphics` 名称 |
| `0x867334`（`0x467334`） | `68 F0 BF 40 00` | 推入 callback `0x40BFF0` |
| `0x867339`（`0x467339`） | `E8 F2 7C 03 00` | 調 `0x89F030` 注册 helper |

这些是 Lua binding 安装点，不是 Lua 脚本源码。主审补查 `89EF10/89F030` 后确认：native 函数指针存入 userdata，实际 Lua closure 为 `8A99D0`（Replace）及 `8A9740`（Reload／LoadGraphics），证据见 [primary-binding-context.json](../../artifacts/cs-t0/tx-03-15-static/primary-binding-context.json)和 [primary-lua-wrappers.json](../../artifacts/cs-t0/tx-03-15-static/primary-lua-wrappers.json)。Replace closure 转换第 2 参数 integer、第 3 参数 string，调用 native 后将 AL 转为一个 Lua boolean；另一个 closure 调用 native 后返回零个结果。native `0x40BF30` 不是直接接收 lua_State 的入口，其有效 ABI 为 ECX=`Sprite*`，栈参数依次为有符号 layer ordinal、指向字符串对象的指针，返回值在 AL，末尾 `ret 8`。字符串对象布局由 `+0x10` 长度、`+0x14` 容量、容量阈值 `0x10` 与小字符串／堆指针分支推得，属于 MSVC `std::string` 风格的静态推导。

主审修订记录：子任务原先写“此路径未发现另一个 Lua 层 wrapper”，当时只追注册和 native 路径，范围不足；该否定结论不再采纳，原始导出保留。Lua closure 的新增证据由主审独立取得。

`0x40BFF0` 是 9 bytes 的 `__thiscall` thunk：`51 51 E8 09 00 00 00 59 C3`。它将 ECX 中的对象继续传给 `0x40C000`，并在栈上附带同一对象；`0x40C000` 末尾 `C2 04 00` 清理这一个栈词，伪代码中的第二参数没有被函数体使用。它不是按 layer 接收参数的方法。

## ReplaceSpritesheet：检查、路径比较和字段写入

`0x40BF30` 的前置检查为 `ordinal < 0 || ordinal >= *(Sprite+0x80)`，任一成立直接返回 false；在这之前没有解引用路径参数。有效时从 `Sprite+0x7C` 取 layer 表指针，以 `ordinal * 0xA0` 选表项，layer 对象的路径字符串在 `+0x08`。因此 `Sprite+0x7C` 是表起始指针字段、`Sprite+0x80` 是 layer 数量、`table + ordinal*0xA0` 是某个 layer 表项；三者不是同一个地址或偏移。

字符串 helper `0x40CAE0`（RVA `0xCAE0`）按长度和内容逐字节比较新旧路径：长度不同即 false；相同长度时按 DWORD 与尾部 byte 比较，完全相等才 true。调用处 `0x40BF81–0x40BF87` 从 `Src` 选择 inline／heap 字符串指针，EDX 放新长度，栈上传旧长度和旧路径指针。helper 返回“内容相等”；调用者在 `0x40BF91` 对非零结果跳到 false 返回。因此同路径返回 false，既不复制路径也不清 `+0x30`。它是字节比较，不做大小写折叠或 canonical path 比较。

路径内容不同时，`0x40CCD0` 将 `Src` 的数据与长度赋给 layer+`0x08` 的字符串对象；随后 `0x40BFB3` 写 layer+`0x30 = 0`，AL=1 返回。代码没有检查新路径是否为空：传入空字符串且与现有路径不同，会走字符串赋值并清 `+0x30`，native 返回 true。其本身不检查文件存在性。代码另有 `layer string address == Src` 的避免自拷贝分支，也会清 `+0x30`；但它位于字节相等检查之后，稳定有效字符串若传入同一原生 string 对象会先命中“内容相等并返回 false”。不能把后续分支当作可通过同对象参数强制重载相同路径的常规 API 行为。

失效 ordinal 和相同路径都不写 layer 字段。特别地，若相同路径 layer 的 `+0x30` 原为 0，该标志仍为 0，后续 `LoadGraphics` 仍可重试；若它原为 1，则仍为 1，后续 `LoadGraphics` 会跳过该层。`ReplaceSpritesheet` 不触碰 `+0x31`，也不清理或交换 layer image/control pair。

## LoadGraphics：逐层判断及 pair 更新

`0x40C000` 读取 `Sprite+0x80` 作为循环边界、`Sprite+0x7C` 作为 layer 表起点，每轮 layer 表项指针按 `0xA0` 增长。对每项先检查该 layer 的 `+0x30`：原始指令 `0x40C043`（RVA `0xC043`）为 `80 7C 03 30 00`，比较 `[layer+0x30]` 与 0；`0x40C048`（RVA `0xC048`）为 `0F 85 61 01 00 00`，非零即跳到循环尾。因此该位在这条函数中是 skip/eligible 状态，不代表 Lua Sprite 整体 `IsLoaded`，也不证明纹理已呈现。

标志为 0 时，函数从 layer+`0x08` 的字符串取得路径，调用 `0x9588A0` 生成候选 `{image, control}`。`0x9588A0` 对传入 C 字符串调用 `strlen`、经 `0x8F5AD0` 做资源路径解析，再到 `0xA118D0`。`0xA118D0` 先走资源 open/resolve helper `0xA17180`；如果它返回空，则把空 image pair 写到输出并返回。Replace 没有空路径拒绝，但静态证据不能证明空串在所有 resolver/Mod 搜索分支的最终路径结果。

`0x40C000` 的本地成功字节仅在候选 image 非空且 image 虚槽 `+0x00` 返回真时置 1；否则置 0。只有成功分支将 pair 送入 `0x40E520` 的 Sprite 图像索引。函数接着从 layer 取得旧 pair；旧 image 非空时，经 `0x408640` 暂时替换／释放当前 pair，并按旧 image 的资源路径调用 `0x40E740` 做索引清理。随后再次经 `0x408640` 关联候选 pair，最后写入结果标志。

关键写点及循环指令：

| VA（RVA） | bytes | 直接所见 |
| --- | --- | --- |
| `0x40C043`（`0xC043`） | `80 7C 03 30 00` | 读取当前表项的 `layer+0x30` |
| `0x40C048`（`0xC048`） | `0F 85 61 01 00 00` | 非零时跳过本层处理 |
| `0x40C146`（`0xC146`） | `88 4C 03 30` | 写回图像非空且 loaded 检查结果 |
| `0x40C14D`（`0xC14D`） | `C6 44 03 31 00` | 本层处理完写 `layer+0x31=0` |
| `0x40C1B0`（`0xC1B0`） | `81 C3 A0 00 00 00` | layer 表项步长 `0xA0` |
| `0x40C1B9`（`0xC1B9`） | `3B 8E 80 00 00 00` | 与 `Sprite+0x80` layer 数量比较 |

`layer+0x31` 在本函数中只有处理路径的写入，没有读取；若 `+0x30` 为非零而跳过，该次调用也不会清 `+0x31`。它的协议、语义和其它调用者不在本子任务闭包内。

`0x408640`（RVA `0x8640`）直接 helper 的原始结尾为 `C2 08 00`，即 ECX=`layer`，栈上按值传入两个 32-bit 词 `{image, control}`，`ret 8`。若 image 非空，它把 image+`0x30..0x3C` 四个 sampler 字段复制到 layer+`0x20..0x2C`；随后经 `40C3B0` 向 layer+`0x98` 复制／retain 候选并交换，内部释放旧目的 pair；`408640` 尾部再通过 control 虚槽 `+0x0C` 与全局回调条件销毁其按值 source pair。这是不同 owner 的两次释放，不能合称释放旧 pair。pair 的字段解释与 layer 表项／表起点分开：图像／控制块位于 layer+`0x98/+0x9C`，不是 Sprite+偏移。

对已写入不同路径的 layer，若接下来的加载检查失败，路径字符串不会恢复。`0x40C000` 仍走 pair 更新步骤并将 `+0x30` 写 0、`+0x31` 写 0；旧 layer pair 没有被失败回滚分支恢复。候选 control 虚槽 `+0x04` 的结果控制候选 pair 是否进入最终交换，因此失败后准确的 control 内容取决于 resolver／control 分支，但“原 pair 保留为当前 pair”的说法不受伪代码支持。`+0x30=0` 允许再次调用时重试。

主审补充：上述失败指候选 image 为空或 image loaded 检查返回假的正常返回路径。`+0x30` 来自最终 pair 关联前的候选检查，不验证 retain 后的目的 pair，不覆盖失效 control、OOM 或并发失败。`+0x31` 的其它消费者仍未知。

## 适用边界

这批证据证明 native `ReplaceSpritesheet` 改写路径并重置逐层 skip 位，随后显式调用 `LoadGraphics` 才进入资源解析／pair 更新。它不证明被替换图像一定可见；draw 还会应用独立的帧、layer 可见性、image/pair 等条件，且本任务未追完绘制判定。也没有从 `+0x31` 的清零行为推断它是脏标志、锁或 generation。

主审正在独立分析 `Reload` 与 ANM layer 重建；本子任务不据此覆盖其结论。本轮只做静态文档与原始导出，没有运行测试、构建、游戏或实验；整体静态文档审阅由主智能体统一执行一次。
