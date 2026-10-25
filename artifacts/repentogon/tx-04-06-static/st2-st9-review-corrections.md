# ST2–ST9 主审、更正与证据边界

日期：2026-10-25。子智能体为用户指定的 GPT-6 Luna / max，继续同一子任务 `st1_reverse`；主智能体独立通过 IDA MCP 导出并读取实际函数、指令、表和官方源码。没有目标执行、IDB 修改、Hook、构建、测试或哈希。主审结果见[补充报告](../../../docs/research/tx-04-06-static-remaining.md)。

## 1. 采纳的结论

- ST3 的 Join 目标、按 device id 分配行、Triggered 19/20/21/14/15 与 Pressed 8/10 区别、最小两 pass 设计；两 pass 不是最低原生帧数证明。主审另读普通/特殊行初始化，补上自动确认可能在 P1 完成创建，以及 pass 内多次查询须幂等。
- ST4 三处完整 5 字节 E8 及上下文；主审另确认第三处原 predicate 不改 ECX，后续 591E76 仍 push ECX，替代调用须保留。
- ST5 一个 role 参数、角色表是 int32 值、selected 是索引、-1 才走随机；CALL shim 的栈解释经双方更正后采纳。
- ST6 thiscall + 两个栈 DWORD / retn8；主审以官方特征、Lua 调用和实际指令补充 unused bool=false 不抑制掉落，普通路径调用 Entity::Remove，不混称析构。
- ST7 index 在 append 前按既有 Player 数写入；Init 内写类型/角色/默认 controller，Join 在工厂返回后设置 K controller。死亡/ghost 偏移不扩大复核。
- ST2/ST8/ST9 主要原始取证由主审承担；子报告对这些章节明确归属，没有声称进行了另一份全面独立审计。主审独立确认了注册配置通知、默认初始化安装的原版断连回调、Game::Exit 先清 dirty 后保存、两个 UI 直接读尾部反例。

## 2. 保留的错误与更正

| 原解读 / 失败 | 实际依据与更正 |
| --- | --- |
| 计划问题把“调用前 8 字节”当两个工厂参数 | 591FDF 唯一 push role，590250 正常/拒绝尾均 retn4。原型是 E8 CALL shim，shim 入口多一个返回地址；拒绝 add esp8 清返回地址+role，数值仍正确 |
| 主智能体早期进度称“清 8 需要修正”；子智能体也曾由一参数推断清 8 错误 | 读取 coop_active_runtime.cpp 的 CreateHook 后撤回；修正的是 8 字节来源的解释，不把 CALL shim 的清栈改成 4。若换 JMP shim 才只有 role4。已向用户说明 |
| 子智能体将 990560/992650 当 RVA 请求，得到 NO_FUNCTION | 正确为 VA 990560/992650、RVA 590560/592650；首轮失败保留在 join-create-raw，后续正确重导保存于 remove-raw |
| 子报告 ST7 初稿将 6A4A20、79E1B0 标为 RVA | 这两项是 VA，正确 RVA 为 2A4A20、39E1B0；主审指出后子报告已修正 |
| 子智能体把 `mov ArgList_0,eax` 当局部保存，否认菜单全局写 | 4AFAED 字节 A3 14 8A B8 00，目标 operand type=o_mem(2)，value=B88A14，为绝对全局 RVA788A14。自动名称不能判断栈/全局；子智能体回 IDA 重读后更正 |
| 子智能体给槽 9 调用点 4AFAA3 | 真正 call eax 是 4AFAA1 的 FF D0；4AFAA3 的 8A D8 是 mov bl,al。主审正文以实际 call 为准 |
| ST6 子报告笼统称虚析构/清理 | 普通公共路径 590D0A 调 vtable+28，目标2A8000=Entity::Remove，写标记/分派67；不等于立即析构释放。特殊分支须单独看待，子报告已修正 |
| 子报告把 2A8000→44CA50 的 callback67 写进 ST2 InputManager 删除 | 这是 Player RemoveCoPlayer 的下游 Entity Remove，属于 ST6/ST8，不是 RemoveDevice；已更正 |
| Manager 初始化 52C341 的 IDA 符号像标准库时区函数 | 原 E8 实际目标4E3FC0，body安装4EE980断连callback；按实际目标与body识别，不采信误匹配的库函数名 |
| 直接调用图 Spawn 到 PGD/callback 无命中 | 590479 是虚调用，人工核表连到34F6A0；图明确不解析虚边。不命中不是不存在副作用 |
| 仅凭调用图有路径便说普通 B 每次会解锁/加统计 | 590780→352E50 属 Baby；Init尾角色18为特殊分支；完整退役flush item queue只有队列非空且种类满足才进AddCollectible。正文保留条件，不把候选路径当每次发生 |
| InputManager 表粗读成48函数槽 | 实际0–46共47槽，下一DWORD是RTTI COL745F4C。主审保存实际47表项和后续值 |

主审原始 JSON 保留请求点与实际函数起点。比如请求 `0x42ACD5` 是 SaveButtonMaps 内的特征点，实际函数 `start_rva=0x42AC90`；另有早期候选请求命中其他实际边界，不按 key 强行命名。只在正文采用已由指令/来源确认的身份。

## 3. 本轮完成与没有证明的事项

完成的是 ST2–ST9 指定问题的静态研究、事实分级、原始证据留存、子结论主审及方案修订；ST5/ST7 保持用户收窄范围。数据不是运行时结果，UI/通知兼容处理还没有实现。

没有证明全映像间接调用闭包、实际 Hook 安装顺序、worker/remove/OOM/EH 并发闭包、任意第三方 Lua 行为、所有特殊角色拓扑，或所有创建/退役窗口均零持久写入。没有把这些 UNKNOWN 变成无风险、PASS 或额外项目 gate。也未触及原版在线会话，相关函数只静态阅读。

最后按现行规范进行一次整体静态文档审阅，检查报告、方案、点位索引及原始证据的对应和修订范围；没有运行校验程序、构建或测试。tx-04-06 的唯一整体 E2E 留在功能全部实现之后。
