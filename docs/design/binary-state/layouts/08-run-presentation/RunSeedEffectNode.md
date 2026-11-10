# RunSeedEffectNode

由唯一布局源生成。布局编号 238；布局版本 57；原生大小 0x14；未知 2B；完整：否。

只记录外部节点ABI，不作为状态记录发送或在Client重建树；+0E..0F仍未知，不能因14B分配假定padding。

原生字节归属：

| 0x0 RunSeedEffectNode* left【本地】红黑树左链接，只属原生容器owner，不发送地址（4B；已核） | 0x4 RunSeedEffectNode* parent【本地】红黑树父链接／sentinel根，只属本进程（4B；已核） | 0x8 RunSeedEffectNode* right【本地】红黑树右链接，不发送／跨进程保留（4B；已核） | 0xC u8 color【本地】树平衡颜色；不是SeedEffect值或绘制颜色（1B；SDK声明） | 0xD u8 is_nil【本地】原生树nil/sentinel标记；只用于有界读取合法性（1B；已核） | 0xE bytes[2] unknown_e【未知】尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（2B；未知） | 0x10 i32 key【权威】完整SeedEffect源key留Host；线上仅RunSeeds的七项显示membership投影（4B；已核） |
|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `left`：9BE290哨兵自环；4240B0查找
- `parent`：9BE290／9BE320；4240B0
- `right`：9BE290／9BE320；4240B0
- `color`：9BE290写word+C=0101h；MSVC树ABI
- `is_nil`：452580／662BC0检查+0D
- `unknown_e`：docs/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `key`：452580／662BC0检查+10；原版SeedEffect.md枚举0..79
