# RoomTransitionEvent

由唯一布局源生成。布局编号 453；布局版本 94；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

记录编码预算≤48B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 43—43B，不含第 9 步记录头）：

块 0：Host显示需求纯值（world，43—43B）：

| +0x0 u64（C/S自有元数据，无原生偏移） transition：Host接受转场序号 | +0x8 i32（C/S自有元数据，无原生偏移） requested_index：Start已接受目标 | +0xC i32（C/S自有元数据，无原生偏移） direction：Start方向 | +0x10 i32（C/S自有元数据，无原生偏移） animation：Start动画 | +0x14 u16（C/S自有元数据，无原生偏移） player：稳定Player关系 | +0x16 i32（C/S自有元数据，无原生偏移） dimension：Start维度 | +0x1A u64（C/S自有元数据，无原生偏移） final_room：实际ChangeRoom完成后room代次 | +0x22 i32（C/S自有元数据，无原生偏移） final_index：最终GridIndex | +0x26 i32（C/S自有元数据，无原生偏移） final_dimension：最终维度 | +0x2A u8（C/S自有元数据，无原生偏移） completed：真实ChangeRoom完成标记；同房动画明确完成 |
|---|---|---|---|---|---|---|---|---|---|

证据：


