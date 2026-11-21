> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R05 单行 v3 续接候选前封印 TEST preparation v1 交接

日期：2026-09-06  
角色：`INDEPENDENT TEST / PRE-CANDIDATE SEAL`  
状态：`READY_FOR_UNSEEN_MT_R05_V3_REVERSE_CANDIDATE`

## 1. 结果

已在任何 MT-R05 v3 REVERSE candidate/builder 出现或被读取前，封印
`MT-R05 {0x0033570B}` 单行续接的独立验收边界。当前冻结 PE 磁盘路径可用，
因此本轮不是 `BLOCKED`：TEST 从物理 PE 独立重算了目标 SHA-256、PE32/I386、
ImageBase、SizeOfImage、call bytes `E8 AD B1 3B 00` 和 rel32 helper target
`0x00AF08BD`。

这只表示候选前验收规则和磁盘目标绑定已准备完成，不是对尚未出现的 v3 候选作
`PASS`。MT-R05 当前仍沿用作者分离 exact5 audit v2 的 `UNKNOWN` 基线；既有 v1/v2
REVERSE handoff 仅以 `historicalContextOnly / acceptedAsCandidateTruth=false` 进入封印，
没有把其中逆向叙述当作本轮候选真值。

## 2. 封印的最小验收边界

候选范围必须精确为 `MT-R05 / 0x0033570B / memmove / exactCount=1`，并绑定本封印
spec、v33 任务接口、v37 检查点、v33 frontier 和 exact5 independent audit v2。
验收顺序固定为：

1. `backingExtent`；
2. `countAndNoWrap`；
3. `sourceLifetimeAndIdentity`；
4. `targetIntervals`。

全部义务都使用 `ALL_REACHABLE_EXECUTIONS`。既有独立基线已经把
`backingExtent` 放在 first-open 之前，v3 candidate 不得重新打开它。若
`countAndNoWrap` 仍是 first-open，必须逐字保留 gap
`R05_AUX_NONEMPTY_FILTERED_EMPTY_CONJUNCTION_OPEN` 及其 countermodel；若要把
first-open 推进到后续义务或申请单行正向证书，必须以绑定冻结 PE proof atoms 的
全称静态证明消解该 countermodel。动态常见值、仅 prose 断言或补造 guard 均不接受。

即使 candidate 自报 `STATIC_CONFIRMED_NO_OVERLAP`，也只能设置
`promotionRequested=true / promoted=false`；candidate 自身必须保持 frontier、
manager-this、whole-image 和 overall promotion 全 false。只有后续作者分离 TEST
audit 可以给出这一个 row 是否可交给 MAIN 合成 frontier 的 verdict。

完整候选 schema 为
`cs-t0.manager-this.mt-r05.filtered-vector.v3.reverse-candidate.v1`，字段、proof atom、
countermodel resolution 和四义务细节见封印 spec。TEST validator 支持后续 candidate
模式，但本轮没有执行该模式。

## 3. 封印产物

| 文件 | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3-sealed-acceptance-spec-v1.json` | `8872E11ED61E153C2B27A394764FC4959C4E1C6035ABE28D40AED69891BF7759` |
| `out/cs-t0/audit-manager-this-mt-r05-filtered-vector-v3-sealed-v1.py` | `290EC5EFB5B0DB008BF78C8964646A7E1912B3EB67346DCA49E9CFC643AEB4C6` |
| `out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3-sealed-preparation-v1.json` | `6189586500BE7E9D3753039BB994254C0ADCB0E7DA60E3AE2890F69C2DD08E90` |

这些名称均带 `v3-sealed`，不会占用 REVERSE 的 v3 candidate/builder 名称。

## 4. 压缩验证

最终封印 hash 下只保留以下最小执行面：

- 一次成功 preparation create-new：`PASS`，`candidateRead=false`；
- 一次 preparation verify-existing：`PASS`，与 create-new 字节一致；
- 一次 wrong-spec-hash probe：exit 1，错误为 `spec requested hash drift`，probe 输出在
  执行前后均不存在；
- 四个互不重复的决策 mutation：目标/范围漂移、证据绑定缺失、反例消解规则弱化、
  非法 promotion 开启；全部拒绝。

封印前的 JSON 草稿曾因重复 `candidateSchemaVersion` key 被 duplicate-key parser 在
任何输出写入前拒绝；删除重复键后才固定上述最终 spec hash。该草稿失败没有生成或
覆盖 preparation，也没有触碰 candidate。

没有运行 Cargo、全量 build、历史宽回归、游戏、目标进程、x32dbg、动态场景或 IDB
修改。IDA 当前数据库也未用于本轮准备；因此当前只把物理 PE 身份/callsite/helper
重算标记为已完成，尚未出现的 v3 语义 proof atoms、writer/caller/alias fixed point 和
countermodel 消解全部留待 candidate 到达后的独立核对。

## 5. 后续调用边界

MAIN 提供 public v3 candidate path 和物理 SHA-256 后，TEST 才可运行一次 candidate
create-new、一次 verify-existing、同四类决策 mutation 和一次 wrong-candidate-hash
fail-before-write。若 candidate 仍保留精确 first-open/countermodel，可验收为失败安全的
单行 `UNKNOWN`；若申请正向证书，四项全称义务、countermodel resolution、proof-atom
物理绑定和禁止自 promotion 必须同时通过。任一失败不得修改 candidate、frontier、
checkpoint、index、生产代码或历史证据。
