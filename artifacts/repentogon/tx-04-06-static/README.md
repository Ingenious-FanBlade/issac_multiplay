# tx-04-06 / ST1–ST9 原始证据

日期：2026-10-25。完成 ST1–ST9 指定静态研究与主审；[ST1 主报告](../../../docs/research/tx-04-06-static-confirmation.md)、[ST2–ST9 补充报告](../../../docs/research/tx-04-06-static-remaining.md)、[实现方案](../../../docs/design/tx-04-06-implementation-plan.md)。逆向子智能体为 GPT-6 Luna / max，主智能体独立通过 IDA MCP 复核。设备通知/UI 兼容分派和副作用控制尚未实现；没有构建、启动游戏、安装 Hook 或运行测试，整体 E2E 尚未执行。

## 资料索引

| 文件 | 内容与限制 |
| --- | --- |
| [st1-subagent-findings.md](st1-subagent-findings.md) | Luna 的最终结论、证据分级和更正记录 |
| [ida-st1-raw.json](st1-subagent-evidence/ida-st1-raw.json) | 子智能体直接导出的输入路径、imagebase、基类表、派生表选定槽与原始函数 |
| [ida-st1-consumers.json](st1-subagent-evidence/ida-st1-consumers.json) | InputManager 表、查询包装与候选调用；`+0x0C` 扫描包含其他类/锁对象，候选不等于设备调用 |
| [ida-st1-ui-functions.json](st1-subagent-evidence/ida-st1-ui-functions.json) | 菜单相关函数候选；按实际 `start_rva` 和指令识别函数，不按请求 key 猜测身份 |
| [ida-st1-keyconfig-confirmed.json](st1-subagent-evidence/ida-st1-keyconfig-confirmed.json) | 正确 `0x4B9AD0` KeyConfig 更新、槽 11/30/33 及菜单消费者的补充原始证据 |
| [ida-st1-slot3-candidate-scan.json](st1-subagent-evidence/ida-st1-slot3-candidate-scan.json) | 有界模式扫描的空结果；不证明不存在其他槽 3 消费者 |
| [st1-primary-tables.json](st1-primary-tables.json) | 主审独立读取的基类、Keyboard、Gamepad 各 38 槽及目标 RVA |
| [st1-primary-ida-evidence.txt](st1-primary-ida-evidence.txt) | 主审 104 个函数的原始字节、指令、入口 xref 与原样伪码，包含关键消费者、三条删除路径与 UI 直接尾部读取 |
| [st1-primary-vector-default.json](st1-primary-vector-default.json) | 槽 18 读取的默认二维值在本轮 IDB 中的 8 字节初值；不证明运行时不可变 |
| [st1-primary-source-evidence.txt](st1-primary-source-evidence.txt) | 本地官方 1.1.3 的 Initialize Hook、名称偏移、菜单 action 短路及 MenuManager 特征 |
| [st1-review-corrections.md](st1-review-corrections.md) | 主审发现、推翻与采纳的记录，保留分析失败和证据边界 |
| [st1-primary-export.py](st1-primary-export.py) | 主审只读 IDA 导出脚本，包含中文功能说明 |
| [st2-st9-subagent-findings.md](st2-st9-subagent-findings.md) | Luna 直接逆向 ST3–ST7、对 ST2/8/9 的有限复核与归属说明；保留错误请求及更正 |
| [Join/创建首轮导出](st2-st9-subagent-evidence/st2-st9-subagent-join-create-raw.json) | Join、角色工厂、mode 与首轮误把 VA 当 RVA 的失败请求；以实际边界与更正后的目标为准 |
| [调用点原指令](st2-st9-subagent-evidence/st2-st9-subagent-callsite-raw.json) | mode、角色选择、Spawn/Remove 调用点原字节 |
| [Remove 重导](st2-st9-subagent-evidence/st2-st9-subagent-remove-raw.json) | 正确 RVA 的 RemoveCoPlayer 等函数 |
| [Spawn/Init 导出](st2-st9-subagent-evidence/st2-st9-subagent-spawn-raw.json) | 构造、Init、SetControllerId 和工厂的实际指令 |
| [wrapper/mode 导出](st2-st9-subagent-evidence/st2-st9-subagent-wrappers-mode-raw.json) | 子智能体独立读取的 action wrapper、predicate、三处调用点 |
| [st2-st9-primary-ida.json](st2-st9-primary-ida.json) | 主审独立导出 61 个请求点对应的实际函数、全部原字节/伪码/xref、InputManager 47 槽、菜单 controller 与 callback 全局引用；内部请求点如 42ACD5 需按 start_rva=42AC90 识别 |
| [st2-st9-primary-callpaths.json](st2-st9-primary-callpaths.json) | 1797 节点有界直接调用图、候选到 PGD/回调入口路径与未解析虚调用；未命中不等于无副作用，分支条件须人工复核 |
| [st2-st9-primary-join-pseudocode.txt](st2-st9-primary-join-pseudocode.txt) | 主审 JSON 中 Join 原样伪码的可读副本；自动变量名不作为 ABI |
| [st2-st9-primary-source-evidence.txt](st2-st9-primary-source-evidence.txt) | 官方 ABI、Player 字段与初始化/移除 Hook、回调、断连重分配 ASM、输入配置保存源码片段 |
| [st2-st9-review-corrections.md](st2-st9-review-corrections.md) | 主审采纳、驳回、更正和静态边界记录 |
| [st2-st9-primary-export.py](st2-st9-primary-export.py)、[补充请求点](st2-st9-primary-targets.json) | 主审只读 IDA 导出；保存实际函数边界，不能按请求 key 猜函数名 |
| [st2-st9-primary-callpaths.py](st2-st9-primary-callpaths.py)、[源码导出](st2-st9-primary-source-export.ps1) | 有界只读调用图与源码片段整理脚本；不执行目标或测试 |

## 地址与重读方法

目标为已打开的 `isaac-ng-v1.9.7.12.exe.unpacked.exe` IDB，imagebase=`0x400000`；各文件记录完整输入路径。正文均使用 RVA，IDA 工具读 VA 时加基址。`min_ea=0x401000` 是首段地址，不能用作 imagebase。

需要重新读取主审原始指令时，在相同目标的 IDA MCP 中调用 `py_exec_file`，`file_path` 指向本目录 `st1-primary-export.py` 或 `st2-st9-primary-export.py` 的绝对路径。脚本只读表、函数与 xref，并写本目录的对应资料；不修改 IDB、运行目标、建立 Hook、计算或校验哈希。其输出是静态资料，不是测试或运行时结果。现有产物已足够审阅，无需为本次工作再运行导出。

本目录的 JSON/文本按导出时的 IDA 信息保留。Hex-Rays 的错误类型、bad-SP、自动函数名和候选分组不提升为 ABI 或设备身份；关键结论以主报告列出的实际指令为准。
