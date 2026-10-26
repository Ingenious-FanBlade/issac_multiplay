# tx-04-09 ASM patch 迁移：构建记录

2026-10-27。代码范围和 SDK 依据见 [迁移记录](../../../docs/design/tx-04-09-asm-patch.md)。

唯一执行入口为 `accept.ps1`，用途是配置、构建 Win32 Release DLL，并静态读取 PE 导入表；不加载 DLL、不部署或启动游戏，不执行哈希校验。

本次结果：**FAIL（构建环境访问限制）**。CMake 配置成功；MSBuild 查询 SDK 版本时访问 `C:/Users/114514/AppData/Local/Microsoft SDKs` 被拒绝，报 MSB4184，在 C++ 编译前停止。未执行 PE 导入核对，没有本次新 DLL，运行时 E2E 为 NOT_RUN；没有自动重跑。

- [配置日志](configure.log)
- [构建日志](build.log)
- [机器可读结果](result.json)
- [可重复执行入口](accept.ps1)（再次执行需要另行授权，不能用重跑覆盖本次失败事实）
