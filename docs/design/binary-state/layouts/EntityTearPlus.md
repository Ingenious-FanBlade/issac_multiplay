# EntityTearPlus

由唯一布局源生成。布局编号 20；布局版本 23；原生大小 0x18；未知 5B；完整：否。

原生字节归属：

| 0x0 u32 vtable【本地】REP扩展本地虚表（4B；已核） | 0x4 optional<bool> flying_override【换算】未设置=-1，false=0，true=1；只在engaged时读payload（2B；已核） | 0x6 bytes[2] unknown_6_7【未知】成员间对齐区尚未对已安装REP二进制核定；不复制（2B；未知） | 0x8 optional<u32>::value_and_engaged water_override【换算】Null=0，有值=(1<<32)\|u32；只读取有效payload，不复制optional内部尾字节（5B；已核） | 0xD bytes[3] unknown_d_f【未知】optional内部尾部对齐区未对已安装REP二进制核定；不复制（3B；未知） | 0x10 optional<u32> init_sound【权威】Host眼泪初始化音效覆盖；Client不重演初始化音效（8B；已核） |
|---|---|---|---|---|---|

状态二进制布局（总计 9—9B，不含第 9 步记录头）：

块 0：EntityPlus 公共可选覆盖（world，9—9B）：

| +0x0 i8 flying_override：未设置=-1，false=0，true=1；只在engaged时读payload | +0x1 u64 water_override：Null=0，有值=(1<<32)\|u32；只读取有效payload，不复制optional内部尾字节 |
|---|---|

证据：

- `vtable`：EntityPlus.h虚析构；client_player_sdk_abi.cpp；PlusHeader sizeof16
- `flying_override`：EntityPlus.h；MSVC optional:70-75；state/entity_extension.cpp
- `unknown_6_7`：官方EntityPlus成员顺序与Win32 ABI
- `water_override`：EntityPlus.h；MSVC optional:70-75；state/entity_extension.cpp
- `unknown_d_f`：MSVC optional与EntityPlus Win32 ABI
- `init_sound`：docs/design/binary-state/tear-pickup-laser-evidence.md
