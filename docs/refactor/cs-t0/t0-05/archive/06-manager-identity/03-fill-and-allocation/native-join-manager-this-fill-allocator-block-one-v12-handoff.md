> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this allocator-block exact-one v12 handoff

## 结论

`memset` exact1 `0x006ED13B` 已按独立静态验证口径闭合为：

`VERIFIED_ALLOCATOR_BLOCK_RANGE_AND_SLOT_NONOVERLAP`

本证书只允许合并 `0x006ED13B` 这一行的范围证明；不重新裁决 `0x006E6F3B`，不提升 whole-image alias，也不提升 manager-this 身份。

## 冻结绑定

- bulk memset v3：`F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB`
- independent range audit v5：`34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`
- verified fill-three v7：`242C05FD1AD0FF9D4851EC7A70EF59095B15F592C72A73DF4F4766BD17508807`
- target PE：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- exact call bytes：RVA `0x006ED13B` = `E8 A5 34 00 00`，目标 `memset` VA `0x00AF05E5`

fill-three v7 只复用同一条 allocator metadata invariant：`sub_AED6A0` 选择 metadata，`sub_AEDA50` 以实际 block stride 生成 freelist 节点。v12 另外独立验证了本行的 `sub_AEDE10` 两种 page case、`sub_AED000` 入口全集、目标分支和 slot 不相交。

## 普通页闭包

令 `U` 为 `sub_AEB3F0` 给出的 page actual usable bytes，`S = DWORD[metadata+24]`。

当 `uint32(metadata+24) < 0x80000000`：

1. `sub_AEDE10` 直接返回 `S`。
2. `sub_AED9B0` 选择同一 `S` 作为 block stride。
3. `sub_AED6A0` 写入 `N = floor(U/S)` 到 `metadata+12`。
4. `sub_AEDA50` 只发布 `base + S*i`，且 `i < N`。
5. 因而 `base + S*(i+1) <= base + S*N <= base + U`，返回块 usable extent 至少为 `S`。

所以本行 `offset 0 + count S <= block extent S`。

## 大页 high-bit 闭包

当 `uint32(metadata+24) >= 0x80000000`：

1. `sub_AEDE10` 在 `0x00AEDE31` 调用 `sub_AEB3F0`，返回 actual usable `U`。
2. `sub_AED9B0` 同样以 `U` 而不是 high-bit sentinel 作为 stride。
3. 大页构造得到 `N = floor(U/U) = 1`；`sub_AEDA50` 发布唯一的 `base` 节点。
4. 返回块 extent 为 `U`，所以 `offset 0 + count U <= U`。

普通页和大页由 `< 0x80000000` / `>= 0x80000000` 完全分割，没有开放 page class。

## metadata producer 与 freelist 闭包

- `sub_AED6A0` 在精确目标中只由 `sub_AED000` 的 `0x00AED0A5` 和 retry `0x00AED0C0` 调用。
- fresh metadata 经 `sub_AEDB40` 或 `sub_AEB300`；后者的完整 page producer 分支是 `sub_AEC9D0`（普通页）和 `sub_AEBDB0`（大页）。
- existing bin metadata 是上述 fresh 路径的复用；`sub_AEDD50` / `sub_AED380` 只重链 metadata，不破坏 `+12/+16/+24` 不变量。
- `sub_AEC9D0` / `sub_AEBDB0` 汇合到 `sub_AEBE30`。复用块来自其 OS reservation pool；fallback 链 `sub_AE59B0 -> sub_AE81B0 -> sub_AE8C00 -> sub_AE8AD0 -> sub_AE8E90` 最终以 null requested address 调用 `VirtualAlloc`。
- `sub_AED9B0` 保证 batch count `q <= N-k`；`sub_AEDA50` 是其唯一的 freelist node publisher rel32 目标。
- `sub_AE6EE0` 返回 `metadata+16` 的 head，并把 head 推进到 `DWORD[head]`，所以它返回的只能是上述已证明节点。

## exact target 与其它入口

目标分支条件是 `low8(a3) != 0 && DWORD[metadata+24] == 0`。它先在 `0x00AED103` 用 clear flag 0 调 `sub_AE6EE0`，再在 `0x00AED10B` 调 `sub_AEDE10`。`sub_AEDE10` 只有这一个 code xref，没有 data xref。

同一计数有两个等价清零实现：

- `byte_C6A080 != 0`：`0x00AED122` 的 `rep stosb`；
- `byte_C6A080 == 0`：`0x00AED13B` 的 `memset(buf, 0, count)`。

该 exact guard 还把本次 count 精化为 0；v12 没有用这个零长度捷径替代普通页/大页的一般性闭包。

若 `a3 == 0` 或 `metadata+24 != 0`，`0x00AED15A` 把原 clear flag 传给 `sub_AE6EE0`：zero flag 不清零，nonzero flag 使用同一已证明 stride 规则，不会改变 metadata invariant。

## 入口与 dispatch 闭包

IDA 只读证据和整段 `.text` rel32 disk scan 一致：`sub_AED000` 恰有 14 个 direct control-transfer xref，其中 13 个 `E8`，一个 `0x00AE6FA2` 的 `E9` tail transfer；PE 中不存在 `sub_AED000` 地址取值。

allocator dispatch table `0x00BF8000` 的 9 个槽是 wrapper 入口，slot 0 为 `sub_AE6EE0`。hash-bound fill-three v7 的 1,924,021 条 defined-instruction semantic operand scan 对 10 个 table fields 命中 0；v12 又绑定 table bytes、raw VA/RVA diagnostics、export 集和 wrapper/direct xref 全集。在 exact target image、无外部模块或注入的范围内，indirect consumer count 为 0。

## slot 不相交

slot `[0x00C71678,0x00C7167C)` 位于目标映像已映射的 `.data` 区间 `[0x00BF8000,0x00C9CAA4)`。

allocator page 的根来源是独立 OS reservation；null-address `VirtualAlloc` 不能与已经映射的 PE image 重叠，pool reuse 继承这一分离。普通页/大页算术又证明 `[buf,buf+count)` 完全包含在该 allocator page 的 block extent 内。因此：

`[buf,buf+count) ∩ [0x00C71678,0x00C7167C) = empty`。

## 产物与复现

- JSON：`out/cs-t0/native-join-manager-this-fill-allocator-block-one-v12.json`
- reproducer：`out/cs-t0/analyze-manager-this-fill-allocator-block-one-v12.py`
- handoff：`docs/refactor/cs-t0/t0-05/native-join-manager-this-fill-allocator-block-one-v12-handoff.md`

创建命令：

```powershell
python out/cs-t0/analyze-manager-this-fill-allocator-block-one-v12.py
```

已有产物确定性复核：

```powershell
python out/cs-t0/analyze-manager-this-fill-allocator-block-one-v12.py --verify-existing
```

reproducer 绑定三份输入 hash、target hash、exact1 membership、call bytes、19 个关键函数 blob、整段 `.text` rel32 xref universe、dispatch table/address-taken/export diagnostics、普通页/大页 case、freelist 算术和 slot 不相交。内建 9 项 mutation suite 必须全部 fail closed。

## 权限边界

本任务只使用 IDA Pro MCP 只读查询和 PE 磁盘读取；没有启动游戏、打开/读取游戏进程、附加调试器、修改 IDB、修改生产代码/测试/冻结证据，也没有执行 runtime mutation 或 manager-this promotion。
