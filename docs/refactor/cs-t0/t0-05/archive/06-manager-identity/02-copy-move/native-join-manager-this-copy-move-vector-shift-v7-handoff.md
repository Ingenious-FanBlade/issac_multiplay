> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this vector-shift-v7 交接

## 结论

本 lane 只做静态逆向，封存 `proofClass=VECTOR_IN_PLACE_SHIFT_WITHIN_BEGIN_END` 的 exact27：

- `closed=2`
- `unknown=25`
- `confirmed overlap=0`
- `managerThisPromotionAllowed=false`

`rejected v4` 只用于按 `proofClass` 与身份字段抽取 exact27，未采信其中任何 proof。exact27 与 hard-v5 exact21、vector-grow-v6 exact29、rejected-v4 其余 73 行均为零交集。

## 新闭包

### `0x0043DA16`

`sub_83D8C0` 的局部 tuple `v117/v118/v119` 在 `0x0043D900/0x0043D922/0x0043D907` 完整清零；地址仅交给 `0x0043D932` 的追加 helper。`sub_732E90` 仅在当前 allocation 尾部追加，容量不足时进入 `sub_428590`；后者按新 capacity 分配，将旧区间复制到新 allocation，并同时安装 begin/end/cap。精确调用处 cursor `p=v6` 从 begin 按 4 字节递增且满足 `p<end`：

`offset=(p-begin)`，`length=end-(p+4)`，因此 `offset+4+length=end-begin<=cap-begin`。destination `p` 小于 source `p+4`，是向低地址移动一个 4 字节元素，重叠方向与 `memmove` 一致。调用后只写 `end-=4`；最终路径只析构该局部 allocation。assert/logging 不参与闭包。

### `0x0043FED9`

`sub_83EAE0` 的局部 begin/end/cap 存储与 cache 在 `0x0043FD41..0x0043FD5A` 全部清零；完整生命周期的写入只有 `0x0043FDB3` 内联追加、`0x0043FDCE` 的 `sub_428590` 扩容/安装、精确调用的 postfix `end--` 与最终析构。cursor `p` 从 begin 按一个 4 字节元素递增到 end：

`offset=4*(p-beginElements)`，`length=4*(endElements-(p+1))`，因此 `offset+4+length=4*(endElements-beginElements)<=4*(capElements-beginElements)`。destination `p` 小于 source `p+1` 元素，是向低地址移动一个元素，重叠由 `memmove` 处理。assert/logging 不参与闭包。

## 保留 unknown

其余 25 行都已记录精确 callsite 与观察到的 tuple root，但缺少完整的 executable-wide caller/constructor/destructor/alias writer universe，因而不能证明所有 admitted state 下的同 allocation、`offset+length<=capacity` 与非空重叠方向。仅有局部 begin/end、容器式字段布局、C++/STL 形态均未用于提升。

其中 `0x003DE616` 与 `0x004352BA` 的完整直接调用集只以 `Src=end` 调用，而精确 memmove 分支要求正 tail；这只证明该分支在这些直接调用前提下不可达，不用 vacuous 结论替代用户要求的非空 row proof，因此仍为 unknown。

## 绑定与复现

- machine: `out/cs-t0/native-join-manager-this-copy-move-vector-shift-v7.json`
  - SHA-256 `6A80A59A3937A5575C82B82DC664D354833F6FADD5482D0A1F7604EB6BDC19E6`
- reproducer: `out/cs-t0/analyze-manager-this-copy-move-vector-shift-v7.py`
  - SHA-256 `5302DBB97A6387DED5CBAE14AA987228C525BB067BB85A90C7DAA6084B2A4B28`
- positive baseline SHA-256 `73905C2A1D27C12344F82147C4F89781BAC08A447D55D4C6E89D8465BB23626D`
- negative audit SHA-256 `3937A58884C84D8A4653612F6C64980C314C191F70D9D5498653BAD06D22CC7C`
- rejected-v4 set-only SHA-256 `FFDAD182B2948579A2D5FE6F1BB6BEE7502400B1BD27ADE87CF9276F9FE96265`
- hard-v5 SHA-256 `D88E92E8F1D2E89072883FAC04072FD2364FA74B7F22ED02D18DDD9E6E9A68AF`
- vector-grow-v6 SHA-256 `DB7512C7DCD1012D40D87626FDA1A8B90F234D7A4A436C8E0C67F6E61D5090DD`
- target SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

复现命令：

```powershell
python out\cs-t0\analyze-manager-this-copy-move-vector-shift-v7.py --self-test --verify-existing out\cs-t0\native-join-manager-this-copy-move-vector-shift-v7.json
```

结果：`PASS exact27 closed=2 unknown=25 overlap=0 mutations=50`。50 个语义变异覆盖 set/binding/intersection、四个闭包维度、不等式、重叠方向、assert/logging 独立性、UNKNOWN 强制提升、aggregate 与权限字段，全部 fail closed。

本 lane 未运行游戏/进程/调试器，未修改 IDB、既有证据、production/tests/index/status。
