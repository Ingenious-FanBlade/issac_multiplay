# NightmareSceneResult

由唯一布局源生成。布局编号 375；布局版本 58；原生大小 0x0；未知 0B；完整：否。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：Nightmare显示代次 | subject：world／具体显示实例 | producer：8FAC20正常Connector／Levels／ClearFloor／slot／程序道具与辅助Sprite提交 | adapter：nightmare_scene_result_fixed_adapter | dependencies：同world结构／显示代次、成员／资源及有序贡献；不重复发送route／mode／Scene帧／Dogma／生产树。 | dependency_types：PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：各用途完整来源覆盖、逻辑坐标及资源内容闭包仍待第10／13步落实；现有原生UNKNOWN保留。 |
|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤8388608B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—5001219B，不含第 9 步记录头）：

块 0：NightmareSceneResult具名当前结果（world，1—5001219B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：Host已确认该共享显示实例有效；0不读旧payload，来源未闭合不是0。 | +0x1 u16 count + count×PresentationSpriteContribution（上限512）（C/S自有元数据，无原生偏移）（仅nightmare_scene_result_present_present__1(present)成立时存在） sprites：正常有序最终Sprite贡献；逐layer／track／分支完整。 | 顺序变长偏移 u16 count + count×PresentationImageContribution（上限4096）（C/S自有元数据，无原生偏移）（仅nightmare_scene_result_present_present__1(present)成立时存在） images：正常有序图像／遮罩／道具贡献及实际内容。 |
|---|---|---|

证据：


