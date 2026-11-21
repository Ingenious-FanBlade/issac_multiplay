> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this copy/move hard-eleven v10 handoff

## 结论

本轮只处理 `local-hard-v5` 保留的 exact 11 项，并逐项要求同时成立：真实 destination backing、`start/count` 上界、完整 caller/forwarder/pointer-writer universe，以及 `end <= extent`。结果为：

- `VERIFIED`：**0**
- `UNKNOWN`：**11**
- 已证明 manager-slot overlap：**0**

因此本轮没有新增 closure，不允许把这 11 项合并进 copy/move 正向计数，也不允许 manager-this 或 whole-image promotion。被独立否决的 v4 `100/100` 没有作为正向证据使用。

## 冻结输入

- `out/cs-t0/native-join-manager-this-copy-move-local-hard-v5.json`
  - SHA-256 `D88E92E8F1D2E89072883FAC04072FD2364FA74B7F22ED02D18DDD9E6E9A68AF`
- `out/cs-t0/native-join-manager-this-copy-move-local-stack-v3.json`
  - SHA-256 `73905C2A1D27C12344F82147C4F89781BAC08A447D55D4C6E89D8465BB23626D`
- `out/cs-t0/native-join-manager-this-copy-move-local-stack-v4-independent-audit-v1.json`
  - SHA-256 `3937A58884C84D8A4653612F6C64980C314C191F70D9D5498653BAD06D22CC7C`
- 精确目标 `isaac-ng.exe`
  - SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
  - x86 / image base `0x00400000` / size of image `0x008FE000`

reproducer 对目标 PE 重新计算哈希并逐一核对 11 条 call 的精确五字节。

## Exact disposition

全部保留 `UNKNOWN`：

```text
0x0001B072 0x0001B3E3 0x002EE04F 0x004FCD78 0x0061D0B4
0x00663CFC 0x006936B7 0x006DCB75 0x006DCCF5 0x006E1030
0x006E103C
```

## 逐类拒绝依据

### EL 两行

`0x0001B072` 与 `0x0001B3E3` 的不足空间分支只调用可返回的 `A112C0`，随后仍分别执行 `4*v15` 与 `4*v42` 的 `memcpy`。没有闭合 destination sublist 的实际 extent，故不能从日志分支推导 `end<=extent`。

### 32-byte stack 反例

`0x002EE04F` 复制 `4*n4-4` 字节到 32-byte stack object。复制路径只证明 `n4!=0`；`n4<4` 与 `n4>=8` 只选择另一个调用参数，不构成 `n4<=9`。冻结的最小反模型 `n4=10 -> count=36 > 32` 仍未被排除。

### 两个 caller-supplied range

- `0x004FCD78`：local sort 只证明相对 `[a1,a2)` 的范围关系；唯一非递归 callsite `0x008F7080` 的 tuple constructor/writer universe 未闭合。`a1=imageBase+0x00871674` 的冻结 slot countermodel 仍成立。
- `0x0061D0B4`：唯一非递归 callsite `0x00A1AD12` 从 `this+44/48` 读取 begin/end，但其 owner `A1A730` 有 17 个 executable callsite；字段构造与 pointer writer 全集未闭合。

### libpng row backing 两行

新核到的正向局部事实不足以提升：

- `A932C0` 按 `ceil(Buf[48]*byte[0x120]*byte[0x123]/8)+1` 分配 `Buf[57]`，可选 `Buf[56]` 使用同一 Size；
- `A94C21` 可交换 `Buf[56]/Buf[57]`；
- `A93417` 初始化 `Buf[51]`，`A9350B` 在 Adam7 pass 中更新它；
- `0x00663CFC` 使用 `Buf[57]+1` 和 `ceil(Buf[51]*bpp/8)`，本地没有 capacity comparison；
- `0x006936B7` 的像素重排在 byte-aligned branch 中保持相对 row base 的关系，但依赖同一 `Buf[57]` backing。

尚未闭合完整字段 writer universe，以及防止 32-bit product/widening 反序的 width/overflow invariant。因此两行继续 `UNKNOWN`；不能只凭“两个 malloc 使用同一公式”或相对像素关系提升。

### image plane 四行

`0x006DCB75`、`0x006DCCF5`、`0x006E1030`、`0x006E103C` 都有唯一 direct owner caller，但 `ADD7A0` 返回 1 的路径在 `0x00ADF597` 复制 caller-supplied plane descriptor。局部 width/stride/border 关系不能证明该外部 descriptor 的真实 allocation extent，也不能证明上下边界已有 border backing，所以四行全部保留 `UNKNOWN`。

## Machine / reproducer

- `out/cs-t0/analyze-manager-this-copy-hard-eleven-v10.py`
  - SHA-256 `E1DFDD540A4997B5CA45284BD6F02DF690D16AAFC866B8AF378C7EB0A035CD9E`
- `out/cs-t0/native-join-manager-this-copy-hard-eleven-v10.json`
  - SHA-256 `631F67A92DE248214DE0A26AA91BDC436E102044115C5029BB6ED083BE9B6BA1`

验证命令：

```powershell
python out\cs-t0\analyze-manager-this-copy-hard-eleven-v10.py --self-test
python out\cs-t0\analyze-manager-this-copy-hard-eleven-v10.py --verify-existing out\cs-t0\native-join-manager-this-copy-hard-eleven-v10.json
```

两条命令均输出：

```text
PASS exact11 verified=0 unknown=11 overlap=0 mutations=39
```

39 组 mutation 覆盖 exact11 集合与顺序、目标 call bytes、逐行 backing/start/count/inequality、UNKNOWN 非法提升、aggregate、输入 binding、权限和最终决定，全部 fail closed。

## 安全边界

本轮仅使用目标 PE 磁盘字节、只读 IDA 查询和冻结 JSON；没有启动游戏、打开进程、附加调试器或修改 IDB。没有修改 production、tests、旧证据、状态或索引，只创建本 v10 reproducer、machine JSON 与 handoff。
