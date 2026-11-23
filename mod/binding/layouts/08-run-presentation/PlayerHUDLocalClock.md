# PlayerHUDLocalClock

由唯一布局源生成。布局编号 435；布局版本 94；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

八类本地HUD共用正式world/body提交，无第三确认序列。435变化不要求重发315；缺历史P8、更新来源或冷连接相位不得确认。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：实际身体HUD实例的纯显示驱动 | subject：body key/use/shape与HUD实例代次 | producer：正常832C10完成及92F610实际返回；护盾输入取允许查询的P8修订 | adapter：hud_local_prepare | dependencies：同目标P3/P4、精确P8效果来源修订、组绑定与必要配置/图像资源；drop锚点不大于update_ordinal | dependency_types：EntityPlayer, PlayerControlGroup | sequence：body | atomic_scope：body-target | validity：同scope、身体use/shape、HUD实例、绑定及完整输入 | invalidation：切房/恢复/换形/绑定退休按scope清理，真实writer/draw借用退出后回收 | contribution_identity：身体/HUD实例/use/shape；沿原world/body目标确认 | source_gap：缺实际消费、更新资格、精确P8或相位时失败，不猜默认值 | envelope：{'mode': 'group', 'subject_type': 'EntityPlayer', 'cacheable': True, 'clock': False, 'topology': True, 'paired_streams': 0, 'passes': ['Context', 'Value'], 'subject_field': 'player'} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤41B；body组≤41B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：身体HUD有效更新时钟与有限驱动（body，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：


