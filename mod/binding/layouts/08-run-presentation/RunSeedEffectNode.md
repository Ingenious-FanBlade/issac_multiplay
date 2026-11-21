# RunSeedEffectNode

由唯一布局源生成。布局编号 238；布局版本 90；原生大小 0x14；未知 2B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

D5：只为RunSeeds容器提供id元素布局；左／父／右地址、颜色与+E..F未知不上线。双端具名owner负责有界遍历和原生Seeds树安装，不能只挂getter替代音乐的内联树读取。

原生字节归属：

| 0x0 RunSeedEffectNode* left【本地】红黑树左链接，只属原生容器owner，不发送地址（4B；已核） | 0x4 RunSeedEffectNode* parent【本地】红黑树父链接／sentinel根，只属本进程（4B；已核） | 0x8 RunSeedEffectNode* right【本地】红黑树右链接，不发送／跨进程保留（4B；已核） | 0xC u8 color【本地】树平衡颜色；不是SeedEffect值或绘制颜色（1B；SDK声明） | 0xD u8 is_nil【本地】原生树nil/sentinel标记；只用于有界读取合法性（1B；已核） | 0xE bytes[2] unknown_e【未知】尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（2B；未知） | 0x10 i32 key【状态】当前SeedEffect的完整id；只读原有节点，线上为有界有序唯一id列表（4B；已核） |
|---|---|---|---|---|---|---|

状态二进制布局（总计 4—4B，不含第 9 步记录头）：

块 0：种子效果节点id（不传树链接）（world，4—4B）：

| +0x0 i32 key：当前SeedEffect的完整id；只读原有节点，线上为有界有序唯一id列表 |
|---|

证据：

- `left`：9BE290哨兵自环；4240B0查找
- `parent`：9BE290／9BE320；4240B0
- `right`：9BE290／9BE320；4240B0
- `color`：9BE290写word+C=0101h；MSVC树ABI
- `is_nil`：452580／662BC0检查+0D
- `unknown_e`：docs/archive/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `key`：452580／662BC0检查+10；原版SeedEffect.md枚举0..79；docs/archive/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）
