# HudMantleEffects

由唯一布局源生成。布局编号 436；布局版本 87；原生大小 0x18；未知 2B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

版本70的条件历史输入视图，不是第二份常规P8或holy_mantle_visible缓存。当前P8或已提交自有快照能满足435修订时复用，不额外发送436；仅冷连接/恢复/冻结时真实缺该修订才随原body/joint目标补完整快照。完整有序配置身份/count/cooldown沿180/150规则，不能把count>0当存在性。原生字节归属引用PlayerTemporaryEffects，不新建原生效果对象。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：身体HUD最近允许护盾查询的历史P8来源 | subject：body key/use/shape与HUD实例代次 | producer：正常护盾查询资格窗口封存的P8完整disabled与有序条目 | adapter：hud_mantle_effects_input | dependencies：仅在435需要的修订不能由同目标或已保留P8满足时发送；与435同身体/use/shape/HUD实例，produced_revision必须等于mantle_source_revision；全部配置资源精确就绪 | dependency_types：EntityPlayer | sequence：body | atomic_scope：body-target | validity：同scope、身体use/shape、HUD实例、绑定及完整输入 | invalidation：切房/恢复/换形/绑定退休按scope清理，真实writer/draw借用退出后回收 | contribution_identity：身体/HUD实例/use/shape；沿原world/body目标确认 | source_gap：冷连接/恢复缺准确历史P8时明确缺依赖，不拿当前P8或默认无护盾代替 | envelope：{'mode': 'group', 'subject_type': 'EntityPlayer', 'cacheable': True, 'clock': False, 'topology': True, 'paired_streams': 0, 'passes': ['Context', 'Value'], 'subject_field': 'player'} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤164000B；body组≤65536B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 0x0 u32 cache_mask【权威】待属性重算mask，不是vptr（4B；已核） | 0x4 vector<TemporaryEffect> entries【容器】完整有界有序身份／count；仅704 cooldown（12B；已核） | 0x10 bool evaluate【权威】Host重算请求（1B；已核） | 0x11 bool disabled【状态】效果查询禁用，独立于列表空值（1B；已核） | 0x12 bytes[2] unknown_12_13【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（2B；未知） | 0x14 Player* owner【本地】本地承载Player回指（4B；已核） |
|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：护盾冻结所需的准确历史P8输入（body，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `cache_mask`：5-player-p6-p8.md
- `entries`：5-player-p6-p8.md
- `evaluate`：5-player-p6-p8.md
- `disabled`：5-player-p6-p8.md
- `unknown_12_13`：5-player-p6-p8.md
- `owner`：5-player-p6-p8.md
