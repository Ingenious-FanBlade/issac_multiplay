# GridFireEmber

由唯一布局源生成。布局编号 205；布局版本 58；原生大小 0x18；未知 0B；完整：是。

版本51：原生24B逐字节归属；状态16B（位置8+age4+maxLife4），速度8B留Host。正常ordinary更新尾部要求0<=age<maxLife<=29，生产器会删除maxLife=0或已到期元素；跨字段条件由容器具名owner检查，不改写/截断。不是Client本地粒子模拟器，禁止积分、随机扰动或计时删除。

原生字节归属：

| 0x0 f32 position.x【状态】相对Grid渲染位置的当前粒子偏移；Render加到格子屏幕坐标，不发送Host屏幕位置：X 分量（4B；已核） | 0x4 f32 position.y【状态】相对Grid渲染位置的当前粒子偏移；Render加到格子屏幕坐标，不发送Host屏幕位置：Y 分量（4B；已核） | 0x8 f32 velocity.x【权威】Update积分和随机扰动输入，Render不读，不发到Client，不据其外推位置：X 分量（4B；已核） | 0xC f32 velocity.y【权威】Update积分和随机扰动输入，Render不读，不发到Client，不据其外推位置：Y 分量（4B；已核） | 0x10 i32 age【状态】当前粒子年龄；Render以float(age)/float(max_life)*AnimationData.frameCount截断整帧，不能因名称为计数器就删除（4B；已核） | 0x14 i32 max_life【状态】Render帧计算的真实除数；正常更新尾部为1..29，必须与age同版且age<max_life，不发到期帧替代原值（4B；已核） |
|---|---|---|---|---|---|

状态二进制布局（总计 16—16B，不含第 9 步记录头）：

块 0：当前粒子位置与绘制寿命输入（world，16—16B）：

| +0x0 Vector position：相对Grid渲染位置的当前粒子偏移；Render加到格子屏幕坐标，不发送Host屏幕位置 | +0x8 i32 age：当前粒子年龄；Render以float(age)/float(max_life)*AnimationData.frameCount截断整帧，不能因名称为计数器就删除 | +0xC i32 max_life：Render帧计算的真实除数；正常更新尾部为1..29，必须与age同版且age<max_life，不发到期帧替代原值 |
|---|---|---|

证据：

- `position.x`：docs/design/binary-state/grid-eight-subclasses-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `position.y`：docs/design/binary-state/grid-eight-subclasses-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `velocity.x`：docs/design/binary-state/grid-eight-subclasses-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `velocity.y`：docs/design/binary-state/grid-eight-subclasses-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `age`：docs/design/binary-state/grid-eight-subclasses-evidence.md
- `max_life`：docs/design/binary-state/grid-eight-subclasses-evidence.md
