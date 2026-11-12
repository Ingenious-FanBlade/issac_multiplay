# ResourceAudio

由唯一布局源生成。布局编号 433；布局版本 71；原生大小 0x0；未知 0B；完整：是。

版本63资源描述；无文件内容／纹理像素／PCM／指针，表到达不等于本地物化ready。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

公共封装布局（总计 4—8198B，不再附加12B状态记录头）：

固定封装（4—8198B；块表中的world仅为生成占位，不限制承运通道或所属序列）：

| +0x0 u8（C/S自有元数据，无原生偏移） audio_class：0选定sample、1音乐声部、2jingle；不是sound ID | +0x1 u8（C/S自有元数据，无原生偏移） has_intro：has_intro | +0x2 ResourcePath（见该类型状态表）（C/S自有元数据，无原生偏移） main：main | 顺序变长偏移 ResourcePath（见该类型状态表）（C/S自有元数据，无原生偏移）（仅audio_has_intro(has_intro)成立时存在） intro：intro |
|---|---|---|---|

证据：


