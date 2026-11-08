> **当前状态（2026-11-07，本次恢复后）：用户要求“先不跑02测试”，已正常收尾。** 第64轮分层枚举名称冲突已修复，第65轮产品／E2E／harness构建部署完成；E01.valid-menu第8次实际通过，两端三池512／128／128 MiB、退出0且cleanup通过。E02第47次仅启动Host，尚无新run或新Client启动便中断；两端节点restored=true，连接级设置保留。E01仍九项通过，E02未通过，E03—E26／P4未执行；新采集性能尚无实测。最新恢复位置见[双人E2E进度](tx-04-21-e2e-progress.md)。以下记录保留原14:44及各轮实际事实，不覆盖本次明确停止。

第61轮产品／E2E／harness构建完成，编排因相对GUI信箱路径被拒绝而未启动游戏；第62轮改绝对路径，E02第44次仅观察Host启动白屏，智能体在未确认标题时主动收尾，两端记录Host退出0／无新转储证据，不能按提前崩溃或用户取消。第45次Host13852／VM7660实际冷19及普通37、61、71、105共5次提交，固定目标和结构依赖事务已运行，但未通过：首因client_state_stale、no_pending，后续operation_no_progress_or_deadline并安全Off；三池首次拒绝0，Client Values峰265625087／536870912。事实记录Host Entity19次总19.682秒、RoomSave19次7.743秒；同机普通源前18个间隔多为2.17—2.76秒，含一次3.63秒，不能把长窗当网络丢包或OOM。步骤2明确fail，两端退出0、cleanup.pass true、save_failures空。第63轮候选：编码同次调用复用短字符串完整转义（64KiB有界）、连续数组图复制不再逐项字符串排序；不跳Getter／字段或复用旧采样。按本地1.1.3 Sprite公开接口，将当前／overlay动画描述完整留在选择所属动态分区，避免内部AnimationData.GetLength被独立提升为Resource换代；真正ANM路径／代次仍保留。E2E新增collect／partition／encode三阶段具名耗时，不进入同步值。新候选未按通过；同tx继续E02及剩余P2，复用44轮八项协议负向和60轮valid-menu。
第60轮：E01.valid-menu第7次实际通过，两端冻结三池512/128/128MiB、退出0、cleanup.pass true、save_failures空。E02.cold第43次Host56448／VM12568实际3次冷提交与5次普通结构提交，已显示NPC／弹幕，但首因client_state_stale，最终Client精确版本未取得；Client Values峰292111239／536870912，三池首次拒绝序号0。两端退出0、cleanup.pass true、save_failures空。该样本证明新额度进入世界路径且本轮无池拒绝，不宣称全场景不再OOM。静态发现发送器仅按报文已发完释放活动目标，可靠依赖未齐时新World可反复替换Client候选；第61轮候选固定活动目标到真实Applied ACK，并将结构依赖取得纳入同精确目标的真实进度（新增依赖／原生步骤才推进，2秒无进度及120秒绝对期限保留）。E2E源快照改取发送器实际接管的不可变闭包、单独有界worker组合，避免选择从未发送的Capture候选或大快照阻塞网络；视觉中性审阅从arm开始适用于两端，不混入性能窗口。增加原有采集的分域耗时事实，不重复采集。候选尚待同入口实机验证；继续E02及全部剩余P2，复用44轮八项协议负向与60轮valid-menu通过。
2026-11-07用户新决定：预算计量／释放修复完成后，将三池内存额度翻倍，Values 256→512 MiB、Wire 64→128 MiB、Workspace 64→128 MiB。第59轮产品／E2E及harness已编译完成，但在Host启动准备前由智能体停止，未启动游戏；用于纳入新额度及异常路径预付寿命补齐，非用户取消。候选已补不完整预检／原生写入异常时的DTO预付保留，临时副本退尽后转交保留owner，下一写前按实际仍持有容量缩回，或完全退休后归还。第60轮同步三池默认／硬上限、值展开／Store／Live账本和正式配置样例；现有单条消息32MiB、fast64KiB、MTU、节点、速率和超时不变。E01正常菜单的真实冻结配置检查新增512/128/128 MiB账本断言，因此重跑E01.valid-menu及E02.cold；其余八项协议／身份负向未触及对应拒绝路径，显式复用第44轮实际结果。新额度／DTO保留计量尚无实机通过，仍同tx继续全部剩余P2。
第58轮E02.cold第41次Host61360／VM12344：首因client_state_stale，持续网络恢复中累计真实提交至少14次／Render725，仍未形成完整E02通过。源差异路径首次Resource为room/map_sprites当前AnimationData.GetLength，结构首次为成员64→88（普通夹具实际增加实体），未取得仅图编号变化导致结构重建的证据。随后热写前预算首拒绝client_dynamic_dto申请90288992／当前229526072／限268435456；当前所有者包含旧client_dynamic_dto90288992、client_target_plan112046528及compact_decode26813344。静态确认installed_memory把完整八倍临时构建预付永久保留，第二热目标再次申请同额；不把逻辑预付直接等同实际RAM。步骤4最终Host精确版本未取得；两端退出0、cleanup.pass true、save_failures空。第59轮候选保留原构建上界，但在实际安装检查完成后遍历本方DTO容器capacity／共享纯值分配，对仍持有的数据保守计费；未用预付在临时副本真正退尽时释放，新增保留分配仅从本次已付租约原子转交，不扩大总池。共享分配用有界主线程索引只计一次；计量工作区写前取得，原生/图像/游戏指针不参与遍历。旧场景完全退休替换后归还已消失DTO额度。协议/超时/速率/输入语义不变；同tx重跑E02并续其余P2，复用第44轮E01九项。Client_state_stale的依赖等待首因及完整性能仍未解决，不能把候选当已通过。
第57轮E02.cold第40次Host49948／VM11636：首次冷19及结构39、63共实际提交3次／Render63，普通夹具完成。Values峰246492503、拒绝序号0；不能仅据此外推恢复全场景无OOM。新诊断记录可靠信用逐项排队／归还，结构71在目标73中排于所有Context之后，后续目标83仍等待Structure81。Client首因operation_no_progress_or_deadline，Host首因publication_no_progress_or_deadline；缺少具体超时操作，暂不把调度推断当根因。完整E02步骤4因Host精确版本未取得失败（源已经退出本路由），不是快照页过期。两端真实退出0、cleanup.pass true、save_failures空。第58轮只补超时操作kind/sequence、Context信用精确超时及源Structure/Resource首处值差异路径诊断，不改变发送率、超时、范围复用、字段或池上限；同tx继续E02及剩余P2，第44轮E01九项复用。
第56轮E02.cold第39次Host30036／VM9044：普通夹具完成并进入正式E02步骤，实际commit4／Render93；首因先为client_state_stale，恢复后才出现Values池耗尽。Client先提交39、63，随后等待Structure71；Host71已采样发布但直到恢复基线93才编码，调度／信用原因待核对，不改写成已证根因。恢复结构112期间compact_decode首拒绝申请9088558／当前261081114／限268435456，其中client_target_plan206743424。精确源快照已正常取得，Client精确版本未取得导致步骤4失败，不能继续归因snapshot_expired。两端退出0、cleanup.pass true、save_failures空、不允许变化0。第57轮候选仅在旧A的原生及纯值owner实际退休替换后释放旧applied_memory，并补可靠依赖真正入队／ContextReady的E2E诊断；不放宽池、超时或判据。E02仍未通过，E03—E26未执行；第44轮E01九项复用，同tx继续P2。
第55轮E02.cold第38次Host61748／VM1508：首次冷19及结构37均实际原生提交，累计commit2/Render37，结构退休控制泵的rebuild1/stop0/failed0/pumping0/main1/owner1及递增进度实际存在；Values峰256214695／限268435456，首次池拒绝序号0。解码预算候选首次越过此前OOM，但不外推全部内存场景。下一热目标61首因objects_player_stream_context，随后native_recovery_exhausted_or_unsafe安全退休；编排setup最终client_world_applied1等待超时，未进入完整E02。失败收尾facts两端真实退出0、finish_error空、不允许变化0；没有cleanup.pass字段，不补造。静态确定热InstallStream先执行非Player写入令o.committed=false，随后RefreshPlayer又要求committed且末尾缺FormalCheckAll。第56轮候选让当前事务暂存Player描述（仍检查主线程/owner/当前Player/不确定状态，不授予展示），同一正式完整成员/关系检查成功才恢复o.committed；失败维持不确定/禁止展示。协议/池/频率上限不变；同tx续E02及剩余项，第44轮E01九项复用。
第54轮E02.cold第37次Host77148／VM11828取得真实worker首因replication_pool_exhausted，主线程首次错误也精确保留，不再被client_transport_finished覆盖。worker终态在预检完成前约259ms发生；首拒绝compact_decode申请50809880／当前237128121／限268435456，所有者client_target_plan198168448、compact_decode26549641、client_structural_model12098360。此前Host无进度超时为次生结果，相关主因假设纠正但历史事实保留。两端退出0、cleanup.pass true、save_failures空、不允许变化0。第55轮候选精确计量Range DocumentTree的SAX arena：按实际4KiB块/短字符串/最终容器分配顺序预扫描，构造栈与查重集合只按并发峰值单列预付；不再为每个标量/键和每个树重复预留旧64KiB最坏值。扫描固定栈、不建DOM或查重集合；第二遍仍严格查重/构造，完成后核对真实Footprint并缩回实际持有。非Range控制消息、wire/schema/能力、256MiB池和输入/状态超时不变；同tx续E02及剩余P2，第44轮E01九项复用。
第53轮在E2E构建时因live.cpp新增诊断缺少stream_e2e.h声明引用失败，未启动任何游戏。已补显式include，第54轮仍同tx续E02及剩余P2，非用户取消或游戏崩溃。
第52轮E02.cold第36次Host63524／VM2340：Client事务37起点219实际被Host接受，但预检结束后第一个Manager的外层连接已为Retiring（phase5），没有client-retire-pump记录，不能归因该guard阻止。追查确定错误交接缺陷：worker在耗时预检中结束，ClientManagerBoundary末尾Finished分支先以client_transport_finished冻结首因；下一轮Poll才交付的真实Failed/Stopped原因被吞。故此前Host结构无进度超时可能为Client先退出后的次生结果，前面对其主因解释仍属待核对假设，不按已证根因。修复Finished分支先Poll优先终态，保留真实原因；E2E同时从worker End记录实际reason，绝不伪造进度。两端退出0、cleanup.pass true、save_failures空、不允许变化0。第53轮同tx继续E02及剩余项，E01第44轮九项复用。
第51轮E02.cold第35次Host68424／VM12572：两阶段预检起点已实际进入Host host-received/host-advanced，精确native_target37、progress198；Client预检37从41223671至41224312ms（641ms），随后没有client-queued新步骤，Host首次转为publication_structural_no_progress_or_deadline，故仅证明起点修复命中，结构事务仍未完成。Client已取得连续真实NativeRetire耗时记录（每约16.7ms一次），不是Manager不运行；未将未定位的guard条件当作结论。两端退出0、cleanup.pass true、save_failures空、不允许变化0。第52轮补只读结构退休控制泵的进入/条件记录及外层连接状态，保持超时、池、原生执行不变；继续同tx E02及剩余P2，第44轮E01九项复用。
第50轮E02.cold第34次Host52812／VM12292仍失败，两端正常退出0、cleanup.pass true、save_failures空、不允许变化0。新增实际诊断：Host超时目标45，普通年龄2000ms、native_target0，冷进度最后211；Client结构37预检begin40802281ms、complete40802890ms（609ms），完成后没有进度入队即收到终止。首条活动事务进度被同步预检挡住。第51轮候选将结构目标固定在planning，依赖齐备后先排精确事务开始、下一Manager再预检，后续候选合并但不顶替planning；待原生退休/重建完成仍按真实步骤推进。另修正同路由结构退休期间的纯值接纳：只对确有rebuild_pending/structure_rebuild/inflight且非停止/失败允许入库，普通退休仍拒绝；不在退休阶段执行新目标。Store Collect显式保留planning闭包，旧目标完成不误删同范围较新候选。无超时/池上限变化；第44轮E01九项复用，继续同tx E02及剩余项，不将候选按通过。
第49轮E02.cold第33次Host65704／VM10960：首次原生提交19实际发生，随后结构37重建，Host仍publication_not_converged、远端身体退出，夹具ordinary未成立。不能把退休控制路径静态修复记为实机通过；目前尚缺进度入队/到达的精确事实。两端正常退出0、cleanup.pass true、save_failures空、不允许变化0。第50轮只补E2E现有路径诊断：目标预检起止/NativePrepare实耗、Client实际进度入队、Host收到与推进、触发超时的精确版本与年龄；不由诊断产生进度或放宽超时，继续同tx E02及剩余P2，复用未受影响的第44轮E01九项。
第48轮E02.cold第32次Host51268／VM7764仍在夹具前失去远端身体：Client首次提交19，收到结构37并进入attempt2退休，Host实际首因仍publication_not_converged，未命中预期结构进度路径；两端已取得的Values拒绝序号0。源事件37完整发送/Client收到均存在，不能以没发目标解释。静态确认PumpStream在Retiring直接返回，结构替换的真实退休步骤没有报告；FinishRetirementIfDrained又将executed_prefix清零，原先大于比较也遗漏重建阶段。候选只为rebuild_pending、structure_rebuild和inflight同时成立且非stop/退休失败的事务保留控制进度，按真实步骤变化报告，完整预检后报告起点；普通退休仍不继续路由。两端退出0、cleanup.pass true、save_failures空、不允许变化0。第49轮同tx续E02及剩余项；第47轮未启动游戏的记录不改写，E01第44轮九项继续复用。
第47轮完成产品/E2E构建，但在Host实际启动前由操作员停止，未启动游戏；为结构事务进度补充UDP乱序收尾：已完整发送的冷目标迟到进度、已确认WorldDynamic迟到进度只无副作用忽略，不能刷新事务期限或复活旧事务。第48轮仍在同tx续E02和剩余项，本轮启动准备尚未确认的文字不表示用户取消。
第46轮E02.cold第30次Host9460／VM904：Client 1176ms冷缓存各计数0，25291ms实际首次提交19/world_applied1，随后结构目标37原生重建进行中，27202ms收到client_transport_finished。Host实际首因publication_not_converged；原生日志明确移除Controller1，夹具第6帧因真实参与者不足拒绝，因此不能把本轮简单算作F8过早。确定性缺陷：Client所有原生进度沿用manifest revision19，Host NativeProgress只接受冷目标，活动结构事务也被普通2秒未收敛规则断开。候选修复让进度绑定当前inflight精确版本及native_structural_step；Host仅接受已完整发送且尚未应用的WorldDynamic，单事务、序号严格推进才刷新进度，沿用2秒无进度及120秒绝对上限，Applied ACK清事务；普通动态2秒规则不变。拒绝未来/未发/已应用/并行事务或错误路由，不由重复进度刷新期限。两端退出0、cleanup.pass true、save_failures空、不允许变化0。空Grid/分页候选未因本次提前失败而取得E02完整覆盖，第47轮同tx继续E02及所有剩余项；E01第44轮九项协议协商结果仍复用。
第45轮E02.cold第29次Host15524／VM7436：首次网络原生提交19实际发生，冷缓存/网络来源窄项已审阅，但后续真实首因grid_required:bodies，E02未通过。静态定位Wall/Decoration的合法空fields被分层器丢弃，现保留仅type/version/空fields的完整schema，不补造字段。后续Values首次拒绝为compact_decode申请59437422／当前267365739，其中client_target_plan223029632；内存问题仍未解决。精确版本对照另因Host快照过期失败：source arm 398654ms、索引409558ms、首页415866ms，原10秒寿命不能跨慢状态采集；源捕获仍平均约452ms，未满足频率要求。两端正常退出0、cleanup.pass true、save_failures空、各端不允许变化0。修复自有E2E快照纯值分页：256KiB固定页、索引领取有界读租约（总120秒/空闲30秒，数量16/正文64MiB不变）、后台只读已存文本；整组状态观测按完成时刻开始下一间隔。以上及空Grid schema须第46轮同tx续测，E01第44轮新能力九项实际结果复用；P2整体仍未完成。
第44轮array-slots-v1候选的E01九项均实际通过：wire第13次Host53532／VM8952；stream-schema第4次72520／5816；compact-schema第4次42080／4740；resource-declaration第4次29360／10040；session-identity第4次44480／8672；framing第4次10380／7684；capability-missing第4次26064／9916；full-room-capacity第8次43752／2784；valid-menu第6次68968／6664。各负向实际sender均命中一次，首因分别stream_capability_version、compact_payload_schema、resource_version_mismatch、live_welcome_nonce、unsupported_capability；所有native_attempt/world_applied0。身份用例接收候选实际1，但接收/原生世界scope及wire身份0，未伪写候选全0。容量两个纯协议探针各9片完整Welcome、不发Ready；额外9片Hello使Host net_route_rejected0→9，真实Client仍AwaitingWorld，Host仅participant0/1，2/3 absent且所有身体0。正常菜单Host/Client session9514318557541795373、connection7282743690020033241，participant1、roster3，与Host Established/generation1一致。双方实际冻结配置acceptance-fixed-peer/frozen1，60/30/20/60Hz设定、总/每Peer937500/312500字节秒、burst24576/8192、reserved3/crossborrow0、fast65536字节/32768节点；设定不是频率达标。九项双方退出0、cleanup.pass true、save_failures空；临时重复快照已滚动清理。E02第28次在Host启动前由操作员为编入精确值比较优化主动停止，非用户取消、未启动游戏，原失败文字保留。第45轮复用本轮E01实际结果，同tx继续E02及剩余项；数组修复/4KiB分配粒度尚无E02全项通过，P2整体仍未完成。
第43轮在产品构建时因自有Allocator位于replica命名空间触发stream::Negotiate与replica::Negotiate的ADL重载歧义，未启动游戏。将实现类型移入replica_detail、对外保留别名后，第44轮产品/E2E均构建成功，开始新必需能力array-slots-v1的完整E01九项。第44轮二进制构建完成后另发现Contents/AtomEqual每ordinary为两树执行CheckValue及排序编码后才判等；源代码改为无字节编码的精确递归比较（对象键序无关，整数/双精度种类及bits、数组序、字符串精确保留），尚未进入第44轮二进制，须后续统一构建/实机覆盖，不把计划中的性能改善记为实测。
第42轮E02.cold第27次Host27196／VM8948，同种子。本轮操作员在夹具F8前提前前置VM，Host physical明确拒绝node_confirmed_window_not_foreground_no_key_sent，故停在step1，不冒充夹具完成。仍已捕获Client真实首因stream_materialize_missing:Sprite.GetFilename:object/2/backdrop_sprites，后续动态DTO申请140970608／当前166601098，现场target_plan126970816、compact_decode39277842。双方退出0、cleanup.pass true、save_failures空、各端不允许变化0。确定性静态缺陷：按层拆未标记固定数组时产生稀疏Lua表，encode的ipairs从缺槽处截断，后部Sprite资源丢失；修复为等长片段数组，stream_absent_piece仅表示本层无片段、从不冒充Null/零/真实状态，最终未闭合仍拒绝。装备固定/动态先合并再递归完成其子片段。新语义新增必需能力array-slots-v1，旧客户端在协商时拒绝，因此第35轮E01九项移入历史、新候选重跑E01；framing2/schema1/compact2编码布局未变。自有Json改4KiB真实chunk，OwnedValueUpperBound/实际arena头部及逐对象预留同步，目的是减少小树64KiB填充浪费而非放大池限；尚待第43轮统一构建与实机确认。快照失败原因诊断本轮因step1未进入aligned，尚未观察，不按猜测归因。
第41轮E02.cold第26次Host44832／VM6520，同种子。Client初始1116ms receive/native_receive/commit/candidates/retained均0；实际network来源、累计原生提交2、Render67，但VM前置实屏最终明确为PreparingWorld/client_state_stale。先前非前置VM截图可能包含宿主窗口遮挡，不能据此认定Client当前房间；此轮按前置后的真实窗口审阅。首等待目标45缺Context40（暂缺不能单独证明永久饿死）；首次native写前错误精确为stream_materialize_missing:Sprite.GetFilename:object。消费域预算修改使动态DTO预留142592080，但当前166966894仍被268435456上限拒绝，修复未通过。E02 step4仍Host精确版本未取得，不能认作普通源必然没有发布：快照生成目前catch吞掉重组/预算原因。双方退出0、cleanup.pass true、save_failures空、各端不允许变化0。补FinishPieces只在失败展开字段路径、E2E快照输出真实failure原因，不改变成功判定或保护限额；第42轮同tx续E02和剩余项，E01协议未改复用第35轮九项。
第40轮E02.cold第25次Host70980／VM2240，同种子。Client初始1140ms receive/native_receive/commit/candidates/retained均0；43979ms真实提交Baseline19，44395ms commit1/receive24/Render19，45456ms首次client_state_stale。新增等待诊断实际定位普通目标45缺Context精确版本38，后续Client动态DTO申请166761264／当前166966894被Values限268435456拒绝；现场target_plan130117568、compact_decode36496886。Host早期来源完整且无拒绝，但继续运行后首次range_freeze申请26568798／当前261128329失败：source_ordinary_temporary147305472、range_freeze92729049、live_envelope19230768，故不能把短窗口无拒绝写成已解决。原生累计提交5、最后Render202仍不足以通过E02；step4仍为Host精确版本未取得。两端退出0、cleanup.pass true、save_failures空。修复候选：保留在途普通目标依赖集合直到依赖入可靠owner且World完整发出；动态DTO仅计实际消费域（其它域仍完整存储/计费）；Freeze按新复制arena上界预付并核对实际；热采集投影完成后按实际arena与后续临时副本上界缩回租约，冷路径继续保守预算。协议未改，E01第35轮九项实际结果复用，第41轮仍续E02及未完成项；尚不声称修复通过。
第39轮E02.cold第24次Host64556／VM9340，同种子。实际屏幕首次取回网络Burning Basement房间；控制时间线33700ms NativeCommitted19、34430ms commit1/receive23/Render19，随后35490ms client_state_stale；本轮首次native failure字段为空，不能把上一轮stream_materialize_missing直接算成本轮已证首因。Client Values峰224616699、拒绝0；Host首拒绝23813598/当前245952808，最近143862160/135919200，总限268435456，工作池3636040。E02仍在step4 Host精确版本未取得失败，双方退出0、cleanup.pass true、save_failures空。缓存完成后释放强源引用的修复未单独解决全部占用，保留此反例。补固定槽租约所有者当前/首拒绝现场和Client待应用等待原因，新增字段不改变预算/恢复保护条件。又静态修复LiveConsumer封套复制预留两倍字符串长期不缩回实际容量：复制前仍申请原保守额度，完成后按真实堆容量与容器计费，内联字符串已计入对象本身，独立ledger和租约同步缩减，超出预留则拒绝。第40轮同tx续E02及剩余项，尚不宣称故障解决。

第38轮E02.cold第23次Host66156／VM1516，同种子。Client实际网络来源，30812ms原生提交Baseline19，31078ms receive19/commit1及Render19；32279ms动态写前首因stream_materialize_missing，随后client_state_stale，Host仍以publication_not_converged收口。Client池拒绝0，Host后来Values申请23551198/当前256818548失败（不能冒充旧Client复制预算故障）。夹具日志路径修复后实际进入step2/3：初始1269ms receive/native_receive/commit/candidates/retained均0；network目标源及首次提交已核，独立两项观察通过不代表E02整项通过。step4因Host精确版本未取得失败，双方退出0、cleanup.pass true、save_failures空。静态发现EncodingCache完成后working从不清、仍强持旧完整Range；改为编码期强持，完成后释放源DOM、仅弱身份加不可变报文供复用，旧动态可清，正在编码与可靠冷目标仍有各自owner。缺成员诊断加具名key且保持稳定失败分类。节点RPC开启NoDelay、请求/回复单次写含换行、固定末尾分页从32KiB改为512KiB（仍低于4MiB回复和128MiB文件上限），双方先正常关闭再取日志。第39轮仍续E02及剩余E03—26，完整P2尚未通过。

第37轮E02.cold第22次Host69184／VM10548，同种子H6GRNTNY。Client池拒绝sequence0、Values峰190129676；实际Cold manifest41原生提交并Render，Client本地prepare30492718→display30498437（5719ms），time line在140116ms world_applied1，140669ms native.total.commit1，142884ms因client_state_stale恢复；不是整项通过。Host有baseline19/41发送和ordinary16/38发送，但无Baseline Applied ACK接纳事实，首因publication_not_converged。静态证实发送器在Baseline/WaitingApplied也启用2秒动态收敛计时，与合法冷安装期限冲突；改为Active才启用，准备/可靠传输原进度及绝对超时不变。后续F8实际ordinary生成8NPC/16攻击，但编排器错读Repentogon/repentogon.log而非真实save/log.txt，并逐页追赶活日志末尾，最终报Host真实动作未完成；该失败如实保留。操作员发现后原生暂停并退出Host run使本轮正常收束，双方最终退出0、cleanup.pass true、save_failures空。修复夹具日志路径、首片固定读取末尾及拒绝中途缩短/无进展页；不把操作员退出混作正常负载段。第38轮继续E02及剩余全套。

第36轮E02.cold第21次Host64508／VM8400，同种子H6GRNTNY；实际越过旧冷模型复制计费点，Client Values首次拒绝285975616字节、占用171281481、上限268435456，最近284679616/167180509；此量对应后续8倍计划预留，仍无native attempt/world applied。Host拒绝sequence0、工作池峰3614787、lua_partial0。双方正常退出0、不允许的访客进度变化0。继续静态修复CopyFrom计量：用相同SAX事件顺序模拟当前SDK的内联字符串、8字节对齐和64KiB chunk容量，替代两倍分配和及短字符串堆估算；8倍计划系数不变。接收范围的纯值DOM改用SDK SAX Populate精确分配最终成员数，避免每个小对象默认16成员槽，保留重复键/深度/节点/长度/有限数/闭合schema拒绝。报文字节及协议版本不变，第37轮继续E02及未执行项，不能按静态估算宣称解决。

第35轮E02.cold第20次实际失败：Client Values首次拒绝请求92809192字节、当时占用214395082、上限268435456；最近拒绝92362100/214008080，尚无native attempt或world applied，不能算冷构建通过。Host本轮池拒绝sequence0、工作池峰3598629、普通源完整且lua_partial0，首因是Client返回的state:replication_pool_exhausted。双方正常退出0，访客不允许进度变化0。静态修复解码预扫描租约不回收最坏额度、Skeleton先复制后移除state仍保留arena，以及每节点128字节粗计的复制预算；改按当前SDK CopyFrom的精确容器分配推导保守chunk上界，解码/冷模型完成后计实际容量，Skeleton直接复制保留字段并独立持有租约。冷模型分配前预付，返回后释放临时模型额度。未提高三池/报文限制、未删世界字段，计划8倍系数不变。第36轮继续E02及未执行项；第35轮E01九项实际结果保留，协议及错误拒绝行为未变。

第35轮实际工作区计费修复后，E01九项已全部重跑通过，正常退出及访客存档比较均通过；wire第12次、stream/compact/resource/identity/framing/missing各第3次、容量第7次、有效菜单第5次。错误拒绝首因保持对应合同，wire双方池拒绝sequence0，工作池峰Host9290/VM18284字节；容量额外9片Hello使route_rejected 0→9且真实Client保持AwaitingWorld。有效菜单双方实际冻结配置60/30/20/60Hz、937500/312500 byte/s、burst24576/8192、reserved3/crossBorrow0、fast65536字节/32768节点，均未放宽。E02第20次正在启动，尚未证明池争用修复已解决世界冷构建，E03—E26未执行。旧节点31、33及34第10次launch内180份与原件逐字节相同dmp临时副本已清，释放3074149236字节，原件与存档保留。

第34轮E02.cold第19次Host18304／VM4084，原生菜单重放H6GRNTNY。实际来源stream_ordinary_committed、lua_partial0，但Client world_applied0，最终server_disconnected；Host Peer首因replication_pool_exhausted，本轮未记录具体池/申请量，不能声称已证唯一根因。两端正常退出0，访客不允许变化0。静态发现EncodeBlob每个可靠消息预占2×32MiB，即独占64MiB工作池，与并发接收/其它编码必然可争用；改为同Writer先计真实长度、先计费后精确reserve payload与wire容量，不提高池或报文上限。新增进程内固定大小首次/最近池拒绝纯值（池、请求、当前、上限及序号），显式Host/Client来源诊断可读。编码字节格式与版本不变，须第35轮同tx实机确认，不将静态修复按通过。

第34轮compact-v2重新完整执行P2，E01九个子项已全部实际通过并正常退出0、访客save_failures为空：wire第11次Host79216/VM9604，stream-schema第2次4532/7688，compact-schema第2次38072/4156，resource-declaration第2次59604/7820，session-identity第2次74564/10852，framing第2次40560/7712，capability-missing第2次69728/7376，full-room-capacity第6次5552/7420，valid-menu第4次19680/1904。首项实测恢复为stream_capability_version，未再命中第33轮bad allocation；其余精确拒绝含compact_payload_schema、resource_version_mismatch、live_welcome_nonce、unsupported_capability。容量探针各收齐9片Welcome、额外9片Hello使Host route_rejected 0→9，真实Client仍AwaitingWorld；有效连接两端wire_session一致、participant1固定，双方实际冻结配置60/30/20/60Hz、937500/312500 byte/s、burst24576/8192、reserved3/crossBorrow0，保护上限未变。当前仅E01通过，E02第19次刚启动、E03—E26尚未执行，不外推世界同步/性能通过；旧v1结果和失败仍为历史。已通过快照用后滚动清理，失败必要诊断保留。

第33轮compact-v2首次实际E01.wire（case第10次）Host PID63164／VM11192：两端菜单已审阅，Client实际首因为bad allocation，未命中预期stream_capability_version，故明确判失败，未把任意Off当正确拒绝。未发出任何游戏协议包，world_applied/native_attempt0；已取回事件中Client private峰516771840、Host695697408，这不是物理/虚拟地址已耗尽的证明。静态路径发现名称字典首次Hello调用会解析数MiB完整Catalog并动态建两张map，移为构建时生成按文本/编号排序的只读表，二分查询不分配也不解析Catalog；具体是否解决本故障须第34轮实机确认。两端正常退出0、收尾通过、save_failures为空。

第32轮候选compact-v2产品/E2E构建成功，实际未启动任何游戏；操作员在第一启动提示前发现当前members不足以稳定保留退休签名编号，主动退出编排补齐field_ids历史映射并去除不稳定的returns派生编号。p2-E01.wire-9的未确认准备文字是这次操作员修正，不是用户取消或新的游戏故障。候选v2当前结果为空，旧v1结果放在progress.protocol_history，原失败和原始case目录保留。下一次仍从完整P2开始。

第31轮E02.cold第18次Host PID45832／VM9004，H6GRNTNY／820218296、StageType2、floor seed2949646326。actual ordinary-room compact-v1为452393字节；首次编码失败的自有纯值分片已取回用于路径分析。60个record的树体计量443341字节，其中Entity136349、Grid146605、Room54540、Presentation87146，其余为具名保留域；这些是本轮原始值分析，非新codec实测。两端退出0、访客不允许变化0。旧节点27—30的240份与原件逐字节相同dmp临时副本已清，释放4098865648字节，原始转储/存档保留。定位两个分层缺陷：嵌套共享Sprite成员使保留域泄漏到WorldDynamic；未加载Sprite完整当前值可作为显式Resource依赖却在热范围重复。现均修正，仍每普通窗完整重读、任一变化即新版本，不宣称未读字段常量。compact-payload-v2新增静态键/公开目录field编号、具名闭合结构、最短ULEB/zigzag整数和严格可逆f32表示，0/1/负零保留种类与符号，未知形状完整回退；不删成员、不量化、不从旧目标继承、不提高64KiB/32K节点/池/深度/字符串上限。受版本变化影响，旧v1的E01九项通过仅保留历史，候选v2须从P2全套重新实际验收。以上新实现尚待统一入口构建/实机，不按估算改判通过。

第30轮E02.cold第17次Host PID73352／VM7476，实际新种子SD4H44GX／2400912021、StageType1、floor seed2513923748。来源继续完整提交，精确超限为ordinary-room(kind5)紧凑编码345388字节／保护上限65536；没有提高上限。VM实际维持AwaitingWorld，原生attempt0、world_applied0，已交接新恢复route3；本轮未复现此前recover/fault二次首因，两端退出0、访客不允许变化0。整项仍失败。下一轮E2E仅在首次编码失败时将已经自有的各字段纯值分成4KiB诊断页，最多4MiB，保留缺页/截断，供逐路径体积分析；不读原生对象、不补默认值、不替代接收/应用，不据此判通过。

第29轮E02.cold第16次Host PID53104／VM3980，同种子同楼层确认，实际来源ready=1、stream_ordinary_committed、sample/publish各45，Lua Partial0，空数组修复已越过前故障。整体验收未通过：Host Peer首因为compact_byte_budget，Client先有client_stream_route_or_owner recover，随后stream_ack_route fault，未原生提交；两端退出0、访客不允许变化0。静态链发现：Host编码失败发Retire后仍有旧Range在途，Client原先退休却留AwaitingWorld并继续Admit；恢复已清可靠发送授权但迟到ACK仍入Acknowledge造成二次错误。修复为退休/准备状态拒用旧Range、撤销阶段忽略经外层路由核对的迟到分片ACK；Prepared激活改与Retire共用有序队列，避免通知/值双队列的交付竞速（竞速为静态可达风险，不把它声称为本轮已证唯一根因）。编码失败增加真实范围及同编码器只计量字节诊断，正式保护上限不扩大。另修复分层拆分吞掉显式空typed table的确定性路径，最终清空仍作为后态保留。需第30轮及后续覆盖验证。

第28轮E02.cold第15次Host PID7924／VM5936，原生UI同种子K96V9G4H。RoomPresentation v5后实际各Lua域原因空、lua_partial=0，已越过fx_color_condition；整世界仍失败stream_source_partition_array、publish=0，VM实际保持AwaitingWorld，未载入本地run。两端正常退出0，访客未出现不允许的进度主体变化。静态定位Lua空分区表被编码为{}，C++合同要求[]；分区六个固定列表新增自有数组标记，图闭包复制保留标记，编码器据此保留空数组；接收端仍严格拒绝非数组并补域／分区诊断。该修复待第29轮同tx实机验证。

# tx-04-21 I36—I38：全窗口隔离、接线审阅与统一编排

第27轮E02.cold第14次按原生UI重放K96V9G4H，日志确认1075232870／StageType2／floor seed3719732016，Host PID18808／VM2812。实际失败精确为FX id4/count3/pass1/mask0/+3c bitsFFFFFFFF，反驳“本次只是空层”的假设；空层适用性修复仍保留，但不能改写为它已经解决本故障。96C620仅对该f32做零比较，未用于坐标/颜色/速度算术；967D40 ctor未写+3c，故不将FFFFFFFF命名为已证明的初始化sentinel。RoomPresentation v5改以具名float32_condition表保存精确bits和Finite/NaN/正负Infinity分类，接收端验证两者一致，只为这个已核零比较字段采用该表示并原位安装；所有算术float仍原有限检查，缺字段和错误分类仍拒绝。不修改Host未初始化字，不补历史、不用默认有限数代替。v2—4原数值兼容仍严格有限，v5新结构须经第28轮同种子实机及后续验收。旧文档的“此条件必为有限数”在本次实际反例下不再作为现行字段合同，历史事实原样保留。旧已结束VM节点21—26内359份dmp副本与原件逐字节相同后清理，释放6114414035字节，原件及存档不动。

第26轮E02.cold第13次Host PID35108／VM8624。实际CurrentEntityReadWitness记录local64/use1、readiness6、birth0；Entity18次采集Partial0，其余Random等最近原因空。普通完整源仍未发布，本轮新缺口为Burning Basement的RoomPresentation fx_color_condition，实际种子K96V9G4H／1075232870。不能把18/18 Entity当整世界通过。静态回读J273 96C620确认：count<=0直接跳过pose循环，+3c浮点及+40 mask背景只在循环内采用；旧采集对空层仍读这些字。仅修复这条未采用字段的适用性（与既有mask/pass未采用分支一样规范0），保留实际采用float的有限检查；非有限失败增加id/count/pass/mask/原始位诊断，不改成不透明word绕过检查。原失败尚未记录具体FX层/count，故“它是否本次根因”须在第27轮同种子实机重放确认。

第25轮E02.cold第12次Host PID77308双人Cellar、VM6916菜单等待，仍未应用世界。纯值生命周期记录给出Factory call80(type1000)返回1667284128→Init call81(type1000/15)，两者parent0，local53/use1、readiness0→1、birth0；不误用本轮local52的Player出生记录。静态核对J273 JoinPass RVA591360：完整PlayerCreate返回后，另一支直接Factory(1000)／虚Init(15)／EntityList::Add及后处理，绕过Game::Spawn，正是既有owner目录漏项。新增在现有JoinPass常驻Hook内的观察Scope，不添游戏写入或双调原链；64项有界表只关联本轮正常原生Factory、同local/use的Init返回和Publish，整个JoinPass同run/outer/depth正常收口、Effect分类及revision0才授予NativeJoinEffectCompleted(6)。保留Ctor／Birth未知，不把它伪造为Game::Spawn出生；Init／重用／销毁／关系修订仍撤销，事件另记CurrentEntityReadWitness。异常、缺失、重入和预算耗尽不恢复资格。待第26轮及后续E2E验证，当前E02仍失败。

第24轮E02.cold第11次Host PID24660普通双人Basement，VM11168标题后等待世界。Random域reason已空，实际读取期间未再发现resource-change，live首因转为live_required_domain_missing，说明两项修复本窗口命中，但未外推整个冷构建通过。当前唯一Lua域缺口为EntityEffect1000/15／POOF01，实际KnownCurrent、native_readiness=1、local52/use1；第32次采集仍Partial。不会把基础Init返回当派生创建完成，也不删烟雾来绕过。90秒仍未安装世界，保留失败；新增Collector既有创建/Init的call/parent/实际返回/出生/readiness纯值事件，继续定位真正漏掉的owner或时序。正常失败收尾及原选项恢复以本轮事实文件为准。

第23轮E02.cold第10次Host PID59624进入普通双人Basement，VM416等待世界，仍超时失败。实际Random首因为stream.lua:97的stream_graph_conflict：各RNG登记参数来自独立Context、node从1开始，直接合到普通域被误当一个命名空间。修复为当前scope内复制重编号并验证引用闭合，不改登记表及历史值／共享和环关系，失败仍Partial。Entity缺口实际为EntityEffect1000/15，进一步保留引用解析／readiness诊断而不假授构造资格。资源具名入口实际为ui_hearts.anm2的stream_resource_sheet；静态核对J273 RVA7CD0的有效层号判断、长度及字节比较（RVA8C30）：路径相同时直接false返回，未写path或loaded。wrapper仅按这条已核实无写分支不建立资源变更Scope，原链仍恰一次；真实不同路径仍进入即撤资格，不按ui_hearts文件名白名单。两项修复待第24轮及后续完整窗口验证。

第22轮E02.cold第9次两端正常进标题，Host PID59028第二槽普通双人Cellar，VM7636仍等待世界。新诊断确认高频实际资源为gfx/ui/ui_hearts.anm2（loaded=true），未把这些写入当成无变化或加路径白名单；Random原首因为field_or_budget_gap:stream_partition_or_budget。Entity仍有application_links／owned_presentation的NotCaptured，但当时未记具体类别，不能归为某一种Effect或假设全部初始化暂态。90秒仍未应用世界，按原判据失败；新增Lua实际异常、未就绪对象类别／Type／Variant和资源Hook具名入口后继续同tx，不缩减RNG／资源字段或发布Partial。

第21轮E02.cold第8次VM PID5916在标题前实际退出3221225477。原始转储确认c0000005，PC010de54b／RVA52e54b，`imul [edi+21450h],4Ch`后的Cutscene队列索引越界，与此前Host同一原生位置；只启用本工程三Lua mod。未进入房间，未宣称冷构建或新诊断已验。先前“仅Host临时关闭SkipIntro”的范围由本次反例修订为两端验收临时正常初始化并人工跳过，设置日志退出恢复用户原文件；不永久覆盖VM的SkipIntro=1。harness旧“准备尚未确认，未启动或发键”文案只描述本次操作回答分支，不能否认VM本轮已启动并崩溃的事实。

第20轮在游戏启动前构建失败：新增资源路径诊断将官方SDK头引入C++20核心，命中已有SDK生成头C4596；不是游戏实机失败。已把已知ANM字段复制移入现有C++17的lua_entity适配单元，核心仅传值调用，未降低核心标准或修改SDK。第21轮重新构建后续跑E02。VM旧失败快照内630份转储副本逐字节与原件比较相同后删除，释放9721225332字节；原始转储、进度和存档保留，不计算哈希。

第19轮E02.cold第7次已使用收紧后的temporary归并及紧凑观测环。Host正常双人房间，私有内存783843328字节、工作集797839360字节，读取期间未发现missing；相较第17轮1.67GiB／lost2990，诊断负担本次下降，但不外推后续重载性能。初始化后Entity/Grid原因转空，Random（index8）仍为stream_source_missing_or_budget，主世界未发布；资源事件集中在地址1361601412的非构造／非析构、非普通读取窗口写入。新增记录已知ANM文件名／loaded（构造前不读未初始化字段），同时修复C++缺partition分支保留Lua原首因，不能用笼统缺正文覆盖真正投影／预算错误。E02仍按90秒未应用世界记失败，继续同tx定位，不以较低内存或菜单连通宣称冷构建完成。

第18轮在首个Host启动确认前为构建同步主动停止，未启动Host或VM游戏；harness旧文案记录“用户请求必要中断”，实际是智能体修复续验，并非用户取消。静态复核把copy来源归并收紧为已经确认temporary的对象，不以“未观测”推定对象是临时副本。该源码修改晚于部分obj编译时间，故补编译后再开始同tx E02；不运行旧对象文件对应的版本来声称当前修复已验。

第17轮E02.cold第5次：nil投影错误已消失；初段Entity报告`entity_application_links:NotCaptured`／`entity_owned_presentation:NotCaptured`，后段54次采集42次Partial、最近Entity/Grid原因均空，说明该缺口是初始化暂态，未据此放宽readiness或删对象。仍无首份发布，资源失效持续。诊断环另实际累计lost2990，Host私有内存1669730304字节；静态确认每条独立Json的默认分配块64KiB（本地RapidJSON allocators.h:251），16K条仅分配块就约1GiB。修复为不可变序列化正文，65536条／32MiB双重上限；确认释放、锁外分页仍保留，真实未读溢出不转为通过。资源代次补充Lua按值ANM识别：使用Lua公开userdata存储范围与SDK sizeof(ANM2)证明内嵌所有权，不猜vtable／私有包装偏移；按值复制保留真实copy来源，未独立观测且未改资源的临时复制链归并来源，已成为真实owner者保持独立代次。新增E2E资源失效纯值事件及各域具名Partial状态供后续定位。两端按失败收尾正常退出，完整冷构建仍未通过，这些修复待同tx续验。

第16轮E02.cold第4次Host PID24852再次正常双人Basement，真实Client仍等待世界。具名域诊断确认Entity/Grid均在`read.lua:363`报`attempt to call a nil value (local 'stream')`：finish中的输出变量`local stream`遮蔽了同名模块函数，投影入口根本未调用。修复输出名为streamPayload，不改变采集字段或Complete判据。该窗口也有实际诊断丢失及90次cursor缺口；不能全部归为已读项假丢失。只读trace分页原来逐条排Manager，在低帧率下排空不足；E2E专用控制线程现以启动时冻结token、严格uint64 cursor及60KiB回复上限直接读取受锁保护的不可变纯值环，游戏／Lua／场景命令仍全部主线程。分页编码已在锁外，Add改为短临界区排队，真实未读溢出／分配失败／主动缺测继续报告，不填补数据。新增失败收尾事实实际记录两端正常退出code0，Client无非日志／选项／已知加载元数据的主体变化；Host第二测试槽变化单列，不用于访客比较。资源身份修复与这些后续修复仍需在成功冷构建及后续整体验收中覆盖，当前E02未通过。

第15轮E02.cold第3次Host PID13620实际进入双人Basement，INIT除零本次未复现；仍因真实VM未安装世界超时。新增诊断实际读到Lua采集87/87 Partial、live_delivered0、stream_native_resource_changed／live_native_resource_changed持续。静态确认资源Hook把所有ANM2构造／复制／析构，包括只读getter的临时副本与尚未进入来源闭包的UI对象，都立即撤销世界。修复资源身份账本增加observed／temporary：真实已观测owner写入仍进入即撤资格；只读采集窗口的构造副本不占用世界写入深度，复制代次继承实际right来源，原链内Reset后正常返回才恢复别名，析构仅退副本身份；原对象变化仍撤销，地址重用仍换代。未观测对象不冒充现有来源修改。另保留各Lua域最近具名Partial原因，仍要求完整普通域和全部依赖，不用Partial发Baseline。修复须继续E02及其后资源／生命周期场景实机覆盖，尚未宣称冷构建完成。

第14轮E02.cold第2次在首个Player构造中实际崩溃，Host PID83052，尚未进入Basement，未提交done／通过。Lua栈为Lifecycle INIT→entity.snapshot→common.read.query；离线原始转储确认`0xc0000094`整数除零，模块基址0x00c60000、PC0x01027626（RVA0x3c7626），直接调用链为`libzhl!Entity_Player::GetEnterPosition`→原生Room坐标换算的`idiv [ecx+0xc]`。本地IsaacDocs的MC_POST_PLAYER_INIT明确警告INIT属性未完整设置，和本次构造时序一致。修复INIT事件只记录真实事件／稳定身份／参数，并显式标注对象全量快照NotScheduled、留待OrdinaryPost；完整字段目录与普通域采集不缩减，也不把分母猜成1或伪造返回值。源诊断已实际输出ready0／stream_native_resource_changed与live_source_gap，说明首份Baseline仍未形成；不把菜单期间原因外推为本次除零的唯一根因。两次冷构建失败均保留，需继续修复／续验。

第13轮E01九子项已全部完成并正常退出／访客存档比较通过。随后E02.cold第1次：Host第二槽Continue灰色，从Isaac普通新run进入Basement，两参与者原生身体出现；VM一直AwaitingWorld／world_applied0，源采样／发布计数0，90秒等待最终超时。Host反复记录已捕获C++异常，窗口后来约4FPS；具体抛出原因当时未输出，不能预判网络或原生Client应用错误。退出轨迹另见`retirement_error device=1 player=1 peer=0`，保留原失败。静态确认E2E环读取不释放已确认项，16K满后会把已取回记录也计入missed；每页32条、每轮最多128次主线程IPC还会拖延控制响应。修复为cursor确认后释放、最多128条且24KiB正文、锁外编码不可变纯值、明确more字段；节点每轮最多16页／200ms，再处理控制请求。未读溢出和真实缺测仍失败。源状态补充实际Capture ready/reason与LiveConsumer原因／计数，未放宽发布资格或补造样本。E02根因及这些修复须在同tx继续实机定位／验证，E01既有协议／菜单行为不受影响。

第13轮valid-menu第2次在VM启动前返回磁盘不足（os error112），没有误记游戏或协议失败。客体C:实际余量0；历史save-before/after完整复制Documents中的旧crash_dumps，每个完成窗口约1GB，原转储不属于本次新故障。逐项从progress中核实八个已通过子项对应节点/launch，仅删除其16份VM临时快照，原始存档／转储、失败材料和original恢复备份均保留，恢复7891697664字节。第3次同构建续验，两端实际菜单连接与身份冻结成立；Host Peer1和Client的实际effective均为固定60/30/20/60Hz、937500/312500byte/s、24576/8192burst、三个预留槽、禁止跨Peer借用，新增读取判据实机命中。正常退出和全P2后续状态以progress为准；用后快照滚动清理，不以扩大磁盘或缩减存档比较掩盖问题。

第12轮E01.valid-menu第1次菜单连接和身份冻结审阅成立，但最后一项固定份额只在Client事实中读到实际配置，Host原有facts仅采集source，不能用部署配置代替真实Peer字段，故明确缺测失败。新增harness专用核对：从Host `cs stream <ordinal> 0`与对应Client `cs stream client 0`读取冻结effective配置，逐项验证60/30/20/60Hz、总937500／Peer312500 byte/s、24576／8192 burst、三个预留槽、禁止跨Peer借用及实际profile。原失败保留，同tx重构建后只重跑未通过valid-menu和后续项；容量及七项协议负向已完整通过，不受该读取修复影响。

第12轮容量第5次实机通过：Host PID8088、VM PID11904，真实Client始终AwaitingWorld、native_attempt／world_applied为0，两个纯Hello探针各10片完整Welcome；三条连接ID互异，超额10片使`net_route_rejected`从0到10。正常退出／存档主体比较通过，只有已有允许的RGON加载计数容器元数据变化。随后进入E01.valid-menu，未把本次菜单成功外推为输入／原生世界／后续性能已通过。

2026-11-07第10／11轮容量续验：第3次容量的两个纯Hello探针各收齐10片Welcome，额外地址的10片Hello全部使共享socket拒绝计数增加；但真实VM Client已因`live_clock_range`退到Off，明确记失败，没有以探针拒绝代替双人连接成功。该窗口正常退出又发现Client槽1新增／变更真实存档主体，超出加载计数元数据白名单；当时用户也在调整VM开场选项，实际写入归因仍为UNKNOWN，保留失败，不还原用户进度或放宽比较。用户随后确认VM已开启SkipIntro，后续只临时关闭Host SkipIntro，保留VM的1。第4次容量再次命中同一断线；E2E首因精确记录Client回显8110343→8110375（32ms），Host接收823039921→发送823039968（47ms），差15ms，各机内部顺序均正确。原2ms负RTT容差小于[GetTickCount64通常10—16ms的分辨率](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64)。修复将区间差的量化容差设为32ms，继续拒绝同机倒序与更大矛盾；实测RTT保留原值，输入投影和年龄误差另计64ms保守上界。第11轮仍失败，修复后的完整容量／后续项待同tx续验；七项协议拒绝不经过此正常时序分支，继续显式复用既有通过，不能声称完整E01或P2完成。

第9／10轮容量夹具修复：关闭SkipIntro后，两端均实际播放正常开场、由智能体按Enter跳过并到标题，真实VM Client成功协商到AwaitingWorld；不把本次未复现外推为启动异常全场景已消失。full-room-capacity第2次随后失败为“容量探针未获Welcome，不能冒称已填满”，这条代码分支表示收到非Welcome报文，原夹具未保存其kind／原字节，具体回复内容保持UNKNOWN。静态确认探针调用纯模型`Capabilities()`，其`native_appliers`为空，真实`live::Agreed`必需四项原生应用能力；修复为ops入口提供真实DLL的同一纯值声明，编码层不依赖网络owner。探针仍不发Ready/Input、不创建身体或世界，也不计入真实Client数。新判据收齐同一Welcome的全部有界分片，保留异常原报文；满槽后的未知地址在共享socket层拒绝，新增固定聚合`net_route_rejected`读取该owner，而不再误读单Peer的`net_foreign`。第10轮继续同tx重构建／部署，只重跑未通过容量子项和后续项；前七项协议负向不受这些夹具及诊断修复影响，原通过和两次容量失败均保留。产品／E2E DLL已编译链接，Rust及后续实机结果待续验。

2026-11-07第8／9轮续验：第8轮已完整通过E01的wire、stream-schema、compact-schema、resource-declaration、session-identity、framing、capability-missing七项，均真实双进程、具名sender命中、实际拒绝／身份审阅及两端正常退出／访客存档比较通过。拒绝首因依次为`stream_capability_version`、`stream_capability_version`、`compact_payload_schema`、`resource_version_mismatch`、`live_welcome_nonce`、`stream_capability_version`、`unsupported_capability`，接收候选与原生安装分开记录。随后full-room-capacity第1次的Host PID26428在启动8秒后崩溃，VM尚未启动、未进入容量协商；Windows记录`0xc0000005 / RVA 0x52e54b`。离线转储确认原始`Manager::Update`在`movups xmm0,[eax+edi+0x20c78]`读取无效地址，eax=`0xeaf8b1a8`；静态对应`Cutscene::_queuedCutscene`索引。已核本机REPENTOGON `SkipIntro=1`，官方`CutsceneSkip.cpp`跳过原始`Show`，而原始`Show`负责设置该索引；这是目前最强静态因果推断，未补造此前写入历史。第9轮仅改E2E启动配置：临时将REPENTOGON `[VanillaTweaks] SkipIntro`设0，正常初始化后按真实画面跳动画，原文件纳入逐节点还原。未改原版／REPENTOGON二进制、协议或安装器，显式复用前七项未受影响结果，从未通过的满员子项继续；不宣称启动异常已实机修好。当前完整E01、E02—E26与P4均未完成。

2026-11-07续验执行进度：用户已在本机完成VM凭据输入，WinRM重新连接成功。第6轮在协商前由智能体停止，原因是安装入口没有临时隔离现有第三方Mod；`Node::install`现逐目录保存并临时写入`disable.it`，最终按原字节还原，实际Host日志仅执行本项目三份Lua入口。第7轮由智能体因Windows防火墙询问遮挡而停止，尚未执行协商；两轮内置停止文字均写“用户请求必要中断”，实际是智能体为修复环境主动结束，用户未取消测试。第8轮固定宿主机协调器／helper为`out/repentogon/e2e/harness.exe`，VM helper为显式临时根下`node-tools/harness.exe`；游戏、配置和临时采样仍逐尝试隔离。经一次管理员许可配置精确Host／VM地址的临时端口规则及Host固定harness程序规则，两次Host启动与一次VM启动未再出现联网确认。管理员进程只等待本tx固定还原信号，结束时撤销规则、精确信任及本次服务状态，避免重复许可。E01.wire第8次已完整通过：实际sender命中、Client在世界分配前以`stream_capability_version`拒绝、身份／scope全零、两端正常退出且cleanup无失败；已知RGON文件仅加载计数元数据变化、正文不变，旧第5次失败保留。当前正继续E01.stream-schema，完整E01及E02—E26尚未通过；P4未执行。

2026-11-07续验准备：用户已重新授权宿主机Host＋现有Hyper-V VM Client的完整P2测试、临时WinRM／精确信任／防火墙配置、自动截图审阅及同tx连续修复续测，并明确无需再次询问测试许可。本次核实VM已登录桌面、两端无游戏进程、旧部署记录均已还原。产品、E2E及Win32 Rust Release构建完成；最初沙箱内MSBuild节点IPC等待，改在沙箱外构建后正常完成，未将构建记作E01—E26通过。旧DPAPI凭据按上次收尾已删除；本机凭据窗口首次因继承PowerShell 7模块路径失败，已修正Windows PowerShell模块路径后重新打开。Windows管理员提示随后返回“操作已被用户取消”，连接准备脚本未启动，本次尚未修改WinRM／TrustedHosts／防火墙或启动游戏。当前必要配合仅为本机Windows认证，测试范围已有授权；等待凭据输入及管理员提示完成后继续原tx，不改变历史E01.wire失败。连接脚本新增显式`-UseSavedCredential`，仅复用本次用户在本机窗口保存的同用户DPAPI凭据，密码不进入聊天、命令参数或日志。

当前进度：2026-11-07，Asia/Singapore。I39已按用户要求进入宿主机＋Hyper-V VM的P2整体验收，产品／E2E构建完成；两次VM系统蓝屏使E01.wire启动准备失败。环境修复后，第五次编排中两端均成功到标题页；VM没有重现Steam加载错误或蓝屏，后续正常退出，启动后相关关键事件检查为空。E01.wire的错误协商与无身份分配观察完成，但收尾按原始字节比较发现RGON存档差异，整项仍保存为失败，未追溯改判。用户随后明确要求本次只修复、不再运行测试，并决定纯加载计数元数据变化不作为进度污染；比较逻辑已修改且仅编译通过。E01—E26尚无完整通过结果，P4未执行，I40与整体验收未完成。

本次存档差异与用户决定：已有前后快照中，三个`rgon_steam_persistentgamedata*.dat`均只有末尾加载计数增加4及末尾容器字段变化，正文逐字节相同。官方`SaveSyncing/SaveFile.h`定义09R格式的末尾计数／校验字段，`SaveSyncing.cpp`的进度同步比较会忽略计数。按用户要求，`saves.rs`仅对已知RGON槽名、相同长度、09R头、全部正文相同且计数确实变化的情况标记`container_metadata_only`，保留原始`bytes_changed=true`与计数变化；tx21不再将此类情况作为访客进度污染。头部、正文、长度、未知格式及单独校验字段变化仍不归入该类；不计算或验证校验值，不回写存档。没有为这些计数增加生产退出阻断。

收尾同时修正：比较失败先记录，再继续正常关闭其他节点，保存完整`cleanup-facts.json`后报告失败；终端只输出简短首因和完整记录路径，避免重复展开所有事实或Steam账户路径。本轮按用户要求停止游戏／测试，已还原游戏部署与临时选项；比较修改只做编译和静态审阅，未重新执行、部署或将原失败改成通过。

2026-11-06—07环境修复事实：两份VM内核小转储均为`0x3B / 0xC0000005`，落在`dxgkrnl!DXGVMBUSMESSAGE::InitializeMessage+0xde`的空指针访问，经`DXG_GUEST_VIRTUALGPU_VMBUS::VmBusSendIsFeatureEnabled`调用，当前进程为`svchost.exe`。宿主机与VM均为25H2／26200.9711，VM分配Intel Arc B580的GPU-P。用户提供的RdpIdd／Remote Display Adapter故障事件在最近一次重启后出现，不能单凭该事件把RdpIdd认作蓝屏根因。两端旧游戏均正常退出或随VM重启结束，旧部署／游戏选项已恢复；连接诊断所需临时WinRM／窄防火墙设置继续按原恢复记录管理。

第二次VM启动显示`Application load error S:0000065432`。失败时Steam日志同时存在连接／更新超时；VM继承的宿主机DNS把Steam域名解析为198.18网段的代理假地址。将VM同一网卡的DNS改为`223.5.5.5`／`2400:3200::1`后，默认解析得到真实地址，Steam公开更新端点HTTP200，Steam活动用户与网页辅助进程恢复。原DNS配置保留供回退。这证明网络／Steam准备状态恢复，尚未证明游戏已能启动；没有把S错误码唯一归因为DNS。

系统修复：用户已明确允许仅在VM安装微软可选预览累积更新[KB5127936（26200.9818）](https://support.microsoft.com/en-au/servicing/os/windows-11/2026/10/kb5127936-windows-11-25h2-26h2-update)，成功后重启VM，宿主机不更新。该更新包含Sandbox相关崩溃修复；[同版本同内核栈的原始报告](https://github.com/microsoft/Windows-Sandbox/issues/131)提供较新构建恢复的反馈，仍须对本VM实际续验，不能据此提前标为已修好。两个官方MSU包共5.43GB已完整下载；首次NoRestart安装于2026-11-07 00:35开始，00:38因`0x80070070`（磁盘空间不足）失败，未重启。安装解包期间C:余量降到约514MB，失败后回收至约6.95GB；安装包与日志保留。VM系统盘64GiB、C:后有恢复分区，准备独立临时更新盘承载安装包／解包缓存，需用户另行明确授权，既有分区不作修改。必要诊断／恢复材料暂存在本次运行目录，完成后按现行规范清理。

随后用户明确允许创建／格式化独立32GiB空白临时盘，更新完成后卸载删除；该操作不格式化现有磁盘、不重置Windows。实际临时VHDX为`F:\VM\VM\Virtual Hard Disks\IsaacCS-tx21-update-temp.vhdx`，连接原VM的SCSI 0:2，在客体核实唯一新增RAW盘后初始化为R:。原系统盘仍为64GiB、四分区不变。安装包已迁入`R:\tx21-update\packages`，第二次NoRestart安装在00:46开始，同时指定R:的ScratchDirectory与仅本次进程TEMP；00:47时C:仍余16.1GB、R:余23.2GB，尚未得到安装完成或重启后验证。临时盘owner／恢复状态由`prepare-tx-04-21-vm-update-disk.ps1`及本次诊断目录记录，需客体确认清理／离线后才卸载删除。

第二次安装于00:54:26成功返回并要求重启，进度回调的`0x80070490`未阻止组件安装完成。正常远程重启因已有交互会话被Windows拒绝后，使用不带`/f`的本机`shutdown /r /t 0`完成用户已批准的重启；中间一次状态记录命令误用WinPS5.1不支持的`ConvertFrom-Json -AsHashtable`参数，未发出重启，改用兼容写法后成功提交。新启动时间00:57:48，实际Windows构建26200.9818、磁盘上的dxgkrnl版本10.0.26100.9818，RollupFix 26100.9818.1.42为Installed。Intel B580、Hyper-V Video、Remote Display Adapter均OK，检查时没有本次启动后的相关关键事件或新增小转储；原四分区的大小／偏移均未改变，Steam已登录。此为环境修复与启动检查事实，尚未验证游戏负载下稳定性，不提前宣称蓝屏已完全消失。

更新成功后，客体专用安装包／临时目录已清理，临时盘核实身份后离线，再由固定VM／VHDX owner脚本卸载删除。临时连接当时仅保留用于紧接的同tx启动验证；用户要求只修复后，Host／VM部署和临时游戏选项均已还原，宿主机TrustedHosts与原Stopped状态已恢复，VM窄允许规则已删除、原TCP阻止规则已恢复、DPAPI凭据已删除。第一次连接收尾因同一已禁用TCP规则的Action变成Allow而拒绝执行；核对程序路径／方向／协议后，按用户原先恢复Block的授权修正并完成收尾，Action变化来源UNKNOWN。正常下载的符号缓存已清理，仅保留中断续验所需首因、快照差异、两次安装结果和恢复位置。VM的系统更新与DNS修复保留，现有分区没有格式化或重置。后续游戏启动／测试等待用户新的继续指示。

启动准备修正：节点增加只读Steam登录、活动PID、实际进程路径、交互Session、Windows账户SID与提升状态检查；请求玩家配合前检查一次，人工确认后、修改游戏文件前再检查一次。账户标识不进入诊断，只记录登录布尔值。该检查已编译，仍需随完整P2续验覆盖，不代替Steam实际初始化或游戏启动结果。每次游戏启动依旧单独取得用户准备确认。

历史实现状态（2026-11-05）：I36—I38的源码、部署恢复、回归目录和唯一入口已接入，并由主智能体静态复读；当时没有构建、运行测试、部署到游戏目录或启动游戏。本文保留该历史事实，不以静态实现代替I39结果。

入口：[实施清单](tx-04-21-implementation-plan.md)、[E2E组件](tx-04-21-e2e-components.md)、[验收矩阵](tx-04-21-e2e-plan.md)、[现行规范](../../refactor/project-conventions.md)。保持同一tx，不设gate、不校验哈希、不补写unit test。

## I36：从网络准备到最后退出writer

新链路继续使用唯一`ClientScene`及既有17—20用途/借用合同。没有新增游戏原生函数、Hook点位或原生对象布局。入口对应如下：

| 窗口 | 实际入口和owner | 继续约束 |
| --- | --- | --- |
| 菜单连接/准备 | `StartClient`、`BeginStream`、`AdmitStream`，Manager主线程 | 准备前Lua/回调/持久/Shutdown保护齐备；网络只传自有值，不进入本地run/Continue |
| 冷构建 | `Pump`→NativeBuild/NativeApply、18—20原有owner | Manager、用途帧、前缀登记、Graphics/Native票据；不整体放行GameStart/RoomInit/Update |
| 普通动态/身体快源 | `PumpStream`→CompileStream/CompileFast→InstallStream/InstallFast | 同Scene、实例/资源代次、完整成员和精确依赖；不执行AI/碰撞/伤害/拾取/库存玩法补算 |
| 结构/资源替换 | `StructuralModel`、原有Player/Entity/Grid资源owner及关系检查 | 先撤旧展示/借用，再按真实前缀替换；未知构造/析构不猜测放行 |
| 原生维护/绘制 | PresentationMaintenance、PresentationDraw、Player Draw用途 | Manager/Render分别限定；真实context、ANM/Image在途使用及已有随机/SFX/Lua分派不扩大 |
| 恢复 | `Recover`→旧Scene退休→新Scope/route完整Baseline | 关闭输入/旧候选；安全退休后才恢复，首因和失败owner仍由连接级账本保留 |
| 停止/关闭 | `StopClient`、`ClientProcessExitDecision`、Shutdown保护 | worker结束不等于native已退休；Off只在借用/owner退尽后交还，退出保护延续到最后持久writer |

`client_isolation.*`提供固定纯值累计，不依赖Recorder。`client.isolation.*`分开原生调用/正常返回、预期跳过/拒绝、Lua玩法跳过/自有菜单UI、native/Lua持久拒绝、随机拒绝、故障、恢复、停止和退休。真实网络Client现在与旧fixture一样，在连接开始冻结九个原玩法原调用计数，不能只检查最终`run_active=0`。

E2E显式启用后，这些事件带本地连接、wire连接、route、Scene/实例/资源/Lua/context代次、Scene阶段、主线程与owner纯值。外线程只记未知身份与故障，不读取Client控制块或游戏对象。ring丢失/诊断分配失败仍是缺测，不影响正式提交/ACK。

持久入口继续覆盖原生save/achievement/unlock/计数、Shutdown直接写入、REPENTOGON附加保存，以及Lua的`Isaac.SaveModData/RemoveModData`、PGD写方法和ESSM路径。ModReference的SaveData/RemoveData经既有早期别名保护；玩法Lua不因新增传输或高频安装而开放。调用见证与Documents/Steam remote/官方ModData逐字节末态比较分别记录。任意未声明Mod的外部I/O不被本文冒称已隔离；原有声明范围保持不变。

E2E的物理来源、canonical接纳/过期、真实普通/odd consumer正常返回、实际Triggered边沿及状态年龄另有纯值记录。一次consumer内重复读取Triggered只记一次查询见证；它与实际玩法结果的语义审阅分开。产品不编译这些逐项明细。`stream_test ... observation-loss`实际丢弃一条诊断并通知缺测；既有既有真实secondary VM/释放借用/错误context演练均要求token及Manager边界；实际同步不依赖通知成功。

## I37：逐成员与逐消息接线审阅

逐成员语义仍以`stream-catalog.json`和`CaptureLayouts`为唯一目录，没有删掉未支持成员来形成“完整”目标。静态主审按以下链路复读，而不是只检查接口声明：

| 链路 | 复读位置 | 结论/边界 |
| --- | --- | --- |
| 来源→范围/版本 | `world_bridge.cpp`、`stream_capture.*`、`stream_model.*` | 统一Registry；ordinary与每身体时间线分开；29/38私有联合来源限制保留，不虚报所有身体60 Hz |
| 范围→线上 | `stream_binary.cpp`、`stream_codec.cpp`、`stream_transport.cpp` | 固定完整布局、显式依赖、wire/framing/schema版本、解压/节点/候选预算；不引入22的字段继承Delta |
| 每Peer发送/取消/确认 | `net/stream_session.*`、`net/live.*`、`net/worker.*` | route与连接复核、可靠补片/进度超时、已发/已收/已应用分开、总桶/单Peer及lane；共享值不跨线程借用native |
| 主线程目标/实际消费 | `client_stream.cpp`、`client_scene.cpp`及18—20 adapter | 冷闭包、结构重建、普通安装、逐身体安装仍由一个Scene负责；实际native提交后形成精确范围ACK |
| 停止/失败owner | `client.cpp`、Scene/retained owner及持久保护 | 控制终态、输入封存、真实退休、恢复上限和首因分别保留；不以socket停止代替世界退出 |
| 产品/测试边界 | CMake、build/package/deploy、`ISAACCS_E2E` | 产品只增固定隔离聚合与必要检查；故障/inspect/负向/视觉暂留/逐项明细只随E2E编译 |

本轮静态发现和修正包括：网络Client缺原调用起点、旧Source夹具取得后态后暂停Host、P4扩展断言只有声明没有执行、E25仅五个概括项、WinRM服务桌面不能提供交互游戏窗口、产品配置不能含空E2E token、helper异常不能经Game兜底Drop强杀游戏、跨allocator临时JSON必须深复制、跨设备资格分母不能直接用另一台机器的绝对时钟、部署目录与恢复路径的边界和逐次记录，以及精确截图期间提交向量仍会变化。这些是静态发现，不是运行失败或通过事实。

`tx21/regression.rs::inventory`逐个读18/19/20现有`scenes`，保存原场景定义、所属tx、正负向和受影响来源/adapter/资源/退休责任。正向用依赖前缀依次布置，避免把`npc-change`等有前置的步骤孤立运行；每个最终条目单独比较真实网络后态/画面。Entity全部16个和Player全部24个具名故障前缀逐个展开。旧19独立事件文件导入器的四个拒绝场景未进入21的完整范围传输，不把未改动的旧导入入口重命名为21网络回归；其历史事实保留，完整最终成员/字段/引用仍由21范围回归覆盖。

菜单容量另用最多三个无世界握手探针：由正式codec生成当前能力的Hello，不发送Ready/Input/Range；填槽须实际收到Welcome，满槽拒绝须观察Host真实foreign计数，不能靠接收超时通过。这些探针不生成身体、不计入P2/P4的真实游戏Client数量。

负向分层明确：`stream_test ... wire <name>`在真实Hello/Welcome形成后仅改一次发送正文，接收端仍实际解码/协商；代理负责片/route/ACK等真实报文损伤；`stream_test ... reject <18|19|20> <name>`从真实已提交网络闭包派生恶意模型，在同一Scene检验原adapter的写前拒绝与A保持，不用本地文件，也不冒充wire错误。前置对象/FX/twin/backup不存在时明确报缺前置。具体拒绝层、首因和原生A必须对应具名场景，不能用任一错误兜底。

21到22的边界没有变化：本tx的完整范围、可靠分片、每Peer确认记录、Scope路由和同连接完整恢复可复用；多基线保留/轮换、对象/字段Delta及`Delta(B,T)`重建未被声明完成。23+的自然切房/整局/重连/Continue仍保留原责任。

## I38：唯一入口与真实节点

2026-11-06按用户本次先执行两人验收的要求，拓扑新增`phase`：`p2`只部署Host＋Client1并完整执行两人E01—E26；`p2-p4`（缺省）保持先两人后四人的原流程。`p2`拓扑可仅列两个真实节点，不要求提前准备另两个Client；四节点拓扑选择`p2`时也只访问前两节点。终端入口与游戏面板共用该选择，不裁减两人子用例，也不将P2通过记成整tx通过。实际拓扑可使用用户指定的宿主机＋Hyper-V VM，物理机/VM分布须按实际说明，不以此声称已覆盖两台物理机。

仅P2完成后仍正常关闭节点、还原部署/设置、注销本次远端任务，清理临时包、备份与采样；由于P4待后续设备，仅保留原运行目录内`progress.json`和`tx21-run.json`用于同tx续验。随后补齐四节点、将`phase`改为`p2-p4`，仍用原目录加`-Resume -ReuseReason <未受影响原因>`继续；存在影响时显式列出`-Rerun`。不把未执行P4写成通过。该补充尚待本次I39实机覆盖。

本次I39准备阶段实际遇到两处既有错误：`runner.rs::sample`负载分支缺闭合括号导致Rust编译失败，备用界面的两处`catch[类型]`缺少空格导致PowerShell解析失败。已展开负载分支并修正catch语法；修正后Rust Win32 Release编译成功，两个入口脚本静态解析成功。此时尚未部署或启动游戏，E01—E26均未执行；仍待确认VM实际地址、安装/用户数据路径和远端连接方式，不能将工具构建记为实机通过。

随后已核实本次Host/Hyper-V VM的独立Windows节点与存档路径、VM交互桌面和安装目录现有写权限；用户在本机凭据提示中提供Microsoft账户密码后，WinRM连接成功。用户明确授权本次临时启动宿主机WinRM并仅将指定VM地址加入TrustedHosts，结束后恢复原值。`scripts/prepare-tx-04-21-winrm.ps1`以Prepare/Inspect/Restore管理原服务状态、精确信任值及同用户DPAPI凭据；拓扑可显式提供本机`credential_path`，口令不写拓扑、日志或远端配置。首次凭据连接曾被拒绝，已还原后重试成功，不能改写为首次成功。

完整产品/E2E构建又发现并修复：`CS_STREAM_PARSE`参数`name`误替换RapidJSON的`i->name`、`NegotiatedFastLimits`只有声明没有实现、旧fixture消息聚合初始化未包含新增租约字段，以及E2E UI缺少RunActive/Busy声明头。补齐后两种构建均成功，仍不等于E01—E26实机通过。节点的`game_root`统一表示含原版exe与Repentogon子目录的安装根；实际启动和日志读取明确进入Repentogon子目录。

首次远端部署在加载磁盘ps1时被VM执行策略拒绝；两端没有启动游戏，Host已还原，VM实查没有helper/游戏或部署记录。第二次发现Windows PowerShell 5.1默认ANSI读取含中文注释的UTF-8脚本会产生语法错误；同一远端字节按UTF-8解析无错误，两端均已还原。现部署/还原通过WinRM执行本次受控脚本块，并显式按UTF-8读取；隐藏节点通过固定代码和正确转义的路径创建交互任务，不修改执行策略。`launch_requested`在helper启动请求前记账，部署尚未进入启动阶段时可直接还原，已有启动请求仍须确认实际退休。上述失败与修复属于同次I39，尚未启动游戏、没有用例通过事实。

唯一入口为`scripts/run-tx-04-21.ps1 -Topology <实际拓扑.json>`。拓扑示例在`tools/harness/scenarios/tx-04-21-topology.example.json`，必须用实际IP、安装/存档/临时目录、已登录的交互用户、物理设备key和真实键位填写，不能把示例路径当本机事实。本轮没有访问远端设备或更改这些设置。

入口仅在I39调用时才构建产品/E2E、打包、备份部署并启动节点helper。远端helper使用当前已登录用户的Interactive任务启动隐藏helper，真实游戏留在该用户桌面；不是在WinRM服务桌面生成一个不可操作的“游戏进程”。任务仅用于本次启动，正常停止后注销。默认不更改全局网络/防火墙，也不发送邮件或其他通知。

`tx21/node.rs`的有界token RPC只提供真实子进程启动/结束、固定控制口、已核窗口物理键、截图和本次文件分页。没有任意shell、任意函数/地址或任意文件路径RPC。节点连续泵显式E2E trace和真实状态/内存，保存缺页/溢出；不从DTO填native事实。进程、Lua和原生对象不进入Rust网络worker。控制连接断开先释放本方按键，不能据此强杀或假称原生已退休。

`tx21/runner.rs::run`按完整`phase_suite`执行，P2所有E01—E26和展开子用例实际完成、清理及总审之后才启动P4游戏。P4保留同一功能子项，扩到四组输入和三Peer，并实际执行扩展断言。没有单项预跑入口或人数SKIP/N/A。产品E24另换产品DLL，禁用fixture/测试token/控制口，走真实UI和外部物理操作。

编排是**需要人工配合的完整入口**：每次启动独立告知节点/目的，只跳开场，到标题/选档页停手；确认ready后才启动/发键。Host选定测试存档并开始新run、VM窗口选择、自然互动和产品UI均有明确开始/结束信号。harness不自动抢焦点；SendInput只发到已核实的当前前台游戏。`physical_script`维护跨步骤held集合及异常松键。按拓扑显式选择临时`PauseOnFocusLost=0`时，保存两个选项文件原字节，正常还原。

具名观察器分开自动事实和人工语义：实际负载、精确版本/成员/字段、原调用隔离、已命中的sender故障、物理/consumer事实及现有归约器数值不能靠输入“通过”覆写；画面、复杂玩法后态、指定原始报文注入和具名退休阶段须审阅对应原始事实，填写具体`observed`说明。未完成注入、未命中边界、缺前置、缺对应版本、未审画面或指标失败仍为未完成。该入口不声称无人值守，也不以一个通用`ok`完成整项。

精确画面段可显式启用`stream_test ... visual on/off`：选中native快照提交后，最多120秒暂留该Client提交向量，Host/来源/网络继续，期间候选仍有界；真实Render完成后再截图/审阅。暂留期间仍推进确认和纯值Collect；Host显式进入同一有时限的视觉中性段，单列收敛计时。退出暂留恢复陈旧年龄起点，避免把人工审阅时间当实际网络故障。此段明确属于视觉中性，不计算30/60 Hz或CPU指标；产品没有该能力。

代理另外有最多20次真实完整动态transfer重送。缓存只保留每Peer一份在收集、一份已完整的至多64片动态消息；延迟/发送队列收紧到7MiB，为这些有界副本留空间，全部仍在原8MiB缓存范围内。重送保持原报文身份，按实际socket发送计费，排队次数不等于已送达或已应用次数。

## 同tx修复、还原与未执行项

### I38补充：从游戏内ImGui操作统一入口

2026-11-05按用户要求增加E2E专用面板，入口为REPENTOGON ImGui菜单的 **Isaac CS → tx-04-21 acceptance (E2E)**。这是上述同一套I39编排的前端，P2/P4次序、完整子用例、实际失败、恢复及人工说明要求都由原runner执行。**补充实现同样只有源码和静态审阅，本轮仍没有构建、部署或实机测试。**

第一次使用需要在进入I39后构建并安装包含本次修改的E2E DLL和UI；已经运行的旧DLL不会自动获得新接口。引导安装配置沿用完整schema=1配置，将`enabled`显式设为`true`，并保留E2E包附带的`"e2e_ui":{"enabled":true}`。产品包没有该配置、原生入口或`e2e.lua`，不会显示测试按钮。REPENTOGON Wiki `changes/Debugging.md`说明ImGui沿用原调试控制台打开键；以用户当前REPENTOGON按键设置为准。

操作顺序：

1. 在房主本机打开面板，填入**实际拓扑JSON路径**；先按现有示例填写真实节点、路径、IP、交互用户和键位。本次只执行两人时设`phase=p2`并填写Host＋Client1；执行完整两人/四人时设`phase=p2-p4`并填四节点。面板不将示例自动当作真实环境。
2. 新运行的目录可留空；同tx续跑则填写原运行目录，勾选`Resume this tx`，填写受影响用例列表和未受影响结果复用原因。列表以逗号分隔，条目仍是原runner的精确`p2/<id>`、`p4/<id>`或整个`p2`、`p4`。
3. 回到没有活动run或C/S会话的菜单，明确勾选I39准备项，点击开始。按提示**从游戏菜单正常退出当前引导游戏**；后台等待请求进程实际退出后才调用统一脚本构建/部署。不会强杀或替换仍在运行的DLL。
4. 房主E2E游戏启动后可重新打开相同菜单，查看阶段、用例、步骤和当前提示。每次启动/人工窗口仍分别点击`Ready`；完成指定操作并停手后点击`Done`。具名观察、总审和修复续测分别提交`observed`、`reviewed`、`retry`的具体说明，数值失败不能通过按钮覆写。
5. 游戏关闭期间、未打开面板时和产品版E24期间，使用独立确认窗口接续同一条提示。正向回答提交后自动收起ImGui；备用窗口如检测到本机游戏仍在运行，会暂存已明确点击的回答，等待用户手动将游戏恢复前台再发送。没有自动抢焦点或按超时推定准备完成；具体目标节点的窗口/物理输入检查仍由原helper执行。

`scripts/start-tx-04-21-ui.ps1`提供持续存在的备用窗口和隐藏编排子进程；隐藏子进程仍调用`run-tx-04-21.ps1`，不是第二套测试引擎。当前GUI只支持**协调器就是本机Host**；远端Client保留各自真实窗口和人工配合，房主面板显示具名节点提示。Windows需安装标准位置的64位PowerShell 7；原终端入口保留兼容。

本机`out/tx-04-21-ui`保存界面profile和信箱。请求经`cs e2e_ui`的有界分块入口进入专用worker；游戏主线程只交换自有字符串，worker只处理文件/进程句柄，不跨线程持有游戏/Lua对象。脚本地址固定在编译时工程的`scripts`目录，不提供任意shell。启动重复检查、唯一窗口互斥、显式会话及逐提示ID防止旧回答或双窗口点击串入下一条提示。两种界面按first-writer规则提交，runner独立验证回答内容；截断、半包、缺观察或错误回答都不形成通过事实。

自有UI仍只有原来的一个`MC_MAIN_MENU_RENDER`回调，没有扩展Client玩法Lua白名单，也没有新增玩法回调。面板构建发生在UI初始化或该菜单回调，避免在ImGui遍历窗口集合时新增窗口。产品切换除不加载界面外，也移除先前E2E部署的`e2e.lua`；最终部署备份继续负责还原用户原文件。正常完成清理GUI本次输出/提示，仅保留界面配置和完成末态；必要中断保留运行位置、输出和恢复材料。ImGui桥、备用窗口、焦点交接和所有既有合同均归I39统一实机覆盖。

### Render回调补充审阅与修正

同日按用户提醒审阅ImGui Render阶段。先读本地原版Wiki的`Isaac.ExecuteCommand`与REPENTOGON Wiki的`ImGui`/`ImGuiCallback`，再读官方`CustomImGui.h::RunPreRenderCallbacks/RunCallback/DrawWindows`及`LuaImGui.cpp::Lua_ImGui_Hide`、`ImGui.cpp::windowProc_hook`。不涉及新的未知ABI/点位，无需逆向。本轮仍未构建或运行测试；以下为静态发现与源码修正。

- **输入恢复过早**：官方`ImGui.Hide()`只设置`menuShown=false`，`Console::_state=2`由后续WndProc消息解除。前一版本Hide后直接让worker发回答，存在编排器过早发键的风险。现在Render只排队，`E2eUiManagerReturned`在原生Manager正常返回且无Render在途时处理；正向回答必须经过两个无本方UI渲染且原生Console未屏蔽的Manager周期才交给worker。必要时只向已经核定的renderer窗口投递`WM_NULL`，让官方WndProc自行处理显示切换；不写Console状态、不发键、不激活窗口。这两个周期不是Authority tick，不推进模拟、不复用旧边沿。start的活动run/会话检查也移至Manager。
- **固定显示与性能混入**：官方固定窗口在`ImGui.IsVisible()==false`时仍会执行Render回调。现在正向回答前关闭并取消固定两个本方面板，验收面板在非交互状态只记固定计数，不做JSON/hex轮询；UI命令不计入正式IPC阶段。UI文件worker在没有可见轮询/待处理工作时休眠，正向回答提交时撤销轮询心跳，不把其磁盘刷新带入自动采样。
- **实机前台不能证明UI已解除**：E2E自动发键除实际前台检查外，增加原生Console、连续空闲Manager、待交接回答及UI worker空闲检查，失败不发送新按键。`stream_test ... clock`提供这些纯值、累计UI屏蔽周期和本方面板Render次数；节点的低频状态同时保存这些实际字段。采样前先完成全体机器关闭UI的准备，再固定各机区间；结束时检查累计计数不变。任何缺字段、无效owner、计数耗尽、UI重新开启或固定显示渲染均不能通过该窗口，失败与必要的完整重测沿原同tx流程执行，不扣掉污染段来拼通过。

Render中的状态读取、字符串/JSON处理和有界排队没有进入Scene安装、输入apply或原生生命周期；后台继续只持有自有值和OS句柄。既有Client菜单唯一白名单、绘制context/借用、Hook常驻与产品/E2E隔离均保持。操作上，自动动作和性能窗口期间保持控制台/ImGui及本方固定窗口关闭；需要查看进度时使用独立确认窗口，待下一条人工提示再打开游戏面板。I39须在原E08界面/控制台恢复及自动物理输入、E21等稳定/性能段、产品阶段和P2/P4完整流程中实际覆盖上述行为；这不是已实测通过的结论。

`progress.json`记录每个P2/P4子项、尝试目录、首因与修复说明。失败后继续同tx定位/修复/重试；只有缺配合、需要重构建部署或实际无法自行收口时中断。`-Resume -ReuseReason <未受影响原因> -Rerun p2/E...,...`显式复用未受影响成功，公共修改要列出P2影响，再继续P4。没有哈希、旧解锁字段或独立作者要求。

部署逐文件备份并在覆盖前写记录；额外选项/Mod启停标记另存原字节。恢复只复制原文件/删除本次新增单文件，创建的目录仅在仍为空时逐个删除。关闭失败不强杀，保留实际owner/诊断。正常全部完成后，脚本还原全部当前及历史尝试节点，注销helper任务，核对绝对临时目录和本次owner记录后清理。中断仅保留继续所需的进度、失败、节点/恢复路径和必要采样；不新增每tx归档artifact。

资料顺序：先查本地原版Wiki `ModReference.md`、`enums/ButtonAction.md`，以及REPENTOGON Wiki `PersistentGameData.md`/回调文档，再查本地官方持久/附加保存/OptionsSyncing源码和项目17—21已核owner。本次只复用已核原生调用/借用及纯值协议，没有新增未知ABI/点位需求，**无需新增逆向，未启动逆向子智能体**。若I39出现必须逆向的具体缺口，仍按本次会话要求启动一个GPT-6 Luna max并由主智能体审阅。

目前只有静态实现/复读事实。两种配置能否构建、远端Interactive任务和运行库场景、P2/P4全部原始注入/负载/画面/隔离/恢复、采样吞吐、频率/预算/延迟/CPU/内存均待I39实际执行。诊断ring/文件预算耗尽会明确缺测，不能截尾或缩小负载判通过；需要分段时在执行前按场景边界安排并保留完整分母。I39失败必须在同tx修复续验，不能用本文或历史18—20结果替代。








