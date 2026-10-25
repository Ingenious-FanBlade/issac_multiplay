# tx-04-06 3.4 / 3.5 纯构建记录

2026-10-25。范围见[编写记录](../../../docs/design/tx-04-06-ops-harness.md)。本目录只保存纯编译/链接与生成的 Hook 清单，不是测试产物；没有部署、启动游戏、运行网络程序或执行测试。

构建入口为 `pwsh -NoProfile -File scripts/build.ps1`，不含任何测试。Win32 / v143 / Release DLL 与 i686 Rust harness 均已纯编译、链接成功，产物在 `out/repentogon/`；没有部署到游戏目录。唯一整体 E2E 尚未执行。

- [build-01.log](build-01.log)：MSBuild 查找 Windows SDK 时，沙箱拒绝访问用户目录下的 Microsoft SDKs 缓存，未进入源码编译。
- [build-02.log](build-02.log)：获取所需构建访问后，生成器因局部变量 `args` 覆盖参数对象而失败。已将局部变量改为 `persistence_args`，保留原始失败日志。
- [build-03.log](build-03.log)：DLL 与 Rust harness 编译链接成功；SDK/RapidJSON/生成 wrapper 的既有提示保留，Rust 有一个未使用 helper 提示。随后移除该 helper，并补齐脚本代次失效和证据收尾。
- [build-04.log](build-04.log)：最终纯构建成功，Rust 无 warning，DLL 增量构建成功。没有执行构建外的任何程序验证、网络试跑或测试。

生成清单归档为 [hook-manifest.json](hook-manifest.json) / [hook-manifest.md](hook-manifest.md)，与 `out/repentogon/` 同步：33 个入口、8 个设备槽、12 处字节补丁。它描述实现与构建产物，不能证明运行时已安装或 V1–V12 已通过。
