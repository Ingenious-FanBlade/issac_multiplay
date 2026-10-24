# 共享访问器与研究辅助入口：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

本表保存原始静态导出中尚未具有充分业务分类的共享helper、注册桥或编译器入口。原始sub_xxx名称只作查找索引，不把反编译器签名升级为可调用ABI，不把任何helper补造成某一实体专用能力。已有明确布局/语义结论以相应模块正文及复核文档为准。

## 源码声明的补丁、调用依赖与自有表

当前没有本模块专用的源码安装点；共享路径见[Entity](entity.md)、[游戏循环](game-loop.md)。下列逆向入口不因此升级为已安装Hook。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x000147F0 | 0x004147F0 | sub_4147F0 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x000173E0 | 0x004173E0 | sub_4173E0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00017400 | 0x00417400 | sub_417400 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x000174A0 | 0x004174A0 | structuredContent | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x00018701 | 0x00418701 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x00018920 | 0x00418920 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x00018BE0 | 0x00418BE0 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x000196A0 | 0x004196A0 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x0001C7D0 | 0x0041C7D0 | sub_41C7D0 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x0001C890 | 0x0041C890 | sub_41C890 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x000257B0 | 0x004257B0 | sub_4257B0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x0002A260 | 0x0042A260 | sub_42A260 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x00031430 | 0x00431430 | sub_431430 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json) |
| 0x00031440 | 0x00431440 | sub_431440 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json) |
| 0x00031560 | 0x00431560 | sub_431560 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x00047740 | 0x00447740 | sub_447740 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x0004BF90 | 0x0044BF90 | sub_44BF90 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00099DB0 | 0x00499DB0 | value | [ida-evidence.json](../../artifacts/cs-t0/tx-01-84-static/ida-evidence.json) |
| 0x000B7080 | 0x004B7080 | sub_4B7080 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x000CDCF0 | 0x004CDCF0 | sub_4CDCF0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x000CF210 | 0x004CF210 | sub_4CF210 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x001B1590 | 0x005B1590 | sub_5B1590 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x001B39D0 | 0x005B39D0 | value | [ida-evidence.json](../../artifacts/cs-t0/tx-01-84-static/ida-evidence.json) |
| 0x001CDB90 | 0x005CDB90 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x0020DEB0 | 0x0060DEB0 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json) |
| 0x0020DFB0 | 0x0060DFB0 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x0020E290 | 0x0060E290 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x0022E020 | 0x0062E020 | sub_62E020 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x00252BE0 | 0x00652BE0 | regions | [field-evidence.json](../../artifacts/cs-t0/tx-01-69-static/field-evidence.json) |
| 0x002661D8 | 0x006661D8 | regions | [field-evidence.json](../../artifacts/cs-t0/tx-01-69-static/field-evidence.json) |
| 0x00276150 | 0x00676150 | sub_676150 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00283410 | 0x00683410 | sub_683410 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x002A8740 | 0x006A8740 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json) |
| 0x002A8990 | 0x006A8990 | sub_6A8990 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x002A92A0 | 0x006A92A0 | sub_6A92A0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x002A92E0 | 0x006A92E0 | sub_6A92E0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x002A9320 | 0x006A9320 | sub_6A9320 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x002A9360 | 0x006A9360 | structuredContent | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x002A9480 | 0x006A9480 | regions | [binding-evidence.json](../../artifacts/cs-t0/tx-01-67-static/binding-evidence.json) |
| 0x002A9490 | 0x006A9490 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x002A96EB | 0x006A96EB | regions | [field-evidence.json](../../artifacts/cs-t0/tx-01-69-static/field-evidence.json) |
| 0x002A9730 | 0x006A9730 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json) |
| 0x002B8D20 | 0x006B8D20 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x002B8E20 | 0x006B8E20 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x002B8F00 | 0x006B8F00 | structuredContent | [ida-evidence.json](../../artifacts/cs-t0/tx-01-86-static/ida-evidence.json) |
| 0x002E0340 | 0x006E0340 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x002E03B0 | 0x006E03B0 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x002E30DC | 0x006E30DC | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x002EE110 | 0x006EE110 | structuredContent | [ida-evidence.json](../../artifacts/cs-t0/tx-01-86-static/ida-evidence.json) |
| 0x002EEF20 | 0x006EEF20 | sub_6EEF20 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x002F8140 | 0x006F8140 | sub_6F8140 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x002FBC10 | 0x006FBC10 | sub_6FBC10 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x002FD350 | 0x006FD350 | sub_6FD350 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x002FE2F0 | 0x006FE2F0 | sub_6FE2F0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x002FE560 | 0x006FE560 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json) |
| 0x002FE613 | 0x006FE613 | regions | [field-evidence.json](../../artifacts/cs-t0/tx-01-69-static/field-evidence.json) |
| 0x00355B50 | 0x00755B50 | regions | [binding-evidence.json](../../artifacts/cs-t0/tx-01-67-static/binding-evidence.json) |
| 0x00355C40 | 0x00755C40 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x003A6450 | 0x007A6450 | sub_7A6450 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x003B0070 | 0x007B0070 | sub_7B0070 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x0041E930 | 0x0081E930 | value | [ida-evidence.json](../../artifacts/cs-t0/tx-01-84-static/ida-evidence.json) |
| 0x0045DDA0 | 0x0085DDA0 | sub_85DDA0 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json) |
| 0x004607A0 | 0x008607A0 | sub_8607A0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00463680 | 0x00863680 | sub_863680 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x00464EE0 | 0x00864EE0 | sub_864EE0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00466060 | 0x00866060 | native remove callback dispatcher | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x004765A0 | 0x008765A0 | sub_8765A0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x0047DCE0 | 0x0087DCE0 | sub_87DCE0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x00481F30 | 0x00881F30 | value | [ida-evidence.json](../../artifacts/cs-t0/tx-01-84-static/ida-evidence.json) |
| 0x004849B0 | 0x008849B0 | sub_8849B0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x00488630 | 0x00888630 | sub_888630 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x00488ED0 | 0x00888ED0 | sub_888ED0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0048A100 | 0x0088A100 | sub_88A100 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0048A270 | 0x0088A270 | sub_88A270 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x0048A3E0 | 0x0088A3E0 | sub_88A3E0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x0048A740 | 0x0088A740 | sub_88A740 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0048ADA0 | 0x0088ADA0 | sub_88ADA0 | [grid-evidence.json](../../artifacts/cs-t0/tx-01-68-static/grid-evidence.json) |
| 0x00490C90 | 0x00890C90 | value | [ida-evidence.json](../../artifacts/cs-t0/tx-01-84-static/ida-evidence.json) |
| 0x00490CF0 | 0x00890CF0 | value | [ida-evidence.json](../../artifacts/cs-t0/tx-01-84-static/ida-evidence.json) |
| 0x004ADF50 | 0x008ADF50 | value | [ida-evidence.json](../../artifacts/cs-t0/tx-01-84-static/ida-evidence.json) |
| 0x004BEDB0 | 0x008BEDB0 | sub_8BEDB0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004BEE10 | 0x008BEE10 | sub_8BEE10 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004BEF40 | 0x008BEF40 | sub_8BEF40 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004BEFA0 | 0x008BEFA0 | sub_8BEFA0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004BF210 | 0x008BF210 | sub_8BF210 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004BF270 | 0x008BF270 | sub_8BF270 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004BF5B0 | 0x008BF5B0 | sub_8BF5B0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004BF610 | 0x008BF610 | sub_8BF610 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004BF920 | 0x008BF920 | sub_8BF920 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004C05C0 | 0x008C05C0 | sub_8C05C0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004C0620 | 0x008C0620 | sub_8C0620 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x004C0680 | 0x008C0680 | sub_8C0680 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x004C06E0 | 0x008C06E0 | sub_8C06E0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x004C0740 | 0x008C0740 | sub_8C0740 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x004C07B0 | 0x008C07B0 | sub_8C07B0 | [grid-evidence.json](../../artifacts/cs-t0/tx-01-68-static/grid-evidence.json) |
| 0x004C0950 | 0x008C0950 | sub_8C0950 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x004C5D06 | 0x008C5D06 | structuredContent | [ida-evidence.json](../../artifacts/cs-t0/tx-01-86-static/ida-evidence.json) |
| 0x004FD900 | 0x008FD900 | sub_8FD900 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x005091A0 | 0x009091A0 | sub_9091A0 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0050A8A0 | 0x0090A8A0 | sub_90A8A0 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0050ABF0 | 0x0090ABF0 | sub_90ABF0 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0050B070 | 0x0090B070 | sub_90B070 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0050CA40 | 0x0090CA40 | sub_90CA40 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0050D720 | 0x0090D720 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x0050D835 | 0x0090D835 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x0050D9B0 | 0x0090D9B0 | sub_90D9B0 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0050DF40 | 0x0090DF40 | sub_90DF40 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0051B130 | 0x0091B130 | sub_91B130 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x00559670 | 0x00959670 | sub_959670 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x00559984 | 0x00959984 | regions | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x00559D00 | 0x00959D00 | sub_959D00 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0055B310 | 0x0095B310 | sub_95B310 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x005A8620 | 0x009A8620 | sub_9A8620 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x005AB700 | 0x009AB700 | Game destruction caller | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x005BA960 | 0x009BA960 | ranges | [root-ida-join-context.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-join-context.json) |
| 0x005BAE40 | 0x009BAE40 | ranges | [root-ida-join-context.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-join-context.json) |
| 0x005BB045 | 0x009BB045 | ranges | [root-ida-join-context.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-join-context.json) |
| 0x005BB250 | 0x009BB250 | ranges | [root-ida-join-context.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-join-context.json) |
| 0x005BB3E0 | 0x009BB3E0 | ranges | [root-ida-join-context.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-join-context.json) |
| 0x005BB530 | 0x009BB530 | ranges | [root-ida-join-context.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-join-context.json) |
| 0x005BB5D0 | 0x009BB5D0 | sub_9BB5D0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x0060F500 | 0x00A0F500 | allocator free wrapper | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x0061E8D0 | 0x00A1E8D0 | sub_A1E8D0 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x0061E970 | 0x00A1E970 | sub_A1E970 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x0061EA20 | 0x00A1EA20 | sub_A1EA20 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x0061ED50 | 0x00A1ED50 | sub_A1ED50 | [root-ida-raw.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-raw.json) |
| 0x0061F280 | 0x00A1F280 | sub_A1F280 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x0061FC00 | 0x00A1FC00 | sub_A1FC00 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x00620710 | 0x00A20710 | sub_A20710 | [root-ida-raw.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-raw.json) |
| 0x006207B0 | 0x00A207B0 | sub_A207B0 | [root-ida-raw.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-raw.json) |
| 0x00620850 | 0x00A20850 | sub_A20850 | [root-ida-raw.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-raw.json) |
| 0x00622940 | 0x00A22940 | sub_A22940 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x006648B0 | 0x00A648B0 | allocation accounting and free | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x0066C650 | 0x00A6C650 | sub_A6C650 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x00689DE0 | 0x00A89DE0 | sub_A89DE0 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x006EF15C | 0x00AEF15C | delete wrapper | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
