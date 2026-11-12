# PlayerControlGroup

由唯一布局源生成。布局编号 196；布局版本 71；原生大小 0x0；未知 0B；完整：是。

身份空间10；每参与者一条canonical输入，最多4参与者/每组64身体/全局256身体。成员从P7反建；anchors唯一、同组且active，保留注册顺序。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 21—149B，不含第 9 步记录头）：

块 0：展示值（world，21—149B）：

| +0x0 u16（C/S自有元数据，无原生偏移） participant：非Null成员号，身份空间9；不得跨代冲突 | +0x2 u8（C/S自有元数据，无原生偏移） ordinal：session内冻结的0—3参与者序号 | +0x3 u64（C/S自有元数据，无原生偏移） control_session：非零已提交控制session | +0xB u64（C/S自有元数据，无原生偏移） binding_revision：非零绑定代次，不与shape或拓扑revision混同 | +0x13 u16 count + count×u16（上限64）（C/S自有元数据，无原生偏移） active_anchors：u16 count+按原注册顺序的Player成员号；唯一/同组/active，最多64；不是每锚点一条输入流。 |
|---|---|---|---|---|

证据：


