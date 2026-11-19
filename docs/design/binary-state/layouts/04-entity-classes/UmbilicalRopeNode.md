# UmbilicalRopeNode

由唯一布局源生成。布局编号 132；布局版本 87；原生大小 0x78；未知 108B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

J273 430C30每节点分配／清零0x78，初建16个；4308D0只读节点前3个f32作绘制，z在反射时变号。+0C—77的108B仍未知，不能整块复制或在Client运行绳索模拟。节点按deque逻辑顺序编码，不发送地址／容量／物理环偏移。

原生字节归属：

| 0x0 f32 x【状态】最终绳索节点X坐标，保留三维分量（4B；已核） | 0x4 f32 y【状态】最终绳索节点Y坐标，保留三维分量（4B；已核） | 0x8 f32 z【状态】最终绳索节点Z坐标，保留三维分量（4B；已核） | 0xC bytes[108] simulation_unknown【未知】模拟临时／历史等未逐字段解释；只确认节点总大小，不复制，不当填充（108B；未知） |
|---|---|---|---|

状态二进制布局（总计 12—12B，不含第 9 步记录头）：

块 0：绳索最终节点坐标（world，12—12B）：

| +0x0 f32 x：最终绳索节点X坐标，保留三维分量 | +0x4 f32 y：最终绳索节点Y坐标，保留三维分量 | +0x8 f32 z：最终绳索节点Z坐标，保留三维分量 |
|---|---|---|

证据：

- `x`：J273 4308D0按逻辑节点读取+0/+4/+8；430C30分配0x78；docs/design/binary-state/familiar-synchronization-review.md
- `y`：J273 4308D0按逻辑节点读取+0/+4/+8；430C30分配0x78；docs/design/binary-state/familiar-synchronization-review.md
- `z`：J273 4308D0按逻辑节点读取+0/+4/+8；430C30分配0x78；docs/design/binary-state/familiar-synchronization-review.md
- `simulation_unknown`：J273 430C30分配及memset0x78；42D6F0访问节点后续多组分量；docs/design/binary-state/familiar-synchronization-review.md
