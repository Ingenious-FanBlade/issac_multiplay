> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this fill-n298 exact-gap v14 handoff

日期：2026-09-02

## 结论

v13 唯一缺口
`SUB_9C8350_NESTED_FORWARDER_CALLER_ROOT_UNIVERSE` 已在当前固定目标、PE 映像和
corrected whole-image v2 的静态范围内闭合。`sub_9CC1A0` 与 `sub_9CE5A0`
所有直接入口、address-taken 和已识别 split/indirect formation 均已枚举；所有实际
进入 `sub_9C8350 -> sub_9C79A0` 的 receiver 都是 `n298+0xFA4`，没有外来
pointee。

因此 v13 的 exact6 可从 `0 VERIFIED / 6 UNKNOWN` 升级为
`6 VERIFIED / 0 UNKNOWN`。这是六行 range/receiver closure 组件，不是全局
manager slot whole-image alias 或 manager-this 总体升格：全部 permission 和
promotion 仍为 false。

## 绑定

- v13：`out/cs-t0/native-join-manager-this-fill-n298-six-v13.json`
  SHA-256 `9DA0FD4893A6D393D5721B973F4E89623B463CB82506BE5A65DCAE6BB5CACFF4`。
- corrected whole-image component v2：
  `out/cs-t0/native-join-manager-this-indirect-split-component-v2.json`
  SHA-256 `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`。
- corrected v2 analyzer：
  `out/cs-t0/analyze-manager-this-indirect-split-component-v2.py`
  SHA-256 `A5B09328D4B86B7B0F6B772980260FFA4D33722C06CB9C3BD9FCBC71231EACB6`。
- 目标 `isaac-ng.exe` observed SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`；
  PE32 x86，image base `0x00400000`，SizeOfImage `0x008FE000`。

corrected v2 的 executable membership 是
`IDA segment has execute permission && within PE SizeOfImage`，实际 `.text` 为
VA `[0x00401000,0x00B18000)`。旧遗漏尾部含 550 个函数起点，但本闭包 reachable
delta 为空。v2 本身仍为 `PENDING_BULK_JOIN`，本产物没有改写或绕过该状态。

## direct xref 与 receiver provenance

IDA 只读 exact、非截断 xref：

| target | exact xrefs | data xref |
|---|---|---:|
| `sub_9C8350` | `0x95915D`, `0x95A243`, `0x9CC254`, `0x9CE68F`, `0x9D9DF7` | 0 |
| `sub_9CC1A0` | `0x918366`, `0x95A1F7`, `0x9CE677` | 0 |
| `sub_9CE5A0` | `0x95A03E` | 0 |
| `sub_959FA0` | `0x908299`, `0x959F64`, `0x98ABDA` | 0 |
| `sub_959E70` | `0x919E97`, `0x91A417`, `0x9D992F`, `0x9D9DEC`, `0x9D9E4D`, `0x9D9E86`, `0x9D9EF4`, `0x9DB1D8`, `0x9DB23D` | 0 |

四个 gap caller 的 receiver：

- `0x918366`：`0x918321` 加载 `n298`，`0x918332` 加 `0xFA4`。
- `0x95A1F7`：`sub_959FA0` 保存 `this` 到 `esi`；`0x95A1BB` 形成
  `ebx=esi+0xFA4`，`0x95A1C5` 传入 `ecx`。
- `0x95A03E`：同一函数在 `0x959FBF` 形成 `ebx=esi+0xFA4`，到
  `0x95A03C` 没有重定义该 receiver，随后传给 `sub_9CE5A0`。
- `0x9CE677`：`sub_9CE5A0` 在 `0x9CE5AC` 保存 `ecx` 到 `edi`，
  `0x9CE641` 原样恢复。

`sub_9CC1A0` 在 `0x9CC1E1` 保存 receiver 到 `esi`，`0x9CC252` 原样送到
`0x9CC254: sub_9C8350`。`sub_9CE5A0` 在 `0x9CE68D` 原样送到
`0x9CE68F: sub_9C8350`。`sub_9C8350` 在 `0x9C835F` 保存 receiver，
两条尾入口 `0x9C83C8` / `0x9C8405` 均恢复同一 `ecx` 并 jump
`sub_9C79A0`。

上游 `sub_959FA0` exact3 中，`0x908299` 和 `0x98ABDA` 都在调用前直接加载
`n298`；`0x959F64` 保持 `sub_959E70.this`。`sub_959E70` 的 exact9
receiver 均由调用点直接加载 `n298`，或保持同一 nonvolatile `n298` 别名。

## address-taken / split / indirect formation

对整个固定 PE 文件的 little-endian absolute VA 和 RVA 扫描得到：

- `sub_9C8350`, `sub_9CC1A0`, `sub_9CE5A0`, `sub_959FA0`,
  `sub_959E70` 的 absolute VA occurrence 均为 0；对应 RVA occurrence 也均为 0。
  它们没有函数指针表或普通 address-taken producer。
- target-specific exact RVA immediate `0x005C8350`, `0x005CC1A0`,
  `0x005CE5A0` 均为 0；absolute page `0x009C0000` immediate 为 0；低位
  `0x8350`, `0xC1A0`, `0xE5A0` immediate 均为 0。
- 唯一 `0x005C0000` immediate 位于 `0x00A81E68`：
  `mov [ebp+DevMode.dmFields],0x5C0000`，是 Windows `DEVMODE.dmFields`
  bitmask，与代码地址形成无关。
- corrected v2 已证明唯一 self-image-base producer `RVA 0x00504A00` 只进入
  DbgHelp/range/symbolization 的只读 consumer，因此不存在 image-base + target RVA
  的可执行转发。

唯一相关 address-taken entry 是 `0x90E748: push offset sub_908070`。它是上游
callback，而不是三个关键 forwarder 的地址；callback body 在 `0x908290` 重新加载
全局 `n298` 后才调用 `sub_959FA0`，因此无论 callback 的调用者参数如何，都不会把
外来 receiver 传入本链。

以上结论限于 corrected v2 的
`EXACT_IMAGE_AND_BOUND_CALLBACK_ALL_THREAD_INDIRECT_AND_SPLIT_COMPONENT_NO_EXTERNAL_MODULE_OR_INJECTION`
范围，不外推到外部模块或注入代码。

## exact6 升级

v13 已证明其余 direct receiver 分区、`n60` embedded selector、stack root、真实
allocation extent 与 manager slot domain 不相交；六行唯一 UNKNOWN source 都是本轮
已闭合 gap。因此六行现在均为
`VERIFIED_STATIC_RECEIVER_UNIVERSE_AND_ALLOCATION_EXTENT`：

| callsite RVA | start byte | count | end exclusive | 新闭合 receiver 的不等式 |
|---|---:|---:|---:|---|
| `0x005C7A15` | 14 | 53 | 67 | `0xFA4+67 <= 0x4D088` |
| `0x005C7BAD` | 296 | 48 | 344 | `0xFA4+344 <= 0x4D088` |
| `0x005C7C8D` | `25932*4 = 103728` | 2028 | 105756 | `0xFA4+105756 <= 0x4D088` |
| `0x005C7E7A` | `31056*4 = 124224` | 124 | 124348 | `0xFA4+124348 <= 0x4D088` |
| `0x005C7EC6` | `31101*4 = 124404` | 75 | 124479 | `0xFA4+124479 <= 0x4D088` |
| `0x005C7ED6` | `31120*4 = 124480` | 62 | 124542 | `0xFA4+124542 = 128546 <= 315528 (0x4D088)` |

## 产物与复现

- `out/cs-t0/native-join-manager-this-fill-n298-six-v14.json`
  SHA-256 `B8BC3D56C763DAEBB078538A67AB18DB6DD96CC7CC3D5C99749A9FEBADB66694`。
- `out/cs-t0/analyze-manager-this-fill-n298-six-v14.py`
  SHA-256 `E86CF1AC0D37A81C62F47492F6499467D4AEE3EA1A6B7DC6114F8FA021961D1F`。

```powershell
python out\cs-t0\analyze-manager-this-fill-n298-six-v14.py --verify-existing
```

预期：

```text
PASS existing=out/cs-t0/native-join-manager-this-fill-n298-six-v14.json sha256=B8BC3D56C763DAEBB078538A67AB18DB6DD96CC7CC3D5C99749A9FEBADB66694 verified=6 unknown=0 mutations=18 gap=closed promotions=0
```

reproducer 绑定 v13、corrected v2、v2 analyzer、目标哈希、PE executable range、
所有语义 anchors、direct rel32 target、absolute/RVA occurrence 数、exact6 顺序与
scaled arithmetic；18 个语义变异必须全部 fail-closed。

## 权限边界

未启动游戏、未附加 debugger、未打开或读取进程、未修改 IDB、production、tests 或
既有证据。`runtimeMutationAllowed=false`、`wholeImagePromotionAllowed=false`、
`managerThisPromotionAllowed=false`。
