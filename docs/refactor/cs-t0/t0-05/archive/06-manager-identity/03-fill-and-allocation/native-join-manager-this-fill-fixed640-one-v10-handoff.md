> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this 固定 640-byte fill exact1 v10 交接

日期：2026-09-02  
状态：`EXACT_ONE_FIXED_640_FILL_ROW_VERIFIED`  
范围：只裁决 `0x0062C9CF`（`sub_A2C980: memset(dst, 0, 0x280)`）的对象范围、完整入口 provenance 与 manager slot 不相交

## 结论

`0x0062C9CF` 在精确目标、`no external module or injection` 范围内已闭合。`sub_A2C980` 的完整 direct 入口只有两处：

| 入口 | `dst` 来源 | 范围结论 |
| --- | --- | --- |
| `0x00A1E165` | `sub_A1E050` 在 `0x00A1E0B5` 形成 `this+0xE8`，并在 `0x00A1E0C2` 送入 `ECX` | 基类构造器的 4 个 direct caller 均闭合到同路径 fresh allocation；目标 end 为 `0xE8+0x280=0x368`，同一构造器随后观察到的完整成员 end 为 `0xE8+0x284=0x36C`，均不超过最小分配 `0x370`。 |
| `0x00A1F8D8` | `ECX=0x00C947E0` 的线程安全静态初始化 | 目标区间为 `[0x00C947E0,0x00C94A60)`；同函数 `rep movsd` 的 `0xA1 DWORD=0x284` 使用和 `sub_A2C980` 的 `dst+0x280` 写入把观察范围闭合到 `0x00C94A64`。该范围完全位于 writable `.data`。 |

manager slot 是 `[0x00C71678,0x00C7167C)`。静态目标满足：

```text
0x00C7167C <= 0x00C947E0
gap = 0x00C947E0 - 0x00C7167C = 0x23164
```

动态四类目标由精确 CRT `malloc` 路径返回 fresh heap storage，不是映像 `.data`；因此在本 artifact 明示的精确镜像、无外部模块/注入范围内，也不与映像内 manager slot 重叠。

本结论只替换 v5 对该一行的 `V4_FIXED_640_BYTE_DESTINATION` 点状缺口。本 artifact 不裁决其他 `memset`、copy/move、whole-image alias、ABA 或完整 manager-this 身份。

## 冻结输入

| 角色 | 路径 | SHA-256 |
| --- | --- | --- |
| exact-78 UNKNOWN 源 | `out/cs-t0/native-join-manager-this-bulk-memset-v3.json` | `F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB` |
| 独立 range audit | `out/cs-t0/native-join-manager-this-bulk-memset-range-audit-v5.json` | `34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F` |
| 目标 `isaac-ng.exe` | `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` | `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C` |

复现器拒绝 predecessor hash、exact1 membership、目标 PE machine/image/section、目标和构造链字节、direct call target、全 `.text` E8 target 集、绝对入口地址、PE export、`malloc` IAT、数值不等式、分类、决议或权限漂移。

## 完整入口和分配来源

### `sub_A2C980`

只读 IDA `XrefsTo(0x00A2C980)` 精确返回：

- code `0x00A1E165`（`sub_A1E050`）；
- code `0x00A1F8D8`（`sub_A1F740`）；
- data xref 为零，结果未截断。

独立磁盘 `.text` raw-E8 relative-target 扫描得到同一个二元素集合。目标 PE 全文件内 `0x00A2C980` 的 little-endian 绝对地址出现次数为零，目标函数也不导出。

### `sub_A1E050`

只读 IDA `XrefsTo(0x00A1E050)` 精确返回 4 个 code xref、零 data xref且未截断：

| base call | 上层来源 | allocation | bytes | end 证明 |
| --- | --- | --- | ---: | --- |
| `0x0090195D` | `sub_901930`；该函数只有 `0x006F664C` 一个 direct caller | `0x006F6636: sub_A0F4C0(0x3F8)` | 1016 | `0x36C <= 0x3F8` |
| `0x009015DB` | `sub_9015B0`；该函数只有 `0x006F6704` 一个 direct caller | `0x006F66EC: sub_A0F4C0(0x3F8)` | 1016 | `0x36C <= 0x3F8` |
| `0x00A21A41` | `sub_A21980` 内部 fresh keyboard object 路径 | `0x00A21A27: sub_A0F4C0(0x370)` | 880 | `0x36C <= 0x370` |
| `0x00A21DF2` | `sub_A21D40` 内部 fresh gamepad object 路径 | `0x00A21DD8: sub_A0F4C0(0x4F0)` | 1264 | `0x36C <= 0x4F0` |

全 `.text` raw-E8 扫描对 `sub_A1E050`、`sub_901930`、`sub_9015B0` 分别精确得到上述 `4/1/1` 个 callsite；这三个入口与 `sub_A2C980` 的绝对 VA 在整个 PE 文件中都没有出现，四个受保护入口均不导出。因此没有把未检查的 forwarder 或 address-taken entry 悄悄排除。

`sub_A0F4C0` 把原请求完整传给 `sub_A648B0`。后者在 `0x00A6492D` 加四字节 accounting header，经 `0x00B187E0` 的 `api-ms-win-crt-heap-l1-1-0.dll!malloc` 分配，并在 `0x00A64959` 返回 `raw+4`。所以交给构造器的 usable extent 等于请求的 `0x3F8/0x370/0x4F0`，不是含 header 的偶然数值。

## 静态对象范围

`sub_A1F740` 的相关顺序为：

1. `0x00A1F783`：形成源 `this+0xE8`；
2. `0x00A1F789`：`ECX=0xA1` DWORD；
3. `0x00A1F78E`：同路径清零长度 `0x284`；
4. `0x00A1F795`：`EDI=dst_=0x00C947E0`；
5. `0x00A1F79C`：`rep movsd`，即复制 `0xA1*4=0x284` bytes；
6. `0x00A1F8CC`：静态初始化时 `ECX=dst_`；
7. `0x00A1F8D8`：调用 `sub_A2C980`。

目标 helper 自身先在 `0x00A2C9C9` 写 `dst+0x280`，再在 `0x00A2C9CF` 清 `[dst,dst+0x280)`。因此静态对象的 row-local 观察范围不是只凭名字猜测，而是 `[dst,dst+0x284)`。PE `.data` 虚拟范围为 `[0x00BF8000,0x00C9CAA4)`，静态对象和 manager slot 均在其中，但数值区间严格不相交。

## 产物与复现

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-fill-fixed640-one-v10.json` | `3B8E2DB2926AE4C6CD0D8A2CCBF2A0E4003AE6C724F8EBF3517F5AD07ED357E6` |
| `out/cs-t0/analyze-manager-this-fill-fixed640-one-v10.py` | `22A3E3958315390FA9F943A042F507BFC2E3FD45FA8D3A0003E63C6CD47C8184` |

```powershell
python out\cs-t0\analyze-manager-this-fill-fixed640-one-v10.py --verify-existing
```

连续两次均得到：

```text
PASS existing=out/cs-t0/native-join-manager-this-fill-fixed640-one-v10.json sha256=3B8E2DB2926AE4C6CD0D8A2CCBF2A0E4003AE6C724F8EBF3517F5AD07ED357E6 verified=1 unknown=0 mutations=55
```

55 项语义变异覆盖 identity/bindings、目标布局与 xref、row/count、caller/allocation、allocator contract、static/slot 和 classification/decision/permissions；全部失败关闭。

## 操作边界

本轮只读取冻结 JSON、目标磁盘 PE 与只读 IDA 数据库。没有启动游戏，没有打开或读取目标进程，没有附加调试器，没有修改 IDB；没有修改 production、tests、既有 formal 产物、index、status 或 research plan。机器 JSON 的全部权限为 false，且明确禁止由本 exact1 artifact 直接提升 whole-image 或 manager-this。
