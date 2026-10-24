> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this SSO exact1 superseder v22 handoff

## 结论

本轮只处理 `RVA 0x000148AA`（VA `0x004148AA`），精确覆盖 1 行、没有附带其他行。结论保持 `UNKNOWN`，不允许 row、whole-image alias 或 manager-this promotion；`overallAccepted=false`，所有权限均为 `false`。

v22 supersede 的是 v21 唯一 UNKNOWN 行中的接收者归因，而不是修改 v21 文件。v21 将 `0x0091F3B0` 的接收者描述为 `UserProfileBase-derived this+0x0C`；逐指令恢复表明，真正链路是：

- `0x0091E4B5 mov ecx,edi`，其中 `EDI=v16=v89=a1`，是 `sub_91DC10` 的状态对象；
- `0x0091E4B7 call sub_91EE90`；
- `0x0091EEBD lea edi,[ebx+0x0C]` 形成状态对象的 SSO 字段指针；
- `0x0091F3AB mov ecx,edi`，`0x0091F3B0 call sub_414870`；
- `sub_91DC10` 中出现的 `UserProfile`/`UserProfileBase` vtable 写入属于临时/profile 对象，不能给该状态对象定型。

实际 receiver 有两个 enclosing-container 根：

1. `sub_8E8240`：`0x008E85BD add ecx,0x4C608`，在 `0x008E85CF` 或 `0x008E85EE` 调 `sub_91DC10`；字段 alias 是 `n298+0x4C614`。
2. `sub_90B150`：`0x0090B64B lea ecx,[edi+0x1230]`，`0x0090B651` 调 `sub_91DC10`；字段 alias 是 `owner+0x123C`。

因此 v21 的 `UserProfileBase-derived this+0x0C` 归因被 v22 明确 supersede；v21 报告、分析器和 handoff 的 hash 均被绑定且未修改。

## 已闭合的精确 census

`sub_414870` 的 exact direct/tail xref universe 为 8 个：

`0x00414545, 0x004D17FF, 0x0051E323, 0x0082AF51, 0x008C90D4, 0x008CD5F2, 0x0091F3B0, 0x009E9E0F`。

其中 `0x00414545` 是 `sub_414520` 的 tail jump；`sub_414520` 的 exact callers 为 `0x0047F2FE, 0x0047F3BE`。

本 seed 的上游 exact xrefs：

- `sub_91EE90`：唯一 `0x0091E4B7`；
- `sub_91DC10`：`0x008E85CF, 0x008E85EE, 0x0090B651`；
- `sub_91DB20`：`0x00906BAD` direct call 与 `0x00B0896C` unowned tail jump。

独立位移 census：

- `0x4C608` 原始 4-byte occurrence 9 个，命中列表 SHA256 `C621A5696B30F8FD369EFFD18F0B45F4E0544F3247834E367C474CFA59ED5E05`；其中 8 个为解码指令，`0x0077C7B4` 是嵌在 call bytes 中的 raw false positive。
- `0x1230` 原始 occurrence 17 个，命中列表 SHA256 `7FBFDCFD13E5352AD12C2C624AE8F1EE0D65C5B3922F57CB26CD1982FC5015BB`；其中 16 个为解码指令，`0x0080D0B5` 是 jump-table/raw false positive。
- `0x00B08966 add ecx,0x1230; 0x00B0896C jmp sub_91DB20` 没有 IDA owning function，是必须纳入 successor relation 的 split/EH member-destructor forwarder。

用于排除错误类型归因的 vtable census：

- `UserProfile` vtable `0x00B82D98`：158 个 raw occurrence，命中列表 SHA256 `61F9D39D413EFF4352D04C3AF67EE941AD2CB90E8676613AE4BF78AE328B4CC6`，另有 unowned heads `0x00A7EDAF, 0x00A7EEA6`。
- `UserProfileBase` vtable `0x00B82E20`：217 个 raw occurrence，命中列表 SHA256 `260D9FC4FFB31924E517C7A9838CE9BD1DE2B05C64A6A65B6F92DFC2B5DC3E7C`，另有 unowned heads `0x008E1F88, 0x008E2ABE, 0x00A7EDFA, 0x00A7EE97, 0x00A7EF17`。

这些 raw/vtable census 是独立从本 seed 生成的；没有把 v4 narrative 当作证明。

## site-local 范围证明及其条件

`sub_414870` 的对象布局为 24-byte MSVC SSO string：`this+0x00` 是 16-byte inline/heap union，`this+0x10` 是 size，`this+0x14` 是 capacity；`capacity<16` 使用 inline storage，`capacity>=16` 使用 heap storage，合法 heap extent 为 `capacity+1`。

在 `0x004148AA` 的 in-place 分支，谓词是无符号 `Size <= capacity-oldSize`；destination 是 `storage+oldSize`，count 是 `Size`，后续 `0x004148B2` 写 terminator。因此如果 incoming SSO invariant 成立，则：

`oldSize+Size <= capacity < capacity+1`。

该不等式和 no-wrap 只在 `capacity>=oldSize`、size/capacity/union tuple 合法时成立。已绑定 `sub_40CCD0` assignment、`sub_40CF50` copy constructor、`sub_40D040` destructor；但目前不能证明它们是双实际根字段的全部 writer。

## 最小首缺口

机器分类：`ACTUAL_RECEIVER_DUAL_ROOT_ALREADY_OFFSET_ALIAS_FIXED_POINT_OPEN`。

必须继续闭合的最小集合是：

1. `0x00B08966/0x00B0896C` unowned split/EH destructor fragment 的 successor 边；
2. `n298+0x4C614` 与 `owner+0x123C` 的 stack spill/reload alias；
3. 携带这两个 already-offset alias 的 return/stack liveout；
4. 接收这两个 alias 的 address-taken/indirect call；
5. 不再出现字面 `+0x4C608/+0x1230` 的 copy/move/assignment/swap/adoption/reallocation writer。

在这个 successor-first fixed point 闭合前，不能证明 incoming capacity/size/union 永远合法，也不能证明 heap union 只能安装自身分配的 `capacity+1` buffer、从未 adopt manager slot `0x00C71678..0x00C7167C`。所以 `explicitSlotOverlapObserved=false` 不能提升为 `fullyAdjudicatedNoOverlap=true`。

## 复现

PE 模式连续运行两次：

```powershell
python out\cs-t0\analyze-manager-this-fill-sso-one-v22.py
python out\cs-t0\analyze-manager-this-fill-sso-one-v22.py
```

输出分别为 `created` 与 `verified`，两次均为：

```text
sha256=E2B7637CE96B8D2AFB29B9FC5455B05A0C7D29BBA2EE425C3FDB917806DBCE27 rows=1 verified=0 unknown=1 mutations=20/20 permissions=false promotions=false overallAccepted=false
```

IDA 只读重放：在冻结的 `isaac-ng.exe` IDB 中运行：

```python
exec(compile(open(r"D:\projects\isaac_hook_reserach\out\cs-t0\analyze-manager-this-fill-sso-one-v22.py", "rb").read(), r"D:\projects\isaac_hook_reserach\out\cs-t0\analyze-manager-this-fill-sso-one-v22.py", "exec"))
```

预期：

```text
PASS mode=ida-read-only verified=out/cs-t0/native-join-manager-this-fill-sso-one-v22.json sha256=E2B7637CE96B8D2AFB29B9FC5455B05A0C7D29BBA2EE425C3FDB917806DBCE27 rows=1 verified=0 unknown=1 mutations=20/20 permissions=false promotions=false overallAccepted=false
```

IDA 重放额外验证 function boundaries、exact xrefs、解码 instruction heads、vtable unowned heads，并确认 `ida_funcs.get_func(0x00B08966) is None`；不修改 IDB。

## 产物

- `out/cs-t0/analyze-manager-this-fill-sso-one-v22.py`  
  SHA256 `B1FA6D2CE5FAC9A26A06B14A2E9A0DA2061A559FE7C62F040EBE95B4450205BC`
- `out/cs-t0/native-join-manager-this-fill-sso-one-v22.json`  
  SHA256 `E2B7637CE96B8D2AFB29B9FC5455B05A0C7D29BBA2EE425C3FDB917806DBCE27`

本轮未运行游戏、未附加调试器、未打开/读取进程、未修改 IDB/生产代码/测试/既有证据。
