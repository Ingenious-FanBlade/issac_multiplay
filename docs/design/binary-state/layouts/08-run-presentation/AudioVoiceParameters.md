# AudioVoiceParameters

由唯一布局源生成。布局编号 381；布局版本 71；原生大小 0x0；未知 0B；完整：是。

记录编码预算≤19B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 19—19B，不含第 9 步记录头）：

块 0：AudioVoiceParameters具名当前结果（world，19—19B）：

| +0x0 u16（C/S自有元数据，无原生偏移） asset：实际已选sample或intro→main音乐资源；不是Getter ID。 | +0x2 u8（C/S自有元数据，无原生偏移） volume_class：0=SFX、1=Music、2=Jingle按本地SFX类别；不携带Host音量。 | +0x3 f32（C/S自有元数据，无原生偏移） gain：每voice最终Options前gain；不反除Host音量、不重复应用mask／ducking／层权重。 | +0x7 f32（C/S自有元数据，无原生偏移） pitch：每voice最终Options前pitch；不反除Host音量、不重复应用mask／ducking／层权重。 | +0xB f32（C/S自有元数据，无原生偏移） pan：每voice最终Options前pan；不反除Host音量、不重复应用mask／ducking／层权重。 | +0xF u8（C/S自有元数据，无原生偏移） loop：明确world用途的循环请求，非后端playing。 | +0x10 u8（C/S自有元数据，无原生偏移） world_paused：明确world期望暂停，个人失焦／设备暂停不广播。 | +0x11 u8（C/S自有元数据，无原生偏移） segment：0=sample、1=intro、2=main；实际逻辑段，不伪装硬件游标。 | +0x12 u8（C/S自有元数据，无原生偏移） mix_rule：0：Options后min(gain*本地音量,1)，用于907C60／907F60；1：Options后线性乘法，用于908340等已核无显式min来源。不反除Host后端值。 |
|---|---|---|---|---|---|---|---|---|

证据：


