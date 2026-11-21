# tx-03-18 Luna：独立 Sprite 最终图像 pair 就绪

日期：2026-10-23（Asia/Shanghai）。目标是为单个独立 Sprite layer 区分图像加载标志与最终 image/control pair，并给出一个有生命周期前提的最小 adapter Readiness 条件。本文是静态逆向与设计推导；没有运行时 retain 失败观察，也没有声称 GL 纹理有效。

## 证据来源与口径

本轮 IDA 元数据只读确认：IDB 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，input path 为同目录 `isaac-ng.exe`，imagebase=`0x400000`，processor=`metapc`。VA 与 RVA 换算为 `RVA = VA - 0x400000`；对象字段偏移与 vtable 内偏移不是 RVA。没有比对磁盘映像，也没有读取、计算或校验哈希。

本轮唯一合格的 Luna 导出是 [luna-image-readiness-v2.json](../../../artifacts/cs-t0/tx-03-18-static/luna-image-readiness-v2.json)，使用的只读脚本为 [luna-export-static.py](../../../artifacts/cs-t0/tx-03-18-static/luna-export-static.py)。首次导出 [luna-image-readiness.json](../../../artifacts/cs-t0/tx-03-18-static/luna-image-readiness.json) 保留原样，但它的 JSON `tx` 字段仍写着 `tx-03-17`，所以不作为本轮元数据主证据。v2 将 `tx` 修正为 `tx-03-18`，仍为同一组当前 IDB 函数、原始指令和表项的只读导出；此修正是采集标签修复，不是运行测试或验收重跑。

`STATIC_CONFIRMED` 表示当前 IDB 可见的函数、表项或指令；`STATIC_DERIVED` 表示受正常顺序和对象生命周期前提约束的推导；`DESIGN_PROPOSAL` 是 adapter 建议；线程、并发或对象有效性未确认处标为 `UNKNOWN`。本文没有 `RUNTIME_OBSERVED` 结果。

## `+0x30` 与最终 pair 是两项独立状态

**STATIC_CONFIRMED。** `0x40C000`（RVA `0xC000`）逐 layer 处理，stride=`0xA0`。进入时若 layer `+0x30 != 0` 就跳过该层；否则按 layer `+0x08` 路径解析临时 image pair。候选 image 非空时，`0x40C073` 读取 vptr，`0x40C075` 读取 vtable `+0`，`0x40C077: FF D0` 才是实际间接调用；返回真后先将局部 `v19` 置 1（`0x40C080: C6 45 EF 01`），再尝试复制 pair 并调用 Sprite image-index helper。失败时仅将 `v19` 置 0。

之后函数另行初始化一个零 pair（`0x40C0E4`、`0x40C0EA`），从临时 control 的 vtable `+4` 尝试保留：`0x40C101–0x40C108` 取 control `+4`、调用并检查返回值。只有该次保留成功才把 image/control 放入 pair。pair 随后经 `0x408640`、`0x40C3B0` 交换到 layer `+0x98/+0x9C`；正常路径中的 `0x408640` 还将 image `+0x30..+0x3C` 的 sampler 值复制到 layer `+0x20..+0x2C`。最后 `0x40C146: 88 4C 03 30` 才把先前计算的 `v19` 写进 layer `+0x30`，并在 `0x40C14D: C6 44 03 31 00` 清 `+0x31`。

因此，`+0x30` 的值虽在函数末尾写入，却在最终 pair 保留前已经由临时 image 的 vtable `+0` 决定；它不取决于后面的 control 是否存在、`TryRetain` 是否成功，或 pair 是否最终留在 layer。可见分支包括 image `+0` 为真而 control 为空或保留返回 false；零 pair仍会交给最终 setter，`+0x30` 仍可写为 1。代码中存在此分支不等于已经观察到正常游戏会触发它。后续调用又会因 `+0x30` 非零而跳过该 layer，所以它也不能充当可重试的完整资源状态。

静态关键字节包括：`0x40C0FD` control 非空测试，`0x40C103: 8B 40 04` 取 control vtable `+4`，`0x40C106: FF D0` 调用，`0x40C10A: 74 2C` 在 false 时跳过 pair 填充；`0x40C13B: E8 00 C5 FF FF` 调最终 layer 写入 helper；`0x40C146` 写 `+0x30`。函数完整原始 bytes 与指令见上述 v2 导出。

## retain helper、getter 与图像虚槽的含义

**STATIC_CONFIRMED。** `0x40C550`（RVA `0xC550`）是 pair copy-constructor 形态的 helper，返回目标地址而非成功 bool。它先将目标 image/control 两个 DWORD 清零（`0x40C55B`、`0x40C561`），只有源 control 非空且其虚槽 `+4` 返回真时才写入两个源字段（`0x40C56D–0x40C57C`、`0x40C59F–0x40C5A2`）。所以源 pair 无 control 或 `TryRetain` 失败时，调用者得到空 pair，不能只检查 helper 返回地址当作保留成功。当前静态代码没有证明 retain 失败曾在运行时发生。

control 表 `0xB1A6E0`（RVA `0x71A6E0`）六槽的原始四字节表项在 v2 中逐项导出。其 `+4` 指向 `0x40C6B0`（RVA `0xC6B0`）：该函数在 control `+4` 的 u16 strong count 为零时返回 false；非零时调用 `+8` 增加 strong count 后返回 true。`+8` 是 increment，`+0x0C` 是 release。`0x40C7F0` 分配 24-byte control，`0x40CC10` 正常初始化 `+0x04/+0x06` 为 1/1 并装入 mutex；`0x40C7F0` 随后写 control `+0x14 = image`。释放路径 strong `1→0` 时调用 Image 虚槽 `+0x34` 并清 `+0x14`。这是正常 control ABI，不是任意指针可安全读取的证明。

**STATIC_CONFIRMED。** `0x408590`（RVA `0x8590`）不是无副作用 getter。它以 layer `+0x98` 的 image 比较 layer `+0x20..+0x2C` 与 image `+0x30..+0x3C`；差异时调用 Image vtable `+0x44/+0x48` 后才更新 sampler 缓存，最后再经 `0x40C550` 复制 layer pair 到输出。PNG 表 `0xB9FF84` 的有效函数项范围按 24 个槽处理（`+0x00..+0x5C`）。v2 为边界查看多导出的 `+0x60` 字是 `B4 14 BB 00`，指向 `0xBB14B4`，IDA 当前将目标标为 `??_R4ImageJpg@Graphics@KAGE@@6B@`，并识别为非函数的 RTTI Complete Object Locator 数据；它不是 PNG 的第 25 个 vslot，本报告不把它并入 PNG 表，也不借此认定相邻 JPG 表的布局。PNG 的 `+0x44/+0x48` 分别指向 `0xA69A50/0xA69AB0`，二者直接调用 `epoxy_glBindTexture` 与 `epoxy_glTexParameteri`。因此调用 `0x408590` 可能改变共享 image 和 GL 状态；输出 pair 的保留失败不会撤销先前副作用。它检查的是 image 指针，不先验证 control、一致性、PNG 类型或 loaded 位，不能作为安全探测器使用。

PNG Image vtable `0xB9FF84`（RVA `0x79FF84`）的 `+0` 指向 `0xA123B0`（RVA `0x6123B0`）。该函数原始指令 `0xA123B0: 8B 41 08; 0xA123B3: 83 E0 01` 读取 Image `+0x08` bit 0 并返回 bool；这表示 Image 自身的逻辑 loaded 标志，不表示任意类型 Image 的 vtable `+0` 都有相同含义。

PNG `0xA12B80` 的首个相关写点 `0xA12B85: 83 4E 08 01` 设置该 bit，再更新 sampler 并调用后续虚槽。PNG 上传 helper `0xA69B30` 将纹理名输出到 Image `+0x58`，调用 `epoxy_glGenTextures`、`epoxy_glBindTexture`、`epoxy_glTexImage2D`，末尾 `0xA69BD6: B0 01` 固定返回 1；所读路径没有用 GL 错误或 readback 分支改写结果。故 `Image+0x08 bit0` 说明引擎逻辑上接受了对应加载路径，不证明当前 GL context、驱动上传、纹理内容或可绘制性。

## 最小 adapter Readiness 建议

**DESIGN_PROPOSAL：** 将一层的受限状态命名为 `FINAL_PAIR_NATIVE_LOADED`。它只代表“在已知正常 owner 顺序下，最终 layer pair 与受支持 PNG Image 的引擎 loaded 标志一致”。检查必须在 Sprite/layer 已由 adapter 持有、仍处于有效代次、图像资源 owner/graphics thread 与调用顺序已知且没有并发替换/退休时进行；否则返回 `UNKNOWN`，不从 raw pointer 探测寿命。

在上述前提下，完成正常加载调用后只检查最终 layer 字段：

1. layer `+0x98` image 与 `+0x9C` control 都非空。
2. control vptr 精确匹配已知 `0xB1A6E0`，且 control `+0x14 == image`；control strong count 大于零只能在对象已由本地 owner 固定、读写顺序受控时作一致性检查。
3. image vptr 精确匹配当前明确支持的 PNG 表 `0xB9FF84`，并确认其已核实的 vtable `+0` 结果为真，即 Image `+0x08 bit0 == 1`。
4. 任一步为空、类型不支持、关系不一致或保留失败则不发布该 layer；线程/代次/lifetime 不明时保留 `UNKNOWN`。

`0x40C550` 的成功保留可用于在正常 owner 线程中取得额外强引用；失败必须按空 pair 处理。不过它内部的 control `+4` 检查与 `+8` 增量分成两个锁区间，不能把它当成跨线程原子 `retain-if-alive`。在没有先行有效 owner 引用时，解引用 raw control/image 以检查 vptr、`+0x14` 或 strong count 可能本身就不安全。精确 vptr 与字段相等也不能证伪悬空指针、ABA/复用或错误 generation。

这个状态不使用 Sprite `+0x109` / `IsLoaded` 或 layer `+0x30` 作 readiness；不把 `Image+0x58` 非零当成 GL 成功；也不表示资源路径就是 adapter 期望路径、其它 layers 已就绪、尺寸满足未来产品 profile 或图形后端真的可绘制。PNG 解码会有尺寸设置、位深、通道和 texel 分配失败分支，但这些检查进入的仍是本地加载逻辑；若产品 profile 另需尺寸范围，须单独定义和读取，不能拿它们替代 pair 一致性或 GL 验证。非 PNG vtable 需逐类型确认语义后才可纳入支持集合。

## UNKNOWN 与执行边界

- 实际 renderer/GL owner 线程、context 存活窗口、资源加载或 release 是否经过其它 worker，仍 `UNKNOWN`。
- 本轮没有看到、触发或模拟 retain 失败；分支结论是当前静态控制流，不是运行时频率或安全性观察。
- control 计数跨线程并发、进程关闭、图像缓存回调重入、坏指针及 native EH/OOM 恢复范围未由本轮闭合。
- 静态表、函数与输入路径不能代替运行时 GL 验收。未运行构建、测试、游戏、实验或 native helper，也未修改 IDB。

相关资料：[tx-03-13 图像资源与 control ABI](tx-03-13-sprite-image-resources.md)、[tx-03-15 换图与 Reload](tx-03-15-sprite-replacement-reload.md)、[tx-03-16 独立 Sprite 展示字段](tx-03-16-independent-sprite-display-fields.md)、[tx-03-17 Sprite owner retirement](tx-03-17-independent-sprite-owner-lifecycle.md)。

