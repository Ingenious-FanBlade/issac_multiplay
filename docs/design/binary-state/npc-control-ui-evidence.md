# 第4步NPC受控UI：来源未决闭合与纠错

日期：2026-11-09（Asia/Singapore）。布局版本43。范围为上轮留下的`controlled_ui_mode`来源未决；没有运行游戏、测试或修改IDB。地址为J273静态VA，ImageBase=0x400000，RVA减去该基址。其他NPC UNKNOWN、Text／Dummy和完整同步接线不在本项完成范围。

## 上轮误解与当前证据

版本42的[同步职责复核](npc-synchronization-review.md)把`Manager+4AFB8`解释为本地设备记录，并把`8E7550`匹配后的slot5称为设备可用性。这是静态分析中的错误：它没有先核对该Manager字段和官方函数声明。上轮因此虚设了需要继续核定设备slot5的来源缺口。原报告保留并加注纠正，不将它改写成当时已经得到正确结论。

本轮先读官方SDK和Lua源码，再复读J273指令：

| 来源 | 直接内容 | 结论 |
| --- | --- | --- |
| `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Manager.zhl` | `NetplayManager _netplayManager : 0x4afb8` | 此处属于原版联机系统，不是本工程输入设备集合。 |
| 同目录`NetplayManager.zhl` | 签名`558bec8b0133d28b49`，声明`NetplayManager::IsIdxLocalPlayer(uint32_t idx)` | J273 `8E7550`正是原版“是否本地玩家”判断。 |
| 官方`repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp:893`附近 | `Player.IsLocalPlayer`调用`GetNetplayManager()->IsIdxLocalPlayer(player->_controllerIndex)` | 公开语义、所属对象和参数用途互证，不是设备连接／可用性查询。正式C/S不调用此getter。 |
| 本地REPENTOGON Wiki `docs/docs/EntityNPC.md`的GetControllerId | -1表示没有控制NPC的玩家 | 与两个原生UI分支的精确哨兵比较一致，不据编号推断C/S参与者身份。 |
| J273 `8E7550` | 遍历原版记录、按记录+8匹配idx；匹配后调用嵌入+3F8对象的slot5；空集合或无匹配返回true | 分支属于原版本地／远端筛选。匹配对象完整内部布局不是C/S本项需要采集的状态，不接入其slot5。 |

该结论没有运行或观察原版Online／Isaac Sync，也不要求读取其当前对象来证实“为空”。本工程既定隔离范围不接入原版联机；不能把“只读”作为引入原版NetplayManager来源的理由。

## C/S所需语义

NPC主Render两处受控UI都先比较`NPC+F1C == -1`，随后才进行原版Online的本地玩家筛选。C/S由唯一Authority提供全部参与者的最终世界，受控NPC的表现不由原版Online本地／远端身份或Client本机设备配置决定。因此只保留“是否受控”的表现值，原始控制者编号仍留Host查询，输入归属继续由本工程独立输入系统维护。

| 源NPC F1C i32 | 线值u8 | C/S表现 |
| --- | ---: | --- |
| -1 | 0 | 不进入受控NPC UI分支 |
| 其他值 | 2 | 进入受控UI分支，仍执行该分支后续可见／资源／alpha等条件 |
| 不可读、窗口失效或来源变化 | 无值，Gap | 不以0或2掩盖失败 |

不猜测“负数都无效”或“编号必须0—3”；原生实际比较只有-1。沿用版本42已经定义的0／2数值，撤销仅为原版本地／远端筛选预留的模式1。版本43解码拒绝1及其他编码，不读取本机NetplayManager重算。布局版本按整数直接比较，不兼容猜读旧42负载。

| +0 u8 controlled_ui_mode：0=无控制者 | 2=受控；1及其他值非法；共1B |
|---|---|

这个u8不是原生bool或ControllerId的内存镜像，不写回F1C，不携带设备地址、成员地址或输入命令。它只进入同key／generation／目标版本的成员表现旁表。

## 读取与解码实现

`mod/src/state/npc_presentation.h/.cpp`新增：

- `NpcControlledUi`：纯值枚举，只有0／2。
- `ReadNpcControlledUi`：只在当前C/S主线程ReadWindow内读取NPC+F1C的4B有符号值，前后两次值必须一致；地址加法、实际读取与generation／线程失效都报Gap。没有分配、原生getter、Lua调用、Manager／NetplayManager地址参数或slot5调用，不注册设备或修改世界。
- `DecodeNpcControlledUi`：有界u8解码，接受0／2，其余报`npc_control_ui_encoding`；返回自有值，不能携带原生指针。

采集调用者仍须先按真实RTTI选择142／143并提供合法窗口，reader不自行授予读取资格。原`entity-current-window-and-confirmed-npc-control-ui-source`未决资格已删除，使用已有`entity-current-window`。生成处理器名字保持`npc_controller_to_ui_mode`／`install_npc_control_ui_mode`，分别指向已实现的读取和纯值解码合同；正式ordinary窗口、成员存储与绘制owner由第10／12／13步接线。

## Client整段绘制边界

原生分支指令已复读：

| 入口 | 无控制者0 | 受控2 | 必须绕过的原版读取 |
| --- | --- | --- | --- |
| `6D025D`，受控NPC HP条条件 | `6D02C5` | `6D0285` | `6D0268—6D0283`的Manager+4AFB8及`6D027C→8E7550` |
| `6D0314`，+DEC副Sprite条件 | `6D03AC` | `6D0340` | `6D0323—6D033E`的Manager+4AFB8及`6D0337→8E7550` |

这是第13步具名adapter的语义与静态续接位置，不是在本轮提前安装跳转。不能仅拦两处call，因为其前面已经读取原版联机vector；不能全局Hook`8E7550`改变其他Player／Familiar等消费者。403物品的独立HP条条件在第一段之前，须保持原逻辑，不能被无控制者0一并抑制。两条受控分支以后的Sprite loaded／alpha、可见和资源条件同样保持。

本轮还明确同一NPC Render的`6CF09D—6CF0E5`：当Spawner（公共+3C8）是Player时，原版会用其ControllerIndex在NetplayManager中判断远端，再从`8FFDE0`取淡出倍率。它不是F1C读取，也不需要新NPC网络字段；C/S的具名draw隔离采用无原版远端筛选语义，需在进入该段之前直接续到`6CF0EB`，保留当前alpha和其他公共最终状态。该隔离接线继续归第13步，不能为了“完成来源”新增原版联机读取或原版Online实机测试。

## 交付与剩余工作

唯一源的转换、读取资格、枚举校验和公开成员说明已修订；整数版本43。N0仍25B，142／143最小仍710B；OverlayEffect144、三deque、Delirium空预留块、根586／597B未知和Pathfinder SDK声明均未改变。生成表和第4／10／12—15步文档保持一致。

本项的静态来源未决已经解决，正式reader和解码已落地；不再要求核定或读取原版NetplayManager slot5。第10／13步仍负责其原定窗口／表现旁表／绘制接线，这与本项来源是否已知分开报告。第15步唯一整体验收才覆盖-1／受控切换、非法1拒绝、窗口/来源变化失败、两段UI、403独立HP条及Spawner隔离，本轮不运行这些测试。

普通Release／Win32／v143产品构建已成功，`ISAACCS_BUILD_E2E_TOOLS=OFF`：布局生成、`npc_presentation.cpp`与基础库编译、SDK ABI对象及完整`zhlIsaacCS`链接通过。没有运行测试、部署、注入或启动游戏，不新增Hook或保留artifact包；临时编辑脚本已清理。
