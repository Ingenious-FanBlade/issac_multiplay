> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this copy/move local-hard v5 handoff

## 结论

本轮仅处理 rejected v4 的四个集合标签所选出的 exact 21 项；v4 的 proof 与 `FFDAD182B2948579A2D5FE6F1BB6BEE7502400B1BD27ADE87CF9276F9FE96265` 未作为任何正向输入。逐项从 exact IDA/raw 重建 `base provenance + offset/length <= capacity + caller/writer universe` 后，得到：

- fully-backed closed：**10**
- retained UNKNOWN：**11**
- confirmed manager-slot overlap：**0**

这不是 whole-image promotion，也不授权与冻结 v3 的 closed 39 合并。正基线仍绑定 `73905C2A1D27C12344F82147C4F89781BAC08A447D55D4C6E89D8465BB23626D`；独立拒绝审计绑定 `3937A58884C84D8A4653612F6C64980C314C191F70D9D5498653BAD06D22CC7C`。

## Exact disposition

Closed 10：

```text
0x003FE992 0x0042E47F 0x0052CCB6 0x0062599B 0x00686467
0x00686C01 0x00686C1B 0x00688C5D 0x00688C89 0x00689A20
```

UNKNOWN 11：

```text
0x0001B072 0x0001B3E3 0x002EE04F 0x004FCD78 0x0061D0B4
0x00663CFC 0x006936B7 0x006DCB75 0x006DCCF5 0x006E1030
0x006E103C
```

三个独立审计反例 `0x002EE04F`、`0x00663CFC`、`0x004FCD78` 全部保持 UNKNOWN。

## 关键硬依据与拒绝点

- 三个 algorithm closed 行分别落到 caller-local dynamic allocation 的 exact `[begin,end)`：4-byte local vector、16-byte caller vector、4-byte exact `4*n` allocation；raw shift 的写入终点严格小于 live end。
- `0x0062599B` 的唯一 caller 分配 89,425 bytes，并用 365 个 245-byte partition；owner 的 `Size < 0x80` 给出 `0+127<=245`。
- inflate scratch object 由 `A89C90` 分配 `0x2AF4`，扣除 4-byte header 后 capacity 10,992；两行分别满足 `64+288=352<=10992` 与 `3552+32=3584<=10992`。
- deflate object 由 `A89BA0` 分配 `0x4DF44`，扣除 header 后 capacity 319,296；两个 coupled clamp 的最坏终点分别为 32,912 与 33,169。
- `0x00686467` 的 A85710 caller universe 为 A614D0/A89F80；三种 output provenance 分别是 exact dynamic allocation、`.data` 64 KiB、ArchivedFile `this+2076` 的 1024-byte embedded buffer，copy length 受 `outputEnd-outputCursor` clamp。
- 两个 EL 行的“capacity check”仅调用可返回的 logger `A112C0`，随后仍执行 memcpy，因此不闭合。
- `0x0061D0B4` 的 begin/end 来自 `this+44/48`；A1A730 有 17 个 direct callsite，构造/指针写者全集未闭合。
- 四个 image-plane 行存在 `ADD7A0` 返回 1 后复制 caller-supplied plane descriptor 的路径，不能统一归因到该函数的 malloc。
- `0x006936B7` 继承 `Buf[57]+1` 的未闭合 allocation extent，与 `0x00663CFC` 同源，不能仅凭局部像素置换关系关闭。

## Machine / reproducer

- `out/cs-t0/analyze-manager-this-copy-move-local-hard-v5.py`
  - SHA-256 `E64DB0D02C2E68985A8B109B1345871B8A47B8EB6E836600AFF191BE41774F9E`
- `out/cs-t0/native-join-manager-this-copy-move-local-hard-v5.json`
  - SHA-256 `D88E92E8F1D2E89072883FAC04072FD2364FA74B7F22ED02D18DDD9E6E9A68AF`

验证命令：

```powershell
python out\cs-t0\analyze-manager-this-copy-move-local-hard-v5.py --self-test
python out\cs-t0\analyze-manager-this-copy-move-local-hard-v5.py --verify-existing out\cs-t0\native-join-manager-this-copy-move-local-hard-v5.json
```

两者均 PASS：`exact21 closed=10 unknown=11 overlap=0 mutations=83`。83 个负向 mutation 覆盖 base provenance、span rule/arithmetic、caller/writer universe、raw evidence atom、UNKNOWN 无证明 promotion、call identity 与 aggregate decision；existing artifact 的 proof semantic drift 均 fail closed。

## 安全边界

本轮只读 exact IDA/PE 与既有冻结证据；没有启动游戏、打开进程、附加调试器或写 IDB。没有修改 production、tests、v1-v7、状态或索引；只创建本 v5 reproducer、machine JSON 与 handoff。
