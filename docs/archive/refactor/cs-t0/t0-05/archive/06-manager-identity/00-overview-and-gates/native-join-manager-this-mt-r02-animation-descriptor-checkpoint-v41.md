> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this MT-R02 animation descriptor checkpoint v41

日期：2026-09-06  
状态：`BOUNDED_STATIC_ADJUDICATED / 0 POSITIVE / 2 UNKNOWN / STRICT_BLOCKED_EXACT_99 / MANUAL_QUEUE_96`

## 1. 当前结论

在 v40 之后单独继续 `MT-R02 {0x00008A62, 0x00008B2A}`。有界静态逆向已把
此前的通用 descriptor 细化为 animation descriptor，并确认两条真实的
`flag=0` preserve/migrate 调用链；独立 TEST 在候选前封印 exact2 contract，最终接受
候选的失败安全分类：

`PASS_SEALED_MT_R02_EXACT2_ALL_UNKNOWN`

两行均为 `0 positive / 2 UNKNOWN`。没有证明 `oldCount<=newCount`，也没有更强的
allocation/span 全称不变量，因此不得进入 positive frontier，不创建空 successor。
v33 strict frontier 仍为 copy/move `323/405`、memset `225/242`、总计
`548/647`、remaining `99`；MT-R02 也未取得用户 assumption waiver，人工逆向策略队列
保持 copy/move `79`、memset `17`、总计 `96`。

## 2. 新增静态语义与停止点

`sub_40A5D0` / `sub_40AAF0` 从 Sprite-like owner 的 `+0x74/+0x78` animation
descriptor 数组按名字选择 stride `0x13C` 的记录。owner 的 primary/secondary runtime
state 分别位于 `+0x30/+0x50`；descriptor `+0x1C/+0x24` 是两组 int32 runtime backing
的元素数。

`flag=0` 进入 `sub_408970` 保留并迁移旧 per-element state。已确认的具体调用链为：

- primary：`Idle` / `Idle2`，经 `0x0043ABE6 -> 0x0040A678`；
- secondary：`HeadLaughUp` / `HeadLaughDown` / `HeadLaughRight`，经
  `0x004E54C5 -> 0x0040AB98`。

两条 caller 都不读取、比较或 clamp descriptor count。目标 callee 在 count 变化时按
`4*newCount` 分配 destination，却在 `oldCount>0` 时按 `4*oldCount` 调用 `memcpy`，
没有 `min(old,new)` 或支配性 hard bound。两行因此继续保留同一最小符号反例：

```text
oldCount=2, newCount=1 -> allocation 4 bytes, copy 8 bytes
```

本轮没有找到把具体 `2 -> 1` 写入上述 animation asset record 的 current-PE producer，
所以不能报告为运行时已发生的越界；但已证 `flag=0` CFG 也没有排除该组合。第一未闭
producer 类是 name-selected animation asset descriptor 的 parser/constructor/adoption/
copy/move，以及 address-taken callback 的 receiver/name/flag/reentry fixed point。按有界
停止要求，不继续枚举 primary `1048` 与 secondary `91` 个完整 selector caller。

## 3. 证据与作者分离验收

REVERSE：

- [animation descriptor bounded successor v3](../02-copy-move/native-join-manager-this-mt-r02-animation-descriptor-bounded-successor-v3-handoff.md)；
- candidate SHA-256：
  `D441CE7910F448153140E9F1F75B4EA692276246829ABE244ACF566241C4F46C`。

候选前封印与独立 TEST：

- [sealed preparation](../02-copy-move/native-join-manager-this-mt-r02-exact2-sealed-preparation-v1-handoff.md)；
- sealed spec SHA-256：
  `7CAEAC7ADE8E53255AAC70E03BC58C8835563F966B706D83BC4CE76B6D822169`；
- [sealed independent audit](../02-copy-move/native-join-manager-this-mt-r02-exact2-sealed-independent-audit-v1-handoff.md)；
- independent audit SHA-256：
  `9677D086F43A65F4177BA21619DD189481AA6CC10E6108FA1696827260CAAB4B`。

TEST 逐行按 `backingExtent -> countAndNoWrap -> sourceLifetimeAndIdentity ->
targetIntervals` 裁决。两行第一处开放义务均为 `backingExtent`，原 countermodel 逐字保留；
`frontierCompositionEligible=false`。

## 4. 压缩测试与权限边界

REVERSE 只做一次作者侧结构校验并拒绝 3 个核心 mutation，不替代独立验收。TEST 只执行：

- preparation 一次 create-new、恰好一次 verify-existing、4 个决策性 mutation；
- candidate audit 一次 create-new、恰好一次 verify-existing、同 4 个决策性 mutation；
- 一次 wrong-hash fail-before-write，probe 文件调用前后均不存在。

没有运行 Cargo、全量 T0 build、历史宽回归、游戏、进程读取、x32dbg、注入或 IDB
修改。manager-this、whole-image、runtime、active、05-05、T0-05 总门和 T0-06 权限
继续关闭。
