# ForcedCameraFocus

由唯一布局源生成。布局编号 290；布局版本 57；原生大小 0x0；未知 0B；完整：是。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—9B，不含第 9 步记录头）：

块 0：ForcedCameraFocus当前结果（world，1—9B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：有强制焦点意图时为1；无意图各端跟随C/S真实身体 | +0x1 f32[2]（C/S自有元数据，无原生偏移）（仅forcedcamerafocus_present_present(present)成立时存在） focus：91E920有效最后焦点；不复制ring／不调用Camera.Update |
|---|---|

证据：


