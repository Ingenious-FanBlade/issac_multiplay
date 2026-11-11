# RunSeeds

由唯一布局源生成。布局编号 230；布局版本 60；原生大小 0x5C；未知 3B；完整：否。

Game+1BB84内嵌；只发送4B start_seed和1B投影。现有树最多80节点且key在0..79，有界只读遍历、根/count前后稳定；未知key／坏树报Gap。不创建、替换或释放原生树。处理器尚待第10/13步接线。

原生字节归属：

| 0x0 bool custom_run【权威】Host自定义局／进度资格输入；HUD只接收最终进度图标判定，不改访客资格（1B；已核） | 0x1 bytes[3] unknown_1【未知】尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（3B；未知） | 0x4 u32 start_seed【状态】当前run开局种子元数据；暂停菜单纯Seed2String，不调用SetStartSeed／Restart或据此生成世界（4B；已核） | 0x8 RNG generation_rng【权威】开局／下一种子生成流；9C0880推进，不能为显示推进或复制到Client（16B；已核） | 0x18 u32[14] stage_seeds【权威】14个楼层生成种子；9C0980还会推进某槽；Client接收当前房间／配置结果，不重演生成（56B；已核） | 0x50 u32 player_init_seed【权威】初始Player生成来源；正式Player状态与身份另承接，不据此创建或回放（4B；已核） | 0x54 RunSeedEffectNode* effect_tree_head【权威】现有种子效果红黑树头；只读生成display_flags，树地址与节点不发送，不调用增删／构造（4B；已核） | 0x58 u32 effect_tree_count【权威】现有树元素数，读取稳定性／80节点预算使用；不把count当独立状态发送（4B；已核） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 5—5B，不含第 9 步记录头）：

块 0：当前run种子元数据与七项显示选择（world，5—5B）：

| +0x0 u32 start_seed：当前run开局种子元数据；暂停菜单纯Seed2String，不调用SetStartSeed／Restart或据此生成世界 | +0x4 u8（C/S自有元数据，无原生偏移） display_flags：bit0..6依次为SeedEffect 8/9/10/29/30/78/79当前membership；bit7=0。只用于具名Render／只读显示查询，不开放玩法生产器；缺来源报Gap |
|---|---|

证据：

- `custom_run`：Seeds.zhl；9BE290／700AE0／97C300
- `unknown_1`：docs/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `start_seed`：原版Seeds.md:GetStartSeed；9C0880写+4；Game+1BB84
- `generation_rng`：Seeds.zhl；9C0880
- `stage_seeds`：Seeds.zhl；9C0880／9C0980
- `player_init_seed`：Seeds.zhl；9C0880最后一步写+50
- `effect_tree_head`：9BE290／9BE320分配14h节点；452580／662BC0→4240B0读取+54
- `effect_tree_count`：9BE290清零+58；Seeds.zhl；RunSeedEffectNode238
