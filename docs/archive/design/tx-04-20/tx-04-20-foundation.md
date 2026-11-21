# tx-04-20 I01—I04：Player目录、纯值目标、正式来源与静态合同

日期：2026-11-04，Asia/Singapore。本轮落实I01—I04；仍属于同一tx-04-20。I05—I40和I39统一E2E按原计划继续。本轮没有构建、运行测试或启动游戏，按会话要求直接提交代码。以下实现状态不代表原生完整Player应用或运行验收通过。

## 逐项交付

| 任务 | 本轮交付 | 当前边界 |
| --- | --- | --- |
| I01 | `gen_player_application_catalog.py`合并Entity／continuity逐签名目录；Player、继承Entity、关联类、配置与资源各行保留原来源，重载不合并；[应用目录](../../../../mod/binding/catalog/player-application.json)记录P／I／R／E、参数域、有效期、共享、安装和清理责任；CMake嵌入正式复制库，19目录链接20 | `classified-not-native-installed`和`I39-not-run`为真实状态，操作不变成RPC，未实现安装有具名consumer而非默认“纯值已处理” |
| I02 | `player_target.*`实现自有不可变PlayerTarget：共享同一EntityTarget、身体Key／shape、控制组、roster顺序、四备份槽、History／continuity和具名关系。Scene与EntityApplyPlan共享同一解析实例；能力增加`player-target-v1` | `native_apply=false`；不改19单Isaac原生支持条件。完整Player计划／owner／提交归I06以后；新源缺半个域或版本错误不回退旧profile |
| I03 | 正式`player-source-v1`扩展、roster顺序和完整成员标志；读取公开库存／主动0..3／口袋0..3／Weapon0..4／效果／costume的实际字段manifest；补当前真实配置ID范围的查询；额外12份内嵌ANM完整姿态，保留未加载Null；root／关系重核增加use／shape | 原公开字段仍在Entity规范本体，History仍在player图内；I05才复查发送省略。没有私有当前writer依据的标量明确Unavailable，非活动／备份不继承别人的PlayerUpdate资格 |
| I04 | [R01—R16静态合同](player-native-static.md)与[机器目录](../../../../mod/binding/catalog/player-native-contracts.json)登记采用／否决、ABI／地址口径、owner、失败前缀、清理和具体未闭合分支；指定GPT-6 Luna max研究，主智能体独立复读源码和指令 | 静态合同不等于所有未来adapter调用均已闭合；被否决的整条Init／Swap／Join等不得整体放行，具体未知随对应实现采用路径继续补证，不冒称全角色已可写／可画 |

## 两域关联与缺失语义

本轮目录含1,264个公开成员签名（跨目录来源合并，重载保留）、12个新增原生成员和16条研究记录；数量表示目录范围，不表示逐字段运行覆盖。

`CaptureRows`保留原采集`outer/owner/mutation_before/mutation_after`；Player解析要求两域同tick、同outer、同owner、相同且稳定的mutation，以及player source／location／canonical_state均指向准确entity scope。body、relations与canonical主体必须一致，shape必须一致。不会从“最新Entity”和“最新Player”拼出一个目标。

Registry仍是唯一稳定Key来源。participant用space9，control group用space10，身体用space1；ControllerIndex／PlayerIndex仅保留为来源字段。每组最多64身体、最多四组、四个全局备份槽；公开roster单独排序，不把256来源遍历预算当作可创建的角色承诺。跨组复用、重复ordinal／成员、错误scope／shape／代次、控制资格矛盾在写前拒绝。死亡与存在、活动与可输入、roster与非roster保持独立。

完整成员集合须通过结束时root／关系重核；失败同时撤销成员完整性。缺席不是删除。引用Null、未解析、未捕获、失败和显式空各自保留。History图仍以整个player component闭合；Entity资源与装备仍以整个entity component闭合，不在分段JSON上另造图。64位值／位集／有符号字段由既有Entity模型深复制保留。

公开完整性与原生完整来源分别记录。投影当前省略的主动／Weapon等成员在`missing`中呈现为`MissingOrProjected`，不冒称Source没采过；I05恢复实际consumer后再提升对应范围。`native_source_complete`当前为false，不能用parser存在来置完整Entity／Player提交位或发网络ACK。

## 正式来源与读取依据

公开字段沿用两套本地Wiki、官方Lua wrapper和现有有界reader，不主动执行GetBombFlags、攻击选择、EvaluateItems、Use／Add／Remove或Update。新增GetCollectibleNum查询覆盖实际Collectible配置ID与OnlyCountTrueItems／IgnoreSpoof四种组合；GetActiveItemSlot使用同一实际ID集合；GetSmeltedTrinketDesc只遍历当前有效Trinket配置ID，普通／金数量在完整desc内。配置枚举／query预算失败保持Partial，不扩大请求预算。没有任意字符串组名的穷举；默认组不冒充所有固有组。

costume／TemporaryEffects／Weapon现有公开完整列表和全部有限槽继续由同一`read.Collect`复制；manifest不重复调用Getter、不重复图定义。`application_player_resources`来自同对象KnownCurrent且readiness>=2的现有原生合法窗口，不因公开根能返回指针就提高私有资格。

Player ctor `VA0x74DC70/RVA0x34DC70`实际构造13份连续内嵌ANM：`+0x41C/+0x530/+0x644`、从`+0x758`开始的7项数组，以及`+0xEE4/+0xFF8/+0x110C`，stride为`0x114`。base／stride／count登记在唯一`world_layout.json`。slot0是既有BodySprite；新源复制其余12份，SDK具名blood_gush／held沿原名，其余仅叫inline_N，不猜语义、不因同文件合并。资源未Load时显式Null，不调用Load补齐Source。构造证明与当前值／绘制证明分开。

## 主智能体审阅

主智能体先读本地TemporaryEffects、ActiveItemDesc、Weapon、CostumeSpriteDesc及EntityPlayer／PlayerManager文档，再复读官方`LuaEntityPlayer.cpp`、`LuaPocketItem.cpp`、`LuaWeapon.cpp`、`LuaCostumeSpriteDesc.cpp`和ZHL布局。独立IDA复读如下：

- `0x990250 SpawnCoPlayer2`：普通分支先ctor、写索引、入主roster、推进RNG、虚Init，正常返回后分配Room Index并Add；-1分支使用manager+0xD0/+D4/+D8的独立vector与Init(1,2,0,seed)，不走普通分支末尾Add。SDK的manager大小0xC8不能封闭该特殊分支。
- `0x7C8370 InitTwin`：对返回值立即解引用，无NULL防护；维护双向EntityPtr、SetControllerId、移动双方坐标并重配HUD。不是纯结构clone。末尾`0x980130`经复读由诊断字符串及实际8槽清理／主副身体绑定确认是AssignPlayerHUDs，不能当作无副作用leaf。
- `0x74DC70`前70条指令：确认13份ANM构造，尤其0x74DD16／0x74DD25分别调用ANM ctor0x402B00。没有由此推断角色私有标量writer或所有资源已Load。
- `0x9CADB0 CreateWeapon`：按类型取工厂，先创建动态对象，再虚Init；未见成功结果NULL防护。`0x9CADE0 DestroyWeapon`先虚ClearReferences，再对当时仍在槽中的对象虚Free(1)，最后清槽；不是“清指针即可退休”。具体动态类及失败／连带Entity仍按R08／R15合同。

研究原始UNKNOWN、曾经失败和19有限Player能力均保留。当前没有解除原生owner、全角色draw、跨域依赖、退休或访客持久保护的具体限制。

## 接续

I05复核P1—P6实际Player消费者和紧凑替代；I06使用同一PlayerTarget建立A→T身体／资源计划；I07以后实现具名owner并按I04拒绝／采用合同处理每个真实调用。全部目标完成后进入I39统一E2E，覆盖source→T→真实后态／画面及18／19回归；本轮没有提前试跑、单元测试或单独artifact。
