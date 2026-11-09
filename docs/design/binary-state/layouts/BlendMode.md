# BlendMode

由唯一布局源生成。布局编号 4；布局版本 42；原生大小 0x14；未知 0B；完整：是。

SDK 声明与 J273 GetMode 写入均确认连续五个 DWORD 的成员顺序；合法值来自官方 Global.zhl 的混合枚举。

原生字节归属：

| 0x0 u32 equation【状态】混合方程（4B；已核） | 0x4 u32 rgb_source_factor【状态】RGB 源因子（4B；已核） | 0x8 u32 rgb_destination_factor【状态】RGB 目标因子（4B；已核） | 0xC u32 alpha_source_factor【状态】Alpha 源因子（4B；已核） | 0x10 u32 alpha_destination_factor【状态】Alpha 目标因子（4B；已核） |
|---|---|---|---|---|

状态二进制布局（总计 20—20B，不含第 9 步记录头）：

块 0：混合模式（body，20—20B）：

| +0x0 u32 equation：混合方程 | +0x4 u32 rgb_source_factor：RGB 源因子 | +0x8 u32 rgb_destination_factor：RGB 目标因子 | +0xC u32 alpha_source_factor：Alpha 源因子 | +0x10 u32 alpha_destination_factor：Alpha 目标因子 |
|---|---|---|---|---|

证据：

- `equation`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `rgb_source_factor`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `rgb_destination_factor`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `alpha_source_factor`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `alpha_destination_factor`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
