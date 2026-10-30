# tx-04-09：改用 REPENTOGON ASM patch 安装器

> **同号历史说明（2026-10-30）**：本文保留2026-10-27的原任务、路径及失败结果。用户新指定的tx-04-09是[四人及多身体支持](observation-expansion/four-participant-control.md)，属于[新的连续推进计划](observation-expansion/README.md)。两者按主题区分；不覆盖本文或原artifact，不把本次编号整理视为ASM patch已通过验收。

日期：2026-10-27。用户要求将 `mod/src/hooks/odd_patch.cpp` 和 `host_patch.cpp` 改用 REPENTOGON 提供的 ASM patch。本次仅迁移这 12 处补丁的编码和安装，沿用统一清单、首次主线程延后安装、Host 后端锁和进程常驻策略。

## 实现

- Odd 使用 `ASMPatch::AddRelativeJump`；Host 的三处 mode 和 Create 使用 `AddInternalCall`，七处 K 通知/UI 分派使用 `AddRelativeJump`。NOP 通过 `AddBytes` 补足清单中的完整覆盖长度。
- 两个安装函数均调用 `sASMPatcher.FlatPatch(target, &patch)`，不再自行计算 rel32、调用 VirtualProtect 或 memcpy 写游戏指令。CMake 补充官方公共头所需的 Zydis/Zycore include；实现仍由既有 libzhl 链接导入。
- 保留裸 shim 的指令体及其 GPR/EFLAGS/x87/MXCSR/XMM 保护。官方 FlatPatch 在原地址编码，不额外建立 trampoline，四处 E8 CALL 的返回地址与 Create 拒绝路径 `add esp,8` 保持原约定。
- 写入前仍核对原指令与覆盖长度；安装器可传播的 C++ 异常被捕获并记录，关闭相应能力。SDK 对缓存刷新失败只记日志，因此调用后仍显式检查 FlushInstructionCache；只有检查成功才报告安装成功。失败不回滚、不重试、不创建 K。
- 成功的 byte_patch trace 新增 `backend=repentogon`，原 id/RVA 与 resident 字段保留。

## SDK 依据与边界

依据本地官方 `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/ASMPatcher.hpp` 与 `.cpp`：

- `AddInternalCall` 生成 5 字节 E8；`AddRelativeJump` 生成 5 字节 E9；FlatPatch 按目标原地址执行 ToASM，负责页权限、写入及缓存刷新。
- 不使用 PatchAt：该接口把原位置改为跳往新页，不符合现有四处 CALL shim 直接接收原生返回地址的栈约定。
- FlatPatch 的 nopRest 使用默认 false，覆盖范围完全来自既有清单，不让安装器扩大指令范围。
- 本次没有修改 SDK。SDK 的 ProtectionGuard 析构在恢复页权限时调用可能抛异常的函数；其析构 noexcept 造成的终止路径不能由外层 catch 宣称已恢复。完整 OOM、系统调用失败及并发异常闭包未获实机验证。

## 验收安排

全部修改完成后，通过一个入口执行一次 Win32 Release DLL 编译/链接与 PE 导入静态核对，保留构建日志、导入表和结果文件于 `artifacts/repentogon/tx-04-09-asm-patch/`。不添加单元测试，不运行游戏、不部署 DLL，不把编译通过作为 12 处补丁的运行时验收。既有游戏 E2E 与历史结果不改写。

## 本次执行结果

执行入口：`artifacts/repentogon/tx-04-09-asm-patch/accept.ps1`。CMake 配置成功；MSBuild 在实际编译前报 MSB4184，无法访问 `C:/Users/114514/AppData/Local/Microsoft SDKs`，导致 Windows SDK 目标版本查询失败。本次构建/导入核对结果为 FAIL（环境访问限制），不是编译器对本次 C++ 修改的诊断；PE 导入核对未执行，也未获得本次新 DLL。遵守不自动重跑约定，没有重新执行或提升权限重跑。

原始记录见 [configure.log](../../artifacts/repentogon/tx-04-09-asm-patch/configure.log)、[build.log](../../artifacts/repentogon/tx-04-09-asm-patch/build.log) 和 [result.json](../../artifacts/repentogon/tx-04-09-asm-patch/result.json)。代码改写完成，编译/链接及运行时行为仍未验证；没有游戏启动、DLL 部署或运行时 E2E。
