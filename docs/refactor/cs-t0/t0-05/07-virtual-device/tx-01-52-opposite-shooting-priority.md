# tx-01-52：相反射击键采用后按方向

日期：2026-10-17。用户明确确认原版行为：保持左射击再按右射击，应以右方向为准，而非相消；要求据此修复。另已采纳无法区分同次采样先后的兜底：保留此前仍按住的优先方向，没有历史时固定右／下。

## 目标、范围与修改前失败判据

在源输入转换为 canonical V2 前保存四个射击方向的按键历史。每条轴上后按方向胜出；释放胜出键时回到仍按住的反方向；只在所有射击方向均释放时结束连续射击。相邻方向保持现有斜向语义。移动仍相减，手柄非冲突轴仍保留幅度。A 真实 primitive projection 与 B 原始方向脚本使用同一转换代码、各自历史；Resolve/query 不推进方向历史。

保持 InputStateDraftV2/InputCmdDraftV2 的 24/64 字节编码、version=2、正常 lead=2、已采纳启动例外、独立 A/B scheduler 状态、48 Hook owner 与恢复/DLL 常驻语义。扩展隔离 coop-active IPC 的脚本源信息并升级其版本，旧 canonical 脚本继续有效，原版/Isaac Sync/旧 V1 接口不变。

FAIL 条件（先于实现登记）：

- 左→左右仍输出左、零向量或 Shoot held=0；右→左右仍输出右；上→上下仍输出上；下→上下仍输出下。
- 保持双键时在无新按下的情况下交替方向；重复 poll/query 改变优先级；释放非胜出键改变方向；释放胜出键不回退剩余键；全部释放后仍残留方向。
- 相邻重叠被改成单轴、移动相反键语义被改变、手柄非冲突幅度被改变。
- A/B 共享方向历史；A 同物理设备因 room/pause generation 重置方向历史；设备/control/K 真正变化后沿用旧优先级；B 旧 generation/过期/非法脚本更新新方向历史。
- 同次采样未知先后未遵守用户兜底；方向转换绕开 codec/queue/Resolve；旧 edge 恢复或 startup continuation 重复 edge。
- 实际查询与已 Resolve canonical 不同，原生/传输错误、Hook 未恢复或 DLL pin 丢失。

未观察场景记 NOT_OBSERVED；输入/查询证据不能代替蓄力画面观察。跨房中性窗口导致的自动发射、tx-01-51 未受 owner 接纳的 Join 分支仍未解决，不能据本次修复宣称这些问题通过。

## 唯一一次最终 E2E 与产物

实现、报告入口和清单全部准备完毕后，确认新进程隔离 Offline、初始仅 A（阿萨泻勒 ID7／键盘）就绪；沿用已有操作授权。只通过 `cs-t0/accept-shooting-priority-e2e.ps1` 执行一次统一构建和真实游戏 E2E。分析和编辑期间不测试、不试跑、不编译；失败不自动重跑。

同一窗口内依次执行 A 左→左右→左→全松、右→左右→右→全松、上→上下→上→全松、下→上下→下→全松；每段双键维持约 2 秒，并重复按下/释放后按键以确认持续蓄力。相邻重叠与移动同时观察。B 原始方向脚本覆盖同样四个序列、全部释放、首次同采样双键兜底和相邻键；若 B Join 未被接纳则明确未观察，不追加创建或切换路线。查询反复读取保持纯读。完成后通过菜单 teardown、正常 stop，核对 pair 缺席、48 槽原字节恢复与 DLL 常驻。

产物：场景 JSON、操作清单、构建日志、source 方向历史与胜出方向事件、物理观察、Submit/Resolve/query/lifecycle 原始事件、独立人工画面观察、逐项结果、恢复和进程保留记录。目录使用 `artifacts/cs-t0/tx-01-52-live-01`；不计算哈希，不覆盖历史失败记录。

## 当前状态

已实现 `ShootingDirections` 和 `CanonicalizeShooting`：逐轴选择新按下键、保持胜出方向、释放回退与右／下首次同采样兜底。历史只在源转换成功后提交；A 的历史随实际 device/control/K，而非 gameplay generation；B 新 generation/显式新脚本/从 canonical 源切回原始方向时建立新源历史。过期 raw step 走原有过期拒绝，不更新源历史；fault raw 脚本只接受正常/duplicate，旧 canonical fault 行为不变。

物理采样和 B 原始方向脚本共享上述函数。IPC ScriptStep 的原预留字携带显式存在标志和四方向 mask；隔离 IPC 升级 v2 防止旧 Native/controller 混用，布局和事件大小保持。新增 ShootingSource=24 记录 raw 四方向幅度、held/selected、上一源状态、重置标记及脚本索引，独立于 canonical 命令 sequence。旧 V1、24/64 字节 V2、48 Hook、lead/启动例外及生命周期处理未变。

场景包含 234 步，按修改前登记的四个方向先后、释放胜出/非胜出、同采样兜底、相邻方向、duplicate 和旧 generation 拒绝；`report-shooting-priority-e2e.py` 将预写场景与同一次真实游戏中的 source→Submit→Resolve→query 配对，并读取独立画面观察。

## 一次最终验收结果

用户确认新隔离场景就绪后，统一入口执行一次；Rust Release、Native/helper v143 Win32 构建成功。PID39220，creation time134366457901640571。513,656 条事件、9,361 Authority tick，nativeError=0、transportFailure=null，所有重复/查询一致性布尔异常为 false。

本轮受控创建 #103、B binding #110、Joined #111 均观察到；B 脚本 27 个预写 segment 全部实际转换并 Resolve。A 四种先后顺序 source 证据 #164912/#192618/#270318/#291814；B 对应 #554/#2618/#4746/#6874；首次同采样右下兜底 #10682。用户分别回复左右“表现全部正常”、上下与移动相邻重叠“表现全部正确”，画面方向及持续蓄力均 PASS。自动专项 result.json 为 36/36 PASS，无方向违规。

额外失败：新增 B 原始射击脚本路径把已 canonical 的 move_x=8192 重建为 raw 后再次经过径向死区，#554 输出4681。A 物理移动及原有 canonical 脚本不走该新增分支。专项分析器只检查方向，因此保留其 PASS，不重跑改判；additional-input-observations.json 如实登记此失败，operator-result.json 补充最终 FAIL。这是实际新增实现缺陷，不是扩大验收或重复同一目标的理由；后续实质修复单独记录。

用户主菜单确认后正常 stop，原生 teardown #513604/#513605、pair 缺席且 root 常驻 #513607、48 槽恢复汇总 #513656；同一游戏进程保留，DLL pinned。没有自动重跑或额外分析器执行。跨房自动发射和 tx-01-51 未接纳 Join 的原因仍未解决，不能据本轮普通 Join 成功覆盖历史失败。
