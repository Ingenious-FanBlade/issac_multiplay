> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Native Join Admission 独立测试结果 v1

工作包：CS-T0-05-04。日期：2026-09-01（Asia/Shanghai）。测试角色独立于主线契约实现和两份逆向交接。

正式新门为 `PASS_STATIC_ADMISSION_REVIEW_VERIFICATION`，退出码 0。合法公开诊断为 `BLOCKED_BY_EVIDENCE`，退出码 1。本结果接受新增证据的局部依据、离线规格反例及拒绝边界，不接受原生无持久化闭合。

`INIT_EFFECTIVE_STATE_PROPAGATION_NOT_FROZEN` 和 `INIT_PENDING_LIFECYCLE_WRITE_EXCLUSION_NOT_FROZEN` 均保留。active、live、CS-T0-05-05、T0-05 和 T0-06 不解锁。

## 执行范围

只读取已批准 SHA 的磁盘 PE，使用冻结独立 PE parser 和 GNU objdump 重新解码。旧 `static_join_closure` 工具只读导入，新测试全部位于 `cs-t0/tests/join_admission/`。未修改旧脚本、旧 tests、Rust/Native 生产源码或二进制。

未枚举、打开或控制游戏进程，未建立 mapping、注入、arm、消费 claim、签发 live 许可或运行 55-slot runtime。完整旧自动门没有重跑；哈希保护不等于重新签发旧生产验收。

正式命令如下；安装位置与用户临时目录不写入产物。

```powershell
pwsh -NoProfile -NonInteractive -File cs-t0/verify-coop-native-join-admission.ps1 -GameExe <approved-disk-exe> -RepositoryRoot <repository> -Objdump <GNU-objdump> -Python <Python-3.13>
```

正式门从头执行，不复用开发期局部 PASS。合并 stdout/stderr 的完整日志以 create-new 保留。随后单独通过公开 inspector create-new 生成发布根 inspection：exit 1；stdout 与文件字节相同；JSON 全结构与正式黑盒 baseline 相同。

## 实际验收结果

| 检查 | 结果 | 边界 |
| --- | --- | --- |
| Init v3 | 30 anchors / 1116 instructions / 187 direct edges | 磁盘字节、独立解码及声明边界 |
| Window v2 | 19 / 408 / 96 | 同上；49 个新 anchor 合计 1524 instructions / 283 edges |
| 旧 Init v2 + Window v1 | 146 / 1994 / 456 | 重新核对冻结磁盘交接，不扩张旧结论 |
| 原生语义局部复核 | Init 8 + Window 7 项，60 条明确关键指令 | 局部含义，不是完整 callee、alias 或可达性证明 |
| 读取元数据 | 21 guards（旧 18 全保留），15 Window scalar，132 Player scalar | owner、offset、width、comparison、value 与绑定来源相符，不证明运行中值保持 |
| 冻结独立 decoder/parser 回归 | 14 tests 通过 | 工具解析、解码反例 |
| 直接契约反例 | 23 项拒绝 | 调用真实 check_contract，不被公开 hash 门遮蔽 |
| 合成模型 | 104 项：4 正向、100 反向 | SYNTHETIC_SPECIFICATION_ONLY，不产生原生证据 |
| 新 parser/reference/handoff schema | 21 项拒绝 | 直接分支检查；其中 7 项在 parsed-handoff loader seam 注入 |
| 公开黑盒 | 5 组、57 断言通过 | 原仓库、完整临时夹具、同字节目标、参数、create-new、别名/junction、密封篡改和恢复 |

契约共 191 条 readerFields。数量不表示 reader 已完整或实现；未知布局、owner 生命周期与容器枚举仍使 `readerContractComplete=false`。读预算是 DESIGN_ONLY 保守上限，不是原生已观测长度。`generationContractSpecified=true` 仅表示规格已指定，同地址双采样不能证明身份，`nativeAbaExclusionProven=false`。

## Init v3 独立语义判定

完整决定与原始指令核对记录见 verification 的 `nativeSemanticReview.initReview`。本测试不是只核 hash、anchor 数量或 UNKNOWN 标签。

| Proof ID | 原始指令支持的局部结论 | 不能据此关闭的范围 |
| --- | --- | --- |
| FRESH_EFFECTS_EMPTY_HELPER_PATH | 构造器清零 effects 头部和 flags，empty-clear 分支绕过 removal | 中间调用的值保持和 alias |
| INIT_USED_SOURCE_ARRAYS_ZERO | post-resize 清零已使用 raw/smelted/pill dword 和 extra byte 区间 | 有效原生边界、后续保持；内容零不等于头指针 canonical-null |
| INIT_ACTIVE_SLOT_IDS_ZERO | 四个 16-byte 零写覆盖各 active slot ID | starting-item/config 插入及后续 writer |
| PENDING_TRINKET_CONFIG_SOURCE | Init 清零 dword pointer +0x17C4；null 排除该贡献；+0x17CC 是 byte/mask 0x02 | P0 零基线和中间保持；pointer 为零时不额外要求未使用 flags 零 |
| DIRECT_FAMILIAR_CARRY_SOURCES | +0x1382 的一个 dword 零写覆盖 +0x1382/+0x1384 两个 signed WORD，后者直接参与 desired | 相邻 +0x1380、P0 来源及后续 writer |
| FAMILIAR33_PILL_HISTORY_SOURCE | owner+0x16D4、index 10 传入 begin+index*4 helper，结果参与 familiar33 desired | index 边界、完整公式与中间保持 |
| TYPE0_IMPLICIT_ROLE_GRANT_FALSE | empty roster 下 type 0 不匹配该 helper 的类型分支 | 其它 implicit grants、所有 Count/Has 来源 |
| FAMILIAR_RECONCILE_ZERO_IS_CONDITIONAL | 读取房间已有 familiar；desired=0 时匹配对象仍可触发 virtual-remove | 完整房间总体、生命周期和全部 desired 来源 |

另独立核对 effects capacity +0x1514 有真实比较依据，pending flags 不得误读为 u32，两个 carry 不得误记为两个 dword。没有把 30 个新窗口推广成完整 Init、全部 151 处 desired、P0 传播或自然窗口闭合。

四项缺口均保留 UNKNOWN：

- `INIT_INTERMEDIATE_SOURCE_PRESERVATION_NOT_FROZEN`：零写不能概括全部中间 callee、alias、bulk write。
- `INIT_COMPLETE_FAMILIAR_DESIRED_INPUTS_NOT_FROZEN`：pill/carry 等新增来源说明 raw inventory 不足；所有可达公式和房间匹配未闭合。
- `INIT_P0_SOURCE_AND_TOPOLOGY_PRESERVATION_NOT_FROZEN`：fresh B 初始化不能证明既有 P0 来源与共享更新拓扑。
- `INIT_PENDING_LIFECYCLE_WRITE_EXCLUSION_NOT_FROZEN`：局部来源事实不能排除 direct persistent write 或证明完整窗口 pending/timer/fade/readonly 保持。

## Window v2 独立语义判定

完整决定与 21 条关键指令核对见 `nativeSemanticReview.windowReview`。

| Proof ID | 原始指令支持的局部结论 | 未闭合范围 |
| --- | --- | --- |
| zero-timers-preserved-by-timer-block-only | signed-positive 测试中零值绕过两个 timer 写入/request | 其它 Room/Grid/player/Lua/alias 路径 |
| normal-initialization-clears-both-timers | 两个立即数 dword 零写成立 | 初始 save/config 到 admission 的保持 |
| dimension-not-two-excludes-pickup-room-timer-writer | dimension!=2 绕过该处赋值 90 | dimension 稳定性与其它 writer |
| room-type-not-sixteen-excludes-specific-room-request | room type!=16 绕过所示 room-request call | 其它 room/Grid 分派和 owner/type 替换 |
| fade-seven-predicate-local-exclusion | helper 的 dword+0x34、byte+0x44 任一零返回 false，绕过该 fade 分支 | 其它 fade 路径和字段保持 |
| player-manager-death-call-local-exclusion | Player byte+0x173==0 绕过所示 death call | B/P0 基线与自然保持 |
| lua-update-callback-local-exclusion | callback bitset 首 byte 的 mask 0x02 clear 绕过该 dispatch | 全 callback 注册、owner 生命周期和其它间接调用 |

ENTRY、INIT、NATIVE_UPDATE、LIFECYCLE 四项归纳义务均 UNKNOWN：尚无候选场景准入、完整 Init 关系、所有自然更新调用摘要或 lifecycle writer 排除。STOP 是 `SPECIFIED_NOT_IMPLEMENTED`：已规定第三 manager pass 及 disable/drain/retire/restore/terminal 等待期间的全部自然更新，但未实现或观测。

Native dirty→flush→clean 和 readonly 下 direct-save 是原生条件路径，并未建立它们在拟定候选场景中的可达性。第三 manager pass 和 stop 等待缺失属于规格反例。Init pending-source 和已有 familiar removal 同样只是条件见证。本次没有可标记为已达候选污染路径的完整原生反例，也没有无污染证明。

## 反例实质与模型边界

- FRESH_B、P0 的 17 个保守来源分别注入非零，包含 pendingTrinketConfig、pillHistory、familiarCarry；raw/slot 零不能掩盖其它来源。未来 B 样本、route witness、错误 scratch direction、已有 familiar 不能冒充合法前置来源。
- 测试中间写后恢复、dirty→flush→clean、readonly 清除再恢复、direct sink、pending/timer 漂移、缺少自然更新或阶段、第三 manager pass、stop 等待。等端点或有限观察次数不能替代归纳；合成轨迹完整性本身是输入假设。
- 容器覆盖溢出、倒序、stride 错位、capacity/entries 不符、重复节点或 owner、tree cycle、多父节点、非 canonical-empty roster、部分可读、预算耗尽。模型不执行原生读取，不证明真实容器布局全部已知。
- generation/identity 覆盖回退、owner 改变、A→B→A、控制权和读集改变、失败后重试、不确定 stop 强制卸载。要求 fail-closed，但 native generation/ABA 证据仍缺。
- 公开密封内容篡改为三份输入各 8 次，共 24 次；另各恢复 baseline。它们只证明 hash 边界与不落报告，不计为语义覆盖。直接契约、parser/ref 与合成模型另行到达实际分支。

## 开发失败与正式重跑

开发期发现并修正三处新 inspector 缺陷：UNKNOWN 覆盖边界整文件引用被错误拒绝；inheritedAnchorBindings 单元素数组被当成对象；Windows abspath 在校验前丢弃文件或父目录的尾随点/空格。修正仅涉及新 inspector；sealed contract 和证据未改。confirmed 字段仍要求可解析 fragment；UNKNOWN 整文件引用不当成证明。

attempt-v1/v2/v3 失败全部保留，attempt-v4 完整 PASS。正式门仅本次 attempt-v1，从头完整执行并 exit 0，没有删除失败、覆盖文件或靠局部重跑制造正式 PASS。

| 开发记录 | SHA256 |
| --- | --- |
| out/cs-t0/native-join-admission-development-check-attempt-v1.json | 822E4813ED11792A001C65A496B802A82EFE065F3F81D8AFF0696ED749334FF0 |
| out/cs-t0/native-join-admission-development-check-attempt-v2.json | 07EA6688D749B6A9AFEEF677B2672CADB1BDCEEDE1F0ED7A2925A1956D681A8B |
| out/cs-t0/native-join-admission-development-check-attempt-v3.json | 039E2862DD39DC84B653CD63FAE1FF9DDD39911EE75E89A1AE019B4CA65D7F63 |
| out/cs-t0/native-join-admission-development-check-attempt-v4.json | 9E292FFE22AE9289571D684CD1E9DA804B5CAAD0AD1C517D76A06C4511C493DF |

另保留 `native-join-admission-alias-development-attempt-v1.json`、Init/Window 两份独立开发磁盘 audit 和两份其它实例 smoke 记录。本 Markdown 第一次 Python create-new 写入遭 PermissionError，文件未创建；失败保留为 `out/cs-t0/native-join-admission-test-results-write-failure-v1.json`，随后仅在目标仍不存在时以 Add File 创建。正式 verification 未受影响。

## 历史保护、退出码与权限

正式前后结果相同：18 个先前 publication binding、20 个先前 verification binding、16 个 inherited reference、164 snapshot 文件、149 original baseline 文件、3 个生产二进制全部 SHA 一致。另核 progress v2 的 9 引用、plan v8 的 12 artifact binding、旧 static verification 的 32 输入，共 39 个去重保护绑定。正式 verification 记录本轮与旧链共 56 个直接 inputBindings，并在结束前全部复核。

旧生产 preflight 仍为 `STATIC_PREREQUISITE` / `BLOCKED_BY_EVIDENCE` / exit 1，原七项缺口未删。正式 matrix 不由本次测试执行。合法新 inspector 总是 BLOCKED / exit 1；非法输入不输出 JSON，不创建或覆盖 report；Python 验证错误 exit 2，PowerShell 参数绑定错误保留其非零退出码。独立 verifier 只有完整成功才 exit 0，失败保留新记录并 exit 1。

runtimeImplemented、runtimeObserved、runtimeReaderImplemented、processInspected、mappingCreated、nativeInjected、armEligible、claimConsumed、activeJoinEligible、liveAuthorizationCreated 与所有下游解锁均为 false。nativeStaticClosureComplete、readerContractComplete、nativeAbaExclusionProven、zeroPersistenceProven 均为 false。13 个 obligation 仍有 12 个 UNKNOWN；已指定的 failure policy 不产生运行权限。

## 正式绑定

| 文件 | SHA256 |
| --- | --- |
| cs-t0/coop-native-join-admission-contract-v1.json | 411C48EB7F0916DBC88D3AE319A841CA508CD0BF5173AFF4357C9C401B93978B |
| cs-t0/coop-native-join-init-closure-v3.json | 17E9EA8DB6EC4F59FE19B89FAD587916A83B88BFB54783FB766501C11BC9B9BF |
| cs-t0/coop-native-join-persistence-window-v2.json | 8E1E345F3CEAF9F62294519C099AF18F93FCC0604D0C90C10A7D594ABFA5CA31 |
| cs-t0/coop-native-join-admission-verification-v1.json | EB11FF0ECE82DB2D6E21D285923E97573BEDCAD7569AD3900B3B359B0BCFE2FC |
| out/cs-t0/native-join-admission-inspection-v1.json | 508148EB14C3AA038C53CC210E01718F3F870DD08751D90509500C494DE8E8F5 |
| out/cs-t0/native-join-admission-verifier-attempt-v1.log | 1969DE20F3844C5B4FA03A8D13F2FDC5B98AA4BCCC1B4813C92A15FDBBBCCACD |

工具源码和两份 Markdown 逆向交接的精确 SHA 由 verification 的完整 inputBindings 保护。本报告晚于 verification 和发布根 inspection 创建；不会反向修改已封印 verification 输入。后续 plan/progress 单向绑定本报告、verification、inspection、日志及保留失败，不回写旧链。
