# FamiliarRandomAlphaResult

由唯一布局源生成。布局编号 358；布局版本 71；原生大小 0x0；未知 0B；完整：是。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：具名实体随机显示贡献 | subject：entity | producer：61E604栈ColorMod.alpha产生后、61E60D压栈前 | adapter：familiar_random_alpha_result_borrow | dependencies：对应实体key/use/shape、必要body／world状态与资源修订、pass／分支／递归贡献；重复展示复用当前不可变结果。 | dependency_types：EntityFamiliar, PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：正常来源缓存与运行adapter尚待第10／13步接线。 | envelope：{'mode': 'group', 'subject_type': 'EntityFamiliar', 'cacheable': False, 'clock': True, 'topology': False, 'paired_streams': 0, 'passes': ['Context', 'RandomPrimary', 'RandomSecondary', 'RandomRecursive'], 'subject_field': ''} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤5B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—5B，不含第 9 步记录头）：

块 0：FamiliarRandomAlphaResult具名当前结果（world，1—5B）：

| +0x0 u8（C/S自有元数据，无原生偏移） state：0=Host明确无该随机覆盖；1=本次已产生alpha。未产生／缺源没有合法记录。 | +0x1 f32（C/S自有元数据，无原生偏移）（仅familiar_random_alpha_result_state_state__1(state)成立时存在） alpha：本次正常产生的最终alpha，不执行随机／owner资格getter。 |
|---|---|

证据：


