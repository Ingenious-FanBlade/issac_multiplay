> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this：miniz exact-nine 数值闭包 v8

## 结论

本轮仅处理 `native-join-manager-this-bulk-memset-range-audit-v5.json` 中两个 miniz proof group 的 exact-nine，并明确排除已由 `fill-three-v7` 封存的 `0x00686BD1`。九行均达到逐行 `state allocation source + offset/domain + count + extent + all callers + numeric inequality` 闭包：`VERIFIED=9`、`UNKNOWN=0`。

本结论是行级数值／caller certificate，不提升 manager-this 同代身份总门；也不覆盖 exact-nine 之外的 memset、copy/move 或运行时观察。

## 冻结绑定与零交集

- range audit v5：`34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`
- bulk memset v3：`F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB`
- fill-three-v7 JSON：`242C05FD1AD0FF9D4851EC7A70EF59095B15F592C72A73DF4F4766BD17508807`
- observed PE：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- exact-nine 与 fill-three-v7 exact-three 的集合交集：`[]`，count `0`

exact-nine 固定为：

`0x00686514, 0x00686524, 0x00686621, 0x006866EC, 0x00687822, 0x0068783A, 0x0068857A, 0x0068858A, 0x00688AD7`

## 两个 allocation-derived extent

`V4_MINIZ_COMPRESS_STATE_EXTENT` 的 payload 由 `sub_A89C90` 创建：`malloc(0x2AF4)` 位于 `0x00A89CBD`，返回块前 4 bytes 是 accounting header，`0x00A89CE8` 形成 `base+4` payload，`0x00A89CEB` 发布到 wrapper `+12`。因此可用 payload extent 为：

`0x2AF4 - 4 = 0x2AF0 = 10992 bytes`

其 allocator 直接调用点闭合为 `0x00A61530`、`0x00A68CA8`；消费者 `sub_A85710` 的全部直接调用点闭合为 `0x00A61560`（`sub_A614D0`）和 `0x00A89FCF`（`sub_A89F80`），两者都传入该 `+12` payload。

`V4_MINIZ_DECOMPRESS_STATE_EXTENT` 的 payload 由 `sub_A89BA0` 创建：`malloc(0x4DF44)` 位于 `0x00A89BD1`，同样跳过 4-byte accounting header，`0x00A89BF2` 形成 payload，`0x00A89BFF` 发布到 `[Size_0+4]`，并在 `0x00A89C05` 调用 `sub_A89A60(payload)`。因此可用 payload extent 为：

`0x4DF44 - 4 = 0x4DF40 = 319296 bytes`

其 allocator 直接调用点闭合为 `0x00A918C0F`、`0x00A91AD29`。`sub_A89DE0` 从 `[Size_0+4]` 取得同一 payload，并沿 `sub_A88BB0`、`sub_A890E0`、`sub_A88650` 链传递。

## 九行逐行证书

| RVA | allocation source / extent | offset 与动态域 | count | all direct callers | 最大端点不等式 | 结论 |
|---|---|---|---:|---|---|---|
| `0x00686514` | `sub_A89C90`, `10992` | `64`；`v108=a1+64` | 144 | `sub_A85710`: `0x00A61560`, `0x00A89FCF` | `64+144=208 <= 10992` | VERIFIED |
| `0x00686524` | `sub_A89C90`, `10992` | `64+36*4=208` | 112 | `sub_A85710`: `0x00A61560`, `0x00A89FCF` | `208+112=320 <= 10992` | VERIFIED |
| `0x00686621` | `sub_A89C90`, `10992` | `7040` | 288 | `sub_A85710`: `0x00A61560`, `0x00A89FCF` | `7040+288=7328 <= 10992` | VERIFIED |
| `0x006866EC` | `sub_A89C90`, `10992` | `64+3488*s+288`，`s∈{0,1,2}`，max `7328` | 3200 | `sub_A85710`: `0x00A61560`, `0x00A89FCF` | `7328+3200=10528 <= 10992` | VERIFIED |
| `0x00687822` | `sub_A89BA0`, `319296` | `36626+288*a2`，`a2∈{0,1,2}`，max `37202` | 288 | `sub_A87300`: `0x00A879AC`, `0x00A879BB`, `0x00A87D6E`, `0x00A885D8`, `0x00A885EA` | `37202+288=37490 <= 319296` | VERIFIED |
| `0x0068783A` | `sub_A89BA0`, `319296` | `34898+576*a2`，`a2∈{0,1,2}`，max `36050` | 576 | `sub_A87300`: 同上五处 | `36050+576=36626 <= 319296` | VERIFIED |
| `0x0068857A` | `sub_A89BA0`, `319296` | `36626` | 144 | `sub_A88560`: `0x00A887F3`, `0x00A88950` | `36626+144=36770 <= 319296` | VERIFIED |
| `0x0068858A` | `sub_A89BA0`, `319296` | `36770` | 112 | `sub_A88560`: `0x00A887F3`, `0x00A88950` | `36770+112=36882 <= 319296` | VERIFIED |
| `0x00688AD7` | `sub_A89BA0`, `319296` | `33170` | 640 | `sub_A88650`: `0x00A88F7C`, `0x00A8905A`, `0x00A89995`, `0x00A89F13` | `33170+640=33810 <= 319296` | VERIFIED |

## 动态域闭包

`0x006866EC` 的状态值不是无界断言。初始 dynamic block type `(n16&7)>>1` 的数学域为 `{0,1,2,3}`；`3` 在 `0x00A864D1` 进入循环前被拒绝；`1` 走 fixed branch、不执行该 clear loop；循环状态从 `{0,2}` 开始，并在 `0x00A86C27` 递减。因此该行所有实际执行的 `s` 精确落在 `{0,1,2}`。

`0x00687822` 与 `0x0068783A` 的 `a2` 域来自 `sub_A87300` 的全部五个直接调用点，而不是推测：`0x00A879AC→0`、`0x00A879BB→1`、`0x00A87D6E→2`、`0x00A885D8→0`、`0x00A885EA→1`，所以精确域为 `{0,1,2}`。

## 产物与离线复现

- 机器可审计 JSON：`out/cs-t0/native-join-manager-this-fill-miniz-nine-v8.json`
  - SHA-256：`CF9A9C0813271864BA921CACC57B682652918EC16240A3BB16E05E90300A0B5E`
- 离线 reproducer：`out/cs-t0/analyze-manager-this-fill-miniz-nine-v8.py`
  - SHA-256：`D0344885C35468ED95867A8AFD569A75154A2331DF8E2D58DB2D402250E5D42D`

复现命令：

```powershell
python out/cs-t0/analyze-manager-this-fill-miniz-nine-v8.py --verify-existing
```

实测输出：

```text
VERIFIED D:\projects\isaac_hook_reserach\out\cs-t0\native-join-manager-this-fill-miniz-nine-v8.json
SHA256 CF9A9C0813271864BA921CACC57B682652918EC16240A3BB16E05E90300A0B5E
ROWS VERIFIED=9 UNKNOWN=0
FILL_THREE_INTERSECTION=0
MUTATIONS_REJECTED=20
```

reproducer 会重新验证三个 frozen JSON 的 SHA、exact-nine membership、v5 proof group/status、fill-three-v7 exact-three 与零交集、PE SHA/header/`.text`、九个 call bytes，并要求 JSON 与预期模型逐字段、逐类型、逐顺序一致。20 个语义变异覆盖 row membership、proof group、allocation source、offset、dynamic domain、count、extent、caller 集、caller 地址、caller closed flag、inequality、unresolved、status、fill-three overlap/binding 和 permissions，全部 fail closed。

## 边界与权限

- 全程只做磁盘静态读取和 create-new 产物写入。
- 未启动游戏，未打开／读取任何进程，未附加调试器，未修改 IDB、生产代码、测试或既有证据。
- `promotionAllowed=false`；上游只能合并这九行证书，不能把本轮单独当作 manager-this 同代身份完整证明。
