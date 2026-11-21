> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this Persistent static exact5 v16 handoff

日期：2026-09-02

## 结论

`STATIC_C812B0_LOGICAL_EXTENT_OPEN` 已在固定目标上静态闭合。exact5 当前为
`5 VERIFIED / 0 UNKNOWN`：

- `0x005262DD`
- `0x005262F0`
- `0x00526342`
- `0x00526352`
- `0x005263B5`

结论不依赖“下一 IDA 名称”或“相邻地址曾被访问”这一个事实。相反，三个相互
独立的相同构造函数布局都给出 `0xF90 = 3984` 字节，并由匹配的函数局部静态
初始化/析构协议闭合逻辑对象身份：

1. `sub_7BBBD0` 的 `0x7BC6ED..0x7BC736` 是完整 MSVC thread-safe
   function-local-static 序列：guard `0xC812AC` 经
   `__Init_thread_header/footer`，`ECX=0xC812B0` 调用 `sub_925F80`，随后把
   `sub_B15BA0` 注册给 `_atexit`；异常路径 `sub_AFFE52` 对同一 guard 调用
   `__Init_thread_abort`。
2. 匹配的 `sub_B15BA0` 对 `0xC812B0+4/+8/+32/+3940` 执行析构/清理。
   因而 `+3940` 是该已注册静态对象的内部成员，不是仅凭相邻读取推测出的边界。
3. `sub_9154F0` fresh 分支分配 `0xFA8`，以 `allocation+0x18` 调用同一
   `sub_925F80`，可用 receiver 尾部精确为 `0xFA8-0x18=0xF90`。
4. 父构造 `sub_950610` 在 `this+0x14` 调用同一 `sub_925F80`，下一成员构造
   位于 `this+0xFA4`，跨度同样为 `0xFA4-0x14=0xF90`。父析构
   `sub_951160` 先逆序析构 `+0xFA4` 下一成员，再清理该 Persistent 成员内部
   `+0xF78 = +0x14 + 0xF64` 等成员。
5. 静态基址 `0xC812B0` 到下一独立对象 `0xC82240` 的跨度也正好是
   `0xF90`；后者仅由另一函数 `sub_7CF9A0` 的三处地址形成引用。

所以逻辑对象 extent 至少为 3984，强于 exact5 所需最大末端 3940。PE `.data`
映射只作为物理可用性原子：目标位于 loader zero-fill virtual tail，而非原始文件
数据；报告明确保留 `PE mapping alone rejected=true`。

## 完整地址形成与 receiver alias

静态区间 `[0xC812B0,0xC82240)` 的全部 direct data xref 精确为五个：

| target | source | 角色 |
|---|---|---|
| `0xC812B0` | `0x7BC707` | constructor receiver base |
| `0xC812B4` | `0xB15BC9` | registered destructor member `+4` |
| `0xC812B8` | `0xB15BED` | registered destructor member `+8` |
| `0xC812D0` | `0xB15BE3` | registered destructor member `+32` |
| `0xC82214` | `0xB15BD9` | registered destructor member `+3940` |

`sub_925F80` direct caller exact8、`sub_9262B0` direct caller exact11、
`sub_91ADF0` direct caller exact5 均由脚本重新枚举。静态 alias 唯一路径为：

```text
0x7BC707 ECX=0xC812B0
  -> 0x7BC713 sub_925F80
  -> 0x925FFC same receiver sub_9262B0
```

静态对象没有进入 `sub_91ADF0` 的 alias；其 exact5 direct receivers 与本区间的
完整 direct-dref universe 均不包含该静态 base。

## exact5 算术

| callsite RVA | start | count | end exclusive | extent | 状态 |
|---|---:|---:|---:|---:|---|
| `0x005262DD` | 56 | 642 | 698 | 3984 | VERIFIED |
| `0x005262F0` | 2792 | 733 | 3525 | 3984 | VERIFIED |
| `0x00526342` | 3591 | 104 | 3695 | 3984 | VERIFIED |
| `0x00526352` | 3695 | 46 | 3741 | 3984 | VERIFIED |
| `0x005263B5` | 3860 | 80 | 3940 | 3984 | VERIFIED |

全部加法无 32-bit 回绕。静态对象区间 `[0xC812B0,0xC82240)` 与 manager slot
`[0xC71678,0xC7167C)` 数值不相交。

## 冻结输入与产物

- v15 exact6 parent：`out/cs-t0/native-join-manager-this-fill-persistent-six-v15.json`
  SHA-256 `B5263197325F56B59D776DF8F73E04DD258EAF876BD9C30AF366EC257625A213`
- range v5 set membership：
  `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json`
  SHA-256 `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`
- corrected whole-image v2：
  `out/cs-t0/native-join-manager-this-indirect-split-component-v2.json`
  SHA-256 `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`
- v16 JSON：
  `out/cs-t0/native-join-manager-this-fill-persistent-static-five-v16.json`
  SHA-256 `A6A0431E9A0FAB0A46ABDFE713389EF01808D5546999715D2A4A9CF039355E4E`
- v16 IDA reproducer：
  `out/cs-t0/analyze-manager-this-fill-persistent-static-five-v16.py`
  SHA-256 `59E43A2D65440FB794A12A1D853F2F3A1289560AA99290443B1600A7E65F23A3`

在冻结 IDB 内执行 reproducer，create-new 与 verify-existing 两次结果分别为：

```text
PASS created exact_rows=5 verified=5 unknown=0 logical_extent=3984 drefs=5 mutations=24
PASS verified exact_rows=5 verified=5 unknown=0 logical_extent=3984 drefs=5 mutations=24
```

脚本绑定 11 个完整函数 byte hash、三份冻结输入、目标 hash、corrected executable
range、PE `.data` 几何、三组 direct caller universe、全静态区间 dref/name partition、
局部静态 guard/header/footer/abort/atexit 语法、fresh/parent 两个 `0xF90` 布局及 24 个
fail-closed mutation。

## 边界

本产物只允许作为 exact5 正向分区合并。`0x0051B6A7` 的 existing map-node gap
未处理，remaining memset/whole-image/manager-context 也未闭合；因此
whole-image 与 manager-this promotion 均为 `false`，全部 runtime/process/IDB/
production/test 权限均为 `false`。

本逆向子任务没有兼任最终独立测试验收；独立测试方仍需按
`isaac-cs-refactor` 分工重放公开产物与 mutations。
