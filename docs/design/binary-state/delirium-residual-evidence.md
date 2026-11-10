# 第4步：Delirium未决事项的静态处理

日期：2026-11-09（Asia/Singapore）。布局版本46。范围为第4步原未决清单的Delirium四项：11B归属、池复用、已经入源的AI值及`+BE8`展示依赖。只读本地官方SDK／Wiki、现有实现和J273 IDA；没有运行游戏、测试、注入、部署或修改IDB。VA基址为`0x400000`，RVA为下列VA减去该基址。

本轮纠正旧记录的三处遗漏：`+BDD—BDF`属于已核8B弹幕打包写入，`+BE4—BE7`属于原生8B Cycle打包存储，`4A13B0`也是companion创建和`+BE8`登记者。旧版本41／42的事实和判断保留，由本文修订现行归属。Delirium专有未知从11B收窄到4B；剩余`+BD4—BD7`仍不能证明为填充，不能宣称字节完整。

## 1. 两个打包存储及剩余空白

### 1.1 弹幕存储`+BD8—BDF`

`4A1A30`先从`+BD8`取u32，分离四个u8：编号、剩余次数、继续概率及相位；`4A1A94`另读`+BDC`的u8 Angle。更新尾部`4A267C—4A26D0`从五个零扩展u8重建一对寄存器：`ECX=0`，依次以四组`SHLD ECX,EDX,8`／`SHL EDX,8`合入其余字节，最后：

```asm
4A26CA  mov [edi+0BDCh], ecx
4A26D0  mov [edi+0BD8h], edx
```

整个结果为`u64(id | remaining<<8 | probability<<16 | phase<<24 | angle<<32)`。五个分量均先零扩展，故高24位确定为零；这三字节属于这个已核打包存储的高位，不是通过“没有单独命中”猜出的填充。`49FA14／49FA1F`初始化低u32为`0xFF`、高u32为0，`4A12A0／4A12A8`重置相位并清高u32，与完整8B边界一致。

官方`libzhl/functions/EntityNPC.zhl:142—144`及`repentogon/LuaInterfaces/LuaDelirium.cpp`仍只公开低u32 AttackID和`+BDC`的u8 Angle。布局改为Host的`delirium_attack_pack:u64`，不扩大公开API宽度，不新增线上字段；官方Angle setter的单字节写入也不会生成高24位状态。这里的u64只描述原生打包存储，不假定原版源代码的具体C++声明。

### 1.2 Cycle存储`+BE0—BE7`

构造`6B45B8／6B45C2`分别将低、高u32置零。原生AI多处将二者一起加载并以`SHRD`解包。例如：

```asm
49FC3F  mov edx, [ebx+0BE0h]
49FC47  mov ecx, [ebx+0BE4h]
49FC55  shrd edx, ecx, 19h
49FC63  and edx, 3FFh
```

`4A2748—4A2780`同样读双u32，并分别按7、15、25位右移。可核定的四段为低7位选择量、bits7—14红色阶段、bits15—24传送计数、bits25—34 AI阶段计数；末段实际跨入BE4的低3位。因此`+BE4`具有正面的读取用途，不能作为独立空白或填充。

`49FCAE／49FCCB`、`4A0C13／4A0C1D`及`4A378F／4A37AF`等已核重打包路径把高u32明确置零；本轮保留这种原生“读64位、按32位中间结果重写并清高字”的实际行为，不把它改造成理想化35位计数算法，也不据此推导永远可忽略高字。布局改为Host的`delirium_cycle_pack:u64`，不是新增Client时钟或状态。

官方Cycle getter／setter仍是`uint32_t*`：IsRedMode使用`(low32 >> 7) & 0xFF`，GetTeleportationTimer使用`(low32 >> 15) & 0x3FF`。公开字段宽度与原生存储范围分开；新增SDK编译断言固定AttackID／Cycle为4B、Angle为1B及其偏移。

### 1.3 `+BD4—BD7`结论

本轮复读`.text`的`+BC8—BEB`直接位移／常量候选，另在Delirium owner、辅助AI、ctor／Init、清理、NPC主／影绘制与既有复合helper中核对附近取址及覆盖该段的宽访问。两个打包存储均从8B对齐的`BD8／BE0`开始，BD0—BD3四字节小状态之后有4B空白，符合对齐假设；现有指令和官方SDK均未提供可区分“编译器对齐”与“未使用保留成员”的正面证据。

**处理结果为保留4B UNKNOWN**：不读取、不复制、不补零、不标填充。扫描覆盖的是已识别操作数和具体调用路径，不是任意计算基址的全映像别名证明。第15步的观察也只能增加实际读写事实，不能仅凭若干帧不变就升级为源级填充。原“11B能否证明为填充”已经拆成7B已核存储和4B仍未知，不再整体重做。

## 2. 普通NPC池槽残留：静态可达与实机观察分开

- `6EEE60`通过`6EF009`所传的`6B4520`一次构造`0x200`个`0xF60`池槽。该ctor的唯一代码引用在建池路径；每次Create没有重新调用ctor。
- `6EC540`的NPC分支`6ED1D0—6ED31B`循环选择同一池的槽。空槽分支直接返回槽地址；满池覆盖分支调用虚Remove／ClearReferences后返回，同样没有清整个对象。
- NPC Remove `6B4E30→6A8000`只处理原生移除／回调／关联；ClearReferences `6B4E60→6A8060`清资源和已登记关系，不将BD8—BE7作为标量范围清零。基类清引用会沿目标的反向登记集合撤指针，它不是对象全区memset。
- 下次NPC Init `6B4A50`清BC8、BCC、BD0—BD3、BE8，**不清BD8—BE7**；其基Init `6A4A20`处理公共Entity，`7D9890(this+414)`限于寻路对象，尾部`846D70`为PostNPCInit回调分派，没有固有的全子类清零。

于是存在明确静态路径：旧Delirium已由`49FA14`等写入非零AttackID／Cycle → 原生正常退休 → 工厂再次选中此槽 → 普通NPC Init且无额外Mod回调改写 → BD8—BE7仍含旧值。这证明**原生路径允许残留并保留旧值**；本轮没有观察某次实际运行选中了这样的槽，不能写成“已实测发生”或声称每次复用均残留。第15步E27只需按正常生命周期记录同槽新use前后的真实原值，不为取得样本强写填充或运行第二套提前测试。

原“普通NPC都不使用该段”也须限缩：`49F3D0`尾部可对非412的companion直接调用`4A2720／4A1A30`，它们消费打包值。142号非412臂保留这36B为Host区域，既不采集其中潜在残留，也不声称所有NPC都没有用途；143仍由稳定type412选择，不因伴随玩法数据增加Client布局或公开状态。

## 3. `+BE8`的来源、消费与展示

### 3.1 两条已核登记路径

1. `49F3D0`变身期间，临时type为410时，`4A0165—4A0196`从公共Child `+3C0`取真实对象，在旧目标`+3CC`反向集合撤掉BE8登记，再写入并经`4C79D0`登记新目标。
2. **原记录遗漏的`4A13B0`**：`4A14F1—4A1519`对根对象BE8执行同类撤旧／挂新；目标来自`424340→6FA210 Game::Spawn`，已经由原生工厂／Init／EntityList Add创建、发布为独立NPC，随后配置资源、调用`4A1540`处理特定多体形态并更新新对象。不能把这条路径误称为只对410的Child取别名，也不能把本地`delirium/ASMDelirium.cpp`独立实验DLL的PatchCompanion当作官方运行时一定禁用该路径的证据。

### 3.2 已核消费者

`49F4B4／49FB1B`把BE8交`49F1A0`参与清场选择；`4A1140—4A11D8`在目标存在且未被移除时，借其HP／相关伤害数据对根对象作虚槽调用，再对目标运行`4A2720／4A1A30`产生运动／弹幕。它属于Host模拟及生命周期关系，不能在Client复演这些helper。

本轮复读NPC Render `6CEC90`、Shadow `6D0650`及既有复合绘制`4E4710／599070→598B10／555410／574930／564630／54D1A0／59CDC0`的实际关系入口：使用的是公共Parent／Child、BEC及三deque，没有将BE8接入绘制的路径。410原生创建的Child按独立成员及公共Child供应；`4A13B0`新生成的companion同样有独立成员／最终ANM2／几何，不需要靠BE8再创建一份或建立另一条绘制引用。

**当前同步决定闭合为Host权威槽**：不预约BE8网络身份，不新增Client reference，不沿BE8驱动Remove；Client本地Init产生空槽后保持为空，异常非空走原有合法owner处理，不能裸清登记地址。成员的出现、变化和退休由Host成员表及实际线上关系决定。正面证据是完整创建／消费路径与已核的Client绘制依赖，并非仅依据“主Render没有直接位移命中”。若第15步发现新的具体画面依赖，仍须保留失败并按实际consumer修订，不能提前宣称所有未来Mod／任意间接访问都已穷尽。

## 4. 现行归属、线布局及后续步骤

原生Delirium段的现行单行表如下；整个0—F60表由唯一源生成，见[完整布局](layouts/EntityNPC_Delirium.md)。

| +BC8 u32 BossType【权威】 | +BCC u32 BossVariant【权威】 | +BD0 u16 TransformationTimer【权威】 | +BD2 u8 RemainingAttacks【权威】 | +BD3 u8 StateD【权威】 | +BD4 bytes[4]【未知】 | +BD8 u64弹幕打包存储【权威】 | +BE0 u64 Cycle打包存储【权威】 | +BE8 EntityPtr companion【权威】；共36B |
|---|---|---|---|---|---|---|---|---|

| DL0／块17【空预留，0B】 | DL1／块18【空预留，0B】；Delirium附加线状态共0B |
|---|---|

- 143专有段为32B权威＋4B未知；根未知597→590B（公共335＋双辅助ANM2的28＋NPC本类223＋本臂4），NPC142仍586B。两者最小线状态均710B，块号与线字段不变；表格中的完整性仍为否。
- 第10步仍在完整ordinary窗口、`49F3D0` owner退尽且type已恢复后选臂。Init后的读取资格沿`world_delirium_owner`，不读取两个Host打包存储或普通NPC残留，不在后台持有原生地址。
- 第11／13步仍不因BossType／Variant、打包值或companion变化强制重建同一NPC；最终资源／姿态／几何和成员表独立提交。BE8不安装、不读回、不做漂移比较。第14步切换旧链时删除旧字符串AI字段写入及无条件shape逻辑，不把新增原生u64描述接回旧writer。
- 第15步沿E09／E25／E28／E29覆盖经Init／不经Init的变身、410与其他companion实际独立成员、资源变化、无重复创建／无玩法重演和正常引用撤边；E27只保留BD4—BD7及其他原有UNKNOWN，同时记录同槽复用事实。BDD／BE4不再列为未知探测段，也不把不上线的打包值作为跨端相等断言。

失败方式：按SDK公开宽度漏掉原生打包高字；把恒零写入改称源级padding；扩大Cycle／Angle公开宽度；把非412 companion的Host数据当无用途；把静态残留路径伪称实测；因BE8不上线漏掉真实独立成员；沿BE8连带删除Client对象；从临时Boss type选错臂；借新字节归属恢复旧shape／AI writer；用未执行的E2E宣称展示完整。

唯一源`entity-classes.json`／`version.json`、SDK编译断言、生成表及第4／10／12—15步合同随本轮更新。第10／13步实际运行接线仍待实施，第15步是唯一统一功能验收。不创建独立artifact包。

## 5. 构建与执行记录

普通Release／Win32／v143产品纯编译与链接完成，`ISAACCS_BUILD_E2E_TOOLS=OFF`；生成器产出版本46表，SDK ABI编译对象、`isaac_cs_state`及完整`zhlIsaacCS`构建成功，MSBuild退出码0。既有SDK提示`CMAKE_REPENTOGON_VERSION undefined, build process unsafe`保留。首次沙箱内构建停在CMake文件扫描阶段、尚未进入编译，已终止；随后沙箱外重跑同一产品目标成功，不把中断记录抹成首次通过。

本轮没有运行测试、游戏、注入、部署或独立校验程序；构建输出位于工作区，未复制到游戏目录。仅做静态资料／源码／生成表复读，BD4未知和未实测的池复用事实照实保留。
