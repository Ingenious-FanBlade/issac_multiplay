# 第5步：正式Player布局源与生成

日期：2026-11-09（Asia/Singapore）。整数布局版本 **47**。本轮完成第5步的正式布局源、生成支持与生成表；实际采集、资源／结构owner、decode／安装及旧链退役分别仍归第8—14步，第15步是唯一统一整体验收。

## 唯一源与布局编号

新增 [`mod/binding/state/player.json`](../../../mod/binding/state/player.json)，由 `version.json` 登记为正式域。构建生成器不读取三份P0—P8设计文档、`player-byte-ledger.json` 或 `player-member-map.json`；这些保留为原始设计与证据。一次性迁移工具用后删除，没有建立第二份运行输入。

新增48个布局，编号150—197：150—176登记Player辅助结构、Plus与Host保存体，177／178为有序ID/count元素，179为Player主动槽，180为条件效果，181—194为14种真实动态Weapon类，195为participant身份登记，196为控制组，197为EntityPlayer。原有1—149及通用ActiveItemDesc12／Weapon13／TemporaryEffect14／Whip135／TemporaryEffects136不改成Player专用合同，历史生成范围继续保留。

Player从Entity继承0—9块，本类10—18对应P0—P8，19—21是固定13份ANM2三投影，22—24是有序costume Sprite三投影。父E1／E4／E5／E6只在Player布局改为body；P1—P8仍是world。没有扩张32位block_mask，也没有把四参与者缩成四身体。

| 物理块 | 正式下界／规则 |
|---|---|
| 10 P0 | 64＋headAnim有效字节；含自有统一绘制拓扑revision |
| 11 P1 | 1B CanFly |
| 12 P2 | 77B直接健康／资源输入 |
| 13 P3 | 88B格式下界；保留完整dense长度、口袋4槽、材料8槽、poop6槽及blocked＋有效innate投影 |
| 14 P4 | 96B；4×24B；空Item的其它五值输出0且不读旧payload |
| 15 P5 | 1—91B；低5位存在掩码，逐槽u16正式view_layout＋该类字段 |
| 16 P6 | 111＋4C B；有序目录及15×7B最终map；负index不读旧layer但仍保留body标志 |
| 17 P7 | 21B；独立激活／列表资格、真实twin／backup／held及C/S组，不恢复native sub／replaced／703四槽 |
| 18 P8 | 6＋6E＋4S＋12A＋171D B下界；S仅collectible704项数；D仅当前可绘节点，ANM2变长内容另计 |
| 19—21 | 13×ANM2子块0／1／2，无重复slot或count头 |
| 22—24 | 各自u16 count，与P6同一有序目录及拓扑，三类Sprite投影分别比较 |

Player整记录格式下界2773B，不含第9步记录头；各块可变内容不能同时取各自上限。正式记录预算32MiB，body组64KiB；快源放不下时走ordinary body-joint并撤销旧候选，超过完整帧／总内存预算时报实际缺口，禁止截断。这里没有实测带宽或性能结论。

## 新增生成能力

`schema.json`、`gen_state_layout.py`及`state/layout.h/.cpp`共同声明：

- `native_type/native_count`展开不上线的保存体／原版通信结构，保留内部UNKNOWN及较弱证据；Host／本地父范围约束子字段用途，不恢复其状态传输。
- `storage=aggregate`的stride固定为0；innate由官方容器成员有界聚合，8B结果pair不是原map分组或节点步长。
- `fixed_count`表达真实固定槽，无线上u16 count；普通容器仍按逻辑数量编码，投影一致性约束包含P6目录与P0三投影。
- `Operation::Choice`和`LayoutChoices()`登记Weapon允许布局；type10／11共用Bone动态类但保留type，五种恒零类型完全没有三项充能输入，其余类型输出对应Modifiers mask。Whip只保留真实+44主引用。
- `FieldConditions()`只引用同块先行定长选择器。效果704通过同resource_epoch真实配置身份判断；可选序列先有D，D=0不读取节点或Sprite。条件处理器在后续接线时绑定，不能由报文提供偏移或函数。
- `FieldSource::Synthetic`标识自有拓扑／资格／控制元数据，不进入CopyRange；`extent=protocol`记录明确无原生对象。控制组为21＋2N B，N≤64，participant只有身份上下文。
- `value_mask`和`LayoutBudgets()`分别保留位投影与编码总预算；具名处理器语义保留在唯一源，不把只有字符串名字的adapter宣称为已运行。

`ReadDequeInto`增加12B残影步长：每块一个12B元素，按逻辑顺序复制到调用方预分配区，块边界按实际分配大小检查；既有4／8B规则保持。没有扩展到任意未知MSVC容器，map／set／innate聚合及具体Player reader仍由第10步接线。

## 原生事实与逐行回映射

[EntityPlayer生成表](layouts/05-player/EntityPlayer.md)完整覆盖0—2E9F，递归UNKNOWN仍 **3707B**；[EntityPlayerPlus](layouts/05-player/EntityPlayerPlus.md)仍 **9B**。Plus大小A8h及后半偏移继续是官方声明＋本地Release ABI模型，未升级为官方DLL实测；公共E9只发原EntityPlus前缀，P3经独立官方成员owner访问itemSpoofs，不按推导偏移直接写Plus。

14种Weapon view保留真实分配大小、未知尾部和factory用途。2026-11-09只读J273的vtable[-1]→COL+0Ch→TypeDescriptor再次登记RTTI；例如Tears的vtable B31EA4／COL B456D0／TD RVA7D3998，Whip的B31BC4／B45A08／RVA7D1664。这里是静态读取，没有IDA修改或运行观察。

`CatalogRows()`保存1265条原应用行，以目录＋一基行号定位，保留owner、parameters／returns及null／空字符串区别。`SupplementaryMembers()`另保留14条native、77条旧宏和6条结构记录。EntityPlayer公开owner的唯一ID覆盖与原始逐行表并存，重载不因生成唯一成员表而丢失；另一Entity目录按原签名链接到正式行，未接管的其他实体不冒称Player覆盖。

GetSubPlayer不能从C/S body_role恢复原native边；分组innate getter不再有Host原组名，IsItemQueueEmpty也不能从“非饰品折Null”投影恢复完整原语义。正式行处置将这些原API保留在Host。IsSubPlayer按PlayerIndex符号派生；部分投影不承诺全部旧getter可任意执行。第8步具名最终HUD缓存／显示决定仍明确为待实现上下文，不恢复无调用方Captured通用字典。

## 生成与编译情况

正式生成入口仍为 `python mod/tools/gen_state_layout.py --out build/repentogon/generated`，同一次生成四份inc和全部原生／线格式单行表。生成期间补齐了固定槽计数、12B deque及复杂元素字节预算，修复其原有声明限制和理论大小溢出；未通过裁字段或清UNKNOWN绕过。

Release／Win32／v143的`isaac_cs_state`纯编译及完整`zhlIsaacCS.dll`链接均通过，退出码0；构建配置`ISAACCS_BUILD_E2E_TOOLS=OFF`。未运行单元、隔离、smoke、E2E或游戏测试，没有注入、部署或实机操作。完整产品首次沙箱内构建停在CMake File Globs且无后续输出，已中断；随后在沙箱外使用既有MSBuild完成相同纯产品构建。实际运行合同及性能只在第15步统一整体验收。
