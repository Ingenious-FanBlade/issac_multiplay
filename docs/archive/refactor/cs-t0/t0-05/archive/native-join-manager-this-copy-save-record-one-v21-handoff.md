> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this copy save-record exact-one v21 逆向交接

## 结论

本轮只处理 `RVA 0x001CBEB5`（`VA 0x005CBEB5`），不重做 `0x005B91EF`。结论是严格 `UNKNOWN`，不是 superseding closure：新增闭包数为 0，copy/move 保持 `315/405`，remaining 保持 90，所有 permissions 和 promotions 均为 false。

决定性原因不是相对 memmove 范围。v7 的 `VERIFIED_BOUNDED` 可以直接复用；真正的反例是请求中的对象布局不成立。目标 receiver 是 record `+0x504`，但该 record 的 allocator vector triple 实际从 `+0x500` 开始。因此请求 receiver 的 `+0/+4/+8` 实际分别是 vector end、vector capacityEnd 和一个独立序列化 scalar，而不是 begin/end/capacityEnd。即使继续穷尽 alias，也不能把这个 receiver 当成所请求的三字段 vector tuple。

此外，两级 selector 都只观察到非负检查，没有静态闭合上界：`DWORD[owner+0x1620]` 未证 `<8`，`DWORD[GameRoot+0x676AC]` 未证落在 `sub_706780` 两个 parent 候选的有效有限域。record copy/move/assignment、parent/pre-offset alias 与 already-offset helper 的全调用者宇宙也仍未闭合。因此不得把 315 提升为 316。

## 冻结绑定与边界

- `out/cs-t0/native-join-manager-this-copy-backing-critical-two-v10.json`
  - SHA-256 `0A5834505E79B4575A2DF53C86FE647E3F6A83E3E8FD4328369C11CC257A65FF`
- `out/cs-t0/native-join-manager-this-copy-range-bounds-v7.json`
  - SHA-256 `C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D`
- `out/cs-t0/native-join-manager-this-indirect-split-component-v2.json`
  - SHA-256 `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`
- `out/cs-t0/native-join-manager-this-manager-tuple-global-seal-v20.json`
  - SHA-256 `E1D42FABAF1651B01BCC1C7005A2958C0510B26F29BE361A2FE8F71AA44D58C9`
  - 只作为不相干 manager tuple 的已接受对照，不转移其闭包结论。
- `isaac-ng.exe`
  - SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
  - PE32 x86，image base `0x00400000`，SizeOfImage `0x008FE000`。

全程只读 PE/IDA 静态分析。没有启动游戏、打开或读取进程、附加调试器、修改 IDB、修改既有证据/状态索引/测试/生产代码，也没有运行时 probe。

## exact-one 与 direct caller

目标 call bytes 为 `E8 03 4A 52 00`。`sub_5CBEA0` 的四个 direct caller 是：

- `0x005C49FD`
- `0x005C4A8E`
- `0x00688D38`
- `0x00688E1F`

本轮只消费 `0x005C49FD` 的 selected save-record 家族；另外三个 caller 家族沿用 v10，不借本轮重新升格。

目标 receiver 的静态形成式为：

```text
parent   = sub_706780(GameRoot)
index    = DWORD[owner+0x1620]
receiver = parent + 0x1A280 + index*0x5DC
```

parent 的第一组 record array 位于 `parent+0x19D7C`，每条 record 大小 `0x5DC`。因此：

```text
0x19D7C + 0x504 = 0x1A280
receiver = selected record + 0x504
```

## 42 个 literal 候选分类结果

对 `.text` 中与该布局有关的 42 个候选逐一完成功能级分类：`0x5DC` stride 候选 34 个，`0x20660` parent-size 候选 8 个。没有从这 42 个 literal 候选发现新的 foreign begin install。

这个结果不能替代 alias closure：helper 可以接收已经加过 `record+0x500`、`record+0x504` 或 parent/pre-offset 的 pointer，自身不必再包含 `0x5DC`/`0x20660` 立即数。首个具体布局反例位于 `record+0x50C` 的 already-offset virtual serialization 路径；首个仍未闭合的 universe 是 record copy/move/assignment 以及传入 already-offset record/tuple pointer 的 direct/indirect/split-tail/forwarded caller。

## lifecycle 与真实 vector 布局

已静态分类的有限 lifecycle 部分：

1. `sub_9C73C0` 对 `parent+0x19D7C` 构造 8 条 record，对 `parent+0x1CC5C` 构造 4 条 record；元素大小均为 `0x5DC`，元素 constructor/destructor 为 `sub_754980`/`sub_754D90`。
2. `sub_9C7800` 通过 EH array destructor 逆序析构两组 record。
3. `sub_754980` 将 `record+0x500/+0x504/+0x508` 清零。
4. `sub_9CF050` 释放并清零 `record+0x500` triple，同时独立执行 `record+0x50C=0`。
5. `sub_9D05D0` 在 `0x009D3E2C` 调用 `sub_7310E0(record+0x500,count)`；增长链为 `sub_732240 -> sub_40CF00 -> sub_41D150`，由 allocator backing 安装真实 begin/end/capacity。
6. `sub_9D45C0` 在 `0x009D6B9D` 读取 `(record+0x504-record+0x500)/4` 作为 vector size。
7. `sub_9D05D0` 在 `0x009D3EBF` 把 `record+0x50C` 交给 virtual reader slot `+0x14` 写入 4 字节；`sub_9D45C0` 在 `0x009D6C22` 把同一 scalar 交给 virtual writer slot `+0x1C`。

所以真实关系是：

```text
actual vector begin       = record+0x500
actual vector end         = record+0x504
actual vector capacityEnd = record+0x508
serialized scalar         = record+0x50C

requested receiver+0 = actual end
requested receiver+4 = actual capacityEnd
requested receiver+8 = serialized scalar
```

这给出了可执行字节上的反模型：`record+0x50C` 有合法的非 allocator scalar writer。将请求 receiver 的三个 DWORD 解释成 vector triple 会把这个 scalar 错认成 capacityEnd。

## selector 有效域缺口

在 `0x005C494C/0x005C4953`，owner index 只被证明 `>=0`。第一组 array 只有 8 条 record，没有支配目标调用的 `index<8` 证明。

`sub_706780` 对 `DWORD[GameRoot+0x676AC]` 也只处理负数返回 null；非负 selector 用于两个 `0x20660` stride parent 候选：

```text
GameRoot + 0x4704C - selector*0x20660
GameRoot + 0x269EC + selector*0x20660
```

没有观察到支配调用的 selector 上界，也未闭合该字段的完整 writer universe。这是独立于错误 tuple 布局的第二个 UNKNOWN 条件。

## machine JSON、IDA reproducer 与 mutation gate

产物：

- `out/cs-t0/native-join-manager-this-copy-save-record-one-v21.json`
  - SHA-256 `A0E9612B212D338C1DC50F9AA859C99F1F14EBB62A7277007E0689026920939C`
- `out/cs-t0/analyze-manager-this-copy-save-record-one-v21.py`
  - SHA-256 `BE65C040507D09CBA4F23C6057ACC54984CD5159DC1FB6944A81C5AC14E03F68`

普通 PE 重放：

```powershell
python out\cs-t0\analyze-manager-this-copy-save-record-one-v21.py --self-test
python out\cs-t0\analyze-manager-this-copy-save-record-one-v21.py --verify-existing
python out\cs-t0\analyze-manager-this-copy-save-record-one-v21.py --verify-existing
```

每次预期：

```text
PASS PE exact1 verified=0 unknown=1 candidates=42 mutations=32
```

IDA 中使用 `py_exec_file` 执行同一绝对路径：

```text
D:\projects\isaac_hook_reserach\out\cs-t0\analyze-manager-this-copy-save-record-one-v21.py
```

预期：

```text
PASS IDA exact1 verified=0 unknown=1 candidates=42 mutations=32
```

IDA 重放额外验证 11 个函数的精确边界与 SHA-256；PE/IDA 生成同一模式无关 payload。32 个 mutation 覆盖冻结绑定、exact-one/exclusion、错误 tuple 升格、scalar 错认 capacity、两级 selector 上界、alias/caller universe、backing 状态、计数、promotion、permissions、remaining predicate 和最终 decision；全部 fail-closed。

## 独立测试交接

本文件是逆向交接，不是最终测试验收。测试子智能体应从磁盘独立运行 `--self-test` 和连续两次 `--verify-existing`，再在冻结 IDA 中运行同一 reproducer；复算 analyzer、machine JSON 与本 handoff 的 SHA-256，并确认：exact-one 只含 `0x001CBEB5`、`0x005B91EF` 明确排除、42/42 分类、requested tuple layout 为 false、两级上界为 false、copy/move 仍为 `315/405`、所有 permissions/promotions 为 false。不得把本轮逆向者自检当作独立接受证据。
