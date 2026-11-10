> **版本46 Delirium未决事项（2026-11-09）**：[静态证据与归属修订](delirium-residual-evidence.md)核定两个8B Host打包存储，BDD／BE4共7B移出独立未知；BD4—BD7的4B仍无填充证明，根未知597→590B，最小线状态仍710B、17／18空预留。池槽残留已有静态可达路径但未实测；补核4A13B0的独立companion创建／BE8登记，BE8继续留Host。实际接线仍归第10／13／14步，唯一功能验收在第15步。 下文保留旧版本记录；BE8仅49F3D0访问的旧结论已纠正。

> **版本43纠正／闭合**：[NPC受控UI来源证据](npc-control-ui-evidence.md)核定Manager+4AFB8是原版NetplayManager，8E7550是IsIdxLocalPlayer。下文版本42将其解释为本地设备可用性是静态分析错误，不能据此接入原版Online。当前C/S只读NPC F1C哨兵得到0/2，新增原生reader及解码，旧模式1拒绝；原始记录保留，完整窗口／绘制接线仍归第10／13步。

# Entity_NPC同步职责复核

日期：2026-11-09（Asia/Singapore）。Mode：PR Review。范围为第4步NPC142／Delirium143的全部专有已知原生段、UNKNOWN边界、实际绘制消费者、公共依赖及生成声明；公共Entity／ANM2沿用此前复核，不重复扩大到全仓库。生成文件只由唯一源重生成，不手工修补。本轮仅静态反编译／源码审阅和普通产品编译，不运行测试；brooks-review的测试步骤按项目规范让位于第15步统一验收。

本次发现并修正三项传输问题。版本升至42，新建144号NpcOverlaySprite场景视图。原版字节事实、SDK声明、历史判断和失败保留；[版本41证据](npc-evidence.md)中的旧传输决定由本文取代。

## 复核结果

| 原生部分 | 结论 | 实际理由 |
| --- | --- | --- |
| 公共Entity E0—E9 | 沿用公共布局 | 最终位置／碰撞尺寸／HP／存在／Sprite／成员关系等已有单独消费者；不在NPC重复发送。 |
| +B64 State | 保留i32 | NPC主Render的273 state9、909:10的state6和910分支直接消费。 |
| +BA4 ProjectileDelay | 保留i32 | `4E4710`在`4E4D48`读取其分段选择；属于曲线显示输入，不能由字段名归为只供发射。 |
| +BB0 V1 | 保留Vector双f32 | `564630`（815）、`54D1A0`（881）与Render964分支消费；其中Y也影响几何，不能只留X。964实机必须另有合法NPC RTTI来源。 |
| +BC4 I2 | 保留i32 | `59CDC0`比较连接段，19:3主Render／Shadow使用；不能由当前位置等价重建。 |
| +C64 ChampionColorIdx | 保留i32 | `6CF121／6CFE14`对21作绘制脉动缩放选择。它与主Sprite最终颜色及BossColorIdx不同。 |
| +F1C ControllerId | **取消raw i32，换算u8受控UI模式** | `6D025D／6D0314`不把它当跨进程身份，而是拿去查询Manager的本地记录集合与`8E7550`谓词。直接写Host编号会让Client不同设备决定画面。 |
| +BEC EntityRef | 保留真实成员引用 | 245／411／816借用其目标Sprite；空／非空不能只用bool代替，也不复制Host地址。 |
| +BF0／+C04／+C18三deque | 保留完整逻辑投影 | 835／906／920、921／815／881及曲线消费者需要真实引用或完整点列，不能只发count／容器头或重跑AI补算。835可把真实NPC child传给曲线helper，不能只按child自己的type删主点列。 |
| +CD8 OverlayEffect ANM2 | 保留除Scale之外的三投影 | `6CFFC3—6CFFDA`在真正绘制前从主Sprite当前OverlayEffect null-frame的+10／+14完整覆写双f32 Scale。因此8B原值不需要传输；其他资源、姿态、颜色、offset、render flags仍保留。 |
| +DEC副ANM2 | 保留完整三投影 | 受控UI分支直接用其loaded／alpha及资源姿态；没有证据把+CD8的Scale派生推广至本Sprite。 |
| +BC8／+BCC BossType／BossVariant | **归Host，取消8B DL0和shape标记** | 它们选择Host代跑AI与生成最终资源。普通采集返回后本体type仍412、RTTI与0xF60对象布局不变；Client不运行这些选择／AI，主／辅助资源与几何独立同步。旧“已经标shape所以必须同步”的理由不成立。 |
| Delirium六项高频AI值、+BE8 companion | 继续留Host | 条件变身／弹幕／传送／伴随逻辑，未见当前Client主／影绘制消费；不构造计时锚点，不复演AI。 |
| StateFrame／ProjectileCooldown／V2／I1／Pathfinder | 继续留Host | AI／寻路输入；V2／I1／Pathfinder仍保持SDK声明或已有证据等级，不因不传输而伪造逐成员已核。 |
| Scale／CanShutDoors／IsBoss／ShieldStrength／IsChampion／再生／BossColorIdx／GroupIdx／DirtColor | 继续留Host | 原生更新、伤害、关门、群组或最终资源／颜色生成输入；Client安装公共最终Sprite／尺寸／HP与独立HUD，不执行这些模拟。 |
| +F00命中历史及+F28／+F38／+F50／+F54／+F58私有关系 | 继续留Host | 命中／AI／连带退休历史；不发送进程内Index，不预约无展示消费者的引用，不驱动Client碰撞／级联Remove。 |
| +F40 Siren拥有的玩法actor | 本地，不同步 | 不是可跨进程借用的roster Player；不构造第二个玩法Player，也不裸清异常非空owner。 |
| 虚表、Lua引用、资源地址、deque proxy／map／capacity／offset | 本地，不同步 | 必须由本地构造、资源及容器owner维护，复制会破坏生命周期和指针归属。 |
| NPC本类223B、Delirium臂11B及嵌套UNKNOWN | 保持UNKNOWN，不发送 | 构造写零、没有直接位移命中或已认出容器头均不是完整语义／填充证据。 |

## 三项修正的证据与影响

1. **无根据的Delirium结构换代**：原源把BossType／BossVariant声明为传输shape，导致未来发布器因Host AI选择变化退休重建同一个NPC。`49F3D0`在`4A0C39／4A0C42`恢复原本体type／variant；`6CEC90／6D0650`及已核复合draw helpers不读BC8—BEB，`6C9510`虽读Boss形态却是碰撞／玩法槽，不能放行至Client。修正后143号臂仍区分真实字节归属，DL0块17与DL1块18均空预留；只发生最终资源／姿态块变化，不制造成员离开、加入或shape增加。真实公共type／variant／subtype发生变化仍走正常结构事务。`world_delirium_owner`仅恢复Init后的读取资格，不能无条件升shape。
2. **重复的OverlayEffect缩放**：`6CFFD1`写NPC+D74、`6CFFDA`写+D78，分别正是CD8+9C／+A0；其输入为同一主Sprite当前null-frame+10／+14。路径只有在null-frame存在、可见及其他绘制条件成立时才调用`6D00C9`的ANM2 Render，因此不存在“未覆写却在此路径画旧Scale”的需要。新增144号专用视图只把Scale归派生；主Sprite null选择器／null-frame资源、公共父引用等须同版本就绪，draw scope保存并恢复本地工作值。通用ANM2及+DEC不变。
3. **跨进程ControllerId**：`6D027C／6D0337`调用`8E7550`；后者遍历Manager+4AFB8记录、比较每项+8与传入ID，找到后在嵌入+3F8对象调用slot5，未找到／空集合返回true。该整数既非网络身份也不是布局稳定资源号。修正为u8三态，保留画面真正需要的分支结果，避免Client设备重分配改变远端世界表现。

受控UI模式：

| +0 u8 controlled_ui_mode：0=无控制者（源F1C=-1） | 1=存在控制者但来源谓词为false | 2=存在控制者且来源谓词为true；共1B |
|---|---|---|

Host的具名转换必须在同一合法窗口核定来源；匹配记录的具体动态slot5及只读字段公式尚须由第10步reader补齐，不允许调用未知虚函数、Lua getter，或把未知默认成true。该缺口已写入`Field.read_requirement=entity-current-window-and-confirmed-npc-control-ui-source`及转换声明；当前并未虚构已经完成原生读取。Client只把三态存进同key／generation／目标版本的表现旁表，不能写F1C、注册设备或改Player控制归属。第13步专用NPC adapter仅替代上述两处受控分支，前面的403物品独立HP条路径保持原逻辑；不更改全局`8E7550`或其他Player／Familiar消费者。adapter尚待接线，当前运行链未切换。

## 当前线布局与完整性

| N0：+0 i32 State | +4 i32 ProjectileDelay | +8 Vector V1 | +16 i32 I2 | +20 i32 ChampionColorIdx | +24 u8 controlled_ui_mode；共25B |
|---|---|---|---|---|---|

N1／N2／N3与完整引用/点列不变；N4双辅助外观最小246→238B，N5／N6保持44／4B；142／143的17／18均空预留。最小NPC721→710B、Delirium729→710B（均含公共391B，不含帧头与实际变长资源/点列）；144号场景Sprite最小ANM2外观减少8B，其他子块不变。它们是布局下界，不是带宽测试结果。

根未知仍586／597B，NPC本类223B与Pathfinder1872B SDK声明边界保持。GetControllerId公开raw值改归Host查询，不能从三态反解设备ID；Delirium十项公开值全部显式归Host。原有必要公开值与真实引用回映射继续保留，不通过缩小UNKNOWN或删除原生目录制造完整。

唯一源、生成表、当前设计及第10—15步合同已同步。第15步须覆盖设备编号不同／本地设备变化但来源UI结果不变、三态转换、403独立HP路径、OverlayEffect实际null-frame缩放及恢复、Delirium经Init和不经Init的资源变化且不发生额外结构换代；本轮没有运行这些测试。

普通Release／Win32／v143产品构建成功，E2E_TOOLS保持OFF：重新生成布局，基础库、既有SDK ABI对象及完整zhlIsaacCS编译／链接通过。没有运行游戏、测试、注入或部署，也未创建artifact包；实际控制UI reader／adapter仍待第10／13步。临时编辑脚本已清理。本次提交按用户确认一并包含当前二进制重构前几轮待提交改动，不将这些旧改动冒称本轮重新实机验证。
