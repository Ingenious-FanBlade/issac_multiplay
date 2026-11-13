# RunAmbush

由唯一布局源生成。布局编号 231；布局版本 75；原生大小 0x4C；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

Game+25B40。全部已知玩法字节留Host；+21..23仍UNKNOWN。REPENTOGON外部ambushWaves/bossAmbushWaves不伪造为对象字段。

原生字节归属：

| 0x0 std::vector<HostWaveSource> wave_source_list【权威】Host候选波次容器，头／容量／元素归生产器；不为只读预览重抽房间（12B；已核） | 0xC std::vector<HostWaveEntry> shuffled_wave_list【权威】Host波次候选有序表，827D60按12B元素随机交换；不发送列表或指针（12B；已核） | 0x18 i32 max_boss_waves【权威】BossRush最大波次数；其余两种max为REPENTOGON全局变量，不在此字节（4B；SDK声明） | 0x1C i32 bosses_per_wave【权威】每波生成Boss数量，Client不生成波次；不据API可读要求复制（4B；SDK声明） | 0x20 bool active【权威】Host挑战激活锁存；827EB0会关门／播放音乐并初始化两份RNG，Client接收各自最终结果（1B；已核） | 0x21 bytes[3] unknown_21【未知】尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（3B；未知） | 0x24 i32 current_wave【权威】Host波次序号；现无必要的原生显示复制理由，声明Mod确需文字时传最终显示结果（4B；已核） | 0x28 i32 wave_delay【权威】挑战开始后30帧生成等待；不按Client tick推进或据此Spawn（4B；已核） | 0x2C RNG selection_rng【权威】第一份波次生产RNG，来源descriptor SpawnSeed；非绘制随机源（16B；已核） | 0x3C RNG spawn_rng【权威】官方rng字段；GetNextWave复制它后抽RoomConfig，正式路径不额外调用（16B；已核） |
|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `wave_source_list`：6ED4B0在25B40清三指针；827D60／828030
- `shuffled_wave_list`：827D60；6ED4B0在25B4C清三指针
- `max_boss_waves`：Ambush.zhl；LuaAmbush.cpp:GetMaxBossrushWaves／GetMaxChallengeWaves
- `bosses_per_wave`：Ambush.zhl；828650
- `active`：827EB0；7E1550清Game+25B60
- `unknown_21`：docs/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `current_wave`：827EB0清+24；LuaAmbush.cpp:GetCurrentWave
- `wave_delay`：827EB0写+28=30
- `selection_rng`：827EB0写+2C四字
- `spawn_rng`：Ambush.zhl；827EB0；LuaAmbush.cpp:SetupAmbushData
