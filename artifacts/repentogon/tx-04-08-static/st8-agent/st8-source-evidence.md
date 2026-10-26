# ST8 源码证据索引

本工件仅索引静态阅读的源码和主审只读 IDA/PE JSON；没有执行构建、测试、生成器或校验，也没有更改 IDB。路径以 `F:/projects_isaac/issac_multiplay` 为本仓库根目录；REPENTOGON SDK 位于 `F:/projects_isaac/REPENTOGON-1.1.3`。

## REPENTOGON HookSystem

- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/IsaacRepentance.cpp:33039–33043`：`Isaac::CreateGridEntity` 的 ECX/EDX ArgData、signature、两个显式参数及 `canHook=true`。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/IsaacRepentance.cpp:42228–42232`：全局 `CreateGridEntity` 的 stack/ECX ArgData、相同 signature、不同类型原型及 `canHook=true`。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/HookSystem.cpp:87–111,158–184,234–275`：Definition 列表、unique-name 映射、定义添加与按 name/type 查找；不按目标地址去重。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/HookSystem.cpp:397–425`：Hook multimap 使用 priority 降序并按迭代顺序安装。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/HookSystem.cpp:558–575,672–685`：按定义安装；`CanHook=false` 返回成功但跳过 detour；成功分支创建 Detour 并取得 original function。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/mologie_detours.h:394–431`：备份目标字节、创建 trampoline、写入入口跳转。
- `F:/projects_isaac/REPENTOGON-1.1.3/loader/loader.cpp:130–190`：加载 mod、调用初始化入口，最后执行 `ZHL::Init()`。
- `F:/projects_isaac/REPENTOGON-1.1.3/repentogon/dllmain.cpp:69–76`：SDK 对 priority 调用先后次序的注释。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/HookSystem_private.h:52–63`：`ArgData::_size` 为 `size_t`，按 x86 4-byte stack alignment 表示大小。

## 当前工程注册、容量、内存及构建入口

- `mod/binding/hooks.json:3–44`：41 行 hooks rows；行 22–23 的 `player_update` / `player_native` 同为 RVA `0x37BD40`。`:47–58` 为 12 byte patches；`:59–62` 是 2 dependencies；`:64` 为 8 pointer hook 名称。
- `mod/src/hooks/registry.cpp:5–27`：`pending[64]`、超限静默不存、按能力组构造 FunctionHook 并写 `registered=true`。
- `mod/src/hooks/cs_hook.h:5–35`：静态构造阶段仅调用 `AddPending`。
- `mod/src/binding/definitions.cpp:38–57,79–92`：按名/type、CanHook 和地址校验；自有定义所有参数均按 NoRegister 栈参数构造。
- `mod/src/bootstrap/mod_init.cpp:37–76`：ModInit 顺序为读取配置、PrepareBindings、RegisterHooks、发布状态。
- `mod/src/hooks/pointer_patch.cpp:8–17,57–74`：独立八槽 pointer hook 路径。
- `mod/CMakeLists.txt:3–5,23–42`：Win32/4-byte pointer build guard；build-time 自定义命令运行 generator 并编译库。
- `scripts/build.ps1:1–17`：配置、build、Cargo build 与 copy，没有测试、E2E 或游戏启动命令。
- `mod/src/runtime.h:69–117,119–145`：Ring 使用 `size_t` 原子索引；Hook 结构中的 priority 为 `int`，registered 是本工程布尔状态。
- `mod/src/ops/trace.cpp:8–18,193–232`：现有约 14 MiB trace ring；普通 FinishTrace 最多等 2 秒；process-detach trace 收口不等 worker。
- `mod/src/ops/command.cpp:359–368`：Game::Exit 对应 RunExit 处理的是 run teardown。
- `cs-t0/native/include/isaac_cs_t0/world_record.hpp:8–11,23–66`：旧原型的容量及 `static_assert` 大小，未运行这些断言。

## 设计与主审 IDA/PE 证据

- `docs/design/tx-04-08/tx-04-08-implementation-plan.md:218–249`：7.1 预算、队列计入 reader reservation/queued/writer in-flight/events、关闭时序与不得在 DllMain join/写文件/释放后台仍引用的内存。
- `docs/design/tx-04-08/tx-04-08-e2e-plan.md:47–50`：E2E 正常关闭、run teardown、最终正常 world close 后游戏退出的场景顺序。
- `artifacts/repentogon/tx-04-08-static/st6-st8-review/main-shutdown.json` 与 `main-shutdown-caller.json`：主审只读 IDA 导出；Shutdown 地址与 main 调用位置、Shutdown 后 main 收尾顺序。该证据不修改 IDB。
- `artifacts/repentogon/tx-04-08-static/st6-st8-review/main-pe-context.json`：输入 unpacked PE 头字段；不代表部署 PE 或实际可分配地址空间。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Isaac.zhl:46–47`：SDK 暴露 `Isaac::Shutdown()` 声明；当前 hooks.json 不含该 Hook 行。
- `F:/projects_isaac/REPENTOGON-1.1.3/repentogon/ImGuiFeatures/ImGui.cpp:777–780`：ImGui shutdown wrapper 的 flag 与 super 调用位置。
