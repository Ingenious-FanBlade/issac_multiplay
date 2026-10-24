> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 caller-heap residual exact17 v29 逆向交接

## 结论

本分片从 frontier v26 的 `copyMove.groups.callerHeap` 读取且只处理以下 17 条：

`0x00008A62`, `0x00008B2A`, `0x0001513A`, `0x000155A4`,
`0x0001AAB0`, `0x0001AEB8`, `0x001CBEB5`, `0x0033570B`,
`0x0061F2C2`, `0x00657E0A`, `0x00657E94`, `0x00658338`,
`0x006583C2`, `0x00658487`, `0x00661577`, `0x00689F63`,
`0x0068E3DB`。

结果为 **0 VERIFIED / 17 UNKNOWN**。目标 PE 上 17 条 exact E8 bytes/targets、
14 个 containing function 的完整 raw E8 caller 和 E9 forwarder universe、两组
address-taken method literal census 均已重放；但没有一条消除 caller-heap v5 的
具体反例。日志后继续、容量/参数域、untyped vtable、container writer 与 raw
slice 反例均原样保留。

本交接由 successor 逆向作者生成，不是独立验收；后续必须另派 tester/reviewer。

## 冻结边界

- caller-heap v5 analyzer SHA-256：
  `E8E78C9AD6D6E07411E6CA1E0287190AA4E1D41D97E73A3454F58AFEC18FF48D`。
- caller-heap v5 JSON SHA-256：
  `5D46EB95A7575EDC6384B4D15D6031C7F78945041FF98216AB1F8AB63DCEF6EA`。
- frontier v26 analyzer SHA-256：
  `7E74CE0C92DBB253059CF4560FEDF29EF2C25975F38E81CF96B0236E62EBFE07`。
- frontier v26 JSON SHA-256：
  `514399E365C79126E9256993549B957BF93800BDF8C1A788CEE588E3FE0328EF`。
- 目标 `isaac-ng.exe` SHA-256：
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。

v5 的 exact22 减去 v26 接受边界正好得到 exact17。被排除的已接受 5 行精确为：

`0x003534FE`, `0x0035352E`, `0x005B91EF`, `0x00688B74`, `0x006957BC`。

本分片没有重开、修改或重新解释这 5 行。

## Exact call replay

| RVA | bytes | resolved target |
| --- | --- | --- |
| `0x00008A62` | `E8 78 7B 6E 00` | `memcpy 0x00AF05DF` |
| `0x00008B2A` | `E8 B0 7A 6E 00` | `memcpy 0x00AF05DF` |
| `0x0001513A` | `E8 A0 B4 6D 00` | `memcpy 0x00AF05DF` |
| `0x000155A4` | `E8 36 B0 6D 00` | `memcpy 0x00AF05DF` |
| `0x0001AAB0` | `E8 2A 5B 6D 00` | `memcpy 0x00AF05DF` |
| `0x0001AEB8` | `E8 22 57 6D 00` | `memcpy 0x00AF05DF` |
| `0x001CBEB5` | `E8 03 4A 52 00` | `memmove 0x00AF08BD` |
| `0x0033570B` | `E8 AD B1 3B 00` | `memmove 0x00AF08BD` |
| `0x0061F2C2` | `E8 18 13 0D 00` | `memcpy 0x00AF05DF` |
| `0x00657E0A` | `E8 D0 87 09 00` | `memcpy 0x00AF05DF` |
| `0x00657E94` | `E8 46 87 09 00` | `memcpy 0x00AF05DF` |
| `0x00658338` | `E8 A2 82 09 00` | `memcpy 0x00AF05DF` |
| `0x006583C2` | `E8 18 82 09 00` | `memcpy 0x00AF05DF` |
| `0x00658487` | `E8 53 81 09 00` | `memcpy 0x00AF05DF` |
| `0x00661577` | `E8 63 F0 08 00` | `memcpy 0x00AF05DF` |
| `0x00689F63` | `E8 77 66 06 00` | `memcpy 0x00AF05DF` |
| `0x0068E3DB` | `E8 DD 24 06 00` | `memmove 0x00AF08BD` |

每条还绑定以 call 为中心的 69-byte PE context SHA-256，机器 JSON 保存完整 ledger。

## Caller、forwarder 与 virtual universe

完整 raw E8 caller 数如下；每组机器 JSON 同时保存全部 callsite VA 与集合 SHA。
所有 14 个函数的 relative E9 forwarder count 均为 0。

| containing function VA | E8 count |
| --- | ---: |
| `0x00408970` | 2 |
| `0x00414E00` | 2 |
| `0x00415170` | 1 |
| `0x0041A790` | 177 |
| `0x0041AB50` | 5 |
| `0x005CBEA0` | 4 |
| `0x00734F30` | 7 |
| `0x00A1F280` | 1 |
| `0x00A57DB0` | 25 |
| `0x00A582E0` | 30 |
| `0x00A583E0` | 9 |
| `0x00A614D0` | 2 |
| `0x00A89DE0` | 2 |
| `0x00A8E1B0` | 2 |

Virtual rows 仍未闭合：

- `sub_A57DB0`：25 个 direct E8 已闭；whole-file method-pointer literal 精确 2
  个，位于 vtable `0xB9ECE0/0xB9ED3C`。这只证明 method address 被安装，不能枚举
  所有 slot-6 indirect callsite 或 `a2` producer。
- `sub_A582E0`：30 个 direct E8 已闭；whole-file method-pointer literal 精确 42
  个，其中 41 个是 devirtualization comparison literal，另一个是 vtable
  `0xB9ED10`。该 untyped vtable entry 仍允许未分类的 indirect `a2` producer。

因此 `0x00657E0A/0x00657E94/0x00658338/0x006583C2` 均保持
`UNKNOWN`，不能用 direct caller census 替代 virtual dispatch fixed point。

## 逐行 start + count / extent / no-wrap

| RVA | 局部关系 | 保留的首缺口 |
| --- | --- | --- |
| `0x00008A62` | fresh extent=`4*newCount`，copy=`4*oldCount` | 两个完整 caller 均未证明 `oldCount<=newCount`；`new=1, old=2` 反例保留。 |
| `0x00008B2A` | 同上，第二 count field | 同一 `new=1, old=2` 反例。 |
| `0x0001513A` | extent=`84*5000`，copy=`84*v4` | `v4==5000` 检查只记录日志后继续；parser/interpolation 域未证明 `v4<=5000`，乘法 no-wrap 亦依赖该域。 |
| `0x000155A4` | extent=`108*5000`，copy=`108*v4` | 同类 5000 限制只日志后继续。 |
| `0x0001AAB0` | dest=`*(a3+4)`，copy=`4*sourceCount` | capacity 不足检查只日志后继续；177 个 direct producer 的 `sourceCount<=capacity` 未闭。 |
| `0x0001AEB8` | dest=`*(a3+4)`，copy=`4*sourceCount` | 五个 direct producer 仍不能证明 capacity relation。 |
| `0x001CBEB5` | start=`kk`，count=`end-(kk+4)` | 四 caller 的 iterator 已定位，但完整 vector begin/end writer set 未闭；必须证明 `begin<=kk<=end-4` 才无回绕。 |
| `0x0033570B` | start=`selected`，count=`end-(selected+4)` | filtered vector 可为空；此时 `end-(begin+4)` 回绕。raw register/stack slice 反例完整保留。 |
| `0x0061F2C2` | dest=`this[49]`，count=`4*((this[5]-this[4])>>4)` | constructor 与 `this[48]/this[49]` 全 writer/extent 未闭。 |
| `0x00657E0A` | caller/virtual `a2`, count=`Size` | untyped slot-6 indirect dispatch 的 `a2` domain/extent 未闭。 |
| `0x00657E94` | caller/virtual `a2`, count=`Size_3` | 同上。 |
| `0x00658338` | caller/virtual `a2`, count=`Size` | `0xB9ED10` untyped indirect dispatch 未闭。 |
| `0x006583C2` | caller/virtual `a2`, count=`Size_3` | 同上。 |
| `0x00658487` | dest=`begin+used`, count=`Size`, loaded extent=`capacity-used` | local check 依赖合法 begin/used/capacity；constructor、replacement `sub_7DCCD0` 与全 writer set 未闭。 |
| `0x00661577` | 第二 static ring extent=`0x10000`，count=stream `Size` | power-of-two 检查只日志后继续，未证明 `Size<=0x10000`；大 count 仍留下 32 位 end wrap。 |
| `0x00689F63` | caller output `a1`, count=`Sizea` | 两个 direct caller 的 output pointer domain 与 capacity chain 均未闭。 |
| `0x0068E3DB` | dest=`*v8`，count=`this+184`，local capacity=`this+180` | local malloc/realloc 分支有界，但 nested constructor 与 `+176/+180/+184` 全 writer 未闭。 |

v5 的 `exactCountermodel`、`nextRequiredInterface` 与
`attemptedInterfaceClosure` 逐行写入 machine JSON；没有以 “PE 中未直接观察到 slot”
替代完整 start/extent/writer/dispatch 证明。

## 产物与复现

- Analyzer：
  `out/cs-t0/analyze-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29.py`
  
  SHA-256 `76E0A41D823DE2B8BDF0C566962F7A5641F36938062540CA6C855CDF3B932F69`
- Machine JSON：
  `out/cs-t0/native-join-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29.json`
  
  SHA-256 `BFE51D1DAEE85058F8904EA1327FECD8A4A1AA8A9EE81CEFD0C01A73CB637DAA`

Create-new：

```powershell
python out/cs-t0/analyze-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29.py
```

两次 existing replay：

```powershell
python out/cs-t0/analyze-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29.py --verify-existing
python out/cs-t0/analyze-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29.py --verify-existing
```

两次均输出：

```text
PASS mode=PE ... sha256=BFE51D1DAEE85058F8904EA1327FECD8A4A1AA8A9EE81CEFD0C01A73CB637DAA verified=0 unknown=17 mutations=85
```

85/85 semantic mutations 均 fail closed，覆盖冻结 bindings、target/scope/summary、
逐行 status/RVA/countermodel/firstGap，以及四条 untyped-vtable 状态。

外部分析器状态仅为 `TIMEOUT_NO_PASS`，retry count 0；没有把不可用当作 PASS。
本轮仅只读磁盘 PE，没有启动游戏、进程或调试器，没有读运行时内存，没有修改
IDB、production、tests、status、index 或旧证据。所有 permissions、promotions 与
`overallAccepted` 均为 `false`。
