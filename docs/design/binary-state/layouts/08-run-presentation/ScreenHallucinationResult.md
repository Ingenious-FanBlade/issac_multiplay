# ScreenHallucinationResult

由唯一布局源生成。布局编号 370；布局版本 60；原生大小 0x0；未知 0B；完整：是。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：屏幕hallucination | subject：world／固定pass | producer：6F89DE—6F8C2A正常消费 | adapter：screen_hallucination_fixed_adapter | dependencies：D0、同world实际图像资源与固定SourceQuad规则。 | dependency_types：PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：前画面纯world内容／坐标闭包及GPU物化仍待接线，不采个人菜单／Online画面。 |
|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤7B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—7B，不含第 9 步记录头）：

块 0：ScreenHallucinationResult具名当前结果（world，1—7B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：当前world hallucination pass有效性；缺图像不改隐藏。 | +0x1 f32（C/S自有元数据，无原生偏移）（仅screen_hallucination_result_present_present__1(present)成立时存在） amount：6F89DE—6F8A33当前最终强度；ManagerFrame%4反馈选择复用D0。 | 顺序变长偏移 u16（C/S自有元数据，无原生偏移）（仅screen_hallucination_result_present_present__1(present)成立时存在） image：Game+25A98正常已完成前画面／同world完整recipe；来源内容闭包未成立时报Gap。 |
|---|---|---|

证据：


