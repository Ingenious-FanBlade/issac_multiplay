# HomingLaser

由唯一布局源生成。布局编号 24；布局版本 23；原生大小 0x54；未知 0B；完整：是。

原生字节归属：

| 0x0 uintptr_t laser_self【本地】所属Laser的本地自回指；保留工厂构造结果，不作为跨实体引用（4B；已核） | 0x4 f32 origin.x【状态】几何起点：X 分量（4B；已核） | 0x8 f32 origin.y【状态】几何起点：Y 分量（4B；已核） | 0xC f32 end_point.x【状态】几何端点；与Laser公开EndPoint是独立字段：X 分量（4B；已核） | 0x10 f32 end_point.y【状态】几何端点；与Laser公开EndPoint是独立字段：Y 分量（4B；已核） | 0x14 f32 parameter【状态】采样参数原值；不推断未知业务名称（4B；已核） | 0x18 u32 sample_num【状态】原生采样参数计数，不等同当前vector长度（4B；已核） | 0x1C std::vector<Vector> optimized【容器】独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples（12B；已核） | 0x28 std::vector<Vector> unoptimized【容器】独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples（12B；已核） | 0x34 std::vector<i32> indices【容器】独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples（12B；已核） | 0x40 i32 cursor【状态】原生i32槽+0x40的当前原值；旧适配器命名cursor，具体业务含义未核，不强加游标范围（4B；已核） | 0x44 f32 spacing【状态】原生f32槽+0x44的当前原值；旧适配器命名spacing，具体业务含义未核，不推断为物理间距（4B；已核） | 0x48 std::vector<HomingLaserBezier> bezier【容器】独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples（12B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 40—720936B，不含第 9 步记录头）：

块 0：几何标量（world，32—32B）：

| +0x0 Vector origin：几何起点 | +0x8 Vector end_point：几何端点；与Laser公开EndPoint是独立字段 | +0x10 f32 parameter：采样参数原值；不推断未知业务名称 | +0x14 u32 sample_num：原生采样参数计数，不等同当前vector长度 | +0x18 i32 cursor：原生i32槽+0x40的当前原值；旧适配器命名cursor，具体业务含义未核，不强加游标范围 | +0x1C f32 spacing：原生f32槽+0x44的当前原值；旧适配器命名spacing，具体业务含义未核，不推断为物理间距 |
|---|---|---|---|---|---|

块 1：几何容器 optimized（world，2—131074B）：

| +0x0 u16 count + count×Vector（上限16384） optimized：独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples |
|---|

块 2：几何容器 unoptimized（world，2—131074B）：

| +0x0 u16 count + count×Vector（上限16384） unoptimized：独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples |
|---|

块 3：几何容器 indices（world，2—65538B）：

| +0x0 u16 count + count×i32（上限16384） indices：独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples |
|---|

块 4：几何容器 bezier（world，2—393218B）：

| +0x0 u16 count + count×HomingLaserBezier（上限16384） bezier：独立拥有的激光几何容器；有限值/数量/全局池预算校验，候选构造成功后交换，不调用CalculateSamples |
|---|

证据：

- `laser_self`：docs/design/binary-state/tear-pickup-laser-evidence.md
- `origin.x`：docs/design/binary-state/tear-pickup-laser-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `origin.y`：docs/design/binary-state/tear-pickup-laser-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `end_point.x`：docs/design/binary-state/tear-pickup-laser-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `end_point.y`：docs/design/binary-state/tear-pickup-laser-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `parameter`：docs/design/binary-state/tear-pickup-laser-evidence.md
- `sample_num`：docs/design/binary-state/tear-pickup-laser-evidence.md
- `optimized`：docs/design/binary-state/tear-pickup-laser-evidence.md
- `unoptimized`：docs/design/binary-state/tear-pickup-laser-evidence.md
- `indices`：docs/design/binary-state/tear-pickup-laser-evidence.md
- `cursor`：docs/design/binary-state/tear-pickup-laser-evidence.md
- `spacing`：docs/design/binary-state/tear-pickup-laser-evidence.md
- `bezier`：docs/design/binary-state/tear-pickup-laser-evidence.md
