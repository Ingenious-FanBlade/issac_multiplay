> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this copy backing critical-two v10 逆向交接

## 结论

本轮仅处理冻结 exact-two：`0x001CBEB5`、`0x005B91EF`。两行均保留严格 `UNKNOWN`；新增闭包数为 0，不允许据此提升 manager-this。

这不是“没有发现危险 writer”的结论。相反，`0x005B91EF` 已找到 v9 writer 集合遗漏的两个真实 alias writer/forwarder，并纠正了一个错误归属；`0x001CBEB5` 则仍无法穷尽父记录和预偏移 tuple alias 的 begin/end/capacity 来源。按 fail-closed 规则，任何未穷尽 writer universe 都不得用立即数或近似引用扫描替代。

| callsite RVA | v10 状态 | 最小未决谓词 |
| --- | --- | --- |
| `0x001CBEB5` | `UNKNOWN` | 穷尽 `sub_706780(GameRoot)+0x1A280+index*0x5DC` 嵌入 tuple 的父记录构造、析构、reset、copy/move 和预偏移 alias writer；证明 begin/end/capacityEnd 每个来源及 `capacityEnd-begin` 的真实 backing extent。 |
| `0x005B91EF` | `UNKNOWN` | 穷尽 `GameRoot+0x1BAA8` 的有效 LEA/ADD/accessor forwarder 和传递后的 tuple writer，闭合 `sub_9BA000`，证明不存在 SSO、外部 buffer 或 assignment 安装非 allocator begin 的路径。 |

## 冻结绑定与执行边界

- `out/cs-t0/native-join-manager-this-copy-backing-four-v9.json`
  - SHA-256 `B1BAAC718A0E94BDA1D002CA4AD6C38FB4EA168D7BAEA59FA3B336DAFAB60A2F`
- `out/cs-t0/native-join-manager-this-copy-range-bounds-v7.json`
  - SHA-256 `C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D`
- `out/cs-t0/native-join-manager-this-copy-backing-six-v8.json`
  - SHA-256 `1F45AC9ADA79174114C8C74C63F4C05CD88304DE44178A8A77A8C65DD994D3D9`
- `isaac-ng.exe`
  - SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
  - x86 image base `0x00400000`，SizeOfImage `0x008FE000`

全程仅使用 PE 磁盘反汇编和只读 IDA 观察；没有启动游戏、打开进程、附加调试器或修改 IDB，也没有修改既有证据、测试、状态索引或生产代码。

## `0x001CBEB5`：嵌入 save-record tuple 未闭合

目标 call bytes 为 `E8 03 4A 52 00`。`sub_5CBEA0` 对 4-byte vector 执行原地 erase；v7 的相对 range 不等式仍绑定为 `VERIFIED_BOUNDED`。四个 direct caller 重新核对为：

- `0x005C49FD`
- `0x005C4A8E`
- `0x00688D38`
- `0x00688E1F`

local `var_154` 和 object `+0x1FA4` 两个子家族可继续保持各自已闭合结论；关键剩余 receiver 在 `0x005C496E/0x005C497F/0x005C4985` 形成：

```text
receiver = sub_706780(GameRoot) + 0x1A280 + DWORD[owner+0x1620] * 0x5DC
tuple    = { begin@+0, end@+4, capacityEnd@+8 }
```

`sub_706780` 根据 `DWORD[GameRoot+0x676AC]` 计算 `index*0x20660`，再从下列两个候选记录选取 leading active byte 非零者；负 index 返回 null：

```text
GameRoot + 0x4704C - index*0x20660
GameRoot + 0x269EC + index*0x20660
```

`.text` 中 little-endian `0x1A280` 原始字节仅命中 RVA `0x001C4981`，但这个唯一命中只能定位当前读取/使用路径。它无法看见下列合法 writer 形态：

- 先取得父 save record，再在 constructor/destructor/reset 内写入 tuple；
- 对父记录做 bulk copy/move/assignment；
- helper 接收已经加过 `0x1A280` 的 record/tuple pointer，因此自身不含该立即数。

所以 begin、end、capacityEnd 的可达来源均未穷尽，`capacityEnd-begin` 的真实分配 extent 也未闭合。最小保留反模型是：一个尚未分类的父记录或预偏移 alias writer 安装 tuple；之后 erase 的所有相对 v7 边界仍成立，但 begin 仍可能别名固定映像槽或缺少容量来源证明。

## `0x005B91EF`：v9 writer universe 已被具体反例否定

目标 call bytes 为 `E8 C9 76 13 00`。三个 direct caller 重新核对为：

- `0x0042A5A3`
- `0x006FA437`
- `0x0090E2D4`

manager-this 关系现可精确写为：

```text
slot VA       = 0x00C71678
GameRoot      = DWORD[0x00C71678]
tupleReceiver = GameRoot + 0x1BAA8
begin/end/cap = DWORD[tupleReceiver+0/+4/+8]
```

`sub_417860(ecx)` 是 canonical accessor，返回 `ecx+0x1BAA8`。因此这里与 manager-this 使用的是同一个 tuple receiver；但固定 slot 是保存 `GameRoot` 的映像地址，不是 receiver，也不是 backing begin。要证明 begin 不等于 slot，仍必须先闭合所有 begin writer。

已独立重核的 tuple writer/路径如下：

1. `sub_6F1020` 在 `0x006F161D/0x006F1627/0x006F1631` 将 begin/end/capacityEnd 全部置零。
2. `sub_9B8F40` 是 v9 `confirmedAliasWriters` 遗漏的真实 writer。`0x0042A65A/0x0042A66C` 将 `GameRoot+0x1BAA8` 作为 receiver；空 vector 分支在 `0x009B901A` 通过 `sub_42C920` 增长。
3. `sub_9B9CD0` 的 manager tuple 增长点是 `0x009B9EBA`。v9 所列 `0x009B9D82` 实际 receiver 为 `tupleReceiver+0xD0`，是另一个嵌套 vector，不能计作 manager tuple begin writer。
4. `sub_9B9150` 是本目标的原地 erase，只修改 end。
5. `0x00AFB562` 将 `GameRoot+0x1BAA8` tail-forward 到 `sub_9B8EA0`，也是 v9 遗漏的 alias writer。`sub_9B8EA0` 先令 end=begin，随后 tail-call `sub_4149D0`；后者释放 backing 并把完整 triple 置零。

对已观察增长路径，容量关系可以闭合到：

```text
sub_42C920 -> sub_40CF00 allocation
sub_41D150 installs [begin, begin+4*size, begin+4*capacity]
capacityEnd - begin = 4*capacity
end - begin         = 4*size
```

这些 helper 没有观察到 inline/SSO 分支或外部 buffer 安装。但这只覆盖已分类路径，不能证明全程序不存在 SSO/assignment/external-buffer writer。

全图扫描没有被当作闭包：IDA 对 `+0x1BAA8` 产生 495 个立即数引用候选；PE `.text` 对原始字节 `A8 BA 01 00` 有 1159 个命中。候选混合字段读取、嵌套字段、forwarder 以及需要逐函数判定的 consumer，原始反汇编还可能从非函数边界产生 data-like decode。由于 `sub_9BA000` 尚未在 begin/end/capacity 粒度闭合，且其余有效 forwarder/传递 callee 尚未完成有限分类，writer universe 仍为不完备。

最小、已落到可执行字节的反例不是假设危险 writer，而是 v9 完备性前提本身已被否定：`sub_9B8F40` 与 `sub_9B8EA0` 两条真实 writer 路径均不在 v9 集合中，且 `0x009B9D82` 还被错误归到 manager tuple。即使新发现的两条路径分别是 allocator-derived 和清零路径，也不能据此推断没有第三条遗漏路径。

## 复现器与 fail-closed mutation gate

复现器：`out/cs-t0/analyze-manager-this-copy-backing-critical-two-v10.py`

```powershell
python out\cs-t0\analyze-manager-this-copy-backing-critical-two-v10.py --self-test
python out\cs-t0\analyze-manager-this-copy-backing-critical-two-v10.py --verify-existing
```

预期输出：

```text
PASS exact2 verified=0 unknown=2 mutations=25
```

25 个语义 mutation 覆盖 frozen hash、exact2、PE/hash/call bytes、关键 writer 字节、嵌入 tuple 算术、v9 遗漏和嵌套 vector 更正、SSO/外部安装未决、writer closure、状态提升、总数、权限与最终决策。任一篡改均 fail-closed。

## create-new 产物

- `out/cs-t0/native-join-manager-this-copy-backing-critical-two-v10.json`
  - SHA-256 `0A5834505E79B4575A2DF53C86FE647E3F6A83E3E8FD4328369C11CC257A65FF`
- `out/cs-t0/analyze-manager-this-copy-backing-critical-two-v10.py`
  - SHA-256 `EB6E5B927BAFBC53724F80AF5007B1F7B6FFDC5DA6D53D6AF2C50610A26B32FC`
- `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-copy-backing-critical-two-v10-handoff.md`
  - 本文件 SHA-256 由独立复核时计算。

## 独立测试交接

本文件是逆向交接，不是最终测试验收。测试子智能体应独立执行 `--self-test` 与 `--verify-existing`，重新计算三份产物 SHA，并检查 exact-two、call bytes、writer inventory、两处 v9 反例、nested `+0xD0` 更正和全部 `UNKNOWN` gate；不得把本轮逆向者的自检当作独立接受证据。
