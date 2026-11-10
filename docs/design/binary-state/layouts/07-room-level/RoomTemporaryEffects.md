# RoomTemporaryEffects

由唯一布局源生成。布局编号 256；布局版本 58；原生大小 0x18；未知 2B；完整：否。

与Player的同名对象不同实例：父用途限定为Host，未知仍按物理字节列出。 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 u32 cache_mask【权威】效果重算控制；同步决定：留Host（4B；推断） | 0x4 vector<TemporaryEffect> effects【权威】效果生产列表；同步决定：Room列表留Host；仅实际显示条件有具名投影（12B；已核） | 0x10 bool[2] evaluate_disabled【权威】更新／禁用条件；同步决定：不发原值；最终实际显示资格由正常Host分支取结果（2B；已核） | 0x12 bytes[2] unknown_12_13【未知】本轮未解释；包括未核字段／可能对齐，不能称填充；同步决定：不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充（2B；未知） | 0x14 Entity_Player* player【本地】实例回指，不是网络玩家身份；同步决定：不发地址（4B；已核） |
|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `cache_mask`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/TemporaryEffects.zhl；804500/7F7F60；布局136；原账本标签：既有布局136
- `effects`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/TemporaryEffects.zhl；804500/7F7F60；布局136
- `evaluate_disabled`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/TemporaryEffects.zhl；804500/7F7F60；布局136
- `unknown_12_13`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/TemporaryEffects.zhl；804500/7F7F60；布局136；本轮补读：SDK bool+10/+11与ptr+14；已有布局136不解释+12..13，本轮不改其证据等级
- `player`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/TemporaryEffects.zhl；804500/7F7F60；布局136
