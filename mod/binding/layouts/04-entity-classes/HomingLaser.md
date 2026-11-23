# HomingLaser

由唯一布局源生成。布局编号 24；布局版本 94；原生大小 0x54；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 uintptr_t laser_self【本地】所属Laser的本地自回指；保留工厂构造结果，不作为跨实体引用（4B；已核） | 0x4 Vector origin【权威】几何起点；版本37展示依赖复核后留Host，不采集、安装或作原值比较；Client使用最终展示状态，不运行采样／更新／半帧模拟（8B；已核） | 0xC Vector end_point【权威】几何端点；与Laser公开EndPoint是独立字段；版本37展示依赖复核后留Host，不采集、安装或作原值比较；Client使用最终展示状态，不运行采样／更新／半帧模拟（8B；已核） | 0x14 f32 parameter【权威】采样参数原值；不推断未知业务名称；版本37展示依赖复核后留Host，不采集、安装或作原值比较；Client使用最终展示状态，不运行采样／更新／半帧模拟（4B；已核） | 0x18 u32 sample_num【权威】原生采样参数计数，不等同当前vector长度；版本37展示依赖复核后留Host，不采集、安装或作原值比较；Client使用最终展示状态，不运行采样／更新／半帧模拟（4B；已核） | 0x1C std::vector<Vector> optimized【容器】独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples（12B；已核） | 0x28 std::vector<Vector> unoptimized【容器】独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples；已核Room::RenderEntityLight VA0x7FC110的Type7分支读取Laser+48C/+490绘制光照，必须保留（12B；已核） | 0x34 std::vector<Vector> indices【权威】历史别名indices；J273 Homing+34为8B双f32点对vector，CalculateSamples按点平滑后复制给optimized；旧i32／4B步长记录错误。Host采样中间缓存，无展示消费者，不传输；Client保留正常构造的本地空vector（12B；已核） | 0x40 i32 cursor【权威】原生i32槽+0x40的当前原值；旧适配器命名cursor，具体业务含义未核，不强加游标范围；版本37展示依赖复核后留Host，不采集、安装或作原值比较；Client使用最终展示状态，不运行采样／更新／半帧模拟（4B；已核） | 0x44 f32 spacing【权威】原生f32槽+0x44的当前原值；旧适配器命名spacing，具体业务含义未核，不推断为物理间距；版本37展示依赖复核后留Host，不采集、安装或作原值比较；Client使用最终展示状态，不运行采样／更新／半帧模拟（4B；已核） | 0x48 std::vector<HomingLaserBezier> bezier【权威】Host路径／采样输入，24B六f32记录的vector；没有展示消费者，不传输。Client保留正常构造的本地空vector，不运行CalculateSamples（12B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 4—262148B，不含第 9 步记录头）：

块 0：几何标量（Host输入，不发送，保留空块号）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 1：几何容器 optimized（world，2—131074B）：

| +0x0 u16 count + count×Vector（上限16384） optimized：独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples |
|---|

块 2：几何容器 unoptimized（world，2—131074B）：

| +0x0 u16 count + count×Vector（上限16384） unoptimized：独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples；已核Room::RenderEntityLight VA0x7FC110的Type7分支读取Laser+48C/+490绘制光照，必须保留 |
|---|

块 3：Host中间点缓存（历史别名indices，不发送，保留空块号）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 4：几何容器 bezier（Host输入，不发送，保留空块号）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `laser_self`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md
- `origin`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `end_point`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `parameter`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `sample_num`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `optimized`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `unoptimized`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/internal-entity-relations-review.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `indices`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `cursor`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `spacing`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
- `bezier`：docs/archive/design/binary-state/tear-pickup-laser-evidence.md；docs/archive/design/binary-state/laser-homing-synchronization-review.md
