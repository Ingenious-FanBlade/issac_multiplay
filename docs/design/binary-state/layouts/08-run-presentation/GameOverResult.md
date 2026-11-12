# GameOverResult

由唯一布局源生成。布局编号 376；布局版本 71；原生大小 0x0；未知 0B；完整：否。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：共享死亡显示 | subject：world／具体显示实例 | producer：970C10正常最终死亡subject／有序项目／击杀者资源 | adapter：game_over_result_fixed_adapter | dependencies：同world结构／显示代次、成员／资源及有序贡献；不重复发送route／mode／Scene帧／Dogma／生产树。 | dependency_types：PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：各用途完整来源覆盖、逻辑坐标及资源内容闭包仍待第10／13步落实；现有原生UNKNOWN保留。 | envelope：{'mode': 'group', 'subject_type': 'world', 'cacheable': False, 'clock': True, 'topology': False, 'paired_streams': 0, 'passes': ['Context', 'Value', 'Sprite', 'Image'], 'subject_field': ''} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤8388608B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—5066759B，不含第 9 步记录头）：

块 0：GameOverResult具名当前结果（world，1—5066759B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：Host已确认该共享显示实例有效；0不读旧payload，来源未闭合不是0。 | +0x1 u16（C/S自有元数据，无原生偏移）（仅game_over_result_present_present__1(present)成立时存在） body：Host最终死亡显示subject；不从Client本地Online或roster第0项选择。 | 顺序变长偏移 u16（C/S自有元数据，无原生偏移）（仅game_over_result_present_present__1(present)成立时存在） killer_config：实际可空击杀者配置资源；场景伤害明确无配置与缺源分别处理。 | 顺序变长偏移 u16 count + count×PresentationSpriteContribution（上限512）（C/S自有元数据，无原生偏移）（仅game_over_result_present_present__1(present)成立时存在） sprites：正常有序最终Sprite贡献；逐layer／track／分支完整。 | 顺序变长偏移 u16 count + count×PresentationImageContribution（上限4096）（C/S自有元数据，无原生偏移）（仅game_over_result_present_present__1(present)成立时存在） images：正常有序图像／遮罩／道具贡献及实际内容。 |
|---|---|---|---|---|

证据：


