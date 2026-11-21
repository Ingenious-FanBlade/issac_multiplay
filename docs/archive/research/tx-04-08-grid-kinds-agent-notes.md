# tx-04-08 Grid 派生类型静态补证

日期：2026-10-29。范围：只读核定 J273 的 Grid RTTI、原生 GridFactory type 分流、Grid 公共字段来源及 e2e-06 attempt 115/capture 43 的未知 Grid 对象。没有改生产代码、layout、IDB，也没有启动游戏、执行测试或哈希操作。

原始结构化材料见 [evidence.json](../../../artifacts/repentogon/tx-04-08-grid-static/evidence.json)；完整函数反编译与代码字节见 [functions.md](../../../artifacts/repentogon/tx-04-08-grid-static/functions.md)。前者逐类记录 vtable、vptr[-1]、COL、TD、CHD、BCA、BCD 的 VA/RVA、原始字节和解码字段，并包含 16 条建议 layout class rows、enum→factory map、公共字段来源及运行样本摘要。

## 结论

1. J273 当前 IDB 的 GridEntity RTTI 名称索引包含 18 张 vftable：一个基类和 17 个直接派生类。17 个派生 RTTI 都在原生 GridFactory 0x706100 的可达分支中设置对应派生 vptr；不是把 Wiki 的 enum 数量当成 C++ 类数。
2. 17 个派生类的 CHD 均含两个 BCD：自身和 GridEntity。自身 BCD 的 PMD 为 mdisp=0,pdisp=-1,vdisp=0；GridEntity base BCD 同样为 offset 0。未发现虚基、多重继承或非零 base adjustment。
3. Room::SpawnGridEntity 0x7E21D0 的正常创建路径先取 GridFactory 对象，发布 Grid 槽、写 Variant/VarData，再调用普通函数 GridEntity::Init 0x705070。所有普通 GridFactory 派生类共享这套 Grid 基类公共字段；不需要新增字段 profile。Door 的 Room::MakeDoor 0x7E4DE0 有独立的复用/创建和内联 GridInit 路径。
4. e2e-06 的 attempt 115/capture 43/tick 92 中，41 个 Grid domain 的 UnknownEntity 全部具有运行时 vptr 0x0167972C。process-1/repentogon.log:1 给出实际模块基址 0x00F60000，所以该 vptr 的 RVA 是 0x0071972C；Wall vftable 的 IDB VA 0x00B1972C 减 IDB imagebase 0x00400000 后也是 0x0071972C。这 41 个对象均为 GridEntity_Wall。旧 capture 对它们的 fields 为空；此处只确认 C++ RTTI 类，不补造 enum type、字段值、Init 或 Factory 历史。
5. 新增 RTTI 项应与通用 Grid schema 分离：schema=false 表示没有新增每类字段 profile；对 Grid domain，Classify 在严格验证该类 exact TD 和 Grid base chain 后返回既有 Class::Grid。不能因此放宽对 TD、COL/CHD/BCD、PMD、内存边界的校验。

## RTTI 地址与继承证据

静态 IDB imagebase 为 0x00400000；下表的 RVA 全部按 VA - 0x00400000 换算。地址格式为 VA / RVA。所有派生类的 COL 均 signature=0, offset=0, cdOffset=0；所有 CHD 均 signature=0, attributes=0, base_count=2。各自 BCD[0] 指向本类 TD，containedBases=1, PMD=(0,-1,0), attributes=0x40；BCD[1] 指向共同 GridEntity TD 0x00BD143C / 0x007D143C，containedBases=0, PMD=(0,-1,0), attributes=0x40。

| C++ 动态类 | vftable VA / RVA | COL VA / RVA | TD VA / RVA | CHD VA / RVA | 本类 BCD VA / RVA |
| --- | --- | --- | --- | --- | --- |
| GridEntity_Decoration | 0x00B196CC / 0x007196CC | 0x00B43CAC / 0x00743CAC | 0x00BD2214 / 0x007D2214 | 0x00B43C70 / 0x00743C70 | 0x00B43B7C / 0x00743B7C |
| GridEntity_Rock | 0x00B1969C / 0x0071969C | 0x00B43D30 / 0x00743D30 | 0x00BD14F8 / 0x007D14F8 | 0x00B43C60 / 0x00743C60 | 0x00B43B4C / 0x00743B4C |
| GridEntity_Pit | 0x00B1A004 / 0x0071A004 | 0x00B43E54 / 0x00743E54 | 0x00BD235C / 0x007D235C | 0x00B43E38 / 0x00743E38 | 0x00B43E1C / 0x00743E1C |
| GridEntity_Spikes | 0x00B1A2E4 / 0x0071A2E4 | 0x00B43E74 / 0x00743E74 | 0x00BD2390 / 0x007D2390 | 0x00B43EA4 / 0x00743EA4 | 0x00B43E88 / 0x00743E88 |
| GridEntity_Web | 0x00B1A578 / 0x0071A578 | 0x00B43F5C / 0x00743F5C | 0x00BD2430 / 0x007D2430 | 0x00B43F4C / 0x00743F4C | 0x00B43F7C / 0x00743F7C |
| GridEntity_Lock | 0x00B19F14 / 0x00719F14 | 0x00B43DD0 / 0x00743DD0 | 0x00BD2328 / 0x007D2328 | 0x00B43DE4 / 0x00743DE4 | 0x00B43DF4 / 0x00743DF4 |
| GridEntity_TNT | 0x00B1A32C / 0x0071A32C | 0x00B43EEC / 0x00743EEC | 0x00BD23C4 / 0x007D23C4 | 0x00B43EDC / 0x00743EDC | 0x00B43EB4 / 0x00743EB4 |
| GridEntity_Fire | 0x00B1978C / 0x0071978C | 0x00B43C14 / 0x00743C14 | 0x00BD21E0 / 0x007D21E0 | 0x00B43C9C / 0x00743C9C | 0x00B43BF8 / 0x00743BF8 |
| GridEntity_Poop | 0x00B1960C / 0x0071960C | 0x00B43B68 / 0x00743B68 | 0x00BD1374 / 0x007D1374 | 0x00B43D18 / 0x00743D18 | 0x00B43ADC / 0x00743ADC |
| GridEntity_Wall | 0x00B1972C / 0x0071972C | 0x00B43BA8 / 0x00743BA8 | 0x00BD224C / 0x007D224C | 0x00B43C50 / 0x00743C50 | 0x00B43C80 / 0x00743C80 |
| GridEntity_Door（已有） | 0x00B1966C / 0x0071966C | 0x00B43CF4 / 0x00743CF4 | 0x00BD15C0 / 0x007D15C0 | 0x00B43D80 / 0x00743D80 | 0x00B43B30 / 0x00743B30 |
| GridEntity_TrapDoor | 0x00B1A484 / 0x0071A484 | 0x00B43F10 / 0x00743F10 | 0x00BD23F8 / 0x007D23F8 | 0x00B43F00 / 0x00743F00 | 0x00B43F30 / 0x00743F30 |
| GridEntity_Stairs | 0x00B2E758 / 0x0072E758 | 0x00B45060 / 0x00745060 | 0x00BD3238 / 0x007D3238 | 0x00B45074 / 0x00745074 | 0x00B45038 / 0x00745038 |
| GridEntity_Gravity | 0x00B2E6DC / 0x0072E6DC | 0x00B45024 / 0x00745024 | 0x00BD3200 / 0x007D3200 | 0x00B45014 / 0x00745014 | 0x00B44FF8 / 0x00744FF8 |
| GridEntity_PressurePlate | 0x00B195DC / 0x007195DC | 0x00B43D50 / 0x00743D50 | 0x00BD2280 / 0x007D2280 | 0x00B43D08 / 0x00743D08 | 0x00B43BBC / 0x00743BBC |
| GridEntity_Statue | 0x00B1963C / 0x0071963C | 0x00B43DA4 / 0x00743DA4 | 0x00BD22F4 / 0x007D22F4 | 0x00B43B98 / 0x00743B98 | 0x00B43CD8 / 0x00743CD8 |
| GridEntity_Teleporter | 0x00B196FC / 0x007196FC | 0x00B43D90 / 0x00743D90 | 0x00BD22BC / 0x007D22BC | 0x00B43BE8 / 0x00743BE8 | 0x00B43D64 / 0x00743D64 |

根类的独立证据仍是已有 GridEntity vftable 0x00B1975C / 0x0071975C、COL 0x00B43B04 / 0x00743B04、TD 0x00BD143C / 0x007D143C、CHD 0x00B43BD8 / 0x00743BD8、唯一 BCD 0x00B43C28 / 0x00743C28。Grid 根不是 GridFactory 的实际返回类；工厂所有返回分支都覆写成派生 vtable。

## Factory：枚举值到 C++ 类

函数证据见 [IDA 原文与原始代码字节](../../../artifacts/repentogon/tx-04-08-grid-static/functions.md)。0x706100（RVA 0x306100）比较 a1 <= 0x1B 后作 switch；每个构造分支调用 GridEntity 基构造，写派生 vptr。共同尾部 0x706636 写对象 +0x04=type，0x706639 写 +0x24=GridIndex。object_domain=Grid 下的文件 schema 是 Grid，不应把下面 27 个 enum case 当 27 个类。

| enum type | Wiki 名称 | Factory 实际 C++ 类 |
| --- | --- | --- |
| 0 | NULL | 无对象（default 返回 null） |
| 1 | DECORATION | GridEntity_Decoration |
| 2, 3, 4, 5, 6, 22, 24, 25, 26, 27 | ROCK, ROCKB, ROCKT, ROCK_BOMB, ROCK_ALT, ROCK_SS, PILLAR, ROCK_SPIKED, ROCK_ALT2, ROCK_GOLD | GridEntity_Rock（十个 enum 值共用一个派生类） |
| 7 | PIT | GridEntity_Pit |
| 8, 9 | SPIKES, SPIKES_ONOFF | GridEntity_Spikes（两个 enum 值共用一个派生类） |
| 10 | SPIDERWEB | GridEntity_Web |
| 11 | LOCK | GridEntity_Lock |
| 12 | TNT | GridEntity_TNT |
| 13 | FIREPLACE（Wiki 标记未使用） | GridEntity_Fire（native factory 有 case 13，构造并返回 Fire） |
| 14 | POOP | GridEntity_Poop |
| 15 | WALL | GridEntity_Wall |
| 16 | DOOR | GridEntity_Door |
| 17 | TRAPDOOR | GridEntity_TrapDoor |
| 18 | STAIRS | GridEntity_Stairs |
| 19 | GRAVITY | GridEntity_Gravity |
| 20 | PRESSURE_PLATE | GridEntity_PressurePlate |
| 21 | STATUE | GridEntity_Statue |
| 23 | TELEPORTER | GridEntity_Teleporter |

Factory 无 case 0，其 default 返回 null；无符号比较将 a1>27（也包括负 signed 输入的位型）送入 default。enum 名称只用于索引对照，类型分流来自目标 IDB 分支和每分支实际写入的 vtable 地址。Door type 16 是 Factory 映射；MakeDoor 还有独立的 door-slot 创建/复用路径。Enum→类是多对一映射，C++ RTTI 类不能反推出未知对象原始 enum，除非当时字段或 Factory 参数有直接证据。

表中名称按用户提供的 [GridEntityType 枚举索引](https://cuerzor.github.io/IsaacDocs/rep/enums/GridEntityType.html) 记录；该页仅作 enum 名称索引，不作为内存布局或 Factory 分支证据。

## Grid 公共字段与读取边界

GridFactory 各分支都会调用 GridEntity common constructor 0x704FB0（RVA 0x304FB0）。该函数写公共描述字段默认值和四个 RNG word；派生构造的额外尾部字段与下面的公共 Grid profile 分开。Grid 基对象在现有 layout 中为 0x154 字节；所有新类的完整 C++ object size 此次标 UNKNOWN，另保存 Factory 的 operator new 请求值供复核，不把申请长度冒称 C++ sizeof。

| Grid 字段 | 偏移 | 初始化/赋值来源 |
| --- | --- | --- |
| type | +0x04 | common ctor 清零；Factory 共同尾部写请求 type |
| variant | +0x08 | common ctor 清零；SpawnGridEntity 在 0x7E2257 写入 |
| state | +0x0C | common ctor 清零 |
| spawn_count | +0x10 | common ctor 清零；GridInit 0x70508D 递增 |
| spawn_seed | +0x14 | common ctor 写 0x16A9DE81；GridInit 0x705079 写 seed 参数 |
| var_data | +0x18 | common ctor 清零；SpawnGridEntity 在 0x7E225D 写入 |
| initialized | +0x1C（byte） | common ctor 清零；GridInit 0x705090 写 1 |
| variable_seed | +0x20 | common ctor 写 0x16A9DE81 |
| grid_index | +0x24 | Factory 共同尾部 0x706639 写 index |
| spawn_frame | +0x28 | GridInit 0x705094 读取 Game +0x25A70，0x70509A 写入 |
| RNG word0 | +0x2C | common ctor 0x70500A 写 0xAA17414F |
| RNG word1/2 | +0x30/+0x34 | common ctor 0x705005 以 8-byte 写 {5,9} |
| RNG word3 | +0x38 | common ctor 0x705011 写 7 |
| collision_class | +0x3C | common ctor 不写；GridInit 0x70507E 置 0 后于 0x705085 调虚函数 slot +0x28，此后值依具体动态类行为 |

公共原始字段覆盖 [object+0x04, object+0x40)；坐标 0810/0811 由已确认 GridIndex 与 Room width 计算，不是对象内字段。上述 GridInit 字段写入不意味着 Factory 返回就拥有读取资格：SpawnGridEntity 在 native Init 正常返回并成功完成 owner 后才是已理解的完成路径。

当前 CopyObject 对所有 Grid domain 共用以下读取条件：Classify exact TD 与完整 base chain 校验通过、+0x1C initialized != 0、对象 +0x24 grid_index 等于本次 Grid 槽来源索引且 <448。随后读取现有 Grid 字段 schema；这不为 FIRST_OBSERVED 伪造构造或 Init 历史，也不改变 +0x3C 的动态最终值语义。Door 的 MakeDoor 内联相同基字段初始化；它的 Grid/door 双来源仍是一个对象身份。

## e2e-06 41 个 UNKNOWN_CLASS 的核定

采样位置为 artifacts/repentogon/tx-04-08-e2e-06/runtime-world/primary/captures.jsonl 中 attempt_seq=115, capture_seq=43：

- authority_tick=92, reason=UNKNOWN_CLASS；capture 有 46 个对象，其中 Grid domain 为 44 个：3 个原来已分类的 Grid 与 41 个 UnknownEntity。这 41 个的 vptr 全部是 0x0167972C。
- artifacts/repentogon/tx-04-08-e2e-06/process-1/repentogon.log:1 记实际模块基址 0x00F60000。0x0167972C - 0x00F60000 = RVA 0x0071972C。
- IDB Wall vtable 0x00B1972C - IDB imagebase 0x00400000 = RVA 0x0071972C。其 COL/TD/CHD/BCD 完整链都在上表，且 Wall→GridEntity 是 offset-0 直接继承。
- 因而实际 RTTI 为 GridEntity_Wall。拒绝原因是 collector 目前的 classes 白名单只含 Grid 根和 Door（以及 Entity 七类），不是 Factory type 或内存地址不合法。Classify 对结构通过但 exact TD 不在表里的对象仍回 Unknown；Grid domain 的 CopyObject 随后早退，所以原 capture 的 41 个 fields 为空。
- 不从 Wall vptr 推断这 41 个的原始 Grid type=15：旧记录没有字段，也没有对应 Factory witness。新一次采集只有在 initialized/index/source gate 均通过时才可读公共字段。

## 新增 layout 行建议、size 与未知

机器可读建议行位于 evidence.json 的 layout_class_rows_additions；其中包含现有 world_layout.json 对应字段、schema=false、每个 class 的两个 base descriptor、共同 GridInit RVA、object_size=null（完整 sizeof 未知）、readable_prefix_size=0x154、Grid 公共字段结束偏移 0x40 和实际 Factory allocation_size。建议新增的 16 个 RTTI key 为 GridEntity_Decoration, GridEntity_Rock, GridEntity_Pit, GridEntity_Spikes, GridEntity_Web, GridEntity_Lock, GridEntity_TNT, GridEntity_Fire, GridEntity_Poop, GridEntity_Wall, GridEntity_TrapDoor, GridEntity_Stairs, GridEntity_Gravity, GridEntity_PressurePlate, GridEntity_Statue, GridEntity_Teleporter；现有 Grid、Door 行继续保留。

IDA Factory 分支的 operator new(N) 请求大小已保存为独立证据字段，不等于已独立证明 C++ 完整 sizeof。示例：GridEntity_Door 在 Factory 和 MakeDoor 均请求 0x2F0，当前 layout Door 行记 object_size=0x2EC，二者差 4。这里将其标为主审复核项，不擅自覆盖旧值或把分配请求当作 sizeof。Generator 当前对 object_size 有整数校验；建议的 null 哨兵需由实现明确按“完整类尺寸未知”处理，并单独使用已证 readable_prefix_size=0x154，不得用猜测数字替代。

其余 UNKNOWN：本次没有闭合 Factory 以外的所有外部/存档构造路径或异常行为；不从单次 Wall capture 推广到未来每个 Grid；不读取或命名各派生类私有字段，不证明特定 owner、Hook 实际命中、对象 ready 或执行侧完整采样。静态 RTTI 名单和 Factory 选择不代替这些边界证据。

