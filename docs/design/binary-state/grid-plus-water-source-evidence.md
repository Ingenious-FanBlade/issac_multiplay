# 第6步：GridEntityPlus水面覆盖的来源、ABI与生命周期

2026-11-10，布局版本54。只读官方源码、已安装官方1.1.3 DLL与J273反编译；没有启动游戏、调用目标函数或运行测试。**本项静态核定完成**，正式源为`mod/binding/state/grid.json`：新增229号[旁表元素字节表](layouts/GridEntityPlus.md)，修订199号最终水面字段的来源合同。实际绑定／采集、Client安装与旧路径迁移仍归第10／13／14步，第15步统一验收。

**关键纠错：Lock不应用旁表覆盖。** 官方只有六个水面Hook，J273却有七个不同函数族；Lock的slot6直接返回自身结果，既不进入官方六Hook，也不进入现有六个C/S wrapper。版本50“七族都应用覆盖／现有wrapper足够”的概括不成立。本轮保留该历史记录并在当前源中纠正；后续不能以六个wrapper宣称全部Grid水面应用完成。

## 1. 官方来源及对象边界

源码均在`F:/projects_isaac/REPENTOGON-1.1.3/`：

- `repentogon/Patches/GridEntityPlus.h:8—15`：虚析构＋一个`std::optional<uint32_t>`，没有Grid回指、Room键或generation。
- `repentogon/Patches/GridEntityPlus.cpp:10—39`：匿名namespace内`_gridEntityPlus[448]`；索引必须0—447，由当前`GridEntity+0x24`读取。返回静态元素的借用指针，不分配。**同索引的门、装饰及其它对象共享同一项**；不同Room也没有独立数组。
- 同文件42—50：Factory的`super`返回后reset，基类Init的`super`调用前reset。
- 同文件57—76：六个Hook先`super(out)`一次，再在optional有值时只改`out->bitFlags`。`if(optional)`检查engaged，并不是检查payload是否非零。
- `repentogon/LuaInterfaces/GridEntities/LuaGridEntity.cpp:48—74`：getter虚调slot6取得最终结果；setter把Lua整数转换为u32，写optional；Reset令optional未设置。API不因Lock无消费者而禁止设置其共享项。

不能把这张表追加进Grid公共340B、从Grid未知字节拼出holder，或用EntityPlus的分裂holder方案套用Grid。229只是旁表元素ABI目录，**不是第19个Grid RTTI、不是独立成员、不是要发送的伴随状态**。

## 2. 已安装官方DLL的静态ABI

只读文件：`F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/Repentogon/zhlREPENTOGON.dll`。PE32/i386，首选ImageBase`0x10000000`。导出`__REPENTOGON_VERSION`的RVA为`0x61C6D0`，其本地指针指向字符串`1.1.3`；下表全部使用**该DLL的RVA**，不能加到J273 EXE基址上。版本字符串不保证其它重新编译产物具有相同RVA，未识别的模块布局不得猜读。

| 元素／入口 | 核定ABI与静态依据 |
| --- | --- |
| 旁表范围 | `module+0x62E650`至`module+0x62FB50`（后界不含），448×12＝0x1500B，官方DLL可写`.data`内的静态数组 |
| slot+0，4B | 本地vptr；ctor`261C00`写`module+5112C4`，正常本地虚析构；不上线，不借此调用析构 |
| slot+4，4B | optional的u32 payload；仅engaged时读取。setter`1174A8`写DWORD，水面消费者`261CEF`等读DWORD |
| slot+8，1B | engaged bool；ctor`261C08`、Reset API`1173B6`清BYTE；handle`261CE6`先比较BYTE再读payload。有效零仍为engaged=1 |
| slot+9—B，3B | optional尾部对齐填充：官方类只有上述成员；MSVC`optional`的4B值＋1B标志按4对齐，数组stride12。setter`1174AB`会从临时值连写标志及后三字节，但消费者只读BYTE+8，不能将整个DWORD视为标志或状态 |
| `GetGridEntityPlus(GridEntity*)`，`261C90` | `__cdecl`；`[ebp+8]`为Grid指针，读+24，unsigned比较`1BFh`，两次LEA算12×index，再加表基址；EAX返回slot或null，普通`ret`由调用方清栈。Lua setter`117495`、Reset`1173AA`都调用它 |
| 匿名index重载，`261C70` | cdecl，栈上传u32 index，`cmp eax,1BFh; ja`，同样按12寻址、返回借用指针 |
| helper，`261CC0` | cdecl `(GridEntity*, WaterClipInfo*)`，只在合法索引且BYTE+8非零时复制DWORD+4至out+0；不分配、不运行Grid逻辑 |
| CRT构造／退出，`2A450`／`4CFCF0` | 向数组构造／析构helper传入表地址、stride`0C`、count`1C0`及正常ctor/dtor，证明静态448项与整个模块生命周期 |

官方DLL导出表没有`GetGridEntityPlus`、旁表地址或handle。不能假设`LIBZHL_API`已导出它，不能直接从libzhl导入，更不能把官方`.cpp`另编进C/S来取得“同名”函数：那会建立第二张不相干的数组。

第10步采用**官方模块当前映射＋上述已核RVA的有界原始读取**，不调用helper／Lua getter。模块绑定必须来自本地已识别的官方模块及其实际代码／数据关系；不能把旧进程地址、同名新编译DLL或EXE基址当作表来源。绑定／读失败均为具名Gap，不是“没有覆盖”。现有布局元数据没有自行实现该运行绑定。

## 3. 生命周期与合法读取owner

| 时机 | 官方行为 | C/S职责 |
| --- | --- | --- |
| DLL CRT完成 | 正常构造448个slot，engaged=false；静态存储本身先零初始化 | 模块初始化完成、生命周期稳定后可借用；不能在CRT未完成或loader teardown期间采集 |
| `Isaac::CreateGridEntity` | 官方Hook`261D50`先call super`261FD0`；返回非空且索引合法才清共享项，随后返回对象 | 等完整Hook链正常返回，不能在原生body刚返回时就发布来源；同index其它实例的override也会被清掉 |
| `GridEntity::Init` | 官方Hook`261DA0`在call super`261FE0`前清共享项，super内或外层mod仍可能重新设置 | Init进入就撤销旧借用／未完成窗口；在整链完成后的当前值重新采集，不能假设最终一定Null |
| Lua Set | 转换为完整u32，写payload并令engaged=1 | Host正常命令；同index各适用类立即共享覆盖，不按对象单独记一份来源 |
| Lua Reset | engaged=false；Reset API只写BYTE+8，不承诺清payload | 无覆盖恢复各自原生规则；不是最终flags=0，不能读取旧payload充当值 |
| Free／Remove／Destroy、换房、退出run或C/S停用 | 此官方实现没有这些清理Hook；表存储继续存在，直到后来的Factory／Init／Set／Reset或模块退出 | 撤销C/S自己的room/use/window借用；不伪造官方清表历史，不为“清理网络状态”调用Reset或析构slot |
| 模块退出 | 正常静态数组析构，存储失效 | 先停读取／撤本地绑定；不能继续使用旧module或slot地址 |

源码中只有Factory／Init调用`ResetGridEntityPlus`。实际机器实现`261D10/261D50/261DA0`可能顺带清payload及填隙，而公开Reset只清engaged；共同语义只有“未设置”，读者不得依赖这些附带字节。不要把这三条reset规则合并成“每个对象析构时自动清”。

正式读取owner是**同一主线程ordinary完整采集窗口中的当前Grid记录＋仍存活的官方模块静态数组**：先证明当前对象/use、实际RTTI／slot6族、Room成员及门别名与index关系，再按index取得slot。六族先有界读取engaged并校验0/1，仅为1时读取payload；未设置时不读payload或后三字节。所有来源和窗口在提交前保持稳定；读取中不调用目标代码、不允许Lua／回调重入，避免同index的Factory／Init或setter插入窗口。失去窗口／生命周期／来源就撤销结果，不能用前后地址碰巧相同掩盖复用。

旁表在模块CRT已有正常构造，不要求每项都额外见证一次Lua setter才能读取；Grid根本身仍遵守原有资格。跨房时撤销借用，不凭旧room缓存推断官方当前值，也不要求人为重放Init来取得资格。Lock当前展示不读旁表，不能因它有一个不被消费的optional而增加来源依赖。

## 4. 七个真实函数族与最终结果

以下为**J273 EXE VA**（ImageBase`0x400000`），与上一节DLL RVA分开。所有七族都写完整`WaterClipInfo`：+0 bitFlags，+4 float=0，EAX返回调用方提供的8B out。

| 实际slot6族 | 原生bitFlags | 官方Plus覆盖 |
| --- | --- | --- |
| `705250`：Grid、Statue、Wall、Fire、Spikes、TNT、Gravity | 57 | 有engaged时替换完整u32 |
| `70F120`：Pit、Teleporter、TrapDoor、Web、Stairs、Decoration | 38 | 同上 |
| `70C360`：Door | 0 | 同上 |
| `95E940`：PressurePlate | 25 | 同上 |
| `711620`：Poop | state==1000 ? 38 : 57 | 同上 |
| `718650`：Rock | state==2 ? 38 : 57 | 同上 |
| `70CCE0`：Lock | collision==0 ? 38 : 57 | **不应用**；实际虚表`B19F2C`指向该函数，整个函数没有call，直接写out两个字后`ret 4` |

基类getter的SDK thunk为thiscall：ECX=this，栈上传out，`call [vtable+24]`，`ret 4`；不要因四个恒量函数没有使用ECX就把整个虚调ABI改成一般stdcall。原生Factory是ECX=type、EDX=index；官方Hook侧cdecl由HookSystem桥接，不能据此按栈参直接调用J273工厂。

对于六族，`final = engaged ? value : nativeDefault`；对于Lock，`final = nativeDefault`。先按真实RTTI核定函数族，不用可变desc.type冒充虚表身份。未知或额外修改的函数链没有证据时保持Gap；本合同不冒充对任意第三方原生Hook的通用重演。

同index的Door／Decoration在未覆盖时分别为0／38，有覆盖0时两者都为0；同index的Lock仍由自己的collision决定。故Host可以在一个窗口内复用一次slot读取，但**最终缓存必须按实例/use区分**，不能只存一份index→final。若Reset后各自默认重新出现，应发布最终值的变化；engaged／payload改变却使final不变时，不因此单独发块。

## 5. 同步与排除

| 内容 | 归属与决定 |
| --- | --- |
| 每个Grid当前最终bitFlags | **同步，完整u32，4B**，沿用G0的synthetic `water_clip_flags`；包括0及高位，不增加字段／块。它不是Grid+3C，也不是8B WaterClipInfo原件 |
| nullable的engaged与payload | Host来源输入；**不直接同步**。六族在Host选择最终值，Lock不消费；Client不模拟未来Reset／Init／碰撞变化，不需要还原optional |
| `WaterClipInfo+4` float | 七族恒零且官方Hook不改；由Client原链派生，**不同步**，不扩成额外4B状态 |
| Grid+3C collision、Poop／Rock state | collision只留Host合成Lock结果；state还有独立绘制消费者，仍按原布局同步。不能因为水面已有最终结果而删除真实绘制state，也不能恢复collision下行 |
| vptr、表／slot／out地址、模块句柄 | **不能跨进程同步或写回**；各自本地生命周期与ABI，不能成为网络身份 |
| 未设置payload、optional尾部3B、整个12B元素／整表 | **不能复制成状态**；未设置值无读取资格，填充无语义，整表还会混入不属于当前Room成员的项与本地虚表 |
| Set／Reset命令、Factory／Init／析构／重置历史 | Host玩法／生命周期与本地来源事实；**不回放、不下发**。Client只安装当前结果，不能通过API或重演Init“恢复旁表” |

这与EntityPlus的既有nullable传输合同不同：Grid已有最终结果的per-instance拦截方案及恒零第二分量，不能仅因两个类有同名optional就复制EntityPlus的线上u64。

Client第13步将最终u32安装到自身owner，按真实实例／use读取；本地+3C先由构造owner初始化中性0，保持原链调用的已定义输入。**现有六wrapper不足，必须通过统一Hook清单补Lock `70CCE0` 第七点**；每条原链一次，返回前覆盖bitFlags，out+4保留原链零。不能通过给Lock安装Host collision、伪装为其它类、手改虚表或重写官方共享旁表补缺。尚未接入第七点时不能宣称Lock应用／ACK完成。

候选构造也会经官方Factory重置同index的本地旁表，这是真实副作用；因此最终结果需存在C/S自有per-instance owner，并在发布前齐备。仅在隔离的C/S Client世界生效，Host／Off及非C/S实例原样透传。逻辑停用撤本地结果，Hook常驻；不能为退出C/S清空官方整表或影响原版／Isaac Sync。退休先撤可消费结果与绘制借用，drain后正常退休Grid；不析构官方静态slot。

## 6. 后续实施与唯一整体验收

- 第10步：实现本地模块／表绑定、有界条件读取与同窗合成；无分配、Lua、getter或RNG调用。Lock跳过旁表，读Host collision，完整u32进入正式typed view。
- 第13步：补Lock第七覆盖点与各实例结果安装，保证原链一次、+4为零、错use／未齐备不ACK；官方旁表不承载下行状态。
- 第14步：同时迁移旧JSON的water<=255限制与getter全量回验；不能因新布局已有u32就宣称旧路径已支持高位或Lock。当前旧运行路径尚未切换。
- 第15步才执行：七族、engaged未设／覆盖0／高位u32／Reset后恢复；Lock设置覆盖无效但同index其它适用类有效；门／装饰／Lock同index的不同默认、Factory与Init对所有别名的重置、Init内重新Set、Remove／换房不假定自动Null、复用／停用／退休撤本地借用。覆盖非法索引、模块缺失／错误来源、非0/1 engaged、读失败／中途失效、错use不ACK及原链一次。只对最终展示值要求一致，不构造optional／填充的线上负例或原值相等要求。

本轮静态核定不改变任何Grid根UNKNOWN或最小状态大小。229的12B全部有归属，其中3B有源码／编译器／机器寻址与消费者依据归对齐填充；不能借此把其它Grid／ANM2的既有UNKNOWN认成填充。第6步完整119项逐行回映射仍未完成。

生成／纯编译记录：首次生成因229新条目缺少schema必需的`conversions`／`validators`空表失败；补齐显式空表后，`cmake --build build/repentogon --config Release --target isaac_cs_state --parallel 2`正常生成四份state表与布局文档，并产出`isaac_cs_state.lib`。该构建配置`ISAACCS_BUILD_E2E_TOOLS=OFF`，没有运行测试或实机操作。生成229为12B原生、0B UNKNOWN、0块／0字段；199及全部派生的`water_clip_flags`仍为4B synthetic Convert，没有把旁表挂成伴随线负载。
