# StatHUDRows

由唯一布局源生成。布局编号 354；布局版本 71；原生大小 0x0；未知 0B；完整：是。

记录编码预算≤109B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 97—109B，不含第 9 步记录头）：

块 0：StatHUDRows具名当前结果（world，97—109B）：

| +0x0 u8（C/S自有元数据，无原生偏移） planetarium_world_qualified：Host资格406最终world许可；个人StatHUDPlanetarium偏好另施加，不写访客进度。 | +0x1 StatHUDValue（见该类型状态表）（伴随源，无根对象偏移） row_0：正常已完成缓存第0行；只复制value／delta／countdown，不重算。 | +0xD StatHUDValue（见该类型状态表）（伴随源，无根对象偏移） row_1：正常已完成缓存第1行；只复制value／delta／countdown，不重算。 | +0x19 StatHUDValue（见该类型状态表）（伴随源，无根对象偏移） row_2：正常已完成缓存第2行；只复制value／delta／countdown，不重算。 | +0x25 StatHUDValue（见该类型状态表）（伴随源，无根对象偏移） row_3：正常已完成缓存第3行；只复制value／delta／countdown，不重算。 | +0x31 StatHUDValue（见该类型状态表）（伴随源，无根对象偏移） row_4：正常已完成缓存第4行；只复制value／delta／countdown，不重算。 | +0x3D StatHUDValue（见该类型状态表）（伴随源，无根对象偏移） row_5：正常已完成缓存第5行；只复制value／delta／countdown，不重算。 | +0x49 StatHUDValue（见该类型状态表）（伴随源，无根对象偏移） row_6：正常已完成缓存第6行；只复制value／delta／countdown，不重算。 | +0x55 StatHUDValue（见该类型状态表）（伴随源，无根对象偏移） row_7：正常已完成缓存第7行；只复制value／delta／countdown，不重算。 | +0x61 StatHUDValue（见该类型状态表）（伴随源，无根对象偏移）（仅stat_h_u_d_rows_planetarium_world_qualified_planetarium_world_qualified__1(planetarium_world_qualified)成立时存在） row_8：正常已完成缓存第8行；只复制value／delta／countdown，不重算。 |
|---|---|---|---|---|---|---|---|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `row_0` → `StatHUDValue`：正常已完成缓存第0行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `row_1` → `StatHUDValue`：正常已完成缓存第1行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `row_2` → `StatHUDValue`：正常已完成缓存第2行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `row_3` → `StatHUDValue`：正常已完成缓存第3行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `row_4` → `StatHUDValue`：正常已完成缓存第4行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `row_5` → `StatHUDValue`：正常已完成缓存第5行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `row_6` → `StatHUDValue`：正常已完成缓存第6行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `row_7` → `StatHUDValue`：正常已完成缓存第7行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `row_8` → `StatHUDValue`：正常已完成缓存第8行；只复制value／delta／countdown，不重算。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
