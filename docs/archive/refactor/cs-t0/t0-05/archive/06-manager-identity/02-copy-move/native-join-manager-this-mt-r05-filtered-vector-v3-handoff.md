> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 MT-R05 filtered-vector v3 有界逆向交接

日期：2026-09-06  
角色：`REVERSE / MT-R05`  
状态：`CANDIDATE_PENDING_INDEPENDENT_TEST / FAIL_SAFE_UNKNOWN_0P1U`

## 1. 结论

`MT-R05 {0x0033570B}` 不能在本轮有界静态范围内提升为
`STATIC_CONFIRMED_NO_OVERLAP`。精确保留：

- first-open obligation：`countAndNoWrap`；
- gap id：`R05_AUX_NONEMPTY_FILTERED_EMPTY_CONJUNCTION_OPEN`；
- countermodel：`auxiliaryRangeCount>0 && filteredCount==0 yields raw transfer length 0xFFFFFFFC`；
- 单行 verdict：`UNKNOWN`，`promoted=false`。

没有创建空 frontier，也没有修改 manager-this、frontier、checkpoint、status、index、
权限或生产代码。

## 2. 冻结目标与 IDA 身份

本轮先执行只读 `survey_binary(detail_level=minimal)` 和 `server_health`：

- IDB：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`；
- input：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`；
- module：`isaac-ng.exe`，PE32/I386，ImageBase `0x00400000`，SizeOfImage `0x008FE000`；
- SHA-256：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`；
- `auto_analysis_ready=true`，`hexrays_ready=true`。

IDA 全程只读，未修改 IDB。

## 3. `STATIC_CONFIRMED` 的局部事实

### 3.1 auxiliary producer 与共同前驱

- `0x00735220–0x00735278` 初始化 auxiliary 三指针局部 vector，并循环 append
  整数 `1..13`；
- `0x0073532C–0x00735367` 再 append `14` 并缓存 begin；因此在正常继续路径上，
  auxiliary count 精确为 `14`；
- 紧接着 `0x00735368–0x0073537C` 将 filtered begin/end/capacity 初始化为零。

因此 auxiliary 非空与 filtered 非空并非由同一 producer 建立；共同前驱只证明
`auxiliaryCount=14` 和 `filteredCount=0` 的初始独立状态。

### 3.2 filtered producer 与完整过滤谓词

`0x0073538A–0x0073539B` 计算全局候选 tuple
`n298+0x2A434/+0x2A438` 的元素数；源计数为零时直接跳到 `0x0073554B`，
filtered tuple 保持为空。

对非零源，append 仍不是必执行。`0x007353D1–0x0073546D` 的 reject 条件包括：

- `n60` 存在、`candidate+0x38==0` 且 `n60+0x269C8` 为 `2` 或 `3`；
- `n60+0x26614>=2` 且 `n60+0x26620` 的虚调用槽 `+0x28` 返回真；
- `candidate+4>=0x282`；
- `candidate+4` 非零且表项 byte 为零，同时 mode/n60 fallback 不成立。

只有通过这些条件后，`0x0073546E–0x0073548E` 才执行 direct/grow append。
所以 `sourceCandidateCount>0` 也不能推出 `filteredCount>0`。

### 3.3 merge、选择与 raw 长度

- `0x0073554B–0x00735558` 计算 `filteredCount-1`；`filteredCount==0` 时直接跳过
  shuffle，没有 nonempty guard；
- `0x007355F1–0x0073560C` 独立检查 auxiliary count，非零即进入 14 项循环；
- `0x0073567B–0x007356EB` 再计算 filtered count；零时跳过 `div`，令
  `selected=filteredBegin`；
- `0x00735700–0x0073570B` 形成
  `filteredEnd-(selected+4)` 的 32-bit raw 长度，零计数下为 `0xFFFFFFFC`；
- `0x00735720–0x00735731` 每次普通移除都令 filtered end 减四，却只按 auxiliary
  count 控制下一轮，因此缺少“后续轮 filtered 仍非空”的 guard。

`0x007356F7` 在 call 前先读取 `[selected]`。这是一次解引用，不是控制流 guard，
不能用来证明 `filteredCount>=1`。fresh-empty 且 begin 为零的路径可能在这里先失败；
而已有 backing 被逐项耗尽时，begin 可仍指向已分配存储。两种情况都不构成
`ALL_REACHABLE_EXECUTIONS` 下的正向安全证明，本交接不声称实机已命中该 memmove。

## 4. 有界 caller / producer 搜索与停止点

`sub_734F30` 的七个直接 callsite 为：

`0x006F566A, 0x006F5EB2, 0x006F7156, 0x006F75AC, 0x006F78C5, 0x006F7EB9, 0x006F84CB`。

逐一检查这些 callsite 的近邻前驱窗口，均只准备 seed/string/state 后调用
`Game+0x1A740`，没有对全局候选计数或“至少一个候选通过完整过滤”的支配 guard。
相关消费者 `sub_734CC0` 与 `sub_736C30` 反而各自含候选计数为零的显式退出，不能提供
全局非空 invariant。

有界 fixed point 在此停止。首个仍缺的证明是：

> 对所有能到达目标的执行，存在至少一个源候选，且至少一个候选必通过全部 reject
> 谓词，并且在每个仍有 auxiliary 项的选择轮次保持 `filteredCount>=1`。

建立该命题需要继续闭合 `n298+0x2A434/+0x2A438` 的全局 producer/lifetime 与所有
间接写入路径；这已超出本轮有界范围。故合取不能静态排除，继续 `UNKNOWN`。

## 5. v3 候选产物

| 对象 | SHA-256 |
|---|---|
| `out/cs-t0/build-manager-this-mt-r05-filtered-vector-v3.py` | `318C5FFFFF55FAE2C94E51C64C751F25EDD849332B922EB41A29264B56185281` |
| `out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3.json` | `DEB2E1301025DA0AB46DFBA2E02017F0BA6A8F27C7EAEE2C7F4FD2C15B9291A6` |

候选绑定预封印 acceptance spec
`8872E11ED61E153C2B27A394764FC4959C4E1C6035ABE28D40AED69891BF7759`，并保持：

- schema：`cs-t0.manager-this.mt-r05.filtered-vector.v3.reverse-candidate.v1`；
- exact scope：`MT-R05 / 0x0033570B / memmove / exactCount=1`；
- `backingExtent=CLOSED`（不回退已独立接受基线）；
- `countAndNoWrap=OPEN`，其后的 lifetime/interval obligations 也不越过 first-open；
- candidate 自身所有 promotion/frontier 决策为 false。

## 6. 压缩作者自检

只运行以下两条；没有运行 sealed 独立 TEST auditor、Cargo、全量 build、历史宽回归、
游戏、目标进程、x32dbg 或动态场景：

```text
python out\cs-t0\build-manager-this-mt-r05-filtered-vector-v3.py
PASS created=out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3.json sha256=DEB2E1301025DA0AB46DFBA2E02017F0BA6A8F27C7EAEE2C7F4FD2C15B9291A6 accepted=0 unknown=1 mutations=3/3

python out\cs-t0\build-manager-this-mt-r05-filtered-vector-v3.py --verify-existing
PASS existing=out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3.json sha256=DEB2E1301025DA0AB46DFBA2E02017F0BA6A8F27C7EAEE2C7F4FD2C15B9291A6 accepted=0 unknown=1 mutations=3/3
```

下一步只应由作者分离的独立 TEST 使用已封印规则验收本公开候选；REVERSE 不执行该验收。
