# RNG 与原生诊断记录：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

Entity DropRNG为+0x3DC起16bytes，InitSeed为+0x3EC；Grid RNG为+0x2C起16bytes，SpawnSeed为+0x14、VariableSeed为+0x20。seed、当前RNG首字和完整RNG对象不是同一概念；不通过调用RNG取样。

原生MT取随机数VA0x6EEF60可追加276-byte trace节点（返回值/native frame/最多64栈地址），rand早返和对象内联RNG可绕过。详细framestate为24-byte frame头＋56-byte实体记录，仅部分分类/运动/DropSeed及摘要；summary是另一条12-byte ring，online/lobby stream不等于世界存档。

纯展示Replica首版不消费gameplay RNG，不要求把服务器所有AI/RNG内部状态复制过去；若以后启用本地gameplay预测、rollback或接管，必须另行确定所需完整依赖状态。现有诊断数据不能直接反序列化成世界。

## 源码声明的补丁、调用依赖与自有表

当前没有本模块专用的源码安装点；共享路径见[Entity](entity.md)、[游戏循环](game-loop.md)。下列逆向入口不因此升级为已安装Hook。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x0000C3A0 | 0x0040C3A0 | sub_40C3A0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x002EEF60 | 0x006EEF60 | sub_6EEF60 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x002EF1F0 | 0x006EF1F0 | sub_6EF1F0 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x005032D0 | 0x009032D0 | sub_9032D0 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x00504E70 | 0x00904E70 | sub_904E70 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
