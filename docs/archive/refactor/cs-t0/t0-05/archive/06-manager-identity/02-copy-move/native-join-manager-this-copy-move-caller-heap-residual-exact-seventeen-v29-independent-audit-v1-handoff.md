> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 caller-heap residual exact17 v29 独立审计 v1 交接

日期：2026-09-05  
角色：`INDEPENDENT_TESTER / TEST-CALLERHEAP-17`  
状态：`PASS_NEGATIVE_CANDIDATE_0V17U_INDEPENDENTLY_REPRODUCED`

## 0. 未封印半成品审查

接手时只有一份未封印脚本，没有 audit JSON 或 handoff，不能计为证据。逐项审查发现并在首次成功 create-new 前修正：

- `0x0068E3DB` 的 69-byte context SHA-256 写成了错误值；独立 PE 重算确认应为 `5F84232B23EAD082C8302BD91FB2FBC8E3D06F88B1CCA89CD8ACF6B438FD1929`；
- 脚本期待了不存在的 candidate decision 枚举 `RETAIN_UNKNOWN`，冻结 JSON 的实际值为 `UNKNOWN_RETAIN_EXACT_V5_COUNTERMODEL`；
- v5 含历史已闭行，其中没有 `v5Status` 字段；UNKNOWN exact22 重算改为显式只选取 `v5Status == UNKNOWN` 的行，并要求集合严格相等。

修正过程中一次预创建执行在读取历史已闭行时以 `KeyError` 失败，尚未写出 JSON；随后才执行下文唯一的成功 create-new。没有覆盖、改写或删除任何证据文件。

## 1. 结论

作者分离的独立测试接受 caller-heap residual exact17 candidate 的失败安全结论：

- 精确全集：`17/17`，无重复、无额外行；
- 正向闭合：`0`；
- 保持 `UNKNOWN`：`17`；
- 与 v26 已接受 5 行的交集：空；
- v5 的 `UNKNOWN` exact22 已独立核对为 `exact17 ∪ accepted5`；
- 14 个 containing function 的 raw relative `E8` caller 全集与集合摘要均从磁盘 PE 重算；对应 relative `E9` forwarder 全部为空；
- 两个 address-taken method 的 whole-file pointer-literal census 分别为 `2` 与 `42`，未类型闭合的 virtual dispatch 缺口仍在；
- 1 个正向基准验证通过，85 个负向语义 mutation 全部被拒绝。

因此 candidate 的 `0 VERIFIED / 17 UNKNOWN` 可以作为独立负向审计结果进入后续证据合成；它不提供任何新正向行。copy/move 正向计数保持 `319/405`，remaining 保持 `86`，不得据此把 `manager-this`、whole-image alias 或正式 frontier 提升为 PASS。

## 2. 作者分离与方法

本 tester 未参与 candidate analyzer、candidate JSON 或 candidate handoff 的编写。独立脚本：

- 不 import candidate analyzer，也不把 candidate decision 当作真值；
- 使用独立 `struct` PE32 parser 读取磁盘目标；
- 从 v26 `copyMove.groups.callerHeap` 重算 exact17 身份，并与 v5 UNKNOWN exact22、accepted5 做全集/差集/互斥检查；
- 对每行重算 exact `E8` bytes、rel32 target 和 69-byte context SHA-256；
- 独立遍历 `.text` 重算 14 个 owner 的所有 raw relative `E8/E9` transfer；
- whole-file 搜索 `0x00A57DB0`、`0x00A582E0` 的 address-taken literal，并核对未闭合 vtable entry；
- 只有在磁盘重算和冻结 v5 countermodel 同时成立时，才接受 candidate 继续保持 UNKNOWN。

candidate 的 85-mutation 自报只作为受检输入；本审计另有自己的 85 个 mutation gate，不以 candidate 自报替代独立验收。

## 3. exact17 逐行结果

| callsite RVA | helper | 独立结果 | 保留的首缺口 |
| --- | --- | --- | --- |
| `0x00008A62` | memcpy | UNKNOWN | `ALLOCATED_FOR_NEW_COUNT_BUT_COPIED_WITH_OLD_COUNT` |
| `0x00008B2A` | memcpy | UNKNOWN | `ALLOCATED_FOR_NEW_COUNT_BUT_COPIED_WITH_OLD_COUNT` |
| `0x0001513A` | memcpy | UNKNOWN | `FIXED_5000_ELEMENT_HEAP_ARRAY_WITH_NON_BLOCKING_LIMIT_LOG` |
| `0x000155A4` | memcpy | UNKNOWN | `FIXED_5000_ELEMENT_HEAP_ARRAY_WITH_NON_BLOCKING_LIMIT_LOG` |
| `0x0001AAB0` | memcpy | UNKNOWN | `DESTINATION_CAPACITY_CHECK_LOGS_BUT_DOES_NOT_STOP_COPY` |
| `0x0001AEB8` | memcpy | UNKNOWN | `DESTINATION_CAPACITY_CHECK_LOGS_BUT_DOES_NOT_STOP_COPY` |
| `0x001CBEB5` | memmove | UNKNOWN | `VECTOR_ERASE_ITERATOR_PROVENANCE_OPEN` |
| `0x0033570B` | memmove | UNKNOWN | `FILTERED_VECTOR_CAN_BE_EMPTY_BEFORE_RANDOM_ERASE` |
| `0x0061F2C2` | memcpy | UNKNOWN | `DOUBLE_BUFFER_POINTER_WRITER_SET_OPEN` |
| `0x00657E0A` | memcpy | UNKNOWN | `DIRECT_CALLERS_BOUNDED_BUT_VTABLE_DISPATCH_UNIVERSE_OPEN` |
| `0x00657E94` | memcpy | UNKNOWN | `DIRECT_CALLERS_BOUNDED_BUT_VTABLE_DISPATCH_UNIVERSE_OPEN` |
| `0x00658338` | memcpy | UNKNOWN | `DIRECT_CALLERS_BOUNDED_BUT_VTABLE_DISPATCH_UNIVERSE_OPEN` |
| `0x006583C2` | memcpy | UNKNOWN | `DIRECT_CALLERS_BOUNDED_BUT_VTABLE_DISPATCH_UNIVERSE_OPEN` |
| `0x00658487` | memcpy | UNKNOWN | `BUFFERED_STREAM_BEGIN_WRITER_SET_OPEN` |
| `0x00661577` | memcpy | UNKNOWN | `SECOND_STATIC_RING_BUFFER_LENGTH_BOUND_OPEN` |
| `0x00689F63` | memcpy | UNKNOWN | `CALLER_OUTPUT_POINTER_DOMAIN_OPEN` |
| `0x0068E3DB` | memmove | UNKNOWN | `REALLOC_BUFFER_OTHER_WRITERS_OPEN` |

原因类分布保持：

- `HEAP_ALLOCATION_CAPACITY_RELATION_OPEN = 2`；
- `CALLER_PARAMETER_DOMAIN_OR_SPAN_OPEN = 9`；
- `POINTER_OR_CONTAINER_LOAD_UNRESOLVED = 5`；
- `RAW_REGISTER_OR_STACK_SLICE_OPEN = 1`。

## 4. caller 与 virtual universe 重算

14 个 containing function 的 raw direct `E8` caller 数为：

| function VA | E8 | E9 |
| --- | ---: | ---: |
| `0x00408970` | 2 | 0 |
| `0x00414E00` | 2 | 0 |
| `0x00415170` | 1 | 0 |
| `0x0041A790` | 177 | 0 |
| `0x0041AB50` | 5 | 0 |
| `0x005CBEA0` | 4 | 0 |
| `0x00734F30` | 7 | 0 |
| `0x00A1F280` | 1 | 0 |
| `0x00A57DB0` | 25 | 0 |
| `0x00A582E0` | 30 | 0 |
| `0x00A583E0` | 9 | 0 |
| `0x00A614D0` | 2 | 0 |
| `0x00A89DE0` | 2 | 0 |
| `0x00A8E1B0` | 2 | 0 |

address-taken census：

- method `0x00A57DB0`：2 个 literal；vtable entries 为 `0x00B9ECE0`、`0x00B9ED3C`；
- method `0x00A582E0`：42 个 literal；未类型闭合的 vtable entry 为 `0x00B9ED10`。

这些全集检查能防止 candidate 隐去已知 direct/absolute transfer，但仍不能把未类型化的间接调用域推成闭合；相应 4 行继续 UNKNOWN 是正确的失败安全结果。

## 5. mutation 与重放

独立 mutation gate 包含：

- 17 行 × 4：篡改 row status、call bytes、countermodel、no-wrap predicate，共 68 项；
- 14 个 owner universe count 篡改；
- 2 个 pointer-literal universe count 篡改；
- 1 个非法 positive promotion。

合计 `85/85` 均 fail closed。未变异的正向基准通过。

执行命令：

```powershell
python .\out\cs-t0\audit-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-v1.py
python .\out\cs-t0\audit-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-v1.py --verify-existing
python .\out\cs-t0\audit-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-v1.py --verify-existing
```

三次结果均为 PASS；create-new 与两次 existing replay 的 machine JSON SHA-256 均为：

`6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD`

## 6. 冻结输入与产物

| 对象 | SHA-256 |
| --- | --- |
| candidate analyzer | `76E0A41D823DE2B8BDF0C566962F7A5641F36938062540CA6C855CDF3B932F69` |
| candidate JSON | `BFE51D1DAEE85058F8904EA1327FECD8A4A1AA8A9EE81CEFD0C01A73CB637DAA` |
| candidate handoff | `1C9271EBC2E9647E3DBAE4EAA67BD6C6F1FB63AC4FAE70E61998422FBE18F8D3` |
| caller-heap v5 analyzer | `E8E78C9AD6D6E07411E6CA1E0287190AA4E1D41D97E73A3454F58AFEC18FF48D` |
| caller-heap v5 JSON | `5D46EB95A7575EDC6384B4D15D6031C7F78945041FF98216AB1F8AB63DCEF6EA` |
| frontier v26 analyzer | `7E74CE0C92DBB253059CF4560FEDF29EF2C25975F38E81CF96B0236E62EBFE07` |
| frontier v26 JSON | `514399E365C79126E9256993549B957BF93800BDF8C1A788CEE588E3FE0328EF` |
| disk target `isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |
| independent audit script | `BF2B21BE10D8D467F4620CD00375D69E9333FA0519C4845BFA501AEF07E80A33` |
| independent audit JSON | `6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD` |

产物：

- `out/cs-t0/audit-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-v1.py`
- `out/cs-t0/native-join-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-audit-v1.json`
- `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-audit-v1-handoff.md`

handoff 自身 SHA-256 在文件生成后单独计算，不自引用写入本文。

## 7. 安全与后续边界

本审计仅读取磁盘文件和磁盘 PE；未启动或打开游戏进程，未读取运行时内存，未注入，未使用或连接 x32dbg/IDA，未修改 IDB、candidate、生产代码、正式 plan/progress/verification/index/frontier 或历史证据。全部 permission/promotion 字段为 false。

后续 `COMPOSE-FRONTIER-SUCCESSOR` 可以绑定本独立审计，将 callerHeap residual exact17 计入负向覆盖；这只表示所有 86 个 copy remaining 行都有作者分离的 UNKNOWN 审计，不会把任何 UNKNOWN 行转成正向证明。manager-this 完整证明仍须逐项闭合真实 backing extent、writer/type fixed point、generation 与 lifecycle 条件。
