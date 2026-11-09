# BitSet128

由唯一布局源生成。布局编号 9；布局版本 42；原生大小 0x10；未知 0B；完整：是。

四个u32的顺序由官方构造函数参数ll/lh/hl/hh明确；J273 SetTearFlags按16B值参数原样复制，按128位保留。

原生字节归属：

| 0x0 u32 ll【状态】低 64 位低 DWORD（4B；已核） | 0x4 u32 lh【状态】低 64 位高 DWORD（4B；已核） | 0x8 u32 hl【状态】高 64 位低 DWORD（4B；已核） | 0xC u32 hh【状态】高 64 位高 DWORD（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 16—16B，不含第 9 步记录头）：

块 0：128 位位集（body，16—16B）：

| +0x0 u32 ll：低 64 位低 DWORD | +0x4 u32 lh：低 64 位高 DWORD | +0x8 u32 hl：高 64 位低 DWORD | +0xC u32 hh：高 64 位高 DWORD |
|---|---|---|---|

证据：

- `ll`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读
- `lh`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读
- `hl`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读
- `hh`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读
