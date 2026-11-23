# OriginalOnlineEntityFrame

由唯一布局源生成。布局编号 261；布局版本 94；原生大小 0x18；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 u32 frame【本地】原版Online帧号；同步决定：不进入C/S（4B；已核） | 0x4 vector<OriginalOnlineEvent56> events【本地】原版56B实体事件vector头；同步决定：不进入C/S、不建立原版事件owner（12B；已核） | 0x10 u32 native_comparison_input【本地】原版Online正常路径保存的6EB740返回值；不将未核业务名当同步RNG；同步决定：不读取或计算作为C/S一致性输入（4B；已核） | 0x14 u32 comparison_value【本地】原版Online实体比较累计值；同步决定：不读取或计算作为C/S一致性输入（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `frame`：413F00
- `events`：413050/4130F0/417EB0
- `native_comparison_input`：413F00末尾
- `comparison_value`：413F00末尾
