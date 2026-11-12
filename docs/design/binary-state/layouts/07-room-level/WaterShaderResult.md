# WaterShaderResult

由唯一布局源生成。布局编号 285；布局版本 71；原生大小 0x0；未知 0B；完整：是。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 32—32B，不含第 9 步记录头）：

块 0：WaterShaderResult当前结果（world，32—32B）：

| +0x0 f32[4]（C/S自有元数据，无原生偏移） final_color：最终颜色RGBA，alpha已经乘waterAmount | +0x10 u32（C/S自有元数据，无原生偏移） color_space：KColor色空间标签 | +0x14 f32[3]（C/S自有元数据，无原生偏移） multiplier_rgb：最终1+(value-1)*waterAmount，不二次乘水量 |
|---|---|---|

证据：



逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:2953 | Room.GetWaterColor | " " / "KColor" | derived → final_color,color_space；WaterShaderResult.final_color | 发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖；原stream行2953归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
| docs/design/tx-04-21/stream-catalog.json:2954 | Room.GetWaterColorMultiplier | " " / "KColor" | derived → multiplier_rgb；WaterShaderResult.multiplier_rgb | 发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖；原stream行2954归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
