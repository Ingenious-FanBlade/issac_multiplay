> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# 自有 Virtual Device ABI 正常功能静态确认 v9

任务：tx-01-15。依据[完整执行设计 v8](virtual-device-owned-abi-static-confirmation-next-work-design-v8.md)完成 W01–W06。配套原始证据见[静态证据 v9](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v9.md)。

本报告只确认限定正常路径的静态事实和设计候选。原版 input manager 的读者及 Native C/S 自有 device 必须隔离；C/S 不依赖或复用 Online Manager、NetInputDevice、FrameStore。保留 38 槽自有 device、六个覆写槽、S2、NEVER、构造前固定 DLL 表、常驻、每 session 一次以及统一输入语义。本文没有修改 contract、schema、生产源码或权限。没有观察运行值，也没有声称正常功能已接入生产。

## W01：Join action 来源与接收层

当前 IDA 只读复核了 Join pass 的五个 immediate store 和相邻设备 callback 查询。每项均为本轮 IDA-DIRECT：

| VA / RVA | 指令 bytes | 十进制 action | 用途 |
| --- | --- | ---: | --- |
| 9BB05D / 5BB05D | C7 45 E0 13 00 00 00 | 19 | 开启或复用 selector，随后查询在 9BB064。 |
| 9BB2B0 / 5BB2B0 | C7 45 DC 14 00 00 00 | 20 | selector 导航，随后查询在 9BB2B7。 |
| 9BB2FE / 5BB2FE | C7 45 DC 15 00 00 00 | 21 | selector 导航，随后查询在 9BB305。 |
| 9BB393 / 5BB393 | C7 45 DC 0E 00 00 00 | 14 | confirm，随后查询在 9BB39A。 |
| 9BB3C2 / 5BB3C2 | C7 45 E8 0F 00 00 00 | 15 | cancel，随后查询在 9BB3C9。 |

不得把 native device vtable slot 21 / +0x54 raw flush 与 logical action 21 混为一项。raw flush 在 A1EE10 路径中复制 native current 到 previous；它不更新 C/S 私有 previous。

consumer 的 receiver 链分层如下：

| 层 | receiver / 输入 | 证据边界 |
| --- | --- | --- |
| Gameplay consumer | Movement、Shooting、Bomb、Item、Pill/Card、Drop 逻辑以 Player 为 this；既有 v8 E03 逐项列出查询 action 与 helper 地址。 | STATIC-INHERITED v8 E03；本轮直接读取了这些函数的 xref 调用者集合，未将继承的逐条 action 伪装成本轮重读。 |
| Player helper | 956110 / 9561E0 / 9562D0 用 Player+0x1618 的正 controller key 请求 pressed / triggered / scalar。 | STATIC-INHERITED v8 E03。 |
| Manager callback | keyed callback 的 this 是原生 Manager；它按 key 在 Manager active device/control pair vector 中寻找 device。Join 9BAC40 也以 Manager receiver 读 action。 | STATIC-INHERITED v8 E03；Manager、device 两个 receiver 分别保留。 |
| Device query | matched device 是 query 回调的 this；pressed、triggered、scalar 分别经 device slot +0x30、+0x34、+0x38。 | STATIC-INHERITED v8 E03；scalar 三层 ST(0) → A20AB0 output/AL → 9562D0 XMM0 继续引用 v8 有效证据。 |
| Association lookup | Item / PillCard 的 90AC70 receiver 是 n298+0x4B3D8 的 list object；第二参数是 Player。 | caller receiver 由 tx-01-13 复核继承；本轮直接读 90AC70 函数体和 xrefs。它不等同于 keyed Manager callback。 |

Native 六类 gameplay consumer 的动作关系沿用 v8 E03：Movement 查 Drop 11 与 movement 0–3；Shooting 查 shooting 4–7，并有条件查 movement 0–3 及 Drop 11；Bomb 查 8；Item 默认查 9、条件查 10，并查 Drop 11；Pill/Card 默认查 10、条件查 9，并查 Drop 11；Drop 查 11 triggered 与持续 pressed。真实 cross-family 是 Movement→Drop、Shooting→Movement/Drop、Item→Pill/Card、Pill/Card→Item。固定 BAB240 = 0,1,2,3 与 BAB370 = 4,6,5,7 继续作为固定方向表，不写成动态未知。

## W02：相同偏移下的不同对象、callback context 与 key

以下布局只使用对象局部名称。偏移 +0x378 不构成跨对象同一字段的证据。

| 对象 | 静态角色和 +0x378 内容 | 可确认关系 |
| --- | --- | --- |
| Gamepad device，A21D40 / A22190 的 this | A21D40 分配 0x4F0 字节并写 Gamepad vftable；A22190 将 0x178 字节 hardware descriptor 复制到 this+0x378。复制末端刚好到对象大小 0x4F0。 | 本轮 IDA-DIRECT。此字段是硬件 descriptor block，不是下方的 source association pair。 |
| 913760 callback list element | callback 在 element+0x378 读取一个 pair pointer；pair+0x0C 是 source-device key。key 匹配断开的 device+0x0C 时，以 902F10(pair,0) 清关联。 | 本轮 IDA-DIRECT；仅限 callback list element，未证明它与 A22190 的 Gamepad this 同一对象。 |
| 90AC70 lookup entry | entry+0x0C 是 own/device key，主查找与 Player+0x1618 比较；失败诊断时才另读 entry+0x378 pair 和 pair+0x0C source key。 | 本轮 IDA-DIRECT。主匹配 own key 与 source association key 必须分开。 |

context 身份及数据流：

1. 952410 在 952DBC 将 ECX 设为 n298+0x4B3D8，并在 952DC2 调 906D60。906D60 注册 913760 callback 与该 owner；906E71 是 callback 地址的 push，实际 A1D610 注册调用在 906E7B。
2. 913760 遍历 callback 参数中的 vector。若 element 等于从断开对象 RTTI cast 得到的 NetInputDevice 指针，就从 vector 删除该 element；否则检查该 element+0x378 的 pair source key，并在相等时清 pair。NetInputDevice cast 的输入是断开的 a1，不是把整个列表元素批量判成 NetInputDevice。
3. tx-01-13 已直接复核 Item/PillCard 的 791AF3、7A3DC9 调用 90AC70 时 ECX 也是 n298+0x4B3D8；本轮 xrefs 直接确认这两个 callsite，90AC70 函数体直接证明它按 list begin/end 遍历。故 callback owner 与 Item/PillCard lookup 使用同一个存储对象；它是此 lookup/context vector，不因此自动等同于 Online roster 或 Player selector。
4. 90AC70 的主条件是 entry own key 与 Main Player 的 +0x1618 key 匹配；source pair key 仅在诊断路径读取。913760 的移除/清理以断开 device key 查询 source pair。两条 key 流不能合并。
5. Offline 空 Online roster、主 Player vector、四个 selector slot、n298+0x4B3D8 lookup/context vector、Manager active vector 是不同状态。范围前提允许创建 B 前已有 A。运行时 “空 Online roster” 不证明 n298+0x4B3D8 vector 空，也不证明 fallback membership 空。9BAC40 中对 list begin==end 的分支只证明该局部 vector 的空条件被使用。

因此 callback/context → list → entry own key / source association pair 的静态关系已比 v8 闭合；剩余 UNKNOWN 是 listener 实际集合、运行时 entry/pair/key、通知是否命中 B，以及这个 vector 在目标会话中的 fallback/roster 内容。

## W03：正常 remove、selector 与 B exit

静态可分成三种操作：

* 正常 Manager remove：A20EB0 在 Manager receiver 上调用 +0x7C、+0x80 通知槽；按 8-byte device/control pair 扫描 Manager active vector，匹配 device 后压缩并删除 pair，再处理 control 引用。当前反编译没有 Player+0x1618 写入。A1FC00 中 A1F280 的 poll 返回 0 走自动移除路径；暂停或退休准备期间不能把 neutral 当 poll=0，否则 device 会提前离开 active membership。
* selector cancel / reselect：9BAC40 的 action 15 在为真时把对应 selector index 设 -1；没有调用 SetControllerId，也没有退出 Player。SetControllerId 7A6450 的 receiver 是 Player，写 Player+0x1618；propagate 参数非零时会更新关联对象，并扫描关联 Player。故 SetControllerId(B,-1,1) 是一个可能影响关联 Player 的 unbind 写点，不能称为 B exit。
* 全 Player 重置：908070 遍历 Main Player vector，对每个 Player 调 SetControllerId(Player,-1,0)。908070 只有 90E6D0 一个数据引用；90E6D0 以 LOST_CONNECTION_TO_SERVER 路径调度它，且有 Online-session guard。它不是 C/S 本地 B exit。9BF530 也遍历所有 Player 先置 -1 再重映射，仅由 959720 调用，属于状态转换中的组清理，不是 B 专属退出。

未找到正常 local B exit 的直接入口。现有静态材料不能判定 B 被移出 Player vector 的时点、B 专属 unbind、SetControllerId 的目标传播范围、与 Manager remove 的合法先后，也不能把 selector cancel 或 Online lost-connection 路径替代此缺边。保留 Pause 候选为 B 正 key 与 active pair 不变、query neutral、poll 返回非零；RETIRING 必须等真实 B exit/unbind 语义明确后再关闭 admission 并显式 remove。NEVER 继续要求保留已构造对象、control 和 DLL。

## W04：带 guard 的 poll → Join → Authority → Room 路径

sub_954CD0 的同一 state=2 更新分支存在下列有条件顺序；不是对所有 frame 或 room path 的全称断言。

| 顺序 / VA | 静态条件与动作 |
| --- | --- |
| 1. 954F7D → A1FC00 | 当前 IDA bytes 为 E8 7E AC 0C 00；紧随 954F84 调 95B310 设备 reselect/update。A1F280 先 native C4←C0、清 C0，再调用 device poll slot +0x78；其零结果沿 A1FC00 自动移除分支。 |
| 2. 9551BD → 6FA540 | 仅 state=2 且 sub_9C3990(n298+315504)、sub_9505E0(v25) 均返回 false 时进入 Join dispatcher。6FADA7 调 9BAC40，正常返回地址 6FADAC。 |
| 3. Join 后 room guards | 同一 outer 路径随后在 n298+0x4B130 非零时跳过；否则 n298+0x4ABBC 的 bit0 为 1 时走另一分支并跳过；其余满足 manager-list 条件的分支在 955231 调 6FADC0。 |
| 4. 6FADC0 → 6FBA4D → 802980 | 6FADC0 有自身 return guards；仅路径到 6FBA4D 时执行 call 802980。C/S source hook 在原函数前 Authority(true)，原函数正常返回后 Authority(false)。 |
| 5. Room/entity/Player阶段 | 802980 的 803E70 call 进入 4186C0。4186C0 迭代 entity update list 并在 41885E 用 each entity vptr+8；另在 418D86–418E4D 迭代 Main Player vector 并调用 Player vptr+0x0C。 |

现有 loop_runtime.cpp anchors：RVA 554F7D→target RVA 61FC00；RVA 555231→target RVA 2FADC0；RVA 2FBA4D→target RVA 402980；RVA 403E70→target RVA 186C0。SOURCE-DIRECT 数组和 IDA-DIRECT call bytes 均见证据 E07。这些是 hook 所用静态 anchor，不是当前进程中已安装/已触发的证据。

Room 与 Player阶段只能按上述可见范围串联。六类 consumer 函数及其对 Player 的 helper 请求沿用 v8 E03；sub_782AF0 内有多个这些 consumer 的直接调用，函数地址另有 B6BDDC 的 data reference。但本轮没有证明该 data reference 是哪一 Player vptr，也没有证明 4186C0 的 vptr+8 或 Main Player vptr+0x0C 必然落到 sub_782AF0/这些 consumer。故 Player consumer 相对 Room entity dispatch 的精确执行点仍是 UNKNOWN；不得把 vtable callsite 本身写成完整静态关联。

## W05：建议的 DESIGN_ONLY 晚发布接续

建议采用一个明确的“Manager pre-poll staging → device poll latch → Room Authority consumes same tick”方案，暂不采纳也不修改 v2：

1. 在现有 954F7D poll call anchor 之前，由主线程 coordinator 为 next_tick = last_completed_tick+1 解析一次 command，形成不可变 GameplayPublication{publication_epoch, target_tick, apply_generation, owner_generation, state}。没有当 tick 输入时也发布一个明确 neutral marker；target tick 在 Authority post 正常提交之前不递增。
2. 自有 device poll 仍是 S2 previous/current 的唯一写者：新 publication_epoch 第一次 poll 时一次性 previous=current、current=publication；同一 epoch 的重复 poll 不推进 previous/current、不重复产生新 edge。native A1F280 对原生 C0/C4 的每次机械搬移仍是独立事实；自有六个 query slot 必须读私有 S2 latch，不把 native C0/C4 的重复搬移当第二次 publication。
3. 6FBA4D Authority(true) 校验 target_tick/epoch 与这次 candidate 相同，只绑定同一 immutable snapshot，不重新 Resolve/推进 previous；Authority(false) 仅在原生 Room 正常返回后提交 tick 并退休 candidate。若目标 Room 本轮未到达或未正常返回，沿用同一 tick/epoch，不额外消耗 edge 或生成下一 tick。其他 Room callsites、异常返回和多个 authority commit 不在本次证明范围。
4. Join 不复用 0–11 gameplay state：独立 JoinCommand{sequence, join_epoch, target_update_epoch, target_authority_tick, mask}。在已确认 caller 6FADA7/9BAC40 pass begin 时，按当前有效 admission 暴露 14/15/19/20/21；正常返回到 6FADAC 执行 JoinPassComplete 并关闭该 pass。若目标 update 没有进入该 pass，旧 event 在下一次 pre-poll epoch 之前关闭/过期，不能跨 pass 自动重放。CloseJoinAdmission 清除待发 Join event。
5. poll=0 保留为 explicit native auto-remove 路径，不用于 neutral、Pause 或等待 B exit 的 RETIRING；这些阶段 poll 返回非零，之后由 coordinator 在 B exit/unbind 语义确定后显式正常 remove。

当前 contract v2 将生命周期转换放在 Authority tick 提交后、下一次 manager update 前，并把每次 poll 的 current→previous→current 定义成唯一快照推进。候选改为 manager pre-poll staging，且只允许每个 publication_epoch 推进一次；Authority 改为同 tick reader/commit owner。它需要更新 contract 对 phase owner、publication_epoch、no-input neutral、same-epoch poll 幂等和失败未提交 candidate 的定义。另需独立 JoinCommand/pass/admission contract，并为持续按住的 8–11 扩充版本化输入语义；这些都是 DESIGN_ONLY，不得静默改变现有 schema v1。

| 输入动作 | Pressed / scalar | Triggered | 当前状态及推荐候选 |
| --- | --- | --- | --- |
| 0–7 按下 | 当前方向幅值非零时为真 | previous=0 且 current 非零时为真 | 当前与候选均支持。 |
| 0–7 持续按住 | 当前方向幅值仍非零 | false | 新 tick 上 publication 更新一次；同 tick 重复 poll 不推进边沿。 |
| 0–7 松开 | current=0，因此 false | false | 中性 publication 清除轴。 |
| 0–7 再次按下 | current 非零 | 前一 tick 为 0 时为真 | 同一 epoch 不重复再发。 |
| 8–11 按下 | 当前实现从 edge_flags 读值，因此单个上升沿 tick 为真 | 当前 edge bit 为真 | 推荐候选将 held 与 edge 分开：按下 held=1、edge=1。 |
| 8–11 持续按住 | 当前实现下一采样没有新 rising edge 时变 false | false | Drop 11 的持续按住路径当前无法表达；版本化候选用 held=1、edge=0，使 Pressed=true、Triggered=false。 |
| 8–11 松开 | 当前实现 false | false | 候选 held=0、edge=0。 |
| 8–11 再次按下 | 新 rising edge 时为真 | true | 候选 held=1、edge=1。 |

本轮源码读取确认 kValidHeldMask 目前只有 kHeldShoot bit0；BuildRawSample 仅以 aim 非零设置该 bit，并将 action8–11 rising edge 写入 edge_flags。ActionMagnitude 8–11 只读 edge_flags，ActionTriggered 8–11 也只读当前 edge bit。建议未来版本为 action8–11 分配 held mask bits 1–4、edge mask 继续表示各动作上升沿，使 pressed/scalar 读 held、triggered 读 edge；需提高输入 state version 并重新审定 wire contract。v1 中新增 held bits 目前会被拒绝，不能宣称已支持。

重复 query 保持 S2 只读：同一 active snapshot 的重复查询返回同一值，不消费 edge。设计可阻止跨 publication/tick 或 Join pass 的隐式重放，但不能单靠只读查询保证同一函数内重复 true query 只产生一次游戏副作用；若要求 exactly-once side effect，需另有显式 operation/event semantics。

## W06：六条手工轨迹与系统状态

以下均为静态手推，不是模拟、harness 或运行测试。

| 轨迹 | 命令/查询 | 可证结果与下一阶段 |
| --- | --- | --- |
| 1. select+confirm 同 pass | Offline scope、A 已存在、B 创建前不存在、虚拟 device 唯一正 key K；新 Join epoch 同时带 19+14。 | 9BAC40 先查询 19，再按 selected key 查询 14；同 pass 可创建并 SetControllerId(K,1)。6FADAC close 本 pass；实际 B 创建/发布仍是规划假设。 |
| 2. 同 pass 重复 query | JoinPassBegin latch E；重复读 action19/14。 | query 只读，返回同一 mask；slot21 raw flush 不消费 Join event。9BAC40 正常完成后 6FADAC close；重复 query 的游戏副作用次数没有静态 exactly-once 保证。 |
| 3. 同 gameplay publication 经多个 Join pass | Gameplay epoch G 未变；第一 pass 有 Join E1，随后第二 pass。 | pass1 关闭 E1；pass2 继续读同一 gameplay G，但没有 E1，需新 Join epoch 或 neutral。现有二 pass 频率不是 binary 必然；该候选阻止跨 pass 重放。 |
| 4. 同 epoch 重复 poll / 下一 tick 无新输入 | E 的第二次 poll、随后一个没有输入的新有效 tick。 | 私有 S2 同 epoch 不推进；新 tick 发 neutral marker，只推进一次并释放轴/held。当前 v2 未定义 epoch 幂等；若重复 read-only query 仍在同 scope，返回值仍可重复为 true。 |
| 5. cancel + close | selector 正 key K、index 有效、B 仍保有 K；Join E5=15。 | action15 真只将 selector index 设 -1；不改 B+0x1618、不退出 Player、不移除 pair。pass complete 后 close admission，后续 gameplay 由新 publication 决定；保留登记时 poll 非零。 |
| 6. retirement / remove | B 已登记且有 K；selector/cache 可能仍关联。 | 首先关闭 Join admission；须先由未确认的正常 B exit/unbind 语义决定 Player 处理，再进入中性 query、poll 非零，最后走 A20EB0 normal remove 并确认 member/key miss。当前缺 B exit 与实际 cache/listener 命中，故此轨迹在移除前标为未闭合；NEVER 保留对象、control、DLL。 |

| 系统链 | 当前静态状态 | 运行值 / 生产缺项 |
| --- | --- | --- |
| P01 注册与枚举 | 原生 Main active vector、Join 遍历、poll=0 auto-remove 结构沿用 v8/v7 证据；本轮直接确认 normal remove 与 lookup-context 不是同一容器。 | 自有 device Init/insert/listener 集合及当前成员未知，production register 未实现。 |
| P02 type/name/binding | 38 槽 / 六覆写槽 / S2 / NEVER 与 v2 contract；32 项模板及历史构造证据继承。 | 自有 vptr/name/binding 的真实初始化与当前 primitive 值未实现/未知。 |
| P03 Offline Join | action19/20/21/14/15 immediate 本轮直读；9BAC40 普通分支、同 pass confirm 结构已有静态证据。 | Online roster 状态未知；独立 Join event/source/admission 尚无 contract 或 production。B 创建/发布按范围保留 DESIGN_ONLY 假设。 |
| P04 key / object / assignment | 90AC70 own key→Player+0x1618；source pair 另列；SetControllerId this=Player。 | 当前 K、selector、Player count、source association 和 fallback membership 是运行值。 |
| P05 gameplay snapshot | 六消费者/action/cross-family/三层 scalar沿用 v8 E03；SOURCE-DIRECT 的 Authority scope 与输入验证见 E08；poll→Join→Room 有条件次序已闭合到虚调用层。 | vtable→Player consumer 的关联、同 tick publication epoch、held 8–11 与 production query 未闭合/未实现。 |
| P06 Pause / retirement / cache | normal remove、poll auto-remove、all-player reset 路径和 callback list identity可静态区分。 | B 专属正常退出、listener 实际集合、notification 命中、真实 pause/retirement 运行值未知；完整退休未闭合。 |

## 范围与现状

文档准确性、正常功能完整性、待采纳设计、运行值、生产未实现分别判定：

* 文档准确性：本报告、证据、README 和引用导航经唯一人工整体静态审阅通过（文档准确性与声明范围）；这不表示正常功能完整或已实现。
* 正常功能静态范围：W01–W06 的有界正常链已分析；对象身份与 context lookup 链相较 v8 得到修正，条件 native update 顺序可静态连接到 Room/entity/Player dispatch 层。consumer vtable关联、B exit、listener 运行态仍是明确的静态边界，不能称为所有正常功能全闭合。
* 待采纳设计：W05 的 pre-poll staging、per-publication_epoch latch、独立 Join event、版本化 held mask 均为 DESIGN_ONLY；没有修改 contract/schema/code。
* 运行值：Offline roster、fallback/context vector contents、virtual key K、实际 pair/listener、current Player/selector counts、native hooks 是否安装及运行时 tick/epoch 均未观察。
* 生产未实现：自有 38-slot ABI 的真实构造/Init/registration、独立 Join event 与 pass completion、正常 B exit/retirement、held 8–11 及跨 family synthetic consumer适配均未接通。

## tx-01-15 唯一人工整体静态文档审阅

审阅按执行设计 v8 §6，在 W01–W06 分析和必要文档编辑完成后只进行一次。总体结果：**通过（文档准确性与声明范围）**。W01–W06 覆盖、直接/继承证据来源、对象与 key 区分、受 guard 限定的 poll/Join/Room 顺序、DESIGN_ONLY 候选、静态缺边、运行值与生产未实现均有对应叙述；未将 consumer vtable关联或 B exit 缺边写成已闭合。报告/证据/README 导航人工对照目标目录通过。正常功能完整性仍不通过：consumer vtable目标、B专属退出与运行态未闭合。

审阅核对范围：本报告、证据 v9、README 当前入口、设计 v8 的 W01–W06 要求、project-conventions 与 contract v2边界、原始 IDA 地址/bytes/source evidence 标记、对象 +0x378 身份、context/own/source key、B exit 缺边、hook 顺序及 guards、input truth table、六条手工轨迹、P01–P06、DESIGN_ONLY 与运行值/生产实现区分。历史 v8 来源审阅失败与错误记录保留在 README，不改旧报告或证据。本轮没有运行链接脚本、测试或构建。
