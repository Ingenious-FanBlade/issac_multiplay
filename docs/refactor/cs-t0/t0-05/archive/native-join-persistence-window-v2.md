> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 完整停止窗口与自然更新传播证据 v2

本版新增完整窗口规格、19个指令窗口和7项局部分支条件。**整体仍为 BLOCKED_BY_EVIDENCE；未证明逐次自然更新保持无持久化不变量。** 没有发现满足全部固定候选条件的已达污染反例；也不能把尚未闭合的调用链判为安全。原窗口v1、Init v2、plan v8及join progress v2均保留原字节。

机器证据为 [coop-native-join-persistence-window-v2.json](../../../../../cs-t0/coop-native-join-persistence-window-v2.json)，SHA-256：`8E1E345F3CEAF9F62294519C099AF18F93FCC0604D0C90C10A7D594ABFA5CA31`。本版19个窗口、408条指令和96条直接边均由只读IDA按真实指令边界提取；这些计数是提交内容，不是独立验收通过的计数。

## 1. 目标、方法与边界

目标为Win32/x86，image base `0x00400000`，image size `0x008FE000`，观察构建SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。已只读核对IDA输入路径、SHA和位数。使用反编译定位、原始反汇编、真实decode和直接xref；最终RVA及长度以机器指令为准，反编译行号不是可靠的指令入口。

没有枚举或打开游戏进程，没有启动、附加、输入、Hook、IDB修改或实机读回；没有运行独立测试。新增文件使用create-new。生产代码、二进制、封印脚本、历史证据和失败结果均不改动。

继续采用普通显式seed新起始房、Keyboard P0、唯一未分配XInput B、普通Isaac、原生初始保存已完成的候选。无mods、特殊seed effects为空、无相关外部callback、无物理gameplay/menu/reset/console输入、干净persistent及稳定owner均需机器依据，不能由场景口述代替。不得写readonly/save-enable、清统计、压制保存、删除B或扩大场景以制造通过。

## 2. 完整窗口修订

v1把窗口称为“两个自然pass”，不足以表达下一完整pass双BOUND和停止等待。本版固定以下顺序：

1. 在**首次主动publication或槽覆盖之前**完成准入；涵盖publication后到首选择pass之前的自然执行。
2. action19所在选择pass完整返回；包括该帧后续自然更新。
3. action14所在确认pass，完整constructor/Init/HUD/tail和Candidate／首次BOUND；包括后续自然更新。
4. **下一不同的完整manager pass**形成第二次连续BOUND。最少3个manager pass；两个UI edge不等于两个update，也不等于双BOUND。
5. disable、in-flight drain、generation退役、55槽恢复及terminal确认期间，所有自然更新仍在窗口内。

`0x5551BD`调用input/manager后，`onlineRoot+0x4B130`会阻止后续推进，`+0x4ABBC`奇偶再决定是否调用`0x555231 → 0x2FADC0`。因此manager pass数、GameUpdate数和墙钟时间必须分别处理。

不指定“安全秒数”或自然update上限。所需证明形式是：对窗口内每一次实际更新，入口不变量成立，全部允许调用保持不变量，且更新内部没有join所致的direct/dirty/save副作用。该归纳步骤目前未成立。停止超时、drain或terminal状态不确定时保持Unsafe及诊断现场；不得强卸载、重注入、native B exit或自动重试。

## 3. 新增局部事实

| 路径 | 已证的局部条件及结果 | 不能外推的部分 |
| --- | --- | --- |
| `0x2F4520` reset | `0x2F45DB/0x2F46A8`把Game的两个transition timer清零。 | 后续初始流程及准入时仍为零尚需传播／读回。 |
| `0x2FB92E..0x2FB9F7` | 两个timer在本块入口为0时，signed-positive分支均跳过，不增减、不发对应request。 | 后续Room/玩家/callback仍会运行；这不是整个update的归纳证明。 |
| `0x35F0E0`的item668分支 | `0x362675`可把Game+67788从非正值改为1；前有room helper和`0x34F090`条件。 | 只确认条件writer；没有证明固定候选可达此分支。Init配置/隐式来源必须排除它。 |
| `0x2EE750` pickup路径 | pickup+528=1、+2C=100、dimension=2且timer非正时，`0x2EE7AB`写90。dimension !=2可排除此特定writer。 | 不能排除函数其它效果或其它timer writer；维度持续性仍需证明。 |
| `0x2F8140` native state restore | `0x2F8786`导入source+1E7B0字节为timer；`0x2F87CC`导入source+1FD94的有符号word；`0x2F86E2`还会重算readonly。 | v1未列这处readonly重算。restore必须被生命周期闭合排除，不能仅检测地址不变。 |
| `0x438210` fade7 predicate | 内嵌Game+26778的+34 dword或+44 byte为0，helper返回false；对应Game+267AC/+267BC。 | 仅关闭此fade7入口，不能证明所有fade writer及这些字段持续不变。 |
| `0x404208 → 0x2FD7C0` | Room+8 !=16排除此特定room请求分支。 | Room更新其它路径、Grid虚调用和别名写仍未闭合。 |
| `0x5BB5D0` player manager | 每个player+173为0时，不执行`0x5BB67F → 0x3A1090`死亡相关调用。 | B未来读回不是prejoin证明，初始活着也不等于整个窗口持续。 |
| `0x2FBBA5 → 0x4607A0` | `[ [image+871690]+0 ]`指向callback bitset，首byte bit2为0可关闭本次`0x472980`dispatch。 | 一个bit不能证明无mods或全部外部callback缺席。 |

这里的item668名称来自IDA native switch-arm标记；真正冻结的是writer、邻接guards和目标地址，不把符号标签当作完整switch数据流证明。

`0x2FBA4D`在timer块之后调用Room update `0x402980`，随后`0x2FBA52`调用PlayerManager更新。Room包含`0x403BD8`的Grid `vtable+8`自然调用；其指针来自Room+24内嵌数组，迭代Room+C乘Room+10。无战斗不能推出这些元素为空或所有虚目标无副作用。另`0x403A27/+45`为entity的vtable+48/+4C谓词调用，本版不无证声称这些谓词本身写持久化；需要解析允许receiver与后续调用组合。

## 4. 给准入契约的字段交接

机器JSON的fieldSources给出owner来源、偏移、宽度和对应anchors。关键源为Game指针槽`image+871678`、online指针槽`image+87169C`；Room是`[Game+18300]`，persistent是online+14内嵌对象。所有这些指针都是32位，不能把指针槽当内嵌对象或忽略owner变化。

除v1 timer/request/fade/pending字段外，新增精确来源包括Game+1830C维度、Room+8类型、Game+267AC/+267BC的fade7直接predicate、+268A4/+268A8后继条件，online+0原生state请求及+4B2A4的state5请求。Lua owner槽为image+871690，owner+0是callback-bitset指针；此处只描述首byte bit2。

任何未来reader仍须逐段检查地址加法、溢出、宽度、读取边界、null、身份及双读漂移。Room网格布局事实**没有**冻结可接受的遍历预算或证明无危险Grid；不得因为宽高可读就遍历任意积。主准入契约必须将未完成的源或预算设为拒绝，而不是把本版fieldSources误当成完整生产reader。

## 5. 未决传播及验收交接

保留`PW-STATIC-01`：缺少从publication前到terminal的ENTRY、INIT、NATIVE_UPDATE、LIFECYCLE归纳组合。具体前沿为完整Init/tail、Room/Grid虚调用、player manager、Lua回调及native state restore。位移搜索能找到上述直接writer，但不能排除alias、bulk-copy、未知虚调用；不得把搜索无更多结果视为闭合。

保留`PW-STATIC-02`：fresh B及受影响P0的config/effective count/familiar来源尚不能组成完整direct-persistent排除。Init逆向交接还指出desired=0可删除已存在的匹配Familiar；因此不能仅依赖Player自身vector空。已证明的共同readonly setter直接复用，不重复要求无界threshold追踪。

两次dirty=0可以夹着“dirty=1 → flush清0 → 后端写出”，readonly=1也不支配全部direct save。机器JSON把这些列为**观测判据反例**，不是固定候选已经到达的污染执行。遗漏第三个pass或停止等待则是窗口覆盖反例。独立测试应区分这三种结论：局部条件支持、完整证明不足、实际候选反例。

独立实例负责磁盘PE/raw bytes、全部408条声明指令、96条direct edges、函数入口prefix decode、前驱SHA和语义反例。缺环境、未执行或覆盖不足不得PASS；逆向实例未运行该验收。所有active/live/arm/claim/override及T0-05-05、T0-05整体、T0-06权限均false，矩阵第11项不变。
