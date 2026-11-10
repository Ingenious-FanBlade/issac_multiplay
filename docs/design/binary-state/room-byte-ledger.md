# 第7步 Room：原生字节账本

2026-11-10；修订2。机器可读源：[room-byte-ledger.json](room-byte-ledger.json)；[同步职责](room-synchronization-review.md)；[剩余字节补证](room-residual-evidence.md)。设计视图，不是生产复制计划；父实例职责覆盖子表。旧1290B根UNKNOWN降至28B；嵌套／动态未知另列，不以未命中／对齐称填充。

| 类型 | 原生大小 | 直属UNKNOWN（不含嵌套／动态） |
|---|---:|---:|
| Room | 0x7898 | 28 |
| Backdrop | 0x5278 | 3 |
| BackdropConfig | 0x13C | 5 |
| FXLayers | 0x598 | 3 |
| FXParams | 0xA0 | 3 |
| FXPose | 0x14 | 2 |
| FX | 0x4C | 5 |
| Ray | 0x14 | 0 |
| RayGroup | 0x2C | 2 |
| Vein | 0x34 | 7 |
| RailManager | 0x2D8 | 0 |
| Camera | 0x94 | 3 |
| CameraRing | 0x10 | 0 |
| RoomShockwave | 0x1C | 0 |
| RoomHellBackdrop | 0x128 | 3 |
| EntityList | 0x400 | 3 |
| EntityListEL | 0x10 | 3 |
| RoomTemporaryEffects | 0x18 | 2 |
| Vector | 0x8 | 0 |
| ColorMod | 0x2C | 0 |
| KColor | 0x14 | 0 |
| AnimationState | 0x20 | 3 |
| LayerState | 0xA0 | 4 |
| ANM2 | 0x114 | 8 |
| ImageStrongPair | 0x8 | 0 |
| RNG | 0x10 | 0 |
| FXPoop | 0x4C | 5 |
| RoomOutputSetHeader | 0x8 | 0 |
| RoomCallbackVector | 0xC | 0 |
| RainSpawnRegion | 0x18 | 0 |
| OriginalOnlineEntityFrame | 0x18 | 0 |
| KilledNPCRecord | 0x18 | 3 |

另有未完全展开的Host／本地动态payload，详见JSON `opaque_nested_payloads`及[补证第8节](room-residual-evidence.md#8-仍未知的确切范围与处理)；这些不计入上表直属数，不能写成嵌套语义已全部核清。

## Room

无vtable：+0是初始化字节；本表仅Room及所列固定内嵌对象，不代表Level／RoomDescriptor完成。

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Room.zhl；J273 VA7DF8F0/7E0190/7E8AD0/7F7F60/804500

| 0x0—0x0 bool initialized【本地／已核】原生构造／Init状态；809590未初始化返回-1；Client owner正常构建后本地维护；线上用记录存在性／有效资格，不覆盖构造字节 | 0x1—0x1 u8 first_update_control【权威／已核】7F7F60首帧分支读取；不等于当前画面存在性；留Host；不让Client Room.Update消费 | 0x2—0x3 bytes[2] unknown_2_3【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；保留具名缺口，不以零值或旧堆数据补齐 | 0x4—0x7 RoomDescriptor* descriptor【引用／已核】当前实际描述owner；不是+0；转当前world内descriptor身份／维度；本地解析，第7步Level侧承接 | 0x8—0xB u32 room_type【状态／已核】当前实际结构值；宽高含边界；R0；改变结构走同world重建，不执行配置spawn | 0xC—0xF u32 grid_width【状态／已核】当前实际结构值；宽高含边界；R0；改变结构走同world重建，不执行配置spawn | 0x10—0x13 u32 grid_height【状态／已核】当前实际结构值；宽高含边界；R0；改变结构走同world重建，不执行配置spawn | 0x14—0x23 f32[4] bounds【状态／已核】实际房间边界，特殊Dungeon边界不能只由shape名猜测；R0；同几何版本安装；L形area/tile可由已核规则派生 | 0x24—0x723 GridEntity*[448] grid_slots【引用／已核】原生真实448槽；SDK数组写447少一项；第6步成员身份／格序号承接；不发448个进程地址 | 0x724—0x743 GridEntityDoor*[8] door_slots【引用／已核】实际八门槽，允许与grid槽别名；不能按门的primary Slot反推；R3八项nullable成员引用；同world解析，保留别名，不驱动过门FSM | 0x744—0x763 i32[8] door_grid_positions【状态／已核】八门实际格序号；R3；几何边界检查，不合并Door目标descriptor索引 | 0x764—0x767 i32 tinted_rock_index【权威／已核】已选蓝石格索引，-1为空；不是额外Grid实例身份；留Host；真实GridRock外观／已选帧由第6步承接 | 0x768—0x76B i32 dungeon_rock_index【权威／已核】已选夹层石头索引；815D20还按Game状态返回-1；留Host；真实GridRock／入口显示另承接，不在Client按索引生成 | 0x76C—0xE6B i32[448] grid_paths【权威／已核】448格动态寻路cost；原SDK456个DWORD覆盖进后面的独立WORD表；留Host；不为Client碰撞／寻路或Getter原值相等发送 | 0xE6C—0x11EB i16[448] npc_grid_reservations【权威／已核】NPC目标格占用／重选避让计时；正值会阻止55BD00选格，1..99每三帧衰减，其他值保持；留Host；不与DWORD path合并、不在Client重跑选格；原E8C..11EB的864B归入本表 | 0x11EC—0x11EF i32 room_clear_delay【权威／已核】清房触发延迟；官方quickRoomClear还修改该逻辑；留Host；最终门／奖励Entity／Greed显示另发，不为Getter相等传生产倒计时 | 0x11F0—0x11F3 u32 frame_baseline【状态／已核】809590：Game+25A70减此基准；+0无效时返回-1；R1原值；使用同world GameFrame，重复显示不改锚点 | 0x11F4—0x11F5 u8[2] init_update_control【权威／已核】11F4更新分支；11F5初始化置1；留Host；不放行Room.Update | 0x11F6—0x11F6 u8 textured_pit_dirty【派生／已核】坑mask surface本地重建通知；不上线；完整Grid／纹理依赖提交后本地dirty，重建81BB30，不放行混合PreRender | 0x11F7—0x11F7 bytes[1] unknown_11F7_11F7【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；保留具名缺口，不以零值或旧堆数据补齐 | 0x11F8—0x11FB Camera* camera【本地／已核】正常构造Camera owner；不传地址；Camera视图另表；不复制ring／执行Update | 0x11FC—0x1203 Vector surface_origin【派生／已核】由本端g_WIDTH/g_HEIGHT产生的surface原点；本地按明确公式重建；不安装Host分辨率偏移 | 0x1204—0x120B Vector render_scroll【状态／已核】当前实际Room滚动输入；Game屏幕震动仍有独立owner；R2同world显示值；不运行Camera.DoUpdate；本端视口换算独立 | 0x120C—0x120C bool red_heart_damage【权威／已核】恶魔／天使机会等玩法生产输入；留Host；StatHUD最终机会走第8步；不是额外画面bool | 0x120D—0x120F bytes[3] unknown_120D_120F【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；保留具名缺口，不以零值或旧堆数据补齐 | 0x1210—0x1213 i32 broken_watch_state【权威／已核】速度模式输入，不是Room的展示时钟；留Host；实体最终状态和显示时相各有owner | 0x1214—0x1217 u32 next_entity_index【权威／已核】原生Entity.Index分配计数，不是网络稳定身份；留Host；正式实体身份／必要排序index由Entity域承接，不复制分配器游标 | 0x1218—0x1617 EntityList entity_list【容器／已核】主／绘制／更新／持久等多索引；正式Entity成员／有序绘制关系承接；本地自建EL，不发对象头或全部查询缓存 | 0x1618—0x1BAF FXLayers fx_layers【容器／已核】随机选图／有序pose／ray／vein和FX参数；R2/R4按子表投影；容器地址、生产器和隐藏残留不复制 | 0x1BB0—0x1BB3 u32 boss_id【权威／已核】boss头像选择输入，不是Entity type；Room raw不上线；真实BossIntro／HUD显示身份资源交第8步，不能漏掉显示结果 | 0x1BB4—0x1BB7 u32 second_boss_id【权威／已核】双Boss第二头像ID；相同Boss时可为0，不能当双Boss存在位；Room raw不上线；实际BossIntro显示资源归第8步 | 0x1BB8—0x1BBB EntityPtr deaths_list_target【权威／已核】死亡名单当前选中目标的本地注册引用；原值留Host；已生成Effect109及其Entity关系承接标记，不由Room重复创建 | 0x1BBC—0x1BBC bool deaths_list_done【权威／已核】死亡名单失败／已结束或奖励已处理的控制位；5CCC10读取；留Host；奖励／属性和标记消失使用实际Entity／Player结果 | 0x1BBD—0x1BBF bytes[3] unknown_1BBD_1BBF【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 | 0x1BC0—0x1BC7 Vector deaths_list_reward_position【权威／已核】名单完成后的奖励生成位置；零向量时由Player附近择位；留Host；实际生成Pickup位置／属性由Entity域发送，不重跑择位／随机／Spawn | 0x1BC8—0x6E3F Backdrop backdrop【资源／已核】背景资源／程序纹理／四Sprite；R2资源结果；详见子表，禁止整块复制或按ID重抽 | 0x6E40—0x6F53 ANM2 light_gradient_sprite【派生／已核】正常本地LightGradient／SpotLight资源；绘制逐灯覆写scale／颜色等；不发通用整Sprite；若实际资源／图层被改变，发必要资源覆盖；最终light pass另有结果，不放行玩法查询 | 0x6F54—0x7067 ANM2 spot_light_sprite【派生／已核】正常本地LightGradient／SpotLight资源；绘制逐灯覆写scale／颜色等；不发通用整Sprite；若实际资源／图层被改变，发必要资源覆盖；最终light pass另有结果，不放行玩法查询 | 0x7068—0x706F i32[2] room_update_counters【权威／已核】Init按RoomType设计数，更新维护；留Host | 0x7070—0x7083 deque<ShopRestockDesc> shop_restock_deque【容器／已核】MSVC deque头；补货队列产生新Pickup与涨价；整队列留Host；已生成Pickup／实际价格由Entity/HUD承接；不ShopRestock／TriggerRestock | 0x7084—0x70C3 f32[16] ambient_gain_smoothing【换算／已核】火／水流／环境等16份当前音量平滑缓存；静态表另含soundID、步长、增益、跨房保留标记；不下发原64B；第8步实际持续voice必须含已选sample和最终world逻辑gain／pitch／pan，不能将声音一起删掉 | 0x70C4—0x70DF RoomShockwave shockwave_a【换算／已核】第一份波纹显示与推进输入混合；只发本相位最终波纹pass参数；不发未来max／speed／推进历史 | 0x70E0—0x70FB RoomShockwave shockwave_b【换算／已核】第二份波纹显示与推进输入混合；与A分别提供最终结果／存在性，不混成一个timer | 0x70FC—0x70FF i32 shockwave_aux_control【权威／物理写入已核】Init置0，未来流程控制；留Host；具体命名未决 | 0x7100—0x7213 ANM2 shockwave_overlay_sprite【换算／已核】Render强制Idle／临时帧；含特殊背景分支；只发当前实际资源／必要层覆盖／显示结果；不推进动画，不把旧动画状态当稳定输入 | 0x7214—0x7217 i32 wall_blood_duration【权威／已核】EmitBloodFromWalls的Duration；452520首参数写入，Room.Update递减；留Host；已产生血Effect／贴图结果由相应Entity／资源承接 | 0x7218—0x721B i32 wall_blood_count【权威／已核】EmitBloodFromWalls的Count；452520次参数写入；留Host；不为重复显示生成血效果 | 0x721C—0x721F u32 puddle_decoration_count【权威／已核】GridDecoration选择动画时的Puddle计数；已有非零值则排除另一个Puddle候选，选中后自增；留Host；实际装饰Grid的资源／动画结果由第6步承接，不恢复候选随机选择 | 0x7220—0x7220 u8 subframe_render_flag【换算／已核】Render波纹半帧时读；SDK将此地址声明int slowdown；最终波纹结果包含该相位；不独立发int，不读取高3B | 0x7221—0x7223 bytes[3] unknown_7221_7223【未知／未知】官方Hush激光补丁经错误SDK _slowdownDuration对+7220作int读取时会连带读本3B；真实J273字节语义仍未明；不发；该补丁为Host模拟判定，不把这次过宽读取当作正确int字段证明，也不称padding | 0x7224—0x7227 i32 pending_boss_deathspawn_count【权威／已核】Boss deathspawn计数；NPC路径增量，TriggerBossDeath先减再决定奖励，Update检查漏触发并写当前Boss数；留Host；实际奖励／BossIntro等结果另承接，不为Client重放死亡／清房流程 | 0x7228—0x7229 u8[2] room_logic_flags【权威／已核】Init和Update使用的逻辑标记；留Host | 0x722A—0x722B bytes[2] unknown_722A_722B【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；保留具名缺口，不以零值或旧堆数据补齐 | 0x722C—0x722F i32 slowdown_duration【权威／已核】真正HasSlowDown/SetSlowDown计时；SDK误指+7220；留Host；452550只将当前值提高到max(current,arg)，非展示相位 | 0x7230—0x7233 i32 troll_bomb_spawn_timer【权威／已核】705270某动作分支写35；Update在正值且模5为0时生成type4/variant3炸弹再递减；留Host；只发送实际已生成Bomb Entity，不在Client用计时重演Spawn | 0x7234—0x723B std::set<i32> header deferred_grid_cleanup【容器／已核】格子破坏／路径变化后待处理的格序号集合；每次正常Update处理并清空；留Host；不复制树／key队列，也不在Client重放Destroy/Update | 0x723C—0x723F i32 greed_wave_timer【状态／已核】PressurePlate／Door／HUD直接显示依赖；R1原值，允许负特殊值；不在Client递减／生成波次 | 0x7240—0x7243 f32 water_amount【状态／已核】水绘制模式／透明度及已有Entity/Grid消费者；R2；不能仅换成bool | 0x7244—0x7257 KColor water_start【换算／已核】水色插值端点；非各自独立最终画面；只取816BB0实际最终KColor＋三乘色浮点；原四KColor不上线 | 0x7258—0x726B KColor water_multiplier_start【换算／已核】水色插值端点；非各自独立最终画面；只取816BB0实际最终KColor＋三乘色浮点；原四KColor不上线 | 0x726C—0x727F KColor water_target【换算／已核】水色插值端点；非各自独立最终画面；只取816BB0实际最终KColor＋三乘色浮点；原四KColor不上线 | 0x7280—0x7293 KColor water_multiplier_target【换算／已核】水色插值端点；非各自独立最终画面；只取816BB0实际最终KColor＋三乘色浮点；原四KColor不上线 | 0x7294—0x729B i32[2] water_lerp_duration_countdown【换算／已核】duration<0使用FX色；0取目标；>0按clamp插值；最终32B water shader投影；Client不计时，不调用颜色setter | 0x729C—0x729F bytes[4] unknown_729C_729F【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 | 0x72A0—0x72A0 bool first_enemy_dead【权威／已核】首个敌人死亡资格；NPC死亡路径置位，Init连同pacifist清零；留Host；已产生奖励／最终显示状态由各自owner承接 | 0x72A1—0x72A1 bool pacifist【权威／已核】无伤／攻击等玩法条件；Familiar碰撞／NPC逻辑读取；留Host；最终属性／表现已由各自域承接 | 0x72A2—0x72A3 bytes[2] unknown_72A2_72A3【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；保留具名缺口，不以零值或旧堆数据补齐 | 0x72A4—0x72CF ColorMod floor_color【状态／已核】实际11浮点颜色；Grid/Door会临时借用；R2完整44B；与Grid同world，不能取Client本地楼层色 | 0x72D0—0x72FB ColorMod wall_color【状态／已核】实际11浮点颜色；Grid/Door会临时借用；R2完整44B；与Grid同world，不能取Client本地楼层色 | 0x72FC—0x72FF i32 delayed_sound460_counter【权威／已核】延迟声音460及之后空间pan生产的计数；-1未激活；原计时留Host；第8步发送实际voice/event与逻辑pan，重复baseline不重播 | 0x7300—0x7307 std::list<KilledNPCRecord> header killed_npc_history【容器／已核】实际是链表头／count，不是原账本猜测的tree；32B节点含8B链接＋24B记录；历史整体留Host；不为Client建立回生／奖励历史或复制节点地址 | 0x7308—0x730F tree header per_pass_light_tree【派生／已核】803E20／804500每pass清树后遍历Entity/Grid重新填充；本地同pass重建；不能把Host旧树地址／节点线上化；纯显示light依赖须齐全 | 0x7310—0x7327 TemporaryEffects temporary_effects【权威／已核】Room效果容器与更新控制，不等于Player效果实例；列表留Host；已核Render效果481等取最终显示资格；Entity颜色／Run结果各归原owner | 0x7328—0x7377 std::set<i32>[10] output_grid_groups【容器／已核】10个事件输出组的格序号集合；8B树头，节点+10为i32 grid index；留Host；不传组表驱动Client格子动作；当前Grid／Entity后态另发 | 0x7378—0x73EF vector<void(*)()>[10] output_callbacks【容器／已核】每组12B begin/end/cap；元素4B原生函数地址，818250真正调用(*fn)()；绝不发送或安装Host函数指针，不将TriggerOutput当同步命令 | 0x73F0—0x73F3 i32 plum_flute_reward_clock【权威／已核】BossID84场景的无伤时间／离场哨兵；-1使奖励选择650（Plum Flute）；留Host；实际Boss Entity／奖励Pickup发送；BossID不是EntityType84 | 0x73F4—0x7453 RainSpawnRegion[4] rain_spawn_regions【权威／已核】最多4份区域，每份origin和两基向量共24B；819530按index*24构造；不发生成区域／采样随机；实际雨Effect135与持续音频结果由相应域承接 | 0x7454—0x7457 i32 rain_spawner_count【权威／已核】雨滴生成区域数量，819530构造／8199D0更新；留Host；实际雨滴／雾Entity与ambient voice另发；不凭数量在Client创建 | 0x7458—0x745B f32 rain_intensity【权威／已核】生成雨效果的强度，不是当前雨滴位置；留Host；当前已产生的Entity和持续音频意图由第4/8步承接 | 0x745C—0x745C u8 rain_control_flag【权威／已核】819530清零的生成控制字节；留Host | 0x745D—0x745F bytes[3] unknown_745D_745F【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；保留具名缺口，不以零值或旧堆数据补齐 | 0x7460—0x7467 Vector water_current【状态／已核】816BB0向shader packet写当前水流；R2两个f32；声音／粒子用Host实际结果，不调用SetWaterCurrent来产生它们 | 0x7468—0x773F RailManager rails【容器／已核】轨道字节和绘制Sprite；不是GridEntity；R4有效extent字节＋资源／层覆盖；按子表处理 | 0x7740—0x7743 f32 lava_intensity【状态／已核】GridLight／纹理坑表面直接读取；不只用于ComputeColorModifier；R2必须保留；不要因最终整屏颜色另发而删除 | 0x7744—0x7763 EntityPtr[8] red_door_outline_refs【权威／已核】UpdateRedKey维护的Effect引用／通知关系；不是门槽本身；对象指针不发；实际轮廓Effect按独立Entity成员绘制，不由此创建或级联删除 | 0x7764—0x7767 i32 special_room_counter【权威／已核】Update负特殊房间分支倒计时并播放／生成；留Host | 0x7768—0x7769 u8[2] room_action_flags【权威／已核】拥有物653检查／批量实体处理控制；留Host；不安装以触发Client玩法 | 0x776A—0x776B bytes[2] unknown_776A_776B【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；保留具名缺口，不以零值或旧堆数据补齐 | 0x776C—0x7893 RoomHellBackdrop hell_backdrop【换算／已核】SDK未列的横向地狱背景连续视图；有混合Update；R2活动资格／实际资源与current显示位移；禁止82EE90移动实体 | 0x7894—0x7894 bool room_bulk_action_pending【权威／已核】Room.Update批量处理实体后清零；不属于背景绘制输入；留Host；不由Client消费 | 0x7895—0x7897 bytes[3] unknown_7895_7897【未知／未知】声明边界内未解释尾部，不能称填充；不读不发；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## Backdrop

程序纹理以内容资源或已证无随机recipe承接；旧RNG轨迹是历史实现，不是新方案默认负载。

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Backdrop.zhl；90BD90；917870/917910；918A60/9191F0

本轮剩余核对：90BD90/90C1F0/917870/918A60/9191F0：只接受+20 bool，+21..23仍无独立语义

| 0x0—0x7 ImageStrongPair floor_buffer【资源／已核】本地FBO/Image owner；内容可为程序生成结果；发送实际被采用内容资源／完整recipe；control、GL/FBO、本地batch不发 | 0x8—0xF ImageStrongPair walls_buffer【资源／已核】本地FBO/Image owner；内容可为程序生成结果；发送实际被采用内容资源／完整recipe；control、GL/FBO、本地batch不发 | 0x10—0x17 ImageStrongPair floor_surface_2【资源／已核】本地FBO/Image owner；内容可为程序生成结果；发送实际被采用内容资源／完整recipe；control、GL/FBO、本地batch不发 | 0x18—0x1F ImageStrongPair temp_backdrop_surface【资源／已核】本地FBO/Image owner；内容可为程序生成结果；发送实际被采用内容资源／完整recipe；control、GL/FBO、本地batch不发 | 0x20—0x20 bool surface2_selected【状态／已核】Render检查的floor surface选择字节；有实际消费者的资源分支选择保留，不发送仅loaded缓存 | 0x21—0x23 bytes[3] unknown_21_23【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x24—0x2B ImageStrongPair gfx【资源／已核】当前真实图像选择，不是裸地址；实际采用的资源引用；不强读已释放构建输入 | 0x2C—0x33 ImageStrongPair water_gfx【资源／已核】当前真实图像选择，不是裸地址；实际采用的资源引用；不强读已释放构建输入 | 0x34—0x3B ImageStrongPair normal_floor_gfx【资源／已核】当前真实图像选择，不是裸地址；实际采用的资源引用；不强读已释放构建输入 | 0x3C—0x14F ANM2 floor_sprite【状态／已核】18/26/35等特殊背景与MegaSatan真实Sprite分支；必要资源／主overlay／层姿态；非采用分支不读取旧值 | 0x150—0x153 u32 backdrop_id【状态／已核】0..62实际背景身份，不能与RoomType混同；保留实际ID；ID本身不代表最终纹理内容 | 0x154—0x4F17 BackdropConfig[63] configurations【资源／已核】XML本地配置库；不是63份动态状态；只解析实际使用资源／显示参数；不发送全部63配置或共享std::string头 | 0x4F18—0x4F1F Vector surface_min【状态／已核】实际背景surface范围；SDK只称unkVec1/2；保留实际向量；不额外赋予未经证明语义 | 0x4F20—0x4F27 Vector surface_max【状态／已核】实际背景surface范围；SDK只称unkVec1/2；保留实际向量；不额外赋予未经证明语义 | 0x4F28—0x4F37 RNG rng【权威／已核】背景构建流；构建包含随机选择和Spawn；留Host，不是第三个D0绘制例外；结果由纹理／Sprite／Entity承接 | 0x4F38—0x4F3B u32 construction_seed【权威／已核】构建seed不是最终纹理身份；留Host；不按seed重放装饰／生成 | 0x4F3C—0x504F ANM2 wall_detail_sprite【状态／已核】实际嵌套动画与资源；只发实际采用显示；controls按C/S本地绑定显示，Host设备按钮选择不强加访客 | 0x5050—0x5163 ANM2 controls_sprite【状态／已核】实际嵌套动画与资源；只发实际采用显示；controls按C/S本地绑定显示，Host设备按钮选择不强加访客 | 0x5164—0x5277 ANM2 controls_buttons_sprite【状态／已核】实际嵌套动画与资源；只发实际采用显示；controls按C/S本地绑定显示，Host设备按钮选择不强加访客 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## BackdropConfig



证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310

| 0x0—0x3 u32 id【资源／已核】本地已加载配置／构建参数；稳定资源配置；不用这些参数在Client随机选纹理 | 0x4—0x1B std::string gfx【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0x1C—0x33 std::string water_gfx【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0x34—0x4B std::string l_floor【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0x4C—0x63 std::string n_floor【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0x64—0x67 u32 walls【资源／已核】本地已加载配置／构建参数；稳定资源配置；不用这些参数在Client随机选纹理 | 0x68—0x6B u32 wall_variants【资源／已核】本地已加载配置／构建参数；稳定资源配置；不用这些参数在Client随机选纹理 | 0x6C—0x6F u32 floors【资源／已核】本地已加载配置／构建参数；稳定资源配置；不用这些参数在Client随机选纹理 | 0x70—0x73 u32 floor_variants【资源／已核】本地已加载配置／构建参数；稳定资源配置；不用这些参数在Client随机选纹理 | 0x74—0x74 bool full_image【资源／已核】实际XML fullimage构建模式，非u32 unk1；结果纹理路线不额外发构建模式；若使用recipe，须显式包含该模式，禁止读高3B | 0x75—0x77 bytes[3] unknown_75_77【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 | 0x78—0x8F std::string props【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0x90—0xA7 std::string rocks【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0xA8—0xBF std::string pit【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0xC0—0xD7 std::string water_pit【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0xD8—0xEF std::string bridge【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0xF0—0x107 std::string door【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0x108—0x11F std::string hole_in_wall【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0x120—0x137 std::string spikes【资源／SDK声明】配置路径；SSO／allocator本地；实际采用资源走稳定目录；不复制字符串对象头 | 0x138—0x138 u8 water_pits_mode【资源／已核】实际水坑图形模式byte，SDK的u32过宽；稳定配置／实际资源用途承接，不能打包邻接reverse为一个u32 | 0x139—0x139 bool reverse_water_gfx【状态／已核】91AC70::RenderWater把GameFrame/2相位乘-1的真实绘制条件；需要保留当前采用配置的这一个方向bool，或等价最终有符号水面相位；纹理像素本身不能代替 | 0x13A—0x13B bytes[2] unknown_13A_13B【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## FXLayers

仅实际采用的有序层／ray组／poop／vein；动态元素另表，父资格不满足时不读旧payload。

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/FXLayers.zhl；7DF8F0；96C620/96CE20/96DB20/96EA50

本轮剩余核对：7DF8F0/968E10/96BF40/96BD30及所有已核pass；+9..B仍未知

| 0x0—0x7 Vector average_player_pos【状态／已核】ray屏幕位置计算实际消费；保留同world实际显示输入；不能以本端单身体重算 | 0x8—0x8 bool loaded【换算／已核】native pass活动条件；规范present；不复制未构造对象／强设loaded | 0x9—0xB bytes[3] unknown_9_B【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0xC—0x17 u32[3] config_stage_type_backdrop【权威／SDK注释／消费职责】旧配置选择／load输入；源码注释仅提示名；原值不发；实际选图和pass另发；不是无证padding | 0x18—0x23 vector<FX> layers【容器／已核】0x4C步长的实际选层；只发有序实际pass/资源/有效pose；不发vector头 | 0x24—0x2F vector<RayGroup> ray_groups【容器／已核】0x2C步长组；只发实际选择组及其有序成员；不发未选组／重新选组 | 0x30—0x33 i32 selected_ray_group【换算／已核】-1无光线组，否则实际选中索引；规范optional选中组；不需要复制全部候选组和原始索引 | 0x34—0x37 i32 poop_countdown【换算／已核】draw当>0采用；>150与<=150不同alpha曲线；只发最终present与alpha；不在Client递减 | 0x38—0x83 FXPoop poop_fx【换算／已核】原生完整FX内嵌体；7DF9EF/7DF9F2直接以FXLayers+38调用967D40构造；只发poop当前present／实际alpha／图像／ColorMod与有效pose；其余生成／过滤参数不发 | 0x84—0xAF ColorMod poop_color【状态／已核】poop实际draw颜色；present时发11个float；最终alpha资格另发 | 0xB0—0xBF Vector[2] bounds【状态／已核】FX和ray屏幕变换读取边界；同world显示值；不能从XML默认值猜 | 0xC0—0x2C7 Vein[10] vein_roots【容器／已核】10根内嵌节点，child可指外部节点；首根child存在时发有界图；child转索引，不传地址；无图时不读残留标量 | 0x2C8—0x4EF ANM2[2] vein_sprites【状态／已核】节点kind选择两份Sprite；draw覆写scale／rotation／颜色；图存在时保留资源／实际姿态／层覆盖；三项覆写值由节点派生 | 0x4F0—0x4F7 Vector vein_update_vector【权威／SDK注释／已读消费者】旧SDK注释提示相关向量；EA50/EAB0不作draw输入；不发；未赋未经核定业务名 | 0x4F8—0x597 FXParams params【换算／已核】FX配置到最终water/light/color的输入；以子表用途投影；不复制全A0 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## FXParams



证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/FXParams.zhl；967B00；818540/8134C0/803E20/816BB0

本轮剩余核对：967B00/96BCA0/818540/816BB0：+1C bool后3B仍无语义，不发A0宽块

| 0x0—0x13 KColor room_color【权威／已核】配置色，非Game current ColorModState；不发；最终Game current六值由第8步唯一承接 | 0x14—0x1B f32[2] brightness_contrast【权威／已核】整屏颜色生产输入；不发；不调用Compute/UpdateColorModifier | 0x1C—0x1C bool use_water_v2【状态／已核】实际水面反射pass选择；R2模式；本地图形选项另管 | 0x1D—0x1F bytes[3] unknown_1D_1F【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x20—0x33 KColor water_color【换算／已核】默认水色，可被seed79及Room插值覆盖；只发816BB0最终water参数，不能仅发这个默认色 | 0x34—0x47 KColor water_multiplier【换算／已核】默认水色，可被seed79及Room插值覆盖；只发816BB0最终water参数，不能仅发这个默认色 | 0x48—0x4B f32 shadow_alpha【换算／已核】8134C0叠楼层／物品／实体决定最终光照强度；只发最终lighting_alpha；不在Client调用依赖玩法的Getter | 0x4C—0x5F KColor shadow_color【换算／已核】叠pitch/darkness/lightning等规则前颜色；只发实际shadow_fill/light_tint和pass资格 | 0x60—0x73 KColor light_color【换算／已核】叠pitch/darkness/lightning等规则前颜色；只发实际shadow_fill/light_tint和pass资格 | 0x74—0x9F ColorMod water_effect_color【权威／已核】5CD070 Effect.Init多处分支取址用于已产生Effect颜色；Room不重复发；Entity最终Color由第3/4步承接，不在Client重跑Effect.Init |
|---|---|---|---|---|---|---|---|---|---|

## FXPose



证据：96C620/96DB20/96CE20；原生0x14步长

本轮剩余核对：9703B0按20B构造只写16B＋2flip；96C620/96DB20/96CE20不读+12..13，仍非padding证明

| 0x0—0x7 Vector position【状态／已核】实际有序纹理偏移；发送两个f32 | 0x8—0xF Vector velocity【权威／已读消费者】位置推进／Host颜色采样查询输入；当前draw不读这8B；不发；Client不运行颜色采样查询补生产，最终Entity颜色由Entity域承接 | 0x10—0x11 u8[2] flip_x_y【状态／已核】实际flip条件；两个规范bool或两位mask；不复制尾部未知2B | 0x12—0x13 bytes[2] unknown_12_13【未知／未知】声明边界内未解释尾部，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 |
|---|---|---|---|

## FX



证据：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0

| 0x0—0x3 u32 id【权威／已核】配置／生成身份；实际draw不按该值重新选；不上线；保留实际pass／图像即可 | 0x4—0xB ImageStrongPair image【资源／已核】已选图像与实际sampler/format；有资格的资源引用，control本地 | 0xC—0x17 vector<FXPose> poses【容器／已核】实际有序pose；只发有效prefix；不发头／容量 | 0x18—0x1B i32 count【派生／已核】draw使用数量；由集合长度表达，保留0的清空语义 | 0x1C—0x2B f32[4] velocity_ranges【权威／已核】XML xMin/xMax/yMin/yMax；生成每pose速度的四端点；留Host；有序当前pose位置／flip发送，不重新抽速度 | 0x2C—0x2F u32 pass【状态／已核】实际pass1..6选择；同步；未知pass不当空／不执行 | 0x30—0x33 i32 stage_filter【权威／已核】配置stage经旧编号换算后保存，供967DB0筛选；不发候选过滤条件；实际被采用层／资源发送 | 0x34—0x37 u32 alt_stage_mask【权威／已核】XML altStages过滤mask；不发；不在Client按楼层重新选层 | 0x38—0x38 bool only_default_backdrop【权威／已核】配置onlyDefaultBackdrop筛选条件；不发；实际选层已知 | 0x39—0x39 u8 parallax_mode【权威／已核】XML parallax存在时置1；96BF40据此按平均Player位置生成pose位置；模式留Host，实际当前pose发送；不复制跟随生产器 | 0x3A—0x3B bytes[2] unknown_3A_3B【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 | 0x3C—0x3F f32 condition color_condition【换算／已核】generic非mask且count>0分支比较==0；只发is_zero条件；NaN/Inf都按原==0语义归非零，不让其进入坐标浮点 | 0x40—0x43 u32 mask_backdrop【换算／已核】mask且count>0判断是否35；只发is_35布尔；不把非采用分支原值读上网 | 0x44—0x47 u32 blend_mode【状态／已核】pass1/2/3/6 BlendMode实际选择；仅实际分支发；不按Image猜 | 0x48—0x48 bool mask【状态／已核】generic分支shader选择；仅适用pass发规范bool；余3B未知 | 0x49—0x4B bytes[3] unknown_49_4B【未知／未知】声明边界内未解释尾部，不能称填充；不读不发；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## Ray



证据：96CE20实际20B步长；96F760/970060析构

| 0x0—0x3 f32 opacity【状态／已核】sin／pow曲线乘数；发实际值 | 0x4—0x7 f32 speed【权威／已核】Render末尾只用于phase+=speed，当前quad已提交；不发；受控draw撤销写回／禁止跨本地Render累计 | 0x8—0xB f32 phase【状态／已核】当前正弦相位；取正常Host该pass实际使用相位；同world重复展示从同一相位开始 | 0xC—0x13 ImageStrongPair image【资源／已核】实际有序光束图像；资源引用；control本地 |
|---|---|---|---|

## RayGroup



证据：96CE20组步长44B；client_presentation_native.cpp::NativeRayGroup

| 0x0—0x1B f32[7] geometry【状态／已核】实际光线组边界与几何因子；仅选中组发送；本地视口变换另管 | 0x1C—0x27 vector<Ray> rays【容器／已核】实际有序光线；仅选中非空组；不能对空组运行先除成员数的native分支 | 0x28—0x28 bool stage_matched【权威／已核】Load实际阶段匹配资格，决定是否纳入组vector；留Host；只发送真正选中组 | 0x29—0x29 bool stage_b_suffix【权威／已核】stages token的b后缀标记；不是绘制相位；留Host；不发原始过滤候选 | 0x2A—0x2B bytes[2] unknown_2A_2B【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 |
|---|---|---|---|---|

## Vein



证据：96EA50/96EAB0；Room ctor10×0x34；tx-04-18-room-application.md

本轮剩余核对：96ED60把+10复制给child；业务名未定且96EAB0不消费，4B仍未知；+30单byte退休控制已单列

| 0x0—0x3 i32 kind【状态／已核】实际两Sprite选择；发送，拒绝超已核0/1范围 | 0x4—0xF Vein*[3] children【引用／已核】三个有序child；有界自有图索引／Null；不传地址；循环／深度／预算失败不能截断 | 0x10—0x13 bytes[4] unknown_10_13【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x14—0x23 f32[4] rotation_scale_factor_opacity【状态／已核】节点最终绘制变换与透明度；发送；Sprite对应字段由本节点覆写，不重复发 | 0x24—0x27 i32 depth【状态／已核】alpha计算实际消费；发送；不把它当递归安全深度替代结构检查 | 0x28—0x2F Vector position【状态／已核】节点绘制位置；发送 | 0x30—0x30 bool retire_or_shrink【权威／已核】树生长／收缩控制字节；继承到child，影响后续factor和删除；不发；当前factor／opacity／图拓扑已有实际显示后态 | 0x31—0x33 bytes[3] unknown_31_33【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 |
|---|---|---|---|---|---|---|---|

## RailManager

SDK_gridflags[447]少一项：sprite从+1C4开始，flags实际448字节恰好接上。

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RailManager.zhl；9B7C70/9B7F00；7DFF75..7DFF91

| 0x0—0x3 Room* room【本地／已核】回指本地Room；由本地owner构造；不发 | 0x4—0x1C3 u8[448] grid_flags【状态／已核】0..447实际轨道，255无轨；不是Grid槽成员；只读width*height真实extent；线上u16 extent＋有序字节；尾部不读，本地清255 | 0x1C4—0x2D7 ANM2 sprite【换算／已核】native绘制每格强制rail/pitrail、帧；只发实际资源／非覆写图层与overlay外观；main动画帧由旗标派生；SetAnimation会重建索引数组须正确借用 |
|---|---|---|

## Camera

跟随算法与视口本地；Host强制焦点的显示意图有独立optional投影，不能抹掉。

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Camera.zhl；91C730/91E920/91EBF0/91EC20

本轮剩余核对：91C730/91EBF0/91EC20/91E920/更新族：+8C bool与+90 f32之间3B仍未知

| 0x0—0x5F CameraRing[6] rings【本地／已核】6个环形历史，容量60/90/300/15/4/120；不传指针／容量／历史；必要强制焦点只投影最后有效Vector | 0x60—0x67 Vector follow_accumulator【本地／已核】跟随历史／滤波中间值；不发；最终Room.render_scroll与C/S镜头策略消费明确当前值 | 0x68—0x6B Room* room【本地／已核】回指本地Room；构造owner提供，不写Host地址 | 0x6C—0x73 Vector current_scroll【本地／已核】跟随／本地视口数值；current_scroll有实际可见性消费者；不整体复制五向量；共享画面用Room.scroll，若选本端镜头则由独立C/S纯UI策略维护 | 0x74—0x7B Vector viewport_half_size【本地／已核】跟随／本地视口数值；current_scroll有实际可见性消费者；不整体复制五向量；共享画面用Room.scroll，若选本端镜头则由独立C/S纯UI策略维护 | 0x7C—0x83 Vector follow_target【本地／已核】跟随／本地视口数值；current_scroll有实际可见性消费者；不整体复制五向量；共享画面用Room.scroll，若选本端镜头则由独立C/S纯UI策略维护 | 0x84—0x8B Vector follow_previous【本地／已核】跟随／本地视口数值；current_scroll有实际可见性消费者；不整体复制五向量；共享画面用Room.scroll，若选本端镜头则由独立C/S纯UI策略维护 | 0x8C—0x8C bool should_override【换算／已核】Camera强制焦点资格，IsPosVisible可读focus ring最后项；需要时发optional focus意图／最后有效坐标；不发完整ring／重放SetFocus/Update | 0x8D—0x8F bytes[3] unknown_8D_8F【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x90—0x93 f32 smoothing【本地／已核】本地跟随算法参数；不发；冻结显示不运行跟随算法 |
|---|---|---|---|---|---|---|---|---|---|

## CameraRing



证据：91EF60/91EFE0；91C730；91E920

| 0x0—0x3 Vector* data【本地／已核】本地环形缓冲管理；不上线；focus projection仅在实际count>0且边界合法时取最后有效Vector | 0x4—0x7 i32 capacity【本地／已核】本地环形缓冲管理；不上线；focus projection仅在实际count>0且边界合法时取最后有效Vector | 0x8—0xB i32 count【本地／已核】本地环形缓冲管理；不上线；focus projection仅在实际count>0且边界合法时取最后有效Vector | 0xC—0xF i32 head【本地／已核】本地环形缓冲管理；不上线；focus projection仅在实际count>0且边界合法时取最后有效Vector |
|---|---|---|---|

## RoomShockwave



证据：7DFBCB..7DFC4D；7F7F60；804500波纹；8131D0

| 0x0—0x7 Vector origin【换算／已核】本次波纹中心；只发实际采用的最终pass输入；不以空值代替不适用 | 0x8—0xB f32 radius【换算／已核】推进参数与当前绘制计算混用；只发最终半帧radius／包络／中心／pass资格；未用未来值不上线 | 0xC—0xF f32 strength【换算／已核】推进参数与当前绘制计算混用；只发最终半帧radius／包络／中心／pass资格；未用未来值不上线 | 0x10—0x13 f32 speed【换算／已核】推进参数与当前绘制计算混用；只发最终半帧radius／包络／中心／pass资格；未用未来值不上线 | 0x14—0x17 i32 current【换算／已核】推进参数与当前绘制计算混用；只发最终半帧radius／包络／中心／pass资格；未用未来值不上线 | 0x18—0x1B i32 maximum【换算／已核】推进参数与当前绘制计算混用；只发最终半帧radius／包络／中心／pass资格；未用未来值不上线 |
|---|---|---|---|---|---|

## RoomHellBackdrop

连续内嵌显示组件视图；此边界来自后续Room独立flag，不是SDK独立类sizeof断言。

证据：7DFFC4..7DFFE6；82E960/82EE90/82EFD0/82F210

本轮剩余核对：82E960/82EE90/82EFD0/82F210：活动+118后3B仍未知

| 0x0—0x3 Room* room【本地／已核】回指当前Room；不发 | 0x4—0x117 ANM2 sprite【状态／已核】地狱背景实际资源、层与当前帧；活动时保留实际资源／必要姿态；pass临时色／quad按明确绘制规则处理 | 0x118—0x118 bool active【换算／已核】82F210入口检查的当前资格；线上present／当前活动分支，不能由本地Stage重新猜 | 0x119—0x11B bytes[3] unknown_119_11B【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x11C—0x11F f32 display_offset【状态／已核】82F210读this[71]的实际当前滚动坐标；发当前值，不按Client帧／speed外推 | 0x120—0x127 f32[2] speed_and_accumulator【权威／已核】82EE90还用speed移动实体／累计下次offset；留Host；禁止调用完整Update |
|---|---|---|---|---|---|

## EntityList



证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityList.zhl；413270/413740；Entity域同步职责

| 0x0—0x1F NativeSpatialIndex*[8] collision_partitions【本地／已核】八个实体空间分区／碰撞索引的本地指针；不传指针或分区网格，不在Client执行碰撞生产；最终Entity后态和显示顺序另承接 | 0x20—0x2F EntityListEL main【容器／已核】原生分类索引，不是八套Entity身份；本地从正式成员和有序render关系建索引；不复制更新列表／缓存 | 0x30—0x3F EntityListEL persistent【容器／已核】原生分类索引，不是八套Entity身份；本地从正式成员和有序render关系建索引；不复制更新列表／缓存 | 0x40—0x4F EntityListEL update【容器／已核】原生分类索引，不是八套Entity身份；本地从正式成员和有序render关系建索引；不复制更新列表／缓存 | 0x50—0x5F EntityListEL render【容器／已核】原生分类索引，不是八套Entity身份；本地从正式成员和有序render关系建索引；不复制更新列表／缓存 | 0x60—0x6F EntityListEL effect【容器／已核】原生分类索引，不是八套Entity身份；本地从正式成员和有序render关系建索引；不复制更新列表／缓存 | 0x70—0x7F EntityListEL temp【容器／已核】原生分类索引，不是八套Entity身份；本地从正式成员和有序render关系建索引；不复制更新列表／缓存 | 0x80—0x8F EntityListEL buffer_template【容器／已核】原生分类索引，不是八套Entity身份；本地从正式成员和有序render关系建索引；不复制更新列表／缓存 | 0x90—0x9F EntityListEL wisp【容器／已核】原生分类索引，不是八套Entity身份；本地从正式成员和有序render关系建索引；不复制更新列表／缓存 | 0xA0—0xA7 u32[2] render_pass_cursor【派生／已核】Room.Render在Room+12B8/+12BC写当前绘制游标／pass；本地受控pass维护；不发Host上次绘制scratch | 0xA8—0xAF i32[2] alive_enemy_counts【权威／已核】新加入增量与已统计敌人数；GetAliveEnemiesCount钳sum到非负；留Host；不为了Getter相等发送计数缓存 | 0xB0—0xB7 i32[2] alive_boss_counts【权威／已核】新加入增量与已统计Boss数；GetAliveBossesCount钳sum到非负；留Host；真实Boss／HUD显示另承接 | 0xB8—0xBB i32 enemy_count_scratch【权威／已核】Update循环中的敌人计数／阈值scratch；留Host；不是显示状态 | 0xBC—0xBF f32 enemy_damage_inflicted【权威／已核】本帧敌人总伤害；Wiki用于伤害充能，不是画面强度；留Host；当前充能／属性HUD已由Player/Presentation承接 | 0xC0—0xC3 i32 previous_enemy_count【权威／已核】Update统计快照，4147A1写本轮统计数；留Host；不重复同步派生计数 | 0xC4—0xCF vector storage header collision_pair_bits【本地／已核】415880按参与碰撞实体数N²准备的位存储；不复制头或collision pair结果；Client不模拟碰撞 | 0xD0—0xD3 u32 collision_pair_cursor【本地／已核】碰撞pair处理游标，415880每轮清零；不上线 | 0xD4—0xD7 u32 collision_subject_count【本地／已核】415880按实际参与实体编号得到的碰撞规模；不上线；不是网络成员数 | 0xD8—0xDF ImageStrongPair shadow_image【资源／已核】构造加载gfx/shadow.png；RenderShadows直接使用本地资源；正常本地标准资源owner；不发Image/control地址，不复制查询／更新系统 | 0xE0—0xE7 map<(Type,Variant,Subtype),EntityListEL> header type_query_cache【本地／已核】416820仅Cache=true时查询／建立本帧类型查询缓存；树节点44B；不发缓存或借用EL；正式Entity成员／身份已独立承接 | 0xE8—0xEB u32 online_checksum_pending_word【本地／已核】原版Online比较生成器的待处理4B；C/S不读取／复制／计算；不执行相关函数 | 0xEC—0xEC u8 online_checksum_pending_bytes【本地／已核】原版Online pending byte数，非普通UI bool；C/S不读取／复制／计算 | 0xED—0xEF bytes[3] unknown_ED_EF【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 | 0xF0—0xF3 u32 online_checksum_accumulator【本地／已核】原版Online内部比较累计值，初值FEDCBA76；C/S不读取／复制／计算，不作为任何推进／一致性条件 | 0xF4—0xF7 u32 online_checksum_control【本地／已核】原版Online分支的累计控制，ctor置1；不接入C/S | 0xF8—0x3F7 OriginalOnlineEntityFrame[32] online_frames【本地／已核】原版Online分支32帧环；每帧事件vector、6EB740返回值及比较值；全768B不进入C/S；不创建／回放／比较该环 | 0x3F8—0x3FB u32 online_frame_head【本地／已核】原版Online环头，按0x1F取模；不上线，不借作C/S时钟 | 0x3FC—0x3FF u32 online_frame_count【本地／已核】原版Online环有效帧数，最多32；不上线，不借作C/S完整性条件 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## EntityListEL



证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityList_EL.zhl；client_presentation_native.cpp::Members/FullSort

本轮剩余核对：SDK bool+0与ptr+4；构造／Destroy／QueryType只按已核成员访问，+1..3仍未知

| 0x0—0x0 bool sublist【本地／SDK声明】是否借用子列表的内存所有权条件；不发；本地独立owner维护 | 0x1—0x3 bytes[3] unknown_1_3【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x4—0x7 Entity** data【本地／SDK声明＋既有应用】EL本地数组管理；其中顺序语义由正式Entity关系承接；不上线原生头；保留真实render tie顺序，不按地址排序 | 0x8—0xB u32 capacity【本地／SDK声明＋既有应用】EL本地数组管理；其中顺序语义由正式Entity关系承接；不上线原生头；保留真实render tie顺序，不按地址排序 | 0xC—0xF u32 size【本地／SDK声明＋既有应用】EL本地数组管理；其中顺序语义由正式Entity关系承接；不上线原生头；保留真实render tie顺序，不按地址排序 |
|---|---|---|---|---|

## RoomTemporaryEffects

与Player的同名对象不同实例：父用途限定为Host，未知仍按物理字节列出。

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/TemporaryEffects.zhl；804500/7F7F60；布局136

本轮剩余核对：SDK bool+10/+11与ptr+14；已有布局136不解释+12..13，本轮不改其证据等级

| 0x0—0x3 u32 cache_mask【权威／既有布局136】效果重算控制；留Host | 0x4—0xF vector<TemporaryEffect> effects【容器／已核】效果生产列表；Room列表留Host；仅实际显示条件有具名投影 | 0x10—0x11 bool[2] evaluate_disabled【权威／已核】更新／禁用条件；不发原值；最终实际显示资格由正常Host分支取结果 | 0x12—0x13 bytes[2] unknown_12_13【未知／未知】本轮未解释；包括未核字段／可能对齐，不能称填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x14—0x17 Entity_Player* player【本地／已核】实例回指，不是网络玩家身份；不发地址 |
|---|---|---|---|---|

## Vector

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

证据：mod/binding/state/base.json::Vector

| 0x0—0x3 f32 x【状态／已核】X 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 f32 y【状态／已核】Y 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|

## ColorMod

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

证据：mod/binding/state/base.json::ColorMod

| 0x0—0x3 f32 tint_r【状态／已核】tint RGBA 的 R 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 f32 tint_g【状态／已核】tint RGBA 的 G 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8—0xB f32 tint_b【状态／已核】tint RGBA 的 B 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xC—0xF f32 tint_a【状态／已核】tint RGBA 的 A 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10—0x13 f32 colorize_r【状态／已核】colorize RGBA 的 R 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x14—0x17 f32 colorize_g【状态／已核】colorize RGBA 的 G 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x18—0x1B f32 colorize_b【状态／已核】colorize RGBA 的 B 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x1C—0x1F f32 colorize_a【状态／已核】colorize RGBA 的 A 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x20—0x23 f32 offset_r【状态／已核】offset RGB 的 R 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x24—0x27 f32 offset_g【状态／已核】offset RGB 的 G 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x28—0x2B f32 offset_b【状态／已核】offset RGB 的 B 分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|---|---|---|---|---|---|

## KColor

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

证据：mod/binding/state/base.json::KColor

| 0x0—0x3 f32 red【状态／已核】红色分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 f32 green【状态／已核】绿色分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8—0xB f32 blue【状态／已核】蓝色分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xC—0xF f32 alpha【状态／已核】透明度分量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10—0x13 u32 color_space【状态／已核】颜色空间标记：已核0=RGB、1=sRGB、2=HSL；不将当前观察标签冒充穷举集合；仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|

## AnimationState

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

证据：mod/binding/state/base.json::AnimationState

本轮剩余核对：403DE0/402CD0/403160及已核draw：+14 bool后3B未找到独立语义；仍未知

| 0x0—0x3 ANM2* animation_owner【本地／已核】回指所属 ANM2；由上层对象关系确定；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 AnimationData* active_animation【资源／已核】当前动画资源项；序号由 ANM2 文件表解析，空指针使用资源空值标记；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8—0xB int* layer_frames【容器／已核】各图层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._layerCount；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xC—0xF int* null_layer_frames【容器／已核】各空层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._nullLayerCount；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10—0x13 f32 anim_frame【状态／已核】动画当前帧值；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x14—0x14 bool is_playing【状态／SDK声明】播放标志兼绘制插值输入；直接安装最终值，不授权Client推进动画；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x15—0x17 bytes[3] unknown_15_17【未知／未知】SDK 字段间未解释的三个字节；不认作填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x18—0x1B i32 currently_triggered_events【权威／已核】Host动画事件消费位；Client安装最终姿态，不推进或重放事件，构造／换动画后本地清零；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x1C—0x1F i32 previously_triggered_events【权威／已核】Host动画事件历史位；不表示可靠事件队列，不进入Client状态或漂移比较；仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|---|---|---|---|

## LayerState

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

证据：mod/binding/state/base.json::LayerState

本轮剩余核对：402CD0只单字节复制+31，407F7A清零；无证据将其当shader；+75..77仍未知

| 0x0—0x3 LayerData* layer_data【派生／推断】由所属 ANM2 文件及数组序号定位的资源定义；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 ANM2* animation_owner【本地／SDK声明】所属 ANM2 回指，不作为网络状态；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8—0x1F std::string spritesheet_path【资源／SDK声明】spritesheet路径元数据资源号；与实际Image选择独立，路径变更不授权Client抢先重载；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x20—0x23 u32 min_filter_mode【状态／SDK声明】缩小过滤模式；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x24—0x27 u32 mag_filter_mode【状态／SDK声明】放大过滤模式；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x28—0x2B u32 wrap_s_mode【状态／SDK声明】S 方向环绕模式；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x2C—0x2F u32 wrap_t_mode【状态／SDK声明】T 方向环绕模式；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x30—0x30 bool sprite_sheet_loaded【本地／SDK声明】本地图层资源加载／重载维护标志；ReplaceSpritesheet可置false同时保留旧Image，不跨进程复制，也不按Host原值覆盖Client加载结果；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x31—0x31 u8 unknown_31【未知／未知】J273 LoadGraphics 将该字节清零；具体语义未明，绝非已证填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x32—0x32 bool flip_x【状态／SDK声明】水平翻转；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x33—0x33 bool flip_y【状态／SDK声明】垂直翻转；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x34—0x3B Vector size【状态／SDK声明】图层尺寸；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x3C—0x3F f32 rotation【状态／SDK声明】图层旋转角度；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x40—0x47 Vector position【状态／SDK声明】图层位置；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x48—0x73 ColorMod color【状态／SDK声明】图层 ColorMod；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x74—0x74 bool visible【状态／SDK声明】图层显示开关；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x75—0x77 bytes[3] unknown_75_77【未知／未知】SDK 布局间未说明的三个字节；不认作填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x78—0x8B BlendMode blend_mode【状态／已核】图层混合模式的五 DWORD 值；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8C—0x8F u32 render_flags【状态／SDK声明】图层渲染位标志；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x90—0x97 Vector crop_offset【状态／SDK声明】图层裁剪偏移；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x98—0x9F KAGE_SmartPointer_ImageBase image_selection【资源／推断】当前实际ImageBase选择，Null使用0xFFFF；Host由ImageBase._name形成资源身份，Client重建引用；重名／程序图像身份仍待第8步核；仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## ANM2

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

证据：mod/binding/state/base.json::ANM2

本轮剩余核对：402CD0单字节复制+108，403160交换，403530 word清108/109；仅证明copy／reset，不证明其业务含义；71..73/102..103/10A..10B仍未知

| 0x0—0x17 std::string file_name【资源／SDK声明】可空ANM2文件资源选择；Host loaded=false编码0xFFFF，true编码文件资源号；不以路径非空猜测存在性；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x18—0x2F std::string default_animation_name【容器／已核】默认动画名字符串；load 阶段读取，保留有界字节串，不当作不可变文件派生量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x30—0x4F AnimationState main_state【状态／SDK声明】主动画状态与独立图层/空层帧选择器；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x50—0x6F AnimationState overlay_state【状态／SDK声明】叠加动画状态与独立图层/空层帧选择器；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x70—0x70 bool overlay_render_first【状态／SDK声明】叠加动画先绘制开关；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x71—0x73 bytes[3] unknown_71_73【未知／未知】SDK 字段间未解释字节；不认作填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x74—0x77 AnimationData* animation_data【派生／已核】由 ANM2 文件动画表建立的资源数组首址；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x78—0x7B i32 animation_count【派生／已核】由 ANM2 文件动画表确定的动画数量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x7C—0x7F LayerState* layer_states【容器／已核】按 layer_count 遍历图层实例状态；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x80—0x83 u32 layer_count【派生／已核】由 ANM2 文件图层数组确定的图层数；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x84—0x87 std::string* null_layer_names【派生／已核】由文件建立的空层名称表；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x88—0x8B u32 null_layer_count【派生／已核】由文件建立的空层名称数量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8C—0x8F std::string* event_names【派生／已核】由文件建立的事件名表；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x90—0x93 u32 event_count【派生／已核】由文件建立的事件数量；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x94—0x9B Vector offset【状态／SDK声明】绘制偏移；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x9C—0xA3 Vector scale【状态／SDK声明】绘制缩放；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xA4—0xA7 f32 rotation【状态／SDK声明】全局旋转；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xA8—0xD3 ColorMod color【状态／SDK声明】全局颜色；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xD4—0xFF ColorMod champion_color【状态／SDK声明】精英色调；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x100—0x100 bool flip_x【状态／SDK声明】水平翻转；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x101—0x101 bool flip_y【状态／SDK声明】垂直翻转；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x102—0x103 bytes[2] unknown_102_103【未知／未知】SDK 字段间未解释字节；不认作填充；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x104—0x107 f32 playback_speed【状态／SDK声明】播放速度兼绘制插值的半帧比例输入；不在Client推进动画；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x108—0x108 u8 unknown_108【未知／未知】初始化为零但语义未明；不作为 shader 状态；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x109—0x109 bool loaded【本地／已核】本地ANM2构造／加载／重置结果；Host只用它确定file_name的可空资源选择，Client由资源owner完成准备后维护，不直接传布尔字节；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10A—0x10B bytes[2] unknown_10A_10B【未知／未知】+0x109 与 shadowLayerIndex +0x10C 间未解释字节；不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充 | 0x10C—0x10F i32 shadow_layer_index【派生／已核】资源中名称为 shadow 的 LayerState 数组序号；不存在时为 -1；仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x110—0x113 u32 render_flags【状态／SDK声明】ANM2 渲染位标志；仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## ImageStrongPair



证据：J273 smart-pointer helpers

| 0x0—0x3 ImageBase* image【资源／已核】当前实际选中的图像；资源号／具名程序图像结果；不传地址 | 0x4—0x7 Control* control【本地／已核】引用计数、析构及所有权；不发送；本地正常owner |
|---|---|

## RNG

普通owner默认Host；两个纯绘制实例另有独立视图，不把该默认传播为它们的发送决定

证据：RNG.zhl；J273 VA7DF4A0/7DF4E0/7DF540/7DF5B0；random-synchronization-review.md §3

| 0x0—0x3 u32 seed【权威／已核】当前xorshift状态；推进覆盖本字，不是身份；留Host；原值、存储地址与控制历史不进入状态帧 | 0x4—0x7 u32 shift_1【权威／已核】右移量；参数不是独立cursor；留Host；原值、存储地址与控制历史不进入状态帧 | 0x8—0xB u32 shift_2【权威／已核】左移量；留Host；原值、存储地址与控制历史不进入状态帧 | 0xC—0xF u32 shift_3【权威／已核】最后右移量；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|---|---|

## FXPoop

FXLayers+38内嵌同样4Ch的FX；以poop专用资格及最终alpha读取，不走generic pass规则。

证据：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0

| 0x0—0x3 u32 id【权威／已核】配置／生成身份；实际draw不按该值重新选；不上线；保留实际pass／图像即可 | 0x4—0xB ImageStrongPair image【资源／已核】已选图像与实际sampler/format；有资格的资源引用，control本地 | 0xC—0x17 vector<FXPose> poses【容器／已核】实际有序pose；只发有效prefix；不发头／容量 | 0x18—0x1B i32 count【派生／已核】draw使用数量；由集合长度表达，保留0的清空语义 | 0x1C—0x2B f32[4] velocity_ranges【权威／已核】XML xMin/xMax/yMin/yMax；生成每pose速度的四端点；留Host；有序当前pose位置／flip发送，不重新抽速度 | 0x2C—0x2F u32 pass【权威／已核实例用途】实际pass1..6选择；poop的这些通用配置／pass位不作直接draw输入，不读不发；使用poop实际图像、颜色、有效pose和最终alpha | 0x30—0x33 i32 stage_filter【权威／已核】配置stage经旧编号换算后保存，供967DB0筛选；不发候选过滤条件；实际被采用层／资源发送 | 0x34—0x37 u32 alt_stage_mask【权威／已核】XML altStages过滤mask；不发；不在Client按楼层重新选层 | 0x38—0x38 bool only_default_backdrop【权威／已核】配置onlyDefaultBackdrop筛选条件；不发；实际选层已知 | 0x39—0x39 u8 parallax_mode【权威／已核】XML parallax存在时置1；96BF40据此按平均Player位置生成pose位置；模式留Host，实际当前pose发送；不复制跟随生产器 | 0x3A—0x3B bytes[2] unknown_3A_3B【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 | 0x3C—0x3F f32 condition color_condition【权威／已核实例用途】generic非mask且count>0分支比较==0；poop的这些通用配置／pass位不作直接draw输入，不读不发；使用poop实际图像、颜色、有效pose和最终alpha | 0x40—0x43 u32 mask_backdrop【权威／已核实例用途】mask且count>0判断是否35；poop的这些通用配置／pass位不作直接draw输入，不读不发；使用poop实际图像、颜色、有效pose和最终alpha | 0x44—0x47 u32 blend_mode【权威／已核实例用途】pass1/2/3/6 BlendMode实际选择；poop的这些通用配置／pass位不作直接draw输入，不读不发；使用poop实际图像、颜色、有效pose和最终alpha | 0x48—0x48 bool mask【权威／已核实例用途】generic分支shader选择；poop的这些通用配置／pass位不作直接draw输入，不读不发；使用poop实际图像、颜色、有效pose和最终alpha | 0x49—0x4B bytes[3] unknown_49_4B【未知／未知】声明边界内未解释尾部，不能称填充；不读不发；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## RoomOutputSetHeader



证据：7DFE55；817F30；818250

| 0x0—0x3 TreeNode* head【本地／已核】MSVC std::set的本地sentinel／root owner；不发地址或遍历为同步数据 | 0x4—0x7 u32 size【权威／已核】事件组内格序号个数；整组留Host，不为C/S发送计数 |
|---|---|

## RoomCallbackVector



证据：818250 group*12的begin/end；7E0190逐12B析构

| 0x0—0x3 void(**)() begin【本地／已核】函数指针数组begin；不传、不安装、不远程调用 | 0x4—0x7 void(**)() end【本地／已核】已注册函数末端；不传 | 0x8—0xB void(**)() capacity【本地／已核】本地vector容量尾；不传 |
|---|---|---|

## RainSpawnRegion



证据：819530构建；8199D0 +24逐区随机生成Effect135

| 0x0—0x7 Vector origin【权威／已核】雨采样区域origin；不传；实际Effect后态另发 | 0x8—0xF Vector basis_a【权威／已核】采样区域基向量A；不传 | 0x10—0x17 Vector basis_b【权威／已核】采样区域基向量B；不传 |
|---|---|---|

## OriginalOnlineEntityFrame



证据：413050六DWORD构造；413F00原版Online条件分支；4130F0→417EB0按56B元素析构

| 0x0—0x3 u32 frame【本地／已核】原版Online帧号；不进入C/S | 0x4—0xF vector<OriginalOnlineEvent56> events【本地／已核】原版56B实体事件vector头；不进入C/S、不建立原版事件owner | 0x10—0x13 u32 native_comparison_input【本地／已核】原版Online正常路径保存的6EB740返回值；不将未核业务名当同步RNG；不读取或计算作为C/S一致性输入 | 0x14—0x17 u32 comparison_value【本地／已核】原版Online实体比较累计值；不读取或计算作为C/S一致性输入 |
|---|---|---|---|

## KilledNPCRecord



证据：815FB0：分配32B链表node，+8写16B＋8B；本视图只覆盖payload

| 0x0—0x7 Vector position【权威／已核】死亡记录位置；历史留Host | 0x8—0xB i32 type【权威／已核】记录NPC类型，部分类型归一；历史留Host | 0xC—0xD i16 variant【权威／已核】记录variant窄值；历史留Host | 0xE—0xF i16 subtype【权威／已核】记录subtype窄值；历史留Host | 0x10—0x13 u32 seed【权威／已核】Entity+3EC的记录seed；不下发历史／重生输入 | 0x14—0x14 bool friendly【权威／已核】flags的友好状态投影；历史留Host；当前实体另有后态 | 0x15—0x17 bytes[3] unknown_15_17【未知／未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；不读不发；窄字段读写跳过该范围，保留未决 |
|---|---|---|---|---|---|---|

## 状态二进制布局草稿

这里只给已定小记录；Room完整资源／引用／图、波纹与跨域pass的最终字段编号和容器头留第9步。reverse_water_gfx新增必要1B方向投影或等价有符号phase，需与当前实际配置／资源同版本；不声称已有正式writer。

**R0 RoomGeometry：最大分支36B**。world/epoch身份、资源版本由外层提供；配置与descriptor归Level，非重新生成命令

| +0x0 i32 descriptor_index（4B） | +0x4 i32 dimension（4B） | +0x8 u32 room_type（4B） | +0xC u32 width（4B） | +0x10 u32 height（4B） | +0x14 f32 bounds[4]（16B） |
|---|---|---|---|---|---|

**R1 RoomTiming：最大分支8B**。以同world GameFrame解释；不在Client推进

| +0x0 u32 frame_baseline（4B） | +0x4 i32 greed_wave_timer（4B） |
|---|---|

**WaterShaderResult：最大分支32B**。816BB0最终：color.alpha已乘waterAmount、multiplier已按waterAmount归一；KColor空间标签保留；不是四端点＋timer

| +0x0 KColor final_color（20B） | +0x14 f32 final_multiplier_rgb[3]（12B） |
|---|---|

**FXPoseVisual：最大分支10B**。不发8B速度／2B未知；有序有效prefix，count另由容器编码

| +0x0 Vector position（8B） | +0x8 u8 flip_x（1B） | +0x9 u8 flip_y（1B） |
|---|---|---|

**RayVisual：最大分支10B**。不发speed／control；实际选组、geometry与显示资格另有记录

| +0x0 f32 opacity（4B） | +0x4 f32 phase_used（4B） | +0x8 u16 image_resource（2B） |
|---|---|---|

**PoopEnvelope：最大分支5B**。图像、颜色和有效pose独立资源/容器；present=false只发首字节

| +0x0 u8 present（1B） | +0x1 f32 actual_alpha（4B） |
|---|---|
