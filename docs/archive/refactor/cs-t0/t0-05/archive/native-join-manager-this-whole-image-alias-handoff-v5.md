> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this executable-range audit and indirect/split component handoff v5

本文是 create-new 静态逆向交接，修正 v4/v1 component 的 executable-range 边界。没有启动或附加游戏，没有使用 x32dbg，没有打开游戏进程，没有修改 IDB、production、tests 或 v1-v6 封印产物。

机器 component：`out/cs-t0/native-join-manager-this-indirect-split-component-v2.json`  
SHA-256：`D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`

IDA 复现器：`out/cs-t0/analyze-manager-this-indirect-split-component-v2.py`  
SHA-256：`A5B09328D4B86B7B0F6B772980260FFA4D33722C06CB9C3BD9FCBC71231EACB6`

## 1. 修正结论

v1 analyzer 的 `IMAGE_END=0x00B00000` 不是 executable-code section end。实际 `.text` 映射到 VA `[0x00401000,0x00B18000)`；遗漏的 `[0x00B00000,0x00B18000)` 内有 550 个 IDA 函数起点。因此以下旧件必须永久保留为失败／中间证据，不能进入 promotion binding：

- `out/cs-t0/analyze-manager-this-indirect-split-component-v1.py`，SHA `976E7DEA2C349CF5FD411D11545EAC225254997DB3A8BF62CAB4168EFA662BAC`；
- `out/cs-t0/native-join-manager-this-indirect-split-component-v1.json`，SHA `EA6560B15FE6F60D694474D05042206662EB07E4C244C8A04B7097C8261532A5`；
- `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-whole-image-alias-handoff-v4.md`，SHA `913640684ACBC485C9409F06BD97CE6857BF3B6552B610CFB61858271E95A1FB`。

v2 不再用任意 `IMAGE_END` 判断 in-image callee，而使用 `IDA segment has execute permission && VA within PE SizeOfImage`。对真实 executable ranges 重跑后：

- reachable function 仍为 5,646；
- reachable bulk-helper callsite 仍为 647；
- helper 分布仍为 memcpy 159 / memmove 246 / memset 242；
- v1/v2 的 reachable function RVA 数组逐项相等；
- v1/v2 的 reachable bulk callsite RVA 数组逐项相等；
- `.text` 遗漏尾部新增 reachable function 为 0，新增 reachable bulk callsite 为 0。

这说明 v1 的边界方法无效，但本次真实 executable-range 重扫没有改变 indirect/split component 的闭包集合。只有 v2 可作为后续证据绑定。

## 2. 目标与 PE section 证据

- IDA 输入：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`
- observed SHA-256：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- canonical SHA-256：`C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`
- architecture：PE32 x86；image base `0x00400000`；SizeOfImage `0x008FE000`；image end VA `0x00CFE000`
- SectionAlignment `0x1000`；FileAlignment `0x200`
- GameRoot slot：VA `0x00C71678` / RVA `0x00871678`，宽度 4。

从固定 SHA 输入的 PE section table 直接解析得到：

| section | RVA start | VirtualSize | mapped end RVA | Characteristics | X | R | W |
| --- | ---: | ---: | ---: | ---: | :---: | :---: | :---: |
| `.text` | `0x00001000` | `0x00716134` | `0x00718000` | `0x60000020` | yes | yes | no |
| `.rdata` | `0x00718000` | `0x000DF948` | `0x007F8000` | `0x40000040` | no | yes | no |
| `.data` | `0x007F8000` | `0x000A4AA4` | `0x0089D000` | `0xC0000040` | no | yes | yes |
| `.rsrc` | `0x0089D000` | `0x00003B20` | `0x008A1000` | `0x40000040` | no | yes | no |
| `.reloc` | `0x008A1000` | `0x0005C93C` | `0x008FE000` | `0x42000040` | no | yes | no |

`IMAGE_SCN_MEM_EXECUTE=0x20000000`。只有 `.text` 的 Characteristics 含该位；其未对齐 virtual end 是 RVA `0x00717134`，按 SectionAlignment 的 mapped end 是 RVA `0x00718000` / VA `0x00B18000`。

IDA segment 交叉验证为：

- `.text` VA `[0x00401000,0x00B18000)`，permissions 5（RX）；
- `.idata` VA `[0x00B18000,0x00B18A28)`，permissions 4（R）；
- `.rdata` VA `[0x00B18A28,0x00BF8000)`，permissions 4（R）；
- `.data` VA `[0x00BF8000,0x00C9D000)`，permissions 6（RW）。

IDA 把 PE `.rdata` 的导入子范围单独显示为 `.idata`，不改变 executable 判定。`[0x00B18000,0x00CFE000)` 的 PE sections 全部无 execute bit；IDA 在该范围内 executable segment 数为 0，executable function start 数也为 0。`[0x00C9D000,0x00CFE000)` 是未单独映射的 section tail/alignment 范围，同样没有 executable PE section。因此 `[0x00B00000, SizeOfImage end)` 中只有 `[0x00B00000,0x00B18000)` 可能含可执行函数，并已由 v2 纳入重扫。

## 3. executable tail 重扫

v2 的 membership predicate 是：

```text
PE SizeOfImage 内 && IDA 所属 segment 带 SEG_PERM_EXEC
```

`.text` 尾 `[VA 0x00B00000,0x00B18000)` 有 550 个 IDA function start，但从已绑定的 98 个 wall-clock roots 做 branch-insensitive direct-transitive BFS 后，没有任何一个进入 reachable set。对应 RVA 区间 `[0x00700000,0x00718000)`：

- `reachableFunctionRvas=[]`；
- `reachableBulkHelperCallsiteRvas=[]`。

机器 component 还保存了 PE header、五个 section、IDA segment、post-text executable-function 空数组和 v1/v2 exact set delta。复现器对上述固定 section 参数、550 个尾部函数、空增量均有 assertion；输入 SHA 或模板 SHA 改变即失败。

## 4. indirect / callback / writer closure（沿用且由 v2 重扫）

original worker-cone indirect callsite 已裁决 38/38：input 10、mod 17、network 6、theora 5，剩余 0。

network worker `0x0069EA99: FF D1` 的唯一 command producer 把 target 固定为 `0x006733C0`：

- `0x0067316D: C7 45 E4 C0 33 A7 00`；
- `0x0067317B: C7 01 C0 33 A7 00`。

其二级 callback 数据流为 `0x00673A0D: 8B 46 10`、`0x00673A3D: FF 55 A4`。10 个 producer callsite 给出的精确 target universe 是：

```text
{ 0, 0x00507C80, 0x00508660, 0x00508830, 0x00508CE0 }
```

四个非零 target 4/4 闭合：前三个的 context `+0x1C90/+0x1C94` callback 字段只有零 writer；`0x00508CE0` 的 reference-control vcall 固定到 `0x000358B0`，全局 callback slot RVA `0x0087F60C` 有 191 个 code reference、0 writer、磁盘初值零。

三个 direct GameRoot writer 的 reverse direct cone 仍为 11 个函数：

```text
0x002F7AC0 0x002FADC0 0x00531050 0x00552410 0x00554CD0
0x00559720 0x005AA040 0x005AB6D0 0x005AB700 0x006EFAC4 0x006EFC46
```

只有两处 address-taken producer：

- `0x0046D447: 68 C0 AD 6F 00` -> `0x002FADC0`，Lua `Game.Update`；
- `0x0046D927: 68 C0 7A 6F 00` -> `0x002F7AC0`，Lua `Game.NextVictoryLap`。

两者最终只到三个已 hook 的 direct writer；production reader 对这些 writer pre-original 单调失效且不复活。

## 5. split/runtime-base closure

全镜像实际指令中没有 exact RVA literal `0x00871678`、`&slot` immediate/LEA formation、`0x00870000 + 0x1678` 或等价已识别 synthesis。唯一实际 `+0x1678` 是 `0x00353943: cmp [esi+1678h],0`，为对象字段只读。

self-image base producer `0x00504A00` 只有 caller `0x005036B0` / callsite `0x00503817`；`0x00503C18` 形成的 computed image pointer 只进入 `0x00504A60` 的 DbgHelp/range/symbolization read-only consumer。因此：

- `SPLIT_CONSTANT_RUNTIME_BASE_ALIAS_PROVENANCE=STATIC_CONFIRMED`；
- `CROSS_THREAD_INDIRECT_TARGET_PROVENANCE=STATIC_CONFIRMED`。

## 6. exact reachable bulk join

v2 保存完整、排序、去重数组：

- `wallClockReachability.wallClockReachableFunctionRvas`：5,646 项；
- `bulkHelperIntersection.reachableBulkHelperCallsiteRvas`：647 项；
- memcpy 159、memmove 246、memset 242。

当前绑定的 whole-image 1,022-callsite bulk index 三件套仍为：

- `out/cs-t0/native-join-manager-this-bulk-helper-destinations-v1.json`，SHA `348F587C9625277E35C0808E54662D0F85E3B2254329D95877228A8B6F8DE36C`；
- `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-bulk-helper-handoff-v1.md`，SHA `16CE5B3338C6A914E79B32696651B8653125CECD60A64908DD31944054EA09B7`；
- `out/cs-t0/analyze-manager-this-bulk-helper-destinations-v1.py`，SHA `0454D0F266F344EE548A3D95F238AB482E49C34D9DA1A588475DCD170C3BEF11`。

基础 index 对 647 项只给出 34 true / 613 false。Phase-2 已拆为 memcpy+memmove 405 项与 memset 242 项；必须等两个 create-new 专项把 647/647 全部裁决为 `fullyAdjudicatedNoOverlap=true`，才可提升 bulk/alias/ABA/manager-this。

## 7. 当前 disposition

v2 component 仍必须保持：

```text
componentDecision.status=PENDING_BULK_JOIN
wholeImagePromotionAllowedByThisComponentAlone=false
```

不能生成 promotable `out/cs-t0/native-join-manager-this-whole-image-alias-v2.json`。最小剩余反模型仍是任一未闭合 reachable bulk callsite 的 computed destination 覆盖 `0x00C71678..0x00C7167B`，在 reader samples 之间写入并恢复 slot，且不经过三个 direct writer hook。

只有在两个 bulk Phase-2 组件正向、精确 join 647/647 且各项 `fullyAdjudicatedNoOverlap=true` 后，才允许 raw-v2 aggregator 把八个 frontier 全部置为字面 `STATIC_CONFIRMED`。本交接本身不允许 promotion。
