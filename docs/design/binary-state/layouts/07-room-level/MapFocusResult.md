# MapFocusResult

由唯一布局源生成。布局编号 300；布局版本 60；原生大小 0x0；未知 0B；完整：是。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—7B，不含第 9 步记录头）：

块 0：MapFocusResult当前结果（world，1—7B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：challenge32正常选择结果；本地折叠不能使必需world结果缺席 | +0x1 i32（C/S自有元数据，无原生偏移）（仅mapfocusresult_present_present(present)成立时存在） focus_safe_index：正常9635B0真正采用的SafeGridIndex锚点，不再次抽样；169无描述时拒绝 | 顺序变长偏移 u16（C/S自有元数据，无原生偏移）（仅mapfocusresult_present_present(present)成立时存在） descriptor_member：锚点对应的活动描述成员 |
|---|---|---|

证据：


