> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this fill libpng exact2 静态证明交接 v18

## 结论与边界

本组件只裁决 `V4_LIBPNG_EXACT_STRUCT_ALLOCATION` 中两个 `memset` callsite：

- `RVA 0x006630F1 / VA 0x00A630F1`：`memset(exact malloc(0xB8), 0, 0xB8)`；
- `RVA 0x00664114 / VA 0x00A64114`：`memset(exact malloc(0x20C)+0x4C, 0, 0x1C0)`。

两行均为 `VERIFIED_STATIC_EXACT_ALLOCATION_NO_SLOT_OVERLAP`。这是只读静态结论，不包含运行时观察，不启动游戏，不附加调试器，不读取进程，不修改 IDB、生产代码、测试或既有证据。组件仅允许后续 aggregate 消费这两份行级 exact certificate；row、whole-image alias 和 manager-this promotion 均为 `false`。

机器结果：

- [`native-join-manager-this-fill-libpng-two-v18.json`](../../../../../../../../out/cs-t0/native-join-manager-this-fill-libpng-two-v18.json)，SHA-256 `2AA38CF80A19BA90EA53E6E7FB1FB393FEECF6CA4A34615A664E1CD93BBC49AE`；
- [`analyze-manager-this-fill-libpng-two-v18.py`](../../../../../../../../out/cs-t0/analyze-manager-this-fill-libpng-two-v18.py)，SHA-256 `33FF38288BF6DA9CBA86C3FC006E2F6BBC50C17E50ED60739AB0F61E9D0D4BA2`；
- analyzer 连续完成 create 与 verify 两次只读 IDA 重放，均报告 `rows=2 verified=2 unknown=0 mutations=22/22`。

## 冻结输入

证明绑定目标 `isaac-ng.exe` SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`、image base `0x00400000`、纠正后的可执行映射 `[0x00401000,0x00B18000)` 与槽 `[0x00C71678,0x00C7167C)`。输入链为：

- memset v3：`F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB`；
- rejected v4：`0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C`，只用于 exact2 集合成员关系，禁止使用其 narrative 作证明；
- independent range audit v5：`34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`；
- corrected whole-image component v2：`D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`；
- corrected whole-image handoff v5：`892640B97BEB9C727CEC90FDE7F19F86441DBBD255486C5269B70ABA909D9F28`。

## 行 `0x006630F1`

call bytes 为 `E8 EF D4 08 00`。`sub_A63000 [0x00A63000,0x00A63100)` 的完整函数 SHA-256 为 `030EB833C04B916F70FF2D3AD884D45A9925EBF3CC4005EDF5366722406AF652`；函数把 EDX 保存到 ESI，并以 `push 0xB8; push 0; push ESI` 调用 `memset`。

正常路径在 `0x009E877D/0x009E8782` 执行 exact `malloc(0xB8)`，经 `[EBP-0x30]` 原样传至 `0x009E89D9`。EH 路径由 unwind-map `0x00BD0888` 绑定 action `0x00B0FAC0`，后者把 `ECX=&[EBP-0x7C]` 并 split-tail 到 `0x009E8FC0`；`context+0x4C == [EBP-0x30]`，非空检查后在 `0x009E8FD7` 调用同一函数。

图像加载路径有两次独立 exact `malloc(0xB8)`，分别保存在 `[EBP-0x28]` 与 `[EBP-0x3C]`。错误分支 `0x00A64BF9` 直接进入 `sub_A97450`；正常分支为 `0x00A65024 -> sub_A973A0 -> 0x00A973D7 -> sub_A97450`。`sub_A97450` 对两个非空 receiver 分别在 `0x00A97462/0x00A97470` 调用 `sub_A63000`。

`sub_A63000` 的完整 code-xref universe 精确为 `{0x009E89D9,0x009E8FD7,0x00A97462,0x00A97470}`，data xref 与间接 caller 均为空；转发器 xref universe 也在机器件中精确冻结。局部 receiver 槽的所有写入只有 exact pointer 与 free 后清零；malloc 失败只传播零，所有 target call 均受非空检查支配。

数值闭包为 `start=0`、`count=0xB8`、`extent=available=0xB8`，末字节 offset `0xB7`。有效连续 `0xB8` 字节 malloc 对象蕴含 `base <= UINT32_MAX-0xB7`，所以 32 位地址不回绕。

## 行 `0x00664114`

call bytes 为 `E8 CC C4 08 00`。`sub_A63F30 [0x00A63F30,0x00A6415E)` 完整函数 SHA-256 为 `D7E5B7561B815F7EAE4CD6D445140122A624A23045379B0C622DC4849D838927`；该行执行 `memset(Block+0x4C,0,0x1C0)`。

`sub_A63A10` 在 `0x00A63A1E/0x00A63A3B` 精确执行 `malloc(0x20C)`，返回值在 `0x009E872D` 写入 `[EBP-0x7C]`。正常 caller 在 `0x009E89F5` 传入该槽地址；EH caller 沿上述 unwind/split-tail 在 `0x009E8FF2` 传入同一 context。`sub_A63F30` 内 Block 只有初始化零与 `*a1` 两个 producer，fill 由非空检查支配。

`&[EBP-0x7C]` 的地址逃逸也已闭合：`sub_9E8EE0`、`sub_9E8E90`、`sub_9E8D40` 只读取 `[this]` 并转发 pointee，不写 `[this]`；libpng error callback 经只读 `sub_9F3160(png+0x48)` 取得 error pointer，`sub_9E6E50` 只写 error object 的 `+0x50`。这些 helper 均由完整函数字节哈希绑定。

`sub_A63F30` 的完整 code-xref universe 精确为 `{0x009E89F5,0x009E8FF2}`，data xref 与间接 caller 均为空。数值闭包为 `start=0x4C`、`count=0x1C0`、`extent=available=0x20C`，且 `0x4C+0x1C0=0x20C`、末字节 offset `0x20B`。有效连续 `0x20C` 字节 malloc 对象蕴含 `base <= UINT32_MAX-0x20B`，所以 `base+0x4C` 及末地址均不回绕。

## 槽不相交与失败路径

两个 destination 均来自成功 CRT `malloc` 的 fresh live heap allocation；冻结槽位于 PE `.data` 静态映射 `[0x00BF8000,0x00C9D000)`。两种所有权域不相交，因此目标范围不能与 `[0x00C71678,0x00C7167C)` 相交。失败分配不会执行 fill；异常清理沿已绑定的同一 receiver 槽与非空条件，不产生替代 destination。

## 重放与验收

在加载了冻结目标且 IDB 完成分析的 IDA 实例中，以 IDA Pro MCP 的 `py_exec_file` 执行绝对路径：

`D:\projects\isaac_hook_reserach\out\cs-t0\analyze-manager-this-fill-libpng-two-v18.py`

脚本以只读 IDA API 检查目标哈希、段边界、83 个 exact anchors、14 个完整函数哈希、10 个 xref universes、冻结输入语义与 exact2 行证书。输出不存在时仅 create-new 机器 JSON；存在时要求逐字节相同并进入 verify。22 个 fail-closed mutation 覆盖冻结输入、目标、纠正的 text end、exact2 scope、call bytes、target/forwarder xref universe、EH unwind/split-tail、两行 allocation/offset/count、receiver writer completeness、32 位无回绕、槽所有权分离及权限/promotion 全 false。

独立验收不得复用本组件的结论作为测试 oracle；应在同一冻结 IDB 中重新运行 analyzer，要求输出 `PASS verified`、JSON SHA-256 精确匹配，并确认未发生 IDB、运行时、生产、测试或先前证据写入。
