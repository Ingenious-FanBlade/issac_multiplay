# tx-04-04 修订：Lua 命令捕获的静态复核

日期：2026-10-24。证据：`STATIC_CONFIRMED`，不表示运行时观察。

通过 `C:/Users/114514/.codex/config.toml` 中已配置的 `ida-mcp` HTTP 端点发现工具并调用；当前会话的自动工具列表未挂载 IDA 命名空间。数据库为 `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，image base `0x400000`。未计算或校验哈希，未修改数据库或运行游戏实验。

- `Isaac.ExecuteCommand` 原生实现 RVA `0x4567F0`、VA `0x8567F0`。`0x856865` 是 `mov [esi+680D8h],edi`；`0x856870` 清空同一字段。`0x85683C` 将 `Game+0x67FD4` 作为 Console this，`0x85686B` 调用 VA `0x688100`（RVA `0x288100`）的 RunCommand，两个附加参数都为零。
- EDI 指向返回用 MSVC `std::string`：长度位于 `+0x10`，容量位于 `+0x14`，初始容量 15；因此捕获字段为 `std::string*`。旧资料和源码写作 `Game+0x68158`，本次更正为 `Game+0x680D8`，即 `Console+0x104`。
- `Console::Print` RVA `0x28DC60`、VA `0x68DC60`：`0x68DC8D` 读取 `[edi+104h]`；非空时 `0x68DCAB` 调用字符串追加函数并在 `0x68DCB0` 跳到返回处 `0x68E0BD`。控制台历史操作只在捕获为空的 `0x68DCB5` 分支中执行。
- 结论：保持 out 为空时调用 Print 的方案；Lua 捕获期间不会写控制台历史，无需直接写捕获字符串。out 非空时只追加 out，避免重复显示。界面仍降低为每 60 帧查询一次。

原始反编译和入口反汇编保存在同目录的 JSON 文件中。`lua-execute-command.json` 是最初将资料里的 RVA 当成 VA 查询导致的反编译失败，原样保留；正确结果是 `lua-execute-command-rva.json` 与 `lua-execute-command-entry.json`。
