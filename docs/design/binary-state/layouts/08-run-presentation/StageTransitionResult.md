# StageTransitionResult

由唯一布局源生成。布局编号 377；布局版本 60；原生大小 0x0；未知 0B；完整：是。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：换层遮罩 | subject：world／显示代次 | producer：909E60正常最终遮罩 | adapter：stage_transition_fixed_adapter | dependencies：同world结构／D0及必要资源；ascent复用GameRun。 | dependency_types：GameRun, PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：正常来源缓存与运行adapter尚待第10／13步接线。 |
|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤512B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 2—306B，不含第 9 步记录头）：

块 0：StageTransitionResult具名当前结果（world，2—306B）：

| +0x0 u16 count + count×ScreenFadeContribution（上限16）（C/S自有元数据，无原生偏移） masks：909E60最终颜色／alpha／顺序，不发送mode／计时／render_kind。 |
|---|

证据：


