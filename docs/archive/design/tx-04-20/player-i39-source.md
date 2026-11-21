# I39：非活动身体来源修复与主审

2026-11-04开始，2026-11-05续核，J273，VA基址0x400000。来源：本地原版EntityPlayer Wiki、REPENTOGON EntityPlayer／PlayerManager Wiki，官方Lua wrapper、EntityPlayer.zhl及现有R01—R16；有具体缺口后由一个GPT-6 Luna max子智能体只读逆向，主智能体独立复读下列采用点位。没有动态逆向、IDB修改或哈希校验。

## 实际问题

Source01导出baseline和0—15共17份目标；16的非roster Soul只有BaseInitReturned=1，八类私有补充NotCaptured。公开Getter可调用不等于私有量取得初始化依据。另发现PlayerLink把真实Null槽送进默认Unresolved的身份查询；已局部修正，非空未知对象仍Unknown。

## 原生点位和纠错

| 点位 | 主审结论 |
| --- | --- |
| 7C1F40／RVA3C1F40 | thiscall、零栈参，7C215C ret。PlayerIndex>=0且16/17根、sub为空时，7C20F4分配2EA0、7C210D ctor、7C212B先挂+1D84、7C213F虚Init对端17/16。完整helper正常返回才形成证言；不是整块copy。 |
| 74F6A0／RVA34F6A0 | 官方Player虚Init(type,variant,subtype,seed)，4栈参；其7B2C90 setup可创建非活动身体。基Init6A4A20返回不是完整Player Init返回。 |
| 7C4620／RVA3C4620 | 官方ChangePlayerType(int,bool)；7C48E5调用上述sub helper。29备用分支7C549F先挂root+1E50，7C54AF虚Init(type38)，7C54B8再写child+1E50=root。主审纠正初读的“Init后才挂父指针”顺序。 |
| 7A3A40／RVA3A3A40 | thiscall两个栈word，第一个是0..3主动槽、第二个低字节语义仍未核定，原样透传；7A44A8 ret8，保存原始返回word，不用EAX判成功。新槽new/ctor/虚Init后配置，已有槽复用。7A4217把旧active写回Game+1BB24+4*VarData。 |
| 995E10／RVA595E10 | 已有Replace观察器给出真实AL成功。Esau helper不检查其AL，所以还要求此成功及替代体恰一次进真实roster、旧active离开roster且在VarData对应槽。 |
| 79E3E0／9A4F00／7A7D30／751C10 | 替换旧active时做离场清理、清理／重置内嵌GameStatePlayer的动态存储并StoreGameState，751C10释放武器与子引用后置零；没有对旧EntityPlayer自身做析构、重构、Init或整只赋值。先前把9A4F00称作“初始化”不够准确，本轮复读作此纠正。旧身体读取依据不从新active借来；各被清理成员按当前Null／空值采集。 |
| A96140 | Esau创建异常前缀可能已挂槽而EH释放对象，未闭合同时清槽/VarData；异常停止相关正式采集，不继续解引用槽或猜测恢复。 |

## 实现合同

新增四个只读owner进入唯一常驻Hook清单，均原样调用original恰一次；Client安装器仍只用基结构构造，不调用这些玩法owner补算。

`Collector`用固定容量表记录真实owner内正常Ctor。嵌套owner全部正常收口后，核对local/use、最终sub或双向backup、类型和非roster关系，再给`NAMED_PLAYER_CHILD_INIT_COMPLETED=4`。没有出生号、active、roster或PlayerUpdate历史被补造。已有创建owner的原始出生流程不受此替代。

Esau只对入场真实703槽且原先有当前字段依据的旧active，在实际Replace AL成功及外层正常返回、同use、VarData1—4、真实槽／roster后态全部成立时给`NAMED_PLAYER_STATE_OWNER_COMPLETED=5`。两个证言不恢复输入或设备关系；Init／Restore／Acquire／Destroy撤销资格，逻辑输入重绑不把已初始化的存储假作未初始化。四槽仍由Authority拥有，不由观测器清理。

真实原版Online IsLocalPlayer隔离占位按external_reference责任保留；完整Player安装不消费原版Online locality。复杂效果／武器对象中的十个无请求参数query，其NotScheduled原样保留于只读视图；完整effects及五个Weapon槽仍严格按具名值安装，实际ReadFailed等不变成值，也不在Client执行查询补算。Source导出先执行正式完整Player纯值解析，缺口带身体Key和字段原因，不等到冷Client才暴露。

Source02已实际通过16/17，并在type29的Partial诊断中观察到38备用身体readiness=4、原生补充完整；type29整体仍因表现域AmbiguousReuse未通过，不能据此宣称全矩阵或Esau证言通过。未观察或异常的其它身体没有自动资格，不用现存指针或Getter返回补造历史。旧源的常规截图有伤害冷却隐藏帧，已改为常规场景清零、单列闪烁和清除场景，旧17份材料不复用作修复后的通过结果。

Source03的原始诊断定位到`HistoryHUD.GetPlayer(1)`所指local133/use2/type38；Entity及Player域已将同一身体登记KnownCurrent，表现域却因其新scope漏登记而AmbiguousReuse。公共`PlayerSources`漏了`GetFlippedForm`遍历。Luna与主智能体分别复读本地REPENTOGON Wiki `EntityPlayer.md`的GetFlippedForm及官方`LuaEntityPlayer.cpp`的`Lua_PlayerGetBackupPlayer`，确认它直接返回当前`_backupPlayer`。已加入每域遍历及窗口末边复核；保留原current/reuse判据。`HistoryHUD::HasTwin`只判断第二槽非空，未被用作存活或来源依据。修复待下一次已提醒的Source续测。

Source04已实际验证Flip遍历修复。Source05拆开的普通Flip又暴露旧active的读取资格被Replace通用撤销：同local/use且仍为backup，但readiness=0。Luna与主智能体复读J273 UseActiveItem的switch5B8820 case711→5C42E1；5C43E8调用995E10，替换正常后没有old Player ctor／Init／deleting dtor。旧体的武器／子引用会被751C10清理并置零、Body ANM可被403530卸载，7D3410清动态存储并将指针／计数归零。这不是旧状态完整保留，亦不是全私有域完整性证明。

沿现有Replace外层／原生双观察器补窄`PlayerStateOwnerCompleted=5`：在LifecycleEnter撤销资格前记旧体已具备的读取依据，限当前29/38互补且双向backup、旧唯一roster／新不在roster；正常内外层AL成功后要求同local/use/run、旧index<0且离roster、新唯一roster、关系仍相符。old的Init／Restore／Ctor／Destroy、全局Restore或角色重配使证言失效。保留原revision、FirstObserved和无birth；不赋输入资格。随后仍从真实旧体读取清理后的全部成员，Null、空集合和缺口各自保留，正式解析未通过则Source继续失败。Source04的重复成员原因仍未由此解决；拆分Birthright动作的结果待实机。

Source07已验证分开普通Flip、获得619、随后Flip均完整，集合次数均为0／1，旧普通backup取得5。第一次把记忆点接在collector.ObserveEnter仍过晚（Source06失败）；现已在原control_enter撤锚前记录，按深度／manager／实参／run消耗，不改变控制失效时序。

Source08失去619后已经真实回到一活动体及一backup，但旧hologram因资格撤销而Partial。Luna与主智能体复读77BD40的77C10C—77C29A：双方均无619且self非hologram时，互设backup、清双向twin，79E3E0清旧体，412D40置valid=0，另写Exists=0、index=-1并990520移出roster；无Ctor／Init。沿已有Player Update及真实native_reached标记补窄同实例转回证言：前态两体原有资格、唯一roster、互惠twin及相对index；完整原生正常返回后要求双向backup、无twin、旧体无效且离roster、主根唯一。Init／Restore／重配／销毁使证言失效，普通Update内任意新Ctor不据此得到4。Source09已实际验证此旧体为5且完整解析通过；随后所有0—40角色和具名专用状态通过，武器矩阵仍在修复，不外推冷Client应用已通过。

## 703使用入口的复核与主审（2026-11-05）

Source14四个真实slot0都装备703／charge1，但Lua `UseActiveItem(703,257,0)`未产生任何备份，正式ItemState均0。先读原版UseFlag／EntityPlayer与REPENTOGON修改说明，再读官方`LuaEntityPlayer.cpp:3247`：数字flags、slot及VarData直接转入原生，Lua数字0不会被当false而丢掉slot。文档的USE_OWNED语义不足以证明加该位就能触发703。

同一Luna max只读定点复核后，主智能体独立执行switch还原并复读：`5B8820` case703确到`5B4E3F/RVA1B4E3F`，这里只复制内部byte后跳`5BC6C3`；此块及所读公共finalizer没有直接`SetItemState`／`7A3A40`调用。这不证明所有Lua调用条件永远无效；有效flags组合仍UNKNOWN，未按猜测增加OWNED。`5BED63/RVA1BED63`虽直接调用helper，但其前为`+1744 voidedCollectibles`循环／排除Void477的递归Use，随后`5BED54`检查`+15F4 ItemState!=0`，不能拿它当普通703分支。

正常Player Update `77BD40`在`77DE1D/RVA37DE1D`以player ECX调用`773990`。主审另读`7772E5—7772EB`：push0、push0、player ECX后调用`7A3A40`。helper `7A3A8F—7A3AAA`先要求slot<=3和该真实descriptor非空，既有703专用分支／四槽合同保持。采用tx09已实机验证的canonical主动输入：Lua只装备和充能，Host物理键及三个远端走同一意图流，各组等实际根及绑定代次变化后再触发下一组。没有调用私有helper、补ItemState或伪造槽。UseActiveItem的short输出是结果flags，不作为Esau变换成功条件；最终仍检查真实四槽及正式完整来源。

Source18对三远端Esau入场给出直接诊断：`readiness=2,current_public=0`，Host为`3/1`。依据2是已有创建／初始化完成的存储历史，逻辑撤绑只撤当前公开资格，不能把它等同0／1；Esau入口仅对既有2或仍具current_public资格的3以上接纳。仍须同次703具名owner、正常内外返回、实际Replace AL成功、唯一替代roster和旧体真实槽后态才给5。Ctor／Init／Restore／Destroy／角色重配和全局恢复使pending失效；Source18随后取得全域Complete及13个Player，尚未越过解析遍历预算，不能写作Source完成。

995E10另由同一个Luna max与主智能体复核：new+170有效时走索引／槽交换，995F33返回AL1；无效new则于995F37—995F49遍历查找旧指针，未找到在995F55返回AL0，找到于995F7B写替代指针，完整清理后996497返回AL1。不存在“index0成功、远端天然失败”的分支。调用7A41F8／7A41F9压new／old、7A420A调用，thiscall manager，ret8；原保护同时要求旧离roster、新唯一及真实槽，故AL1的索引交换不能误作旧体替换证言。

## 35／40举持、抛掷及落地（2026-11-05）

先读本地IsaacDocs EntityPlayer.TryHoldEntity、Entity.HasEntityFlags及enums/EntityFlag，REPENTOGON EntityPlayer.TryForgottenThrow，再读官方LuaEntityPlayer.cpp2516及ZHL。Luna max定位后，主智能体独立复读7C84B0／7D14C0／7C87B0伪码和77C7AB—77C8F2指令。真实顺序是40:TryHoldEntity(35)，随后35:TryForgottenThrow(dir)；后者要求35、真实twin、twin.Held=this及18E4（官方header peeBurstCooldown）<=0。

7C84B0拒绝已占用holder、Null、self及已HELD／THROWN的35；成功写holder+140C并置ItemState604。7C87B0对Player target于7C88F1置高flags的04000000（THROWN），7C8969清02000000（HELD），收尾清holder+140C。77BD40正常Update的77C7AB—77C8ED维护实际轨迹；77C8B1比较PositionOffset.Y，正值路径把350／1E54归零，再经412DA0清THROWN。350是PositionOffset.Y，不命名为公开Height；夹具不写轨迹或标志。

验收分别要求正式同窗目标的35 HELD和40持物Key、离手后THROWN且实际速度、随后同Key/use的THROWN转假、held为空、PositionOffset.Y=0和实际运动后速度收敛。公开HasEntityFlags要求所有指定标志，因此HELD／THROWN分开查。实际运行结果仍见统一验收记录，不以静态结论代通过。

## Esau新活动体的独立存储依据补核（2026-11-05）

Source21首个四槽已完整；恢复前首轮新体的通用Replace／Restore观察撤销了资格，三个远端最终为0。trace同时证明首次Replace前new仅1，第二次new为旧backup的5；不能把1直接接受。

同一Luna max与主审复读995E10、7A5660入口及7C5445—7C54B8：995E51／995E5E先互设+267C，996476／996480才清。9963A3虽调用7A5660，但7A56A3检查+267C非零，7A56B0直接退，守卫前无call；它没有执行完整Player blob恢复。7A7D30是把旧Player序列化到旧+8384。7A71C0仍执行Familiar／attached Entity部分恢复，7B3980可执行配置／道具处理，均纳入整个owner正常收口，不称完整scalar恢复。外层7A4231以a4=1 ChangeType；7C544B跳过该函数的附加Player Ctor／Init分支。

新增证言分别记录：本次实际Ctor后完整74F6A0正常返回的同local/use，或入Replace前已有2／4／5的复用体。BeforeControl先限定实际manager、old唯一roster／new不在roster，再固定候选；0／1只有“已观测本次完整Init”的独立记录才有资格，不从数值或指针猜测。唯同次Replace、候选+267C指old且old+267C反指候选的7A5660短路不撤候选；真正Init／Restore／Ctor／Destroy／ManagerRestore及异常仍使其失效，输入和通用逻辑读取撤销保持。整个703正常完成、Replace实际AL及pointer substitution、临时互指清零、old进入对应真实槽、new唯一roster和两侧local/use均吻合后，才给新体同样的具名stateowner5。所有字段仍由正式同窗读取及完整解析确认，不继承旧字段值、不补出生、不恢复输入，也不声称原版保存blob已完整恢复。

PartialCharge公开属性已由同一Luna及主审复读：官方LuaEntityPlayer477—489返回同一ActiveItemDesc指针；863B93／863C4E以offset18注册PartialCharge闭包，getter8A4510读取float，setter8A4570以luaL_checknumber取得float并写回（8A45C3—8A45CC）。本地Wiki的float与实际Lua一致，SDK int声明错误不能用于造景；本轮没有任何rawbits写值。未满、真实647拥有条件和原生归零判断继续分别验证，不把调用返回当成0.5已存在。


## 水面Getter依赖真实Player库存（2026-11-05）

Client02崩溃转储的实际原生RVA369E7F及两层返回369E44/2B3266，与本地Wiki、官方LuaEntity.cpp814/823、EntityPlus.cpp163和Luna max静态定位一致。主审独立复读769E32—769E87、6B3255—6B3266及两函数伪码：GetWaterClipInfo先查询collectible62，再递归191，最终读取`player+16B8`的真实库存data[191]。base Init尚未建立派生库存时，空data导致读2FC。官方water override是在super之后覆盖，不能保护此前读取。故Player先安装全部库存与Plus容器，再调用既有公共字段水面setter/getter；既不提前运行Player Update，也不把DTO比较当作native校验。Client03已越过该路径提交，随后Sprite后态问题另记。

PartialCharge更新补核：Player::Update77BD40于77E583调用78D000，7943F7起的4.5Volt循环以+158C、stride20访问四槽，仅在Charge或电池仍有余量时于7944EE累加；满槽跳过写入，没有通用归零。584/59特殊active重配7C95B1—7C9614可清描述符，移除时7C9630仅清id。主审已独立确认公开float getter/setter，后续静态归零路径尚不足以解释Source26，保持UNKNOWN；Source27的未满、647及partial0.5是实际E2E结果，不外推旧失败的原因。

## ANM完整姿态借用补核（2026-11-05）

同一Luna max定位copy402CD0与Swap403160后，主审独立读取两函数完整伪码。copy借助403CE0深复制AnimationState的phase、playing、事件与两组selector，并逐层新建LayerState及Image strong；offset94/98及bitflags110清零。Swap逐项交换拥有资源并重绑LayerState owner，但不交换offset及overlayFirst70。完整Player frame因此给真实copy补齐已命名标量，再在CPU drain后的Swap外显式恢复offset/overlayFirst；不得将copy+Swap视作天然全姿态快照。未知异常前缀继续保留owner，不重试原生析构、不伪报drained。


## 普通房间主层caller的Render mode（2026-11-05）

Luna max定位Berserk环为758850中的Type27分支：704实际TemporaryEffect.cooldown/config.cooldown百分比，而非Samson积量。主审独立复读758850、758D50、90A7A0、Render784A60伪码及785F3B—5F：Player不dead且Room mode1/3才进入角色进度条/旁体HUD绘制。Source已有704.cooldown148及完整inline+CBC姿态，不补新数据。

主审再复读原RoomRender指令804F10/804F1A、806190—8061E8：普通主层cursor0、mode1；最后一段仍mode1，随后调用所有已消费Entity的slot7 Post，最后清mode。Client此前只在water DrawBand设置mode3/1，普通逐体调用漏caller上下文。现在水面预pass后到Entity Post的Client渲染栈显式借用mode1，水面分段保持原mode3/1，Post后及异常离栈恢复原值。Host/Off、输入controller和原版Netplay均没有新增改写或诊断入口；不运行玩法/ANM Update，不靠本机帧推进还原充能。

## Bone Knife普通采样与插值帧（2026-11-05）

同一Luna max研究、主审独立复读93ECD0、940D80和B2D7A0起vtable及940DD0—E03指令：vtable+0C=93AAF0 Update、+10=940D80 Interpolate。实际Source27 pose Knife132为Type8/Variant1/SubType0、Parent Player5，普通T中Sprite.Rotation=-90、位置324/380；较后damage-flash同Key为0、320/384。SubType0的Interpolate在940DD7或940E03调用[eax+0C]，即实际Knife Update，而非纯插值字段；其Variant1且非Flying路径可由几何角写+EC。Render主draw直接消费ANM；Shadow6AD450不写该rotation，SubType3另有插值角度写者但不适用于此样本。

Luna早先把+30错读Variant、漏读间接Update并一度排除Subtype0，均已纠正；主审以vtable和实际指令而非早期伪码推断采纳结论。旧pose截图前具体哪次Update写0没有逐调用运行证据，保持UNKNOWN；不能将Client按T的-90静态显示误报为漏Player射击字段，也不能在Client调用原生Update来修图。E2E新pose等待8个真实Render帧准备并严格核普通T与ready窗口的实际Knife位置/角度，Render诊断只作对照而不覆盖正式T。后续32处理平滑/时间轴，20仍按指定普通目标的原生静态应用范围报告。

## 伤害闪隐的实际判据（2026-11-05）

本地原版Wiki说明GetDamageCooldown为无敌帧。官方REP GetBlinkLockTime仅读Player+1338、控制头部fired-frame锁时，不能把NativePlayer.BlinkTime误称伤害闪烁计时。同一Luna max定位后，主审独立复读Player Render784A60、784BCA—C07及7A2630：damageCooldown+13B8为正且余6在0/1/2、1398/139A均0、Game+25AFC不为30、无FLAG_NO_TARGET(0x10)，才跳至Render尾部隐身；其余继续绘制。该门不读Manager/Game frame。Client确实调用同一vtable Render，未发现漏caller。

Source29普通T的87余3应显示，后续86余2可隐藏；旧截图实际倒数写入时刻未逐调用记录，不冒充已证86。原清除场景83也并非归零，已作为夹具失败纠正。新正态min89选择能同时落在闪隐半周期的准备窗口，ready另记实际DamageCooldown和公开frame，与普通T分列；清除等待真实0。1398/139A两字节及Game25AFC枚举用途不臆命名，条件与原值照采。
