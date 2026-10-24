> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 Init v4 cache / familiar / P0 静态支线 v4

整体仍为 **BLOCKED_BY_EVIDENCE**。本证据提交完整静态盘点和明确有前提的局部证明，没有证明完整 Init/P0 同步闭合，没有运行时见证或生产 reader 实现。本逆向实例不验收自己的结果。

证据：out/cs-t0/native-join-init-v4-cache-p0-evidence-v4.json  
SHA-256：1E17F6F7224EEC1F94F86CDF5C2A06B41E2DBC5BFB7DD231DC86C8589D95A491

## 来源与盘点

承接 plan v10、progress v4、Init v3 的目标与单向哈希。共47函数、16084条code指令，cache363570共2051实际call，含151个reconcile。151仅交叉核对，独立验收须按磁盘指令逐地址核查。所有call、显式内存写、退出及检测到的CFG回边均逐项登记。Ctree最近定义和结构条件只是辅助证据，不能代替到达定义与可达性证明。

reconcileInventory逐消费者给variant/subtype、desired公式、source width/sign、五个栈实参push RVA/ESP、ECX receiver和Room匹配。min/max原始cmovl为有符号；wordcarry与AL截断分别说明。没有修改IDB、生产代码、旧证据或游戏状态。

## 局部 STATIC_CONFIRMED 的范围

- 有效Effects begin==end时，所列count/presence getters在访问entry前返回0；没有据此宣称Init全程保持空。
- 每个main player的有效238/239/626/627均零、count2及两行6-byte栈表有效时，组合表保持全零，六个对应desired为零。输入来自每位玩家，不只是B。
- 普通Isaac(type0)直接排除3D2B70(type18)、3D2DF0(type36)的charge/cache身体。
- 3CE420在1544/1546零、有效501/T156缺失、half-max-health可表示且非负等条件下，在3CE4E1幂等写1544=0，并置/清154A；不进入减币递归或health调整。不能简写成无写入。
- callback463E90的静态owner→bitset中bit100清零可排除Lua调度；无mods人工声明不够。
- raw1..732循环须证明vector至少733元素且全零。不能从另一张config表长度推出。
- Player1E68为空排除37034B递归；有效getter的1E6C是另一指针。

具体前提、指令顺序、结论见localProofs；每项都标注premisesGloballyEstablished=false，不得把局部条件命题封装成完整安全结论。

## Room、直接实体与额外玩家

363370在desired0时仍移除已有匹配familiar。必须同时证明Room无原生匹配：DC8=0、D76=0、172非零、subtype与410/414 owner条件成立者为空。Player自有空vector不是Room证明。

36E9E1/36EA51/36EB56另有道具519直接路径：查询整个type3集合，按DC8非零、D76零筛选，未检查当前Player owner。零desired仍可移除旧匹配，正desired可直接生成3/51。

36EE1C后用trinket180/effects180及道具667计算数量，扫描main成员的3BC owner、角色和effects667，再删除或生成相关玩家。零目标不排除已有匹配的删除；36F07B、36F1C0、36F1CB分别登记。

01AF60也不是纯读：重置/填充QueryOwner+7C/+74 scratch、调用分配/复制helper、累加879C48/879C4C统计值。未证明这些写通向persistent污染，也没有因零匹配或readonly把它们当作无副作用。alias及callee边界仍保留。

## P0 与 reader

3BC02B实际GetPlayer0，3BC034以mask=-1进入3597E0。Game26614<2时遍历当时main向量，将源共享字段写到每个目标，允许source=self有意别名。p0Dataflow逐字段列出所有写入。3596D2有效Has109、3CE420健康条件均须对P0和B在消费点证明保持。

readerSlices记录同步需求的原生owner/offset/width/layout/边界：raw/effects/active/pill/trinket/pending/extra/map、related、1E68和1E6C、各carry/health/transformation、temporary/config/seed/callback/main和Room ptr/capacity/count。它们只是规格补充，没有reader实现，不自动满足旧guard。漂移、替换、生命周期和未排除ABA永久失能，不自动重试；B来源不得用未来读回替代。

remainingUnknowns给首未决读写、路径和缺少事实；cacheCallInventory保留每个未闭合实际callee，不用笼统数量代替证明。当前没有确认本候选可达persistent污染反例；这是证明不足。

独立验收必须核字节/解码/CFG并审查前提能否推出结论。哈希或数量通过不能代替语义。遇这些UNKNOWN不得计整体PASS。natural window未扩展，active/live、05-05、T0-05、T0-06继续关闭，矩阵11由主线保持NOT_STARTED。

## 历史失败、语义更正与入口结构

v1/v2/v3 JSON、v1报告、失败builder及每次报告来源完整保留。v1 writer位误用CF_CHG1=1，实际CF_CHG1=4；首受影响RVA36358E是mov [ebp+var_10],eax。全部变化列在corrections，不能仅比较数量。

530A70在disable byte0时，即使Effects begin==end仍于530A7C调用530980。后者自己检查header；原不可达声明错误，当前该call保持UNKNOWN。条件零返回仍要求消费点有效空输入与中间源保持，不能由早先零写或未来B读回替代。

v2生成MD时Windows默认GBK读UTF8失败；v3修正编码并加入语义/异常状态及loop.status；v4完整只读对照sync_evidence.py，补原始chunk字节、successorRvas、calls(kind DIRECT/INDIRECT/TAIL)、exits和标准operand字段，详细operand数据仍保留。原始bytes/CFG/目标未变，块外指令与可能tail转移显式UNKNOWN。每个summary的call/exit按真实RVA对齐。没有为了接口把UNKNOWN隐藏或改成PASS，也没有自己独立验收。

当前所有functionCoverage的semanticClosureStatus和exceptionalControlFlowStatus为UNKNOWN，proofFrontiers含lastProven/nextUnresolved；reusedEvidenceBindings按明确schema绑定原始提取，历史失败binding明确不得作为语义通过依据。健康条件指SAR1后值非负且signed16可表示，不放宽负数。最终待独立实例验收的是本v4 JSON与本报告。
