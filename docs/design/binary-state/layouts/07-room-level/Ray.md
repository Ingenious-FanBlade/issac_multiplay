# Ray

由唯一布局源生成。布局编号 246；布局版本 87；原生大小 0x14；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 f32 opacity【状态】sin／pow曲线乘数；同步决定：发实际值（4B；已核） | 0x4 f32 speed【权威】Render末尾只用于phase+=speed，当前quad已提交；同步决定：不发；受控draw撤销写回／禁止跨本地Render累计（4B；已核） | 0x8 f32 phase【换算】当前正弦相位；同步决定：取正常Host该pass实际使用相位；同world重复展示从同一相位开始；正式投影：96CE20当前提交时的phase_used；取正常draw缓存而非phase+=speed后的内存（4B；已核） | 0xC Image* image.image【权威】实际有序光束图像；同步决定：资源引用；control本地：实际选中图像由第8步资源身份承接（4B；已核） | 0x10 Control* image.strong_control【本地】实际有序光束图像；同步决定：资源引用；control本地：本地引用计数／析构控制块（4B；已核） |
|---|---|---|---|---|

状态二进制布局（总计 10—10B，不含第 9 步记录头）：

块 0：Ray当前结果（world，10—10B）：

| +0x0 f32 opacity：sin／pow曲线乘数；同步决定：发实际值 | +0x4 f32 phase：当前正弦相位；同步决定：取正常Host该pass实际使用相位；同world重复展示从同一相位开始；正式投影：96CE20当前提交时的phase_used；取正常draw缓存而非phase+=speed后的内存 | +0x8 u16 image：实际有序光束图像；同步决定：资源引用；control本地 |
|---|---|---|

证据：

- `opacity`：96CE20实际20B步长；96F760/970060析构
- `speed`：96CE20末尾
- `phase`：96CE20实际20B步长；96F760/970060析构
- `image.image`：96CE20实际20B步长；96F760/970060析构；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `image.strong_control`：96CE20实际20B步长；96F760/970060析构；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
