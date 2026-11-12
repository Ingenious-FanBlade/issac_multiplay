# PlayerHUDLocalClock

由唯一布局源生成。布局编号 435；布局版本 71；原生大小 0x0；未知 0B；完整：是。

八类本地HUD共用正式world/body提交，无第三确认序列。435变化不要求重发315；缺历史P8、更新来源或冷连接相位不得确认。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：实际身体HUD实例的纯显示驱动 | subject：body key/use/shape与HUD实例代次 | producer：正常832C10完成及92F610实际返回；护盾输入取允许查询的P8修订 | adapter：hud_local_prepare | dependencies：同目标P3/P4、精确P8效果来源修订、组绑定与必要配置/图像资源；drop锚点不大于update_ordinal | dependency_types：EntityPlayer, PlayerControlGroup | sequence：body | atomic_scope：body-target | validity：同scope、身体use/shape、HUD实例、绑定及完整输入 | invalidation：切房/恢复/换形/绑定退休按scope清理，真实writer/draw借用退出后回收 | contribution_identity：身体/HUD实例/use/shape；沿原world/body目标确认 | source_gap：缺实际消费、更新资格、精确P8或相位时失败，不猜默认值 | envelope：{'mode': 'group', 'subject_type': 'EntityPlayer', 'cacheable': True, 'clock': False, 'topology': True, 'paired_streams': 0, 'passes': ['Context', 'Value'], 'subject_field': 'player'} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤41B；body组≤41B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 41—41B，不含第 9 步记录头）：

块 0：身体HUD有效更新时钟与有限驱动（body，41—41B）：

| +0x0 u16（C/S自有元数据，无原生偏移） player：真实身体引用 | +0x2 u64（C/S自有元数据，无原生偏移） binding_revision：同控制组绑定代次 | +0xA u64（C/S自有元数据，无原生偏移） update_ordinal：该HUD实例已正常完成的有效显示更新次数；非Authority tick/渲染帧/ACK | +0x12 u32（C/S自有元数据，无原生偏移） sample_game_frame：最后一次有效HUD更新的真实GameFrame；冻结期间不随到包推进 | +0x16 u8（C/S自有元数据，无原生偏移） critical_health：同次HUD生产的red+eternal+soul+bone原生32位和是否等于1 | +0x17 u64（C/S自有元数据，无原生偏移） mantle_source_revision：最近允许护盾查询时封存的P8效果来源修订；冻结/冷连接必须取得该修订的完整纯值输入 | +0x1F u64（C/S自有元数据，无原生偏移） drop_anchor_update：最近实际消费Drop状态变化或恢复时的已完成HUD更新锚点 | +0x27 u8（C/S自有元数据，无原生偏移） drop_held：该身体在832C10尾部92F610实际消费后的Drop持续状态 | +0x28 u8（C/S自有元数据，无原生偏移） drop_anchor_phase：锚点更新完成后的有限0..4相位；只用于初建/变化/纠正，不是逐帧浮点结果 |
|---|---|---|---|---|---|---|---|---|

证据：


