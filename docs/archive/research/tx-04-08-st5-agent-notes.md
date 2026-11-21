# tx-04-08 ST5：目标 RTTI、分类与身份边界

日期：2026-10-26。工作范围：对 J273 目标的 Entity/Grid RTTI、继承、slot 0 删除包装、析构和 EntityFactory 池取得路径做只读静态核对。原始结果保存在 [st5-agent artifacts](../../../artifacts/repentogon/tx-04-08-static/st5-agent/)。没有修改 IDB，没有运行构建、测试、校验脚本或游戏。

证据标签：**STATIC_CONFIRMED** 是目标 IDB 的表/指令；**CURRENT_SOURCE** 是 SDK 或旧原型源码；**DESIGN / 推导** 是受静态事实约束的身份规则；**UNKNOWN** 是本次未闭包路径。IDB 输入名为 isaac-ng-v1.9.7.12.exe.unpacked.exe，imagebase 0x400000，x86；实际内存地址应以进程当前模块基址加 RVA 读取。只读 py_eval 记录的 IDB 段为 .text 0x401000–0xAAA000、.idata 0xAAA000–0xAAA7C8、.rdata 0xAAA7C8–0xB87000、.data 0xB87000–0xC04000。

## 1. RTTI 与七类身份表

**STATIC_CONFIRMED**：从 vtable 前一 word 读 COL，再由 COL 读 TD/CHD，并沿 CHD 的 BCD 数组检查各基类。下表地址为目标 IDB VA；TypeDescriptor 同时列出 VA/RVA。Entity 七类的 CHD 都有本类与 Entity 两项，Entity 位于 offset 0；BCD 的 mdisp=0、pdisp=-1、vdisp=0、attributes=0x40。因此可在当前支持集合里按精确目标 TD 和基图分类，不需要调用实体方法。

| 类 | vtable VA / RVA | COL VA | TD VA / RVA | slot 0 Free VA | slot 1 Init VA | CHD 基类 |
| --- | --- | --- | --- | --- | --- | --- |
| Entity | 0xB18224 / 0x718224 | 0xB439E4 | 0xBD1244 / 0x7D1244 | 0x6A3EC0 | 0x6A4A20 | Entity |
| Player | 0xB1BDB4 / 0x71BDB4 | 0xB44010 | 0xBD13A8 / 0x7D13A8 | 0x74E970 | 0x74F6A0 | Player → Entity |
| NPC | 0xB187D0 / 0x7187D0 | 0xB43A08 | 0xBD1344 / 0x7D1344 | 0x6B4870 | 0x6B4A50 | NPC → Entity |
| Familiar | 0xB15090 / 0x715090 | 0xB4373C | 0xBD1310 / 0x7D1310 | 0x6070C0 | 0x607AA0 | Familiar → Entity |
| Effect | 0xB14D00 / 0x714D00 | 0xB43618 | 0xBD1270 / 0x7D1270 | 0x5CCF30 | 0x5CD070 | Effect → Entity |
| Pickup | 0xB18E8C / 0x718E8C | 0xB43A80 | 0xBD146C / 0x7D146C | 0x6DD670 | 0x6DD7C0 | Pickup → Entity |
| Tear | 0xB15FB8 / 0x715FB8 | 0xB43844 | 0xBD152C / 0x7D152C | 0x662DF0 | 0x662F50 | Tear → Entity |
| Projectile | 0xB15B1C / 0x715B1C | 0xB437E4 | 0xBD13D8 / 0x7D13D8 | 0x64F470 | 0x64F4B0 | Projectile → Entity |
| GridEntity | 0xB1975C / 0x71975C | 0xB43B04 | 0xBD143C / 0x7D143C | 0x705020 | 0x408030 | GridEntity |
| GridEntity_Door | 0xB1966C / 0x71966C | 0xB43CF4 | 0xBD15C0 / 0x7D15C0 | 0x706730 | 0x408030 | Door → GridEntity |

本次还只读抽查了 Bomb（TD RVA 0x7D149C）、Slot（0x7D20B0）、Knife（0x7D1590）、Laser（0x7D140C）、Dummy（0x7D2164）和 Entity_Text（0x7D2030）虚表；它们各自 CHD 都包含 Entity 基类。这六类不在七类专用 schema 内，应成为带原因的 UnknownEntity，不能猜成 NPC。抽查清单不是全部派生类名录，完整本轮读取值见 RTTI 原始 JSON。

**CURRENT_SOURCE**：REPENTOGON SDK 的 libzhl/functions/Entity.zhl 声明 Entity 与七类派生类（Entity 虚接口约在第 254 行，NPC/Familiar/Pickup/Player 定义分别约在第 112/76/90/613 行；Effect/Tear/Projectile 类型定义约在各文件前部）；GridEntity.zhl:25 与 GridEntityDoor.zhl:7 是独立的 Grid 根及 Door 子类。SDK 中的 skip 标记本身不证明目标 vtable 是否覆盖；上表 slot 0/slot 1 地址来自目标 IDB，且七类 slot 1 均不同于 Entity::Init。

**分类规则（DESIGN）**：

1. 只对可读、对齐且有界的对象地址读取 vptr；从 vptr-4 读取 COL，再逐级验证 COL、TD、CHD、BCD 地址、签名、计数和范围。只比较当前目标登记的 TD RVA 与精确的基类关系。
2. 确认 Entity 基类后，且精确匹配七个专用 TD 之一，才分别输出 Player、NPC、Familiar、Effect、Pickup、Tear 或 Projectile。Entity 基类已确认但专用 TD 不匹配时输出 UnknownEntity 和原因为何不支持；Entity 基类也未确认时不套 Entity 头布局。
3. Grid 使用单独的 Grid domain。GridEntity_Door 属于 GridEntity 继承图，不进入 Entity HP、flags 或 Entity 析构规则。
4. 不以 Factory 的类型分支、Entity Type 数值区间、Variant/Subtype、getter、Lua 或 dynamic_cast 分类。目标虚表/RTTI 证据缺失、冲突或超出白名单时保留 Unknown 与原因，禁止猜测。

**旧原型限制**：cs-t0/native/src/world_record.cpp:144–170 的 HasBase/Classify 使用旧 TypeDescriptor 偏移（含 0x85D6DC、0x85D6AC、0x85D900、0x85D70C、0x85D7D4、0x85D678、0x85D5AC，Grid 为 0x85D7A4），在 imagebase 0x400000 下与 J273 的新 TD RVA（七类为 0x7Dxxxx）不匹配；旧代码的固定映像跨度 0x8FE000 也不等于本目标 IDB 已加载段。不得把旧常量或旧范围复制为当前读边界。

## 2. 析构、删除包装与池取得

下表 slot 0 是该类型自己的删除包装；派生对象常规析构路径中会调用 Entity 基析构 0x6A3EF0，但该事实只限于静态可见的正常路径。

| 类 | slot 0 删除包装 | 正常 Entity 析构路径 |
| --- | --- | --- |
| Entity | 0x6A3EC0 | 直接调用 0x6A3EF0 |
| Player | 0x74E970 → Player dtor 0x74F260 | 0x74F67F: call 0x6A3EF0 |
| NPC | 0x6B4870 → NPC dtor 0x6B48A0 | 0x6B4A2D: call 0x6A3EF0 |
| Familiar | 0x6070C0 → Familiar dtor 0x6070F0 | 0x6071C9: call 0x6A3EF0 |
| Effect | 0x5CCF30 | wrapper 内清理后 0x5CCF97: call 0x6A3EF0 |
| Pickup | 0x6DD670 → Pickup dtor 0x6DD6A0 | 0x6DD7A3: call 0x6A3EF0 |
| Tear | 0x662DF0 → Tear dtor 0x662E20 | 0x662F2F: call 0x6A3EF0 |
| Projectile | 0x64F470 | 0x64F47C: call 0x6A3EF0 |
| GridEntity | 0x705020 | Grid 自有清理，不调用 Entity dtor |
| GridEntity_Door | 0x706730 | Door/Grid 成员清理，不调用 Entity dtor |

Entity::Free 0x6A3EC0 在 0x6A3EC6 调 0x6A3EF0，随后测试参数 bit 0（0x6A3ECB/0x6A3ECF）；置位时才调用 allocator wrapper 0xA8717B（size 0x410，调用点 0x6A3ED7）。派生删除包装同样区分析构与 bit 0 释放分支，例如 Player 0x74E970 → 0x74F260 后检查 bit、NPC 0x6B4870 → 0x6B48A0、Tear 0x662DF0 → 0x662E20；slot 0 不是单一基类包装。

Grid 的 0x705020 清理 Grid 成员并在 flag 条件下调用 allocator；Door 的 0x706730 清理多个 Door 子成员、重置 Grid 基类 vptr，再清理 Grid 成员，二者均不进入 0x6A3EF0。因此 Entity 基析构 Hook 会漏掉 Grid/Door 的释放路径；反之，Grid/Door wrapper 不能作为 Entity 析构观察。

### EntityFactory::Create

**STATIC_CONFIRMED**：0x6EC540（RVA 0x2EC540，函数大小 0xF49）反编译显示按池数组步长/类型分支选槽。以其中 n9001 == 1000 路径为例，0x6ED079 检查池项偏移 +0x170 的 byte；发现空槽就返回该槽地址（正常分支末端 0x6ED1A5），取得过程未见在这个 Factory 路径中调用构造/析构或 allocator。饱和时若参数 a3 非零，0x6ED09E 返回 null；这里仅记录原始参数，不把其语义臆称为 force。Entity_Player 的 n9001==1 分支在 0x6EC575 记录不能生成并返回 null。

池覆盖路线在其他类型池中可见：满池时查 vtable +0x38 的 CanOverwrite，必要时扫描；随后设置池项 +375 临时 byte，调用 vtable +0x28 Remove（如 0x6ED44D）和 +0x44 ClearReferences（0x6ED45E），复位标志并返回同一池地址（0x6ED473）。扫描无可覆盖项的路径会记录“don't overwrite”警告，但仍继续覆盖所选槽；因此 CanOverwrite 不能被描述成严格阻止覆盖的保证。直接取空池项与这条覆盖路径都说明 Factory 取得不必经过对象析构。地址偏移、pool stride 与各类型分支细节见完整 decompile 原文。

**身份边界（DESIGN / 推导）**：

- 一次已确认的非空 Factory 返回可登记该池槽的一次新的逻辑 use；同一地址被重新返回时递增 use_generation，不要只等析构来判断复用。null 不建立 use，也不构成出生。
- Factory 返回不证明物理对象新分配或原对象已析构；不因同址或 Factory 返回单独创建新的物理 local_id。构造、owner 外层 Init/Publish 正常完成等证据须独立闭合；真实出生事件和完整创建边界交由 ST6。
- Init 的 slot 1 已显示七类都使用各自派生函数，Grid Init 也是独立函数。只 Hook Entity::Init 0x6A4A20 不能覆盖七个派生 Init 入口；派生 Init 正常返回仍不能替代 Spawn/Morph/Restore/Replace 外层 owner 完成。不得把重复 Init 直接当出生。
- Remove、ClearReferences、离开任一来源、基类析构、slot 0 wrapper 返回、allocator 调用分别记录；它们不是同一个销毁事件。析构过程中 Entity vptr 会重置为基类，不能把拆解中的 Entity vptr 重新分类成新 Entity。
- 本地临时身份用 (recording_generation, domain, address) 做当前窗口内关联，同时记录 local_id、use_generation、birth_event、revision 与所有 source/index membership。不同 domain 不合并；地址复用未闭合时引用保持 ambiguous/unresolved。

## 3. Grid/Door 物理别名

**STATIC_CONFIRMED（结合主审 ST4 原始证据）**：GridEntity 与 Entity 是不同 RTTI 根；Door 仅派生自 GridEntity。MakeDoor 路径在 0x7E4E21 读取 doorSlot→gridIndex 映射，0x7E4F7B 写 Room Grid 槽，0x7E4FE7 把同一 Door 指针写 door 槽。故同一 Grid 域地址可以同时带 (Grid,index) 与 (door,slot) membership；doorSlot 不是第二个对象身份。引用主审 main-sources-door-restore-replace.json 和 main-grid-free.json，Agent 原始查询则含 Grid/Door RTTI 与两个 slot 0 wrapper。

仅当观察到目标槽被清空、door alias 变化或 Free 请求时，分别更新相应来源/生命周期证据；请求和单一 slot 变化不能证明所有 alias 都消失，更不能单独证明物理释放成功。

## 4. 未闭合路径与风险

- **UNKNOWN：** 本次静态正常路径不证明所有构造、Init、恢复/替换、外部 Factory 调用、Pool owner 和源槽变化都已穷尽；调用者正常完成、Publish 与 Morph 的出生边界归 ST6。
- **UNKNOWN：** Hook 是否注册、Hook priority/共享链最终顺序、基/派生 Hook 覆盖完整性、运行 callback 状态不由 IDB vtable 证明，归 ST8。
- **UNKNOWN：** 异常、SEH、强制退出或第三方 subclass/mod 改写 vptr 的路径；各类 Free wrapper 返回后 allocator 的运行结果、真实物理释放时刻及线程安全未由静态反编译闭包。
- **UNKNOWN：** 固定 TD 名单只支持当前目标版本与明确白名单；未知 TypeDescriptor、损坏元数据、非零基类调整或多重继承不得按已支持布局读取。当前七类虽然均为 offset-0 单继承，不能外推到未知类。
- Grid/Door 的 alias 关系不推出“释放即物理销毁”；保留 decoration / 空槽与其他 Grid 删除分支须按主审 GridFree 原始证据分别处理。

## 5. 原始证据索引

- [RTTI/COL/CHD/BCD、slot 0/1、段和输入上下文](../../../artifacts/repentogon/tx-04-08-static/st5-agent/rtti-col-chd-vtables.json)
- [Entity 与派生/Grid/Door 删除包装的反编译、指令、常量](../../../artifacts/repentogon/tx-04-08-static/st5-agent/free-wrapper-analysis.json)
- [Factory 全部反编译与 xref 结果](../../../artifacts/repentogon/tx-04-08-static/st5-agent/factory-decompile.json)
- [基/派生析构反编译与直接 call 0x6A3EF0 的 disassembly](../../../artifacts/repentogon/tx-04-08-static/st5-agent/derived-dtor-evidence.json)
- [查询口径和证据文件说明](../../../artifacts/repentogon/tx-04-08-static/st5-agent/query-notes.md)
