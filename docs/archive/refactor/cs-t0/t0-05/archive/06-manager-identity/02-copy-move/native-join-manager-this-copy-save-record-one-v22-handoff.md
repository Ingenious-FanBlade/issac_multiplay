> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 selected save-record copy/move v22 逆向交接

## 结论

本轮只处理 `RVA 0x001CBEB5`（`VA 0x005CBEB5`），不触碰已接受的 `0x005B91EF` row。结论是严格 `UNKNOWN`：v21 的 receiver 布局错误已经纠正，两级 selector、目标循环 cursor、`memmove` 参数和已知 allocator 链均已闭合；但 selected record / `record+0x500` 的无 literal alias-forwarder 全局 fixed point 尚未生成。因此不能把 copy/move 从 `315/405` 提升到 `316/405`。

这不是 v21 的旧 UNKNOWN。v21 错把 `sub_706780` 返回值视作对象 base，从而把目标 receiver 误报为 `record+0x504`。实际关系是：

```text
slotBase = sub_706780(n60)
objectBase = slotBase + 4
recordBase = objectBase + 0x19D7C + index*0x5DC
target receiver = slotBase + 0x1A280 + index*0x5DC
                = objectBase + 0x1A27C + index*0x5DC
                = recordBase + 0x500
```

所以真实 vector triple 为 `record+0x500 begin / +0x504 end / +0x508 capacityEnd`。

## 目标局部语义

`0x004561A0` 从 `[receiver+0]` 生成 begin iterator；`0x00456190` 从 `[receiver+4]` 生成 end sentinel；`0x0042C710` 比较 iterator 与 sentinel；`0x0040C3A0` 解引用；`0x005CBAD0` 每次增加 4。只有 `0x005C49BB` 读到 `DWORD[kk]==0x1A6` 才在 `0x005C49FD` 调用 `sub_5CBEA0`。

因此 `kk=begin+4*j`、`begin<=kk<end`、`kk+4<=end`。`sub_5CBEA0` 的参数精确对应：

- destination：`[kk,end-4)`；
- source：`[kk+4,end)`；
- length：`end-(kk+4)`；
- 更新：`end-=4`，begin/capacityEnd 不变。

在 vector order/alignment 前提下，`kk+4`、`end-(kk+4)` 与 `end-4` 均不回绕。相对范围继续只复用 v7 的 `VERIFIED_BOUNDED`，没有重证 v7。

## 两级 selector 与对象 extent

### `owner+0x1620`

PE `.text` 中 raw `20 16 00 00` 恰有 11 个 occurrence，当前 IDB 逐条分类为 6 read 与 5 write：

- `0x007540D9`：Entity_Player 构造写 `-1`；
- `0x009B8FF6`：primary player 写 `0`；
- `0x009B9E9D`：仅在 `maxExisting<=2` 时写 `maxExisting+1`，范围 `0..3`；
- `0x009BD20A`：由 `0x009BD1B2 recordCount<8` 支配；
- `0x009BD281`：由 `0x009BD24C recordCount<8` 支配。

构造值域为 `{-1,0..7}`；目标在 `0x005C494C` 检查非负，所以 selected path 上 `0<=index<8`。

### `n60+0x676AC`

本轮绑定并重放 v13 JSON/analyzer，同时独立验证 PE raw `AC 76 06 00` 的 7 个 `.text` occurrence 和四个 writer：`0x006F46FD=-1`、`0x00705FA7=1-old`、`0x0070636F=0`、`0x00706388=1`。`sub_705EE0` 的负值路径令 `edx=0`，0/1 路径保持在 0/1，故进入 `sub_706780` 的有效值域为 `{0,1}`。

`sub_6F1020` 在 `n60+0x269EC` 构造两个 slot，stride `0x20660`；每个 wrapper 由 `sub_6F2970` 在 slot+4 构造 `sub_9C73C0` 对象。n60 allocation extent 为 `0x68E88`。`sub_9C73C0` 在 object+`0x19D7C` 构造 8 个、stride `0x5DC` 的 record array A；最坏 index 与 vector triple 仍在 `0x20660` 对象 extent 内。

## 已分类的 vector writer / reader

- `sub_754980`：构造时清零 `record+0x500/+0x504/+0x508`；
- `sub_754D90 -> sub_4149D0`：析构时释放 allocator backing 并清零 triple；
- `sub_9CF050`：reset 时取 `record+0x500`、清零 triple、释放旧 backing；
- `sub_7B0BE0`：从 `owner+0x1FA4` assignment 到 `record+0x500`。容量不足走 `sub_6A0D30 -> sub_40CF00`，安装 allocator begin/end/capacity；随后复制 source size bytes 并令 dest.end=dest.begin+size；
- `sub_7AE4B0`：反向读取 record vector 写 player vector，不安装 record begin；
- `sub_9D05D0`：`0x009D3E2C` 形成 `record+0x500`，`0x009D3E35` 调 `sub_7310E0`；增长链 `sub_732240 -> sub_40CF00 -> sub_41D150` 安装 allocator triple；
- `sub_9D45C0`：读取 `(record+0x504-record+0x500)/4` 与 backing elements；
- `sub_5CBEA0`：selected erase 只更新 end。

已知非 null begin 只有两类：`sub_6A0D30` 或 `sub_41D150` 安装的 `sub_40CF00` allocation。两类均有 `capacityEnd-begin` allocator extent 关系。

## 候选与 caller census

IDA instruction-level candidate 集仍为 42：34 个有效 `0x5DC` operand 与 8 个有效 `0x20660` operand；42/42 已函数分类，未发现 foreign begin install。PE raw byte层分别有 52 与 8 个 occurrence，analyzer 分开固定 raw count，避免把 unaligned/cross-instruction raw bytes 冒充 instruction candidates。

analyzer 还从整个 `.text` 独立扫描 rel32 `E8`，精确固定以下 direct call 集：

- `sub_5CBEA0`：4 callsites；
- `sub_7B0BE0`：6 callsites；
- `sub_7AE4B0`：6 callsites；
- `sub_754980`：4 direct callsites（另有 array-constructor data references）；
- `sub_754D90`：4 direct callsites（另有 EH/data references）；
- `sub_9CF050`：6 callsites；
- `sub_9D05D0`：7 callsites；
- `sub_9D45C0`：4 callsites。

没有从 42 个 decoded literal candidates 或这些 direct call 集发现新的 tuple writer。

## 首个精确缺口

`RECORD_PLUS_500_ALREADY_OFFSET_FORWARDER_FIXED_POINT_NOT_BUILT`：尚无 mode-independent finite fixed-point 从 selected object/record roots 追踪 `record` 与 `record+0x500` alias，覆盖 return value、stack spill/reload、indirect call、CFG split-tail 与 forwarded callee，并把所有 live-out/call frontier 归零。

42/42 literal 分类和所有上述 direct `E8` call 集不能排除一个不含 `0x5DC/0x500/0x20660` literal 的 caller：它接收已经偏移好的 `record+0x500`，再转发给通用 tuple assignment helper，安装 external begin，同时仍满足本地 `begin<=end<=capacityEnd`。在这个反模型被 finite context manifest 排除前，不能宣称所有非 null begin 均 allocator-owned，也不能生成 superseding composite verified row。

下一步的最小充分条件是：从 selected slot/object/record source contexts 产生有限 manifest，逐 CFG/调用边追踪 record 与 tuple alias；所有 direct/indirect/split-tail/forwarded call 和 block live-out 到 fixed point 后，逐个 begin install 只允许 NULL 或上述两条 allocator 链。

## 产物与复现

- analyzer：`out/cs-t0/analyze-manager-this-copy-save-record-one-v22.py`
- machine JSON：`out/cs-t0/native-join-manager-this-copy-save-record-one-v22.json`
- analyzer SHA-256：`70543708B550348FF2A52C3F13721EBE4DEF3DD8DF04706198B8BA5C44CB3499`
- machine JSON SHA-256：`13BB9F33F0B500F4E4BEE74C9BE6628CA849F26F1E6A4065BE78DB94B9A5C23E`

PE create-new 前先执行过 self-test，create-new 后连续两次：

```text
PASS PE exact1 verified=0 unknown=1 candidates=42 ownerHits=11 parentHits=7 mutations=38
PASS PE exact1 verified=0 unknown=1 candidates=42 ownerHits=11 parentHits=7 mutations=38
```

当前冻结 IDB read-only replay：

```text
PASS IDA exact1 verified=0 unknown=1 candidates=42 ownerHits=11 parentHits=7 mutations=38
```

38 个 mutation sentinel 覆盖 receiver 回退、selector/extent、cursor/no-wrap、候选/call 集、伪造 alias closure、伪造 backing verified、315→316 计数、0x5B91EF 混入、bindings 与 permissions；全部 fail-closed。PE 与 IDA 使用相同签名、函数完整字节 SHA、raw hit/call scan 和 canonical payload；本轮没有运行游戏、打开/读取进程、附加 debugger 或修改 IDB。

所有 permissions/promotions 均为 false，copy/move 保持 `315/405`，remaining 保持 90。独立测试者应从磁盘重新运行 `--self-test`、连续两次 `--verify-existing`，再在当前 IDB 执行同一脚本；还应复算三份 v22 产物 hash，并确认 `0x005B91EF` 只出现在排除集。
