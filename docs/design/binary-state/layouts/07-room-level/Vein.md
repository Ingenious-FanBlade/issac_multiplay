# Vein

由唯一布局源生成。布局编号 248；布局版本 71；原生大小 0x34；未知 7B；完整：否。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 i32 kind【状态】实际两Sprite选择；同步决定：发送，拒绝超已核0/1范围（4B；已核） | 0x4 Vein*[3] children【换算】三个有序child；同步决定：有界自有图索引／Null；不传地址；循环／深度／预算失败不能截断；正式投影：三个child转本有界图节点索引，FF明确空；拒绝循环／无owner／超限（12B；已核） | 0x10 bytes[4] unknown_10_13【未知】本轮未解释；包括未核字段／可能对齐，不能称填充；同步决定：不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充（4B；未知） | 0x14 f32[4] rotation_scale_factor_opacity【状态】节点最终绘制变换与透明度；同步决定：发送；Sprite对应字段由本节点覆写，不重复发（16B；已核） | 0x24 i32 depth【状态】alpha计算实际消费；同步决定：发送；不把它当递归安全深度替代结构检查（4B；已核） | 0x28 f32 position.x【状态】节点绘制位置；同步决定：发送：X 分量（4B；已核） | 0x2C f32 position.y【状态】节点绘制位置；同步决定：发送：Y 分量（4B；已核） | 0x30 bool retire_or_shrink【权威】树生长／收缩控制字节；继承到child，影响后续factor和删除；同步决定：不发；当前factor／opacity／图拓扑已有实际显示后态（1B；已核） | 0x31 bytes[3] unknown_31_33【未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；同步决定：不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充（3B；未知） |
|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 35—35B，不含第 9 步记录头）：

块 0：Vein当前结果（world，35—35B）：

| +0x0 i32 kind：实际两Sprite选择；同步决定：发送，拒绝超已核0/1范围 | +0x4 u8[3] children：三个有序child；同步决定：有界自有图索引／Null；不传地址；循环／深度／预算失败不能截断；正式投影：三个child转本有界图节点索引，FF明确空；拒绝循环／无owner／超限 | +0x7 f32[4] rotation_scale_factor_opacity：节点最终绘制变换与透明度；同步决定：发送；Sprite对应字段由本节点覆写，不重复发 | +0x17 i32 depth：alpha计算实际消费；同步决定：发送；不把它当递归安全深度替代结构检查 | +0x1B Vector position：节点绘制位置；同步决定：发送 |
|---|---|---|---|---|

证据：

- `kind`：96EA50/96EAB0；Room ctor10×0x34；tx-04-18-room-application.md
- `children`：96EA50/96EAB0；Room ctor10×0x34；tx-04-18-room-application.md
- `unknown_10_13`：96EA50/96EAB0；Room ctor10×0x34；tx-04-18-room-application.md；本轮补读：96ED60把+10复制给child；业务名未定且96EAB0不消费，4B仍未知；+30单byte退休控制已单列
- `rotation_scale_factor_opacity`：96EA50/96EAB0；Room ctor10×0x34；tx-04-18-room-application.md
- `depth`：96EA50/96EAB0；Room ctor10×0x34；tx-04-18-room-application.md
- `position.x`：96EA50/96EAB0；Room ctor10×0x34；tx-04-18-room-application.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `position.y`：96EA50/96EAB0；Room ctor10×0x34；tx-04-18-room-application.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `retire_or_shrink`：967AA0清+30单字节；96F090沿child传播／清除并驱动收缩／退休；96EAB0不读此flag
- `unknown_31_33`：967AA0清+30单字节；96F090沿child传播／清除并驱动收缩／退休；96EAB0不读此flag；本轮补读：96ED60把+10复制给child；业务名未定且96EAB0不消费，4B仍未知；+30单byte退休控制已单列
