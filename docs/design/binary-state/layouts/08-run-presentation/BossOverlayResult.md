# BossOverlayResult

由唯一布局源生成。布局编号 379；布局版本 71；原生大小 0x0；未知 0B；完整：否。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：BossOverlay显示实例 | subject：world／显示代次 | producer：41CC80正常消费；41CD30正常选择已完成 | adapter：boss_overlay_fixed_adapter | dependencies：同world显示代次／D0与资源；不发五定义／persist／selected_entry。 | dependency_types：PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：正常来源缓存与运行adapter尚待第10／13步接线。 | envelope：{'mode': 'group', 'subject_type': 'world', 'cacheable': True, 'clock': True, 'topology': False, 'paired_streams': 0, 'passes': ['Context', 'Value'], 'subject_field': ''} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤1048576B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—1048576B，不含第 9 步记录头）：

块 0：BossOverlayResult具名当前结果（world，1—8912908B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：41CC80正常已确认可绘资格。 | +0x1 ANM2（见该类型状态表）（伴随源，无根对象偏移）（仅boss_overlay_result_present_present__1(present)成立时存在） sprite：当前实际Sprite姿态；480×270比例本地派生。 |
|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `sprite` → `ANM2`：当前实际Sprite姿态；480×270比例本地派生。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
