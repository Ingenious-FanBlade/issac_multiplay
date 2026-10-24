> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 admission v2 independent test results v1

日期：2026-09-01。角色：独立测试；只读磁盘 PE，无进程枚举、游戏进程读取、注入、游戏输入或生产构建。本文 create-new，不修改旧验收条件。

## 正式结论

**FAILED_SYNCHRONOUS_INIT_P0_PROOF，exit1。原生同步证明 UNKNOWN。**

本轮15项检查全部实际执行。14项工具、模型、清单、公开 CLI 或历史保护检查通过；原生同步语义检查为 `COMPLETED_WITH_UNRESOLVED`。`PASS_LOCAL_CHECKS_ONLY` 只表示局部检查完成，不是 Init/P0 整项证明通过。未发布新计划或进度；当前仍为 plan v10 / progress v4，三个可变索引未改。实机矩阵主项仍 `NOT_STARTED`，所有 active/live/downstream 权限为 false。

正式命令（路径脱敏）：

```text
pwsh -NoProfile -NonInteractive -File <repository>/cs-t0/verify-coop-native-join-admission-v2.ps1 -GameExe <disk-target> -RepositoryRoot <repository> -Python <python313> -Objdump <GNU-objdump244>
```

环境：Windows、Python 3.13.7、GNU objdump 2.44。目标只按批准 SHA256 验证：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`，PE32/x86，基址400000，映像大小8FE000。没有把静态结论标为运行时观察。

## 实际检查与限制

| 检查 | 结果与明确边界 |
| --- | --- |
| v2 契约 | 38项；拒绝 signed/unsigned 替换、错误宽度/掩码、前提冒充已满足、未来读回及权限扩张。191 reader、21 global guards、3 conditional premises、11 aggregate reader UNKNOWN保持分离。 |
| 同步离线模型 | 51项；字节重叠、别名链、写后恢复、第二轮污染、漏调用/consumer、错误剪枝、已有 matching familiar、未初始化字节及 ABI 数值范围。它们不是原生完整轨迹。 |
| 独立解码反例 | 20项；真实合成 PE 编码验证漏写者/stack写、SSE与string区别、word/dword、控制目标进入指令中间、缺边/出口、未归属指令、FSTP隐含x87操作数的受限规范化。 |
| 直接 JSON parser | 8项；重复字段、非有限数、无效UTF-8/格式，以及合法大于旧16MiB、低于新版64MiB上限的完整函数证据。不是通过hash变异替代parser测试。 |
| 结构反例 | 19项；缺根函数、指令、call、writer kind/width、出口/边、重复覆盖及把 UNKNOWN/反例强升为完成。合法未决路径保留为 UNKNOWN。 |
| 原始清单 | core45+side47，共92函数、30750指令、3672 CALL、12848已分类写者、217出口，与磁盘字节独立重建逐RVA集合对照。不是靠总数判通过。 |
| reconcile/P0 | 精确核对151个真实363370调用点及755个实际push位置，并核ECX赋值位置。未把Ctree近邻定义或IDA栈差当作全部路径的到达定义证明；所有desired及P0保持仍UNKNOWN。 |
| 公开入口 | 6组、71断言；原仓库/完整临时fixture/同字节目标一致；缺参/重复参、错误目标、sealed篡改、create-new、ADS/DOS、尾点/空格/父目录别名、junction拒绝。合法始终BLOCKED/exit1，非法无JSON产物。 |

独立审查接受8个有限条件式lemma：3579C0的44字节copy；2030的静态factory注册写；5F34D0、5F3320、60D850的Tears局部路径；29550的signed比较；17850和176F0的getter。它们均不建立调用者入口前提或对象生命周期。

cb24的bitset+0/mask01000000、cb54的+4/mask00400000以及2FE462的原始JL已独立核对。两callback在mask前读取registry+10的指针值；clear分支不解引用context，因此不能额外假定context非空。cb54还提前读Effect+2C。29553的SETGE与JL是有符号语义；80000000/FFFFFFFF等值区分它们与unsigned比较。前提值未在实机或预发布reader中建立。

四项整合证明全部UNKNOWN：fresh B来源保持、全部desired与实际Room matching、P0来源及拓扑、完整manager返回。主要剩余项是跨callee别名/资源和source保持、constructor store到consumer的到达关系、已有实体移除、parent/secondary递归、callback掩码与owner保持、Effect旧pool内容、未证循环/异常返回。独立解码还明确保留177个间接调用、15个间接跳转、trap/SEH/noreturn问题、1条未归属指令和230条尚未分类的指令效果；这限制了完整效果/写者证明，不能由清单一致消除。

530A7C确实CALL530980，空Effects不能把该调用裁为不可达；只可在有效empty header前提下推导getter结果。desired为0仍不能排除已有matching familiar的移除。没有确认候选场景可达的持久化污染反例；未决覆盖与真实反例分开记载。

## 历史保护与失败留存

正式报告316输入均在末尾复核；174个既有源文件/测试保持原hash（100 Rust/manifest、59 native、15旧工具文件）；旧v1的56输入、164快照、149 baseline、3生产文件保持。旧v1公开报告全结构等于已发布版本，旧生产preflight仍exit1且原7缺口完全相同。旧身份及固定UNKNOWN语义没有用于v2成功判定；旧helper只承担历史回归/只读基础设施。

本轮开发失败均保留，正式输入列出25份admission-v2开发/失败记录，core/sidecar显式绑定另外的逆向失败历史，包括SSE/string误判、writer CF_CHG位错误、空Effects错误剪枝、缺隐式stack和switch出口、MD首次写入失败。测试侧还保留旧16MiB parser边界、FSTP显示适配、RepositoryRoot别名及scope绑定格式适配失败。它们没有被覆盖为PASS。

正式验收后第一次依赖审计错误地把历史失败记录中的 `affectedSource`（修复前的代码hash）当作当前输入；已另存 `out/cs-t0/native-join-admission-v2-post-verification-audit-attempt-v1.json`，SHA `12CD24EC7783EB33170F5724C8E9A9BBB33879DA6A4C38A4831134DED2058DC3`。316正式输入未漂移。后续依赖审计只展开本轮正式产物的显式SHA边，旧历史/快照及失败诊断均为hash核验的opaque叶；不重新解释快照相对路径。

## 本报告输入绑定

| 相对路径 | SHA256 |
| --- | --- |
| cs-t0/coop-native-join-admission-contract-v2.json | 427EA15D1CC723B31759122106A35A77E03A18E0F10C656DB863DDBA0B04E7B5 |
| cs-t0/coop-native-join-init-closure-v4.json | 9EC4241EDC902E9C51482DA3B2EF508CE232007D392D84470D6C920476B5CFEA |
| out/cs-t0/native-join-init-v4-cache-p0-evidence-v5.json | 9B646AC8A7EBB4EB4CEABB2F7E081945B9C840F48A49A81315823DB9929BC2DC |
| cs-t0/coop-native-join-admission-verification-v2.json | DF2992EA34536BA05AB059CCD8E954DBCCF024752FE32E0CA014BCD90E4AEBA5 |
| out/cs-t0/native-join-admission-inspection-v2.json | 90FDBEFB6B91C77B6CD31407999FC8F3AE2A197F55678F79F76D1450532C9AD5 |
| out/cs-t0/native-join-admission-v2-verifier-attempt-v1.log | 3105F7877187440A6832FEDF007EE94D1793DEAB5A95CA9E4840AD780AA5DEE7 |
| cs-t0/coop-plan-v10.json | CABE3489941C907F6776E954EC255F49AD8613E274AB6014FA3688FB0EF6E658 |
| cs-t0/coop-native-join-progress-v4.json | 887967EA8330EFD5DA5D8A8A536CF88A735CBA731F67C10B68F3A602AF0B31AD |

本文不绑定自身或未来审计hash，不回写任何sealed输入；后续独立审计可单向绑定本文。正式结论不是全静态闭合、生产reader/runtime实现、55槽实机通过或新运行授权。
