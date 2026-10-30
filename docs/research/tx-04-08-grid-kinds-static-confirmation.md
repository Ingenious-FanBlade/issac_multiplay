# tx-04-08：Grid 类型补核与主审

> **后续实机范围**：e2e-07中44个Grid全部识别、整份录制无UNKNOWN_CLASS，证明本次场景的分类扩表路径有效；并不表示所有17个派生类均获动态覆盖。完整采集仍因Effect读取依据失败，见[结果](../design/tx-04-08/tx-04-08-e2e-results.md)。本报告主体仍为静态事实。

2026-10-29，Asia/Singapore。用户要求先补充Grid种类，再修复全量核对性能；逆向指定一个GPT-6 Luna/max子智能体，主智能体复核。代理完成[研究笔记](tx-04-08-grid-kinds-agent-notes.md)，主智能体随后独立读取同一目标IDB的18类RTTI、Factory及Grid ctor/Init原始指令。没有运行游戏、离线工具、测试或哈希，也没有修改IDB。

## 核定范围

目标仍为官方REPENTOGON1.1.3 / Repentance+ v1.9.7.12.J273，IDB imagebase=0x400000。名称索引与Factory分支共同确认Grid根及17个派生类；现有清单已登记根和Door，本次新增其余16类，总RTTI条数由10变26，文件schema仍为七类Entity加公共Grid。

| 新增RTTI类 | vtable RVA | TD RVA |
| --- | --- | --- |
| Decoration | 0x7196CC | 0x7D2214 |
| Rock | 0x71969C | 0x7D14F8 |
| Pit | 0x71A004 | 0x7D235C |
| Spikes | 0x71A2E4 | 0x7D2390 |
| Web | 0x71A578 | 0x7D2430 |
| Lock | 0x719F14 | 0x7D2328 |
| TNT | 0x71A32C | 0x7D23C4 |
| Fire | 0x71978C | 0x7D21E0 |
| Poop | 0x71960C | 0x7D1374 |
| Wall | 0x71972C | 0x7D224C |
| TrapDoor | 0x71A484 | 0x7D23F8 |
| Stairs | 0x72E758 | 0x7D3238 |
| Gravity | 0x72E6DC | 0x7D3200 |
| PressurePlate | 0x7195DC | 0x7D2280 |
| Statue | 0x71963C | 0x7D22F4 |
| Teleporter | 0x7196FC | 0x7D22BC |

主审逐项回读vtable[-1]、COL五个word、CHD四个word和每个BCD六个word：COL signature/offset/cdOffset均0；CHD signature/attributes均0，根count=1、派生count=2；派生的self TD在首项，Grid根TD为0x7D143C，所有PMD为0/-1/0、attributes=0x40。它们满足现有严格分类器的基链判据，不需要放宽未知TD、偏移或虚继承检查。主审保存了独立提取的[完整结构与虚表槽](../../artifacts/repentogon/tx-04-08-grid-static/main-rtti-review.json)。

## e2e-06 反例归属

主智能体回读原始REPENTOGON日志首行，模块基址为0x00F60000。attempt115/capture43/tick92的41个未知Grid对象vptr均为0x0167972C，换算RVA为0x71972C；与目标Wall虚表一致。结合当前Classify必须exact TD且清单遗漏Wall，已定位分类不完整的直接原因。这里只确认这些对象的实际RTTI，不从C++类名补造当时的enum type、字段值或出生/初始化历史。

## Factory 与公共读取依据

主审独立反编译0x706100，并回读switch、Wall/Fire/Door分支和共同返回的原始指令；完整原文见[主审Factory](../../artifacts/repentogon/tx-04-08-grid-static/main-factory-decompile.txt)，字节及ctor/Init见[主审原始提取](../../artifacts/repentogon/tx-04-08-grid-static/main-factory-init-review.json)。

- Factory的0值和无符号>27分支返回null；1..27按代理报告的17个派生类分配。Rock共用2–6、22、24–27，Spikes共用8/9。共同尾部0x706636写+4=type，0x706639写+24=GridIndex。
- Wiki的GridEntityType只作枚举名称索引。其13/FIREPLACE标“未使用”，目标0x706412仍明确写Fire vtable；这证明工厂分支存在，不证明该类型自然出现在所有场景。
- 0x704FB0公共构造初始化Grid描述/RNG；0x705070的Grid Init先调派生InitSubclass，正常返回后0x705090写initialized+1C=1，0x70509A写spawnFrame+28。公共读取继续要求已核定RTTI、initialized非零、index与Room槽一致、来源/root/线程前后稳定。不因扩表伪造owner返回、出生或当前读取资格。
- 新类完整sizeof未知，清单用object_size=null，C++诊断表用0表示未知；单列已确认readable_prefix_size=0x154及Factory实际分配请求。代理最初建议数据出现`0x0x154`重复前缀，主审指出后已修正；生产条目由主审独立提取数据构建。
- Door原SDK声明object_size=0x2EC，而Factory在0x7061F8原指令`68 F0 02 00 00`请求0x2F0，MakeDoor也有同请求。保留两项不同含义及出处，公共读取只使用Grid前缀，不把分配长度当作sizeof。

## 实现与尚未验证

`world_layout.json`增加16个exact RTTI条目和0–27枚举映射；RttiClass、名称表和生成器同步扩展，26类仍有完整登记检查。未知类仍Unknown；78字段、15来源、Hook79/96与常驻逻辑不变。完整DLL纯编译链接成功，日志为`artifacts/repentogon/tx-04-08-defg-build/dll-build-grid-kinds.log`。未部署或运行验收，不能据静态修复声称已取得2,000完整采集；e2e-06原始失败与用户停止归约事实保持不变。
