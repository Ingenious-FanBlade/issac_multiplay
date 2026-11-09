# TemporaryEffects

由唯一布局源生成。布局编号 136；布局版本 42；原生大小 0x18；未知 2B；完整：否。

版本39为Player P8提供外层布局：disabled＋完整vector同版本提交；CacheFlag mask／evaluate请求归Host，Player回指本地，原生未知2B及每外部元素4B未知分别保留。不调用Add/RemoveEffect、ClearEffects、EvaluateItems或更新／到期。

原生字节归属：

| 0x0 u32 effect_cache_flags【权威】Host待重算属性CacheFlag mask；不是虚表。Client安装最终属性，不运行EvaluateItems。（4B；已核） | 0x4 std::vector<TemporaryEffect> effect_entries【容器】原顺序完整临时效果列表；配置身份／count／cooldown独立保留，不按id去重或由costume反推。（12B；已核） | 0x10 bool should_evaluate_cache【权威】Host下一次属性重算请求；Client使用最终属性，不写入或消费请求。（1B；已核） | 0x11 bool effects_disabled【状态】90A7A0/90A900等展示查询先检查的禁用开关；独立于容器是否为空，须与内容同目标版本安装。（1B；已核） | 0x12 bytes[2] unknown_12_13【未知】SDK字段间两字节未核；不因对齐归填充。（2B；未知） | 0x14 Entity_Player* effects_player【本地】本地承载Player回指，由外层P8 owner建立与退休，不发送Host地址或额外网络关系。（4B；已核） |
|---|---|---|---|---|---|

状态二进制布局（总计 3—163843B，不含第 9 步记录头）：

块 0：临时效果展示开关（body，1—1B）：

| +0x0 u8 effects_disabled：90A7A0/90A900等展示查询先检查的禁用开关；独立于容器是否为空，须与内容同目标版本安装。 |
|---|

块 1：有序临时效果（body，2—163842B）：

| +0x0 u16 count + count×TemporaryEffect（上限16384） effect_entries：原顺序完整临时效果列表；配置身份／count／cooldown独立保留，不按id去重或由costume反推。 |
|---|

证据：

- `effect_cache_flags`：docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `effect_entries`：docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `should_evaluate_cache`：docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `effects_disabled`：docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `unknown_12_13`：docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `effects_player`：docs/design/binary-state/player-equipment-plus-synchronization-review.md
