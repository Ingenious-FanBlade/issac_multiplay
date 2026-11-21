# tx-03-10：主智能体复核与整体静态审阅

日期：2026-10-22（Asia/Shanghai）。本轮只处理状态同步迁移的首项任务：循环分派层的Replica gameplay/presentation边界研究。用户指定GPT-6 Luna / max子智能体做逆向，主智能体审阅结论。没有推进实体创建/移除、网络schema或baseline实现。

## 复核依据

主智能体重新读取当前IDA的关键伪代码、原始指令和operand目标，而非只采纳子智能体摘要。IDB为`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，输入路径为同目录`isaac-ng.exe`，imagebase `0x00400000`；直接读取banner为`Binding of Isaac: Repentance+ v1.9.7.17.J460`。这些信息限定本次静态观察，不证明磁盘文件或任意同名版本与IDB内容相同。

- [primary-evidence.json](../../../artifacts/cs-t0/tx-03-10-static/primary-evidence.json)：8个函数、12段指令，覆盖slot4、odd helper、render child、Room呈现与HUD候选。
- [primary-followup-evidence.json](../../../artifacts/cs-t0/tx-03-10-static/primary-followup-evidence.json)：8个函数、6段指令，补充正确VA的FSM、driver、动画目标帧设置等。
- [primary-call-targets.json](../../../artifacts/cs-t0/tx-03-10-static/primary-call-targets.json)：9处直接call的原字节与IDA operand目标。
- [primary-leaf-decompilations.json](../../../artifacts/cs-t0/tx-03-10-static/primary-leaf-decompilations.json)：动画选择、状态重置及阴影/可见性相关调用的直接反编译返回。
- [primary-lua-evidence.json](../../../artifacts/cs-t0/tx-03-10-static/primary-lua-evidence.json)：3个函数、3段指令，补全标准尾部与render的Lua实际执行链。
- [primary-render-call-context.json](../../../artifacts/cs-t0/tx-03-10-static/primary-render-call-context.json)：VA `0x80EAF3`的实际参数准备；ECX为Room内EntityList，栈参数为Room内坐标向量。

只读导出脚本为[export_primary_review.py](../../../artifacts/cs-t0/tx-03-10-static/export_primary_review.py)与[export_primary_followup.py](../../../artifacts/cs-t0/tx-03-10-static/export_primary_followup.py)。通过IDA MCP的`py_exec_file`读取当前数据库；默认以create-new写产物。重复提取时给`export(output_file)`传入新路径；followup也可在其执行namespace指定`TX0310_FOLLOWUP_OUTPUT`为新文件。没有游戏执行、IDB修改或哈希计算。

## 主审纠正的结论

| 项目 | 初始表述/风险 | 直接复核后的结论 |
| --- | --- | --- |
| 普通Room call目标 | 子智能体一次手算为VA `0x807980`，误称旧记录错误 | VA `0x6FBA4D`字节`E8 2E 6F 10 00`；IDA operand为`0x802980`。下一指令`0x6FBA52`加位移`0x106F2E`同样得到`0x802980`；旧目标正确。 |
| FSM Player来源 | 子智能体把返回RVA `0x4319CC/0x431A14/0x431A34`当VA，读到资源函数并错误排除更新 | 正确返回VA为`0x8319CC/0x831A14/0x831A34`。对应call VA `0x8319C9/0x831A11/0x831A31`均为`FF 50 0C`；ECX来自Game的Player向量成员。这些是实际静态Player slot3调用路径。 |
| odd helper的分支 | 旧tx-01-66摘要容易读成每个PlayerUpdate后再调slot4 | VA `0x6FD5EB`比较Type；`0x6FD5F1`的`75 3A`转向Type!=1的slot4；Type==1调用`0x6FD609` slot3并经`0x6FD62B`跳过slot4。两者互斥。原始历史记录保留，在新报告和当前Entity入口澄清。 |
| 动画帧函数 | 将`0x408E00`直接称作逐帧推进器 | ECX为动画state，XMM1为目标帧，`state+0x10=min(nextafterf(frame_count,0),target)`；它不在函数内计算旧帧加dt。只能列为目标帧设置/重算层状态候选，不能宣称自主播放已经闭合。 |
| Lua helper | 子智能体初稿只关注末尾`luaL_unref`，误称未见回调执行 | `0x8607A0/0x863680`先调用`0x872980`，后者在VA `0x87299C`读取registry、`0x8729AF/0x8729BB`压两个integer、`0x8729CB`调用`lua_pcallk`，随后才ref/settop和外层unref。不能把已存在的脚本执行路径标为“仅清引用”。 |
| 展示状态写入 | 仅以写内存或虚槽编号归类为gameplay | HUD/sprite缓存写入与权威世界副作用分别说明。Grid呈现虚槽、Entity Update、可见性/阴影调用的receiver和参数不同；相同槽偏移不代表相同语义。 |
| 对象基址与参数 | 混用Manager/Game/Room，漏算DWORD索引倍率，或把伪代码缺省参数当实际ABI | driver的奇偶与odd开关属于Manager+`0x4ABBC/0x2A3C0`；FSM非零判断为Game+`0x1B83C`，并非Game+28175字节；odd附加位置分支条件属于Room+`0x7884`。VA `0x80EAF3`的ECX=Room+`0x1218`，另push Room+`0x1204`坐标参数，不能写成只给Room+4612的单参调用。 |

上表保留本次实际分析错误及修正，不改写旧失败，也不把初始误述作为最终事实。子智能体曾因模型服务容量不足中断；沿同一GPT-6 Luna / max继续交付，主智能体的原始复核产物单独保留。

## 已接受的研究结论与限制

driver在普通owner之前已可恢复staged位置；odd helper还含特殊FSM的Player更新、Type!=1位置阶段、直接坐标写入及额外对象向量的slot3更新。后者的集合语义不由该函数内无Type检查的调用自动证明。普通Room call、标准owner或中性输入均不是已证明的全gameplay停止点。

render child实际调用Room呈现准备/渲染与Lua post-render分派，HUD维护自己的展示状态。`0x6B0EB0`位置阶段又能进入含条件性Grid创建代码的`0x6AB440`。这些直接事实足以否定把整段原调用当作纯展示的捷径；它们不表示每次调用必然生成Grid或触发持久副作用。

本轮交付的是带原始指令的分派层地图、候选切断点与损失说明，以及动画目标帧候选。没有证明全部深层callee、派生类、Lua和资源调用的副作用闭包，也没有可直接启用的安全完整Replica展示边界。缺失内容仍是具体工程未知，不能据此实现任意写回或开始下一任务。

## 本轮整体静态文档验收

主智能体在上述分析、纠正和全部编辑完成后，仅执行一次整体静态文档验收。对象为[研究正文](tx-03-10-replica-update-presentation-boundary.md)、本审阅记录、`docs/archive/README.md`、Hook的README/game-loop/entity入口及状态同步方案；证据包含子智能体原始MCP结果、主智能体复核和收尾执行的[汇总导出](../../../artifacts/cs-t0/tx-03-10-static/loop-render-export.json)。

审阅内容为规范与单任务范围、VA/RVA/receiver/ABI、事实与推导的区分、已纠正的反例、候选切断点的损失、未知项以及新增引用。统一静态入口为[review_documents.ps1](../../../artifacts/cs-t0/tx-03-10-static/review_documents.ps1)，它只读取文档与JSON并检查本轮新增引用，结果见[final-static-review.json](../../../artifacts/cs-t0/tx-03-10-static/final-static-review.json)。重复审阅必须显式指定新的输出文件，不覆盖本轮结果。

此验收只评价本次静态研究交付，不评价游戏运行、Hook安装/恢复或Replica安全性。本轮未运行游戏、注入、输入、构建、E2E或单元测试；未计算或校验哈希，未修改生产源码和IDB。IDA保留原数据库，未控制或附着调试器。除本任务外的实际逆向与迁移实现均未推进。
