# EffectSpriteTrailData

由唯一布局源生成。布局编号 27；布局版本 87；原生大小 0x18；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 版本40：节点64B布局沿用版本38；逻辑数量上限127，完整块为u16 count及0—127个完整节点，最大8130B；正常空payload不同于不存在。

原生字节归属：

| 0x0 u32 trail_vtable【本地】SpriteTrailData具体虚表（4B；已核） | 0x4 std::deque<EffectTrailNode> trail_nodes【容器】指针map承载的有序deque；每节点由606190独立分配64B。只按逻辑起点／数量遍历，不把节点当连续数组或传输proxy／map／capacity／offset。 版本40：5DA480—5DA50D在正常Update尾部裁至count<128；完整采集窗只接受0—127，不截短、不抽样。原生Update中途的128不是合法采集状态。（20B；已核） |
|---|---|

状态二进制布局（总计 2—8130B，不含第 9 步记录头）：

块 0：轨迹头（仅根投影的present）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 1：轨迹节点（world，2—8130B）：

| +0x0 u16 count + count×EffectTrailNode（上限127） trail_nodes：指针map承载的有序deque；每节点由606190独立分配64B。只按逻辑起点／数量遍历，不把节点当连续数组或传输proxy／map／capacity／offset。 版本40：5DA480—5DA50D在正常Update尾部裁至count<128；完整采集窗只接受0—127，不截短、不抽样。原生Update中途的128不是合法采集状态。 |
|---|

证据：

- `trail_vtable`：docs/design/binary-state/slot-projectile-effect-evidence.md
- `trail_nodes`：docs/design/binary-state/slot-projectile-effect-evidence.md；docs/design/binary-state/effect-synchronization-review.md；docs/design/binary-state/effect-next-five-evidence.md
