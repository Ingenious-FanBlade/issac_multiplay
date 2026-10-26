# ST6–ST8 主智能体静态读取说明

日期：2026-10-26，Asia/Singapore。只读取目标 IDB/PE 与本地源码，保存研究材料；未构建、运行测试/生成器/格式校验/游戏，未计算哈希，未改 IDB。

- `main-owner-functions.json` 是第一次 MCP 返回的完整外层信封，其内 stdout 被服务端截断，不能当作完整函数材料。按工具返回的本地 output URL 取回 `main-owner-functions-full.json`，后者包含完整 stdout 和静态函数资料。读取/反序列化已有资料只用于阅读，未作格式通过判定。
- 首次手工输入 `0x6F9D30/0x7E4DA0/0x8139B0` 位于其它函数内部，实际解析为 `0x6F9B30/0x7E4B40/0x813950`。原查询如实保留；不将这些误选函数当成 Spawn/MakeDoor/GridFree。随后按 ST1 地址表重新读取准确入口，见 `main-exact-owner-functions.json`。
- `main-exact-owner-functions.json` 包含 Spawn、NPC/Pickup Morph、GridSpawn/MakeDoor/GridFree、ReviveGhost、Game::Exit、PlayerRestore 的反编译及逐条地址/字节/指令。ABI 以原始指令和官方元数据为准，不照抄 Hex-Rays 推测签名。
- 首次材料中的 `0x993A90/0x995E10` 是正确的 ManagerRestore/Replace，保留在 `main-owner-functions-full.json`。
- `main-shutdown.json` 是按 `Isaac.zhl:46–47` 完整特征唯一命中的 `0x982C90`；`main-shutdown-caller.json` 记录其 caller `main 0x90AC60`。main 退出循环后直接调用 Shutdown，再做平台清理并返回，没有后续 Manager 循环。
- `main-pe-context.json` 只读当前 IDB 输入文件的 PE 头。PE32/i386、LAA 为真，不外推部署文件或运行可用虚拟地址空间。
- `main-source-excerpts.txt` 是主审逐行读取的当前工程及 SDK 摘录，保存文件路径和一基行号。它们是源码事实，不是已安装 Hook 或运行行为。
- `main-lua-spawn.json` 独立读取 Game/Isaac 的公开 Spawn 原生桥，确认参数次序、隐藏第八参和自动 seed 路线；`main-spawn-seed-helper.json` 记录全局 RNG/追溯存储副作用。
- 代理 ST6 初稿把 Replace 的 `replacement+170h` 分支方向写反；主审回读 `0x995E57/0x995E64` 和后续 roster swap 指令予以纠正，以完整 owner 材料为证。`main-morph-registration.json`、`main-morph-adapters.json`、`main-morph-closures.json` 还补齐代理原先未定位到的两个 Morph Lua 注册与实参读取；原始查询和修订范围在主审报告中列出。

本目录只保存主审材料；各代理独立材料位于同级 `st6-agent/`、`st7-agent/`、`st8-agent/`。最终接受范围与修订见 `docs/research/tx-04-08-st6-st8-static-confirmation.md`。
