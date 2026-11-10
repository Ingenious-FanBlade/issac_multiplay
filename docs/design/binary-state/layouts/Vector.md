# Vector

由唯一布局源生成。布局编号 1；布局版本 56；原生大小 0x8；未知 0B；完整：是。

J273 SDK 所列 Vector 为两个连续 f32；角度公开成员可由两分量派生。

原生字节归属：

| 0x0 f32 x【状态】X 分量（4B；已核） | 0x4 f32 y【状态】Y 分量（4B；已核） |
|---|---|

状态二进制布局（总计 8—8B，不含第 9 步记录头）：

块 0：二维向量（body，8—8B）：

| +0x0 f32 x：X 分量 | +0x4 f32 y：Y 分量 |
|---|---|

证据：

- `x`：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `y`：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
