> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this fresh-vector exact-one v17 handoff

## 结论

`memset` exact1 `0x0068DF56` 保持：

`UNKNOWN_PREMULTIPLICATION_SUM_PRODUCER_BOUND_OPEN`

冻结局部链证明 `v37=EBX` 来自 `cvttss2si`，且执行路径上 `1 <= v37 <= INT_MAX`。但 `old_count = DWORD[node+4]` 在 `0x00A8DF2F` 读出后，于 `0x00A8DF39` 直接执行 unchecked `ADD EAX,EBX`。现有材料没有枚举 `DWORD[node+4]` 的全部 producer/writer，也没有逐 producer 证明：

`0 <= old_count <= UINT32_MAX - v37`

因此，后续乘 4 的 overflow 路径不能补救已经发生的加法回绕。最小未排除反模型是 `old_count=UINT32_MAX, v37=1`：数学和为 `UINT32_MAX+1`，机器加法结果为 0。原 v4 的 `4*v37 <= 4*(v37+old_count)` 恰好把“加法未回绕”当成了未证明前提；独立 range audit v5 已将该行判为 `INSUFFICIENT_ROW_LOCAL_RANGE_EVIDENCE`。

## 精确绑定

- row：RVA `0x0068DF56` / VA `0x00A8DF56`
- function：RVA `0x0068DA40` / `sub_A8DA40`
- call bytes：`E8 8A 26 06 00`
- destination：fresh allocator result retained in `LODWORD(v54)`
- fill length：`4 * v37`
- bulk memset v3 SHA-256：`F289A5FA82B205BA1AA9989021C26671938BA75444A90356487D5688D3F677AB`
- rejected shared narrative v4 SHA-256：`0324F6E8DB07BE739315A4955F153EC1353D084F1F16FFBD6DDA7816C7649D6C`
- independent range audit v5 SHA-256：`34B45C6C4DBB6A540527E5A756382939B77E1EDCD87E8222FA40D7113186617F`

## 产物与复核

- JSON：`out/cs-t0/native-join-manager-this-fill-fresh-vector-one-v17.json`
- verifier：`out/cs-t0/analyze-manager-this-fill-fresh-vector-one-v17.py`

```powershell
python out/cs-t0/analyze-manager-this-fill-fresh-vector-one-v17.py --verify-existing
```

verifier 绑定三份输入 hash、exact-one 行身份、原 v4 隐含不等式与 v5 拒绝结论；12 项 mutation 覆盖输入漂移、地址/上下界、unchecked-add、开放 producer/bound/no-wrap、UNKNOWN 分类、权限与 promotion，必须全部 fail closed。

## 权限边界

本报告只复用冻结本地静态材料；未启动游戏、打开/读取进程、附加调试器、修改 IDB、生产代码、测试或既有证据。该行不可 promotion；whole-image alias 与 manager-this promotion 均为 false，全部权限保持 false。
