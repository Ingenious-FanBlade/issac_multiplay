# tx-01-53：原始射击脚本保留已编码移动量

日期：2026-10-17。tx-01-52 已一次实机确认请求的相反射击优先级（36项专项 PASS，用户画面通过），但新 B 原始射击脚本分支对已 canonical 的移动量重复应用死区，新增实现缺陷使补充整体结果 FAIL。本次只修复该分支，不更改已通过的方向选择器或 A 物理采样。

## 修改前失败判据与唯一验收

失败方式：原始射击 mask 存在时改变 step.move_x/move_y、再次死区、再次归一化/量化；混入其他步移动量；相反射击状态不再遵守后按优先；旧 generation/非法/过期输入更新源历史；原始方向与 canonical 脚本混用产生隐式移动；query 与 Resolve 不同；创建重复、原生/传输错误、Hook 未恢复或失去 pin。

B 脚本其余字段已是合法 V2 canonical：只将四射击方向转换为 canonical aim，将原移动 pair 原样组合，held/edge 保持既有规则；不能把 q15/32767 再送入 raw movement deadzone。A 仍由真实 raw 移动一次 canonical 化。

全部修改与入口完成后，经用户确认新进程隔离 Offline、初始仅 A 场景就绪，执行 `cs-t0/accept-script-movement-e2e.ps1` 一次。沿用操作授权；本次不是重跑 tx-01-52。源码/分析阶段零测试、失败不自动重跑。

场景按 tick 显式给出原始射击 mask 和合法 canonical 移动 pair，覆盖零、低于 raw deadzone 的非零值、8192、正负满幅、合法斜向，与相反键先后/全部释放结合；正常、duplicate、旧 generation 拒绝。验收直接逐字段对应预写 fixture→source→accepted→Resolve→native Movement/Shooting query。A 不重做已通过的四组人工蓄力操作；需要正常画面让 B 脚本完成，随后菜单 teardown、pair 缺席、48 Hook 恢复、DLL 常驻。

产物目录 `artifacts/cs-t0/tx-01-53-live-01`，包含原始场景、清单、构建、events、逐项结果、恢复和进程保留；不计算哈希、不覆盖52历史。

## 已完成实现

B raw 分支不再将已编码移动 pair 除以32767送入 raw canonicalizer；射击单独转换成功后，原样恢复已在初始化验证的 step.move_x/move_y。方向历史和 A 原始移动转换完全保持52实现。场景106步，其中101个原始射击 step、4个原 canonical step 和1个旧 generation 拒绝；对应 source/accepted/Resolve/query 分析入口在验收前准备完成。

## 一次最终验收结果：PASS

用户确认完全重启和新隔离场景准备好后，统一入口执行一次。Rust Release、Native/helper v143 Win32 构建成功；PID30168，creation time134366465017232503，初始窗口前台。171,783 条事件，3,178 Authority tick，nativeError=0、transportFailure=null，重复 Resolve/commit/consumer/accept 与不一致 query 均 false。

设备单次注册 #50、受控创建 #103、精确 B binding #110 和 Joined #111；106步脚本完成 #5807。101/101 原始方向步骤逐字段保留移动 pair 并被 Resolve，4/4 原 canonical 步骤（96–99）原样消费；两条原生路径的 Movement/Shooting 查询、duplicate/旧 generation 拒绝均 PASS。source 例：move_x=1 #468、8192 #1274、(23169,23169) #4410、move_y=-8192 #3066，不再重复死区。专项 result.json 为8/8 PASS，violations=[]。

用户确认主菜单后正常 stop，原生 teardown #171731/#171732、pair 缺席/root=1 #171734、48 槽恢复汇总 #171783；pinned=true，同一游戏进程存活。控制入口退出0，未重跑测试或分析器。

本次完成新脚本移动量修复；后按射击方向及蓄力的画面证据仍对应52，未重复人工测试，也不把本轮结果用于改写52额外失败。跨房自动发射与51未受 owner 接纳 Join 的具体原因仍未解决；完整 T0-05 E2E 未完成。
