# RayGroup

由唯一布局源生成。布局编号 247；布局版本 87；原生大小 0x2C；未知 2B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。 只读被选中且非空组，present组的rays不可为空；不读取其他候选。

原生字节归属：

| 0x0 f32[7] geometry【状态】实际光线组边界与几何因子；同步决定：仅选中组发送；本地视口变换另管（28B；已核） | 0x1C vector<Ray> rays【容器】实际有序光线；同步决定：仅选中非空组；不能对空组运行先除成员数的native分支（12B；已核） | 0x28 bool stage_matched【权威】Load实际阶段匹配资格，决定是否纳入组vector；同步决定：留Host；只发送真正选中组（1B；已核） | 0x29 bool stage_b_suffix【权威】stages token的b后缀标记；不是绘制相位；同步决定：留Host；不发原始过滤候选（1B；已核） | 0x2A bytes[2] unknown_2A_2B【未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；同步决定：不读不发；窄字段读写跳过该范围，保留未决（2B；未知） |
|---|---|---|---|---|

状态二进制布局（总计 30—40990B，不含第 9 步记录头）：

块 0：RayGroup当前结果（world，30—40990B）：

| +0x0 f32[7] geometry：实际光线组边界与几何因子；同步决定：仅选中组发送；本地视口变换另管 | +0x1C u16 count + count×Ray（上限4096） rays：实际有序光线；同步决定：仅选中非空组；不能对空组运行先除成员数的native分支 |
|---|---|

证据：

- `geometry`：96CE20组步长44B；client_presentation_native.cpp::NativeRayGroup
- `rays`：96CE20组步长44B；client_presentation_native.cpp::NativeRayGroup
- `stage_matched`：968E10解析stages token，以b后缀置高byte、匹配时置低byte；96F8A0只复制+28/+29两字节
- `stage_b_suffix`：968E10解析stages token，以b后缀置高byte、匹配时置低byte；96F8A0只复制+28/+29两字节
- `unknown_2A_2B`：968E10解析stages token，以b后缀置高byte、匹配时置低byte；96F8A0只复制+28/+29两字节
