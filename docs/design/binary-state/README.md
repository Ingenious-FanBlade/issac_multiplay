# 原生二进制状态同步重构：总计划

**2026-11-12 RE-10 静态完成**：[Manager 50 个业务调用点与音频／本地维护](client-display-delegation/RE-10/README.md)已逐项分类。音乐 tick 不随相机 paused 跳过；SDK `SoundEffects::ClearVolumeModifier` 是条件性混音刷新，撤销“通用 SFX tick”猜测。当前 SFX 乘数、音乐读取的种子效果／dizzy／红房位、输入泵复用、本机 A／B 保持与回放旧节流处理交第 8／10／13 步，E36／E37 仅登记至第 15 步。下方“新开／待做”保留审阅时点，当前 RE-10 静态工作已完成；未改运行代码／wire／版本，未测试。

**2026-11-12 RE-1 补遗与 RE-7—RE-9 审阅，仅文档**：经 IDA J273 复核后，用户决定 Q11（RoomTransition 显示字段同步 Host 当前值，Client 不运行转场 FSM）与 Q12（保留 `client_entity_draw_paused`，Host 发送 paused 位与终结分支枚举），新开 RE-10（Manager 尾部非 Game 调用分类，待做）；并采纳 HUD 可见性 `Game+0x22E90` 按 Q8 同步、诅咒同步 raw 字段、SFX 回放 FrameDelay=0、Pα 由已安装 alpha 判定。见[修改方案](client-display-delegation.md)及下方第 1 节决定 13、14。未改代码，未测试。

**2026-11-12 RE-7—RE-9 静态研究完成**：本地 Wiki／REPENTOGON SDK 优先，补齐[不可算值注入](client-display-delegation/RE-7/README.md)、[音频／文字／演出来源与 Q7 作用域](client-display-delegation/RE-8/README.md)、[原生转场 ABI／阶段与显示拆分](client-display-delegation/RE-9/README.md)。完整 RoomTransition Start／Update 含玩法写入，不能只替换 ChangeRoom 后放行；最终目标按 Host 实际换房结果关联。仅文档，版本71不变，未改运行代码或运行测试；布局收缩、采集与 Client adapter 继续后续步骤，第15步唯一统一验收。

**2026-11-12 RE-5／RE-6 静态研究完成**：[Render 覆盖与已知写集](client-display-delegation/RE-5/README.md)确认 Minimap／ItemOverlay 在 Game::Render 树内，定稿本地缓存、同步字段暂存恢复与 E28 漂移范围，纠正旧 Tear 十进制偏移误记。RE-6 已并入 RE-5；本次只补布局源说明／证据和研究文档，不改变 wire、版本或运行代码，未测试。其余 Render 写入不做静态闭合，交第 15 步 E28。

**2026-11-12 方向调整，仅文档**：用户决定借鉴 isaac-lan，改为只采集客机需要的状态，显示交给客机本地完成（包装原生 `Game::Render`），帧号用方案B（Client 原生帧号对齐 Host）。[修改方案](client-display-delegation.md)列出各步、布局源、Hook、退役、验收和新增逆向任务的改动。

- Q1—Q4 已按推荐确认；同日追加 Q5（切房走原生过渡）、Q6（放行回调分派）、Q7（音效按来源去重）和 Q8（内联显示标量同步当前值）。RE-3—RE-5 审阅后再追加 Q9（同步当前房间描述的 SpawnSeed）和 Q10（保留 `client_entity_draw_projectile` 作恢复作用域），并补入关闭插值时的相机节奏与验收 E40。RE-1 补遗与 RE-7—RE-9 审阅后再追加 Q11（转场状态同步 Host 当前值）和 Q12（paused 位与终结分支枚举），新开 RE-10（Manager 尾部调用分类，待做）。
- 逆向 RE-1—RE-9 及 RE-1 新增补遗已完成本轮静态研究，RE-6 已并入 RE-5（[成果](client-display-delegation/README.md)）；据此修订：显示推进根改挂 Manager 边界，`render_sort` 改由 Client 自有全排维护，Backdrop 构图与生成隔离的 hook 保留，Minimap／ItemOverlay 不另设绘制调度，已知本地 Render 写入不作漂移比较，转场改为具名显示拆分（后被 Q11 取代：转场字段同步当前值，Client 不运行转场 FSM）。静态完成不代表采集／原生安装已实施。
- 改动前的快照已提交为 `3977b46e`。
- 本文第 1、3、4、6、7、9 节、[规范第 17 节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)和第 0—15 步文档已按方案修订。
- 未改代码，未运行测试。

**2026-11-12 概率查询暂缓**：按用户要求暂停该部分后续工作；已确认的C++接口来源、代码停点、产品编译结果和恢复事项已记入[第五项进度](10-run-presentation-capture.md#当前停点概率查询暂缓)。概率部分及第五项整体均未完成；未启动游戏或运行测试。

**2026-11-12 第10步第五项实施中，版本71**：[Run与显示／音频生产缓存](10-run-presentation-capture.md)记录Run窄投影、正常生产缓存、逐身体Stat／History和HUD驱动、已核屏幕包络与部分SFX状态／控制。每个参与者详细面板与道具历史对应自己的控制组。全部文字／瞬态贡献、程序History与音乐等仍未完成；第五项没有标记完成。只做静态分析、生成与产品编译，无测试，第15步唯一统一验收。

**2026-11-12 HUD布局与纯值模块，版本70**：[实施说明](hud-local-implementation.md)登记315派生字段退出、311/312普通／特殊条件分支、独立435时钟、正式编解码及Client纯值显示模块。产品DLL纯编译／链接已完成；正常采集／完整发布／资源后端／候选校验与原生绘制仍待接线，未运行测试，第15步唯一验收。

**2026-11-12 HUD本地化决定，仅文档**：[现行HUD合同](hud-local-presentation.md)将低血周期闪烁、Holy Mantle资格、普通主动／饰品图标、材料与法术队列绘制、Drop渐变、基础属性格式化及多人HUD排版改为Client纯显示生成。优先复用已提交状态／资源／时钟，补齐最小低血判定与实际Drop驱动；完整心格、三色触发闪烁、特殊图像、有序库存／History及属性变化相位／概率仍保留同步。此前公共绘制八项不裁剪。规范及第8—15步交接同步；本次未改schema／生成表／代码或整数版本，无生成、构建或测试，第15步唯一验收。

**2026-11-12 第10步第四项 Grid／Room／Level来源闭合，版本69**：[实施记录](10-grid-room-level-capture.md)已补背景正常生产的有限recipe／实际用途绑定、完整光照投影和波纹资源。按用户决定，水面／波纹由Client纯显示推进，经典／反射为本地画质选择；Host保留世界参数与资源。来源接线已实现，完整发布／本地物化仍按第11／13步推进；未运行测试，第15步唯一统一验收。

**2026-11-11 第10步第四项续，版本68**：[Grid与Room／Level](10-grid-room-level-capture.md)已接Grid全部原生类型／旁表，补入Room／Level结构、L0—L3及已核只读投影，八门改为同宽Grid引用数组。Room／Level全域尚未完成：背景程序内容／实际pass、完整光照／水面模式与特殊波纹Sprite来源仍缺，严格失败且不发布不完整WorldFrame。产品纯编译／链接完成，无测试；第15步仍唯一统一验收。

**2026-11-11 第12步第3项完成，版本67未变**：[影子状态／候选事务](12-client-shadow.md)已实现准确成功base继承、完整根／投影与旧链接核对、自有Frame及结构意图、强制完整validator／纯值缓存提升、joint共同提交与实际busy清理分离。产品DLL纯编译／链接完成，无测试；完整域／结果／调度／主线程接线仍待第4—7项及第13步，第15步唯一验收。

**2026-11-11 第10步第四项完成，版本67**：[Entity公共部分、全部已登记子类与Player](10-entity-player-capture.md)已接原生只读源、全部身体ordinary联合根、逐身体快源及Clock提交后的纯值交接；引用快照与资源目录均有明确后台寿命。两项计时按用户决定改原始i32，QueueItem改配置资源引用。产品DLL纯编译／链接完成，包含当前Host桥不完整类型问题的修正；未运行测试，其余采集域／完整发布继续后续，第15步唯一验收。

**2026-11-11 第12步第2项完成**：[表管理实现](12-client-tables.md)已接精确epoch自有快照、成员退休／借用历史、资源内容共享／数字图／独立准备状态、五快照及共享池子额度。状态库纯编译完成，最终随共享布局源67；本项未改布局。整体产品构建遇当前Host桥BinaryEntities不完整类型错误，未宣称DLL通过；正式Client接线仍待第3—7项／第13步，无测试，第15步唯一验收。

**2026-11-11 第12步第1项完成，版本仍66**：[接口、数据寿命与失败处理](12-client-contract.md)确定已有借用视图的真实寿命、后台唯一修改者、精确快照／候选／批次、主线程准备／完成／排空接口及具名失败分类，特别区分逻辑inflight撤销与实际native busy排空。[第12步](12-client-decode.md)同步具体顺序及现有数字视图事实；本项仅文档与静态审阅，无新增运行代码、生成、构建或测试。第2—7项运行实现待后续，第15步唯一验收。

**2026-11-11 第10步第三项完成，版本仍66**：[计划执行器](10-capture-executor.md)已按同源数字表执行整根、条件／规范零、嵌套／容器／共享投影与动态槽，核计数组、纯值／字节／访问预算并失败回卷。具体原生来源、转换／资源和Collector／worker仍待接线；缺绑定明确失败。产品纯编译／链接完成，无测试，第15步唯一验收。

**2026-11-11 第10步第二项完成，版本仍66**：[公共采集基础设施](10-capture-foundation.md)新增窗口证言／资格与数字计划绑定、预分配纯值根事务及BodyValueSource适配、无锁cold／latest ordinary／body／256控制交接。缺源显式失败，支持快槽不足返回ordinary joint需求；Release／Win32产品纯编译／链接成功，真实各域来源、完整遍历与游戏桥／worker仍待后续。未运行测试，第15步唯一验收。

**2026-11-11 第10步第一项完成，版本仍66**：[当前合同与来源缺口](10-contract-source-gaps.md)整理当前要求、Collector／Lua旧链与新纯值接口边界、读取资格、G01—G15缺口及28条显示合同来源；[第10步](10-host-capture.md)同步修正过期未注册说明和复制表／执行器区别。实际采集／缓存／纯字节交接仍待实施；本项只改文档，无生成、构建或测试，第15步唯一验收。

**2026-11-11 仅新增动态压缩，版本66**：[world/body独立zstd](9-world-body-zstd.md)新增双方可选协商、独立帧level3、压后不更小时回原文、动态收发／预算支持。64KiB解压保护与其他全部协议／增量／动画／音频及9Mbps额度保持；实际压缩率与性能留第15步。

**2026-11-11 第9步收尾完成，版本65**：[预算／容量／时间合同](9-budget-capacity.md)及[同源静态账本](layouts/09-wire-format/budget-plan.md)完成第9步最后工作。按用户新决定模组可用10Mbps的90%＝9Mbps，三端各375000B/s；明确初建／恢复本Peer借用、8／10秒截止与1MiB总额、候选／账本／事件／内存容量，并扣除完整消息的40B信封。第9步线格式与纯值协议完成；真实来源、调度、完整影子、原生应用及退役继续10—14步，第15步唯一验收。密集设计负载存在超预算项，静态核算不表示全部原版场景已达标。

**2026-11-11 第9步第五项完成，版本64**：[版本、接受与恢复合同](9-acceptance-recovery.md)完成完整Frame／成员表／控制消息、严格整数版本协商及纯值接受状态机，明确旧base补ACK、原始joint凭据、实际发送账本与部分应用失败后base=0重建。349布局／2806字段／85条件，公共头尺寸保持；第13步恢复规则同步修订。剩余预算核算和10—14步运行接线，第15步唯一验收；无测试。

**2026-11-11 第9步第四项完成，版本63**：[资源协议实现](9-resource-protocol.md)、[生成数字合同](layouts/09-wire-format/resource-protocol-plan.md)。八表／九个启用kind的本地typed payload、有限recipe／图形规则、资源消息、不可变目录与按目标闭包已接状态库。新增425—433，349布局／2806字段／85条件；309／366补齐ColorOffset上下文为212／213B，像素／blob仍禁用。Release／Win32纯编译完成，无测试；第9步完整Frame／成员表／控制与最终接受、10—14运行接线和15验收继续待后续。

**2026-11-11 第9步第三项完成，版本62**：[显示批次外层封装](9-presentation-envelope.md)、[同源数字合同](layouts/09-wire-format/presentation-envelope-plan.md)。414—417组／依赖／贡献／音频事件的prefix和整批编解码已接状态库；保留真实生产修订、完整主体身份、缓存epoch、typed贡献定位、窄依赖和连续事件水位。28条用途规则同源生成，仍340布局／2772字段／84条件，保留world/body两序列和原固定头字节。Release／Win32状态库纯编译完成，无测试。完整资源payload／图形允许集／Frame与最终接受入口、10—14运行接线及第15步验收仍未完成。

**2026-11-11 用户调整资源与显示要求**：[现行合同](local-media-presentation.md)：横幅／fortune传文字与必要样式，由Client本地生成；当前Client切房采用完整合法目标就绪后直接切换，不要求前后surface平滑动画；hallucination采用本端历史图像与同步参数本地合成。尽量不传图片／纹理／音频／PCM，资源默认只传本地引用与有限描述，通用blob不作为媒体兜底。原版必要内容、world/body与10Mbps／7.5Mbps等指标保持。最初仅修订设计；随后第9步第二项已按版本61迁移文字／本地历史与取消的切房依赖，并拒绝像素／blob正文。当前340布局／2772字段／84条件，同源生成及状态库纯编译完成；完整资源payload、运行接线与第15步验收仍待后续。下方旧像素／surface必发要求按本合同替代。

**2026-11-11 第9步各域正文实现，版本60**：[正文实现与边界](9-domain-body-codec.md)、[同源数字计划](layouts/09-wire-format/body-codec-plan.md)。各域正文编解码已实现，布局版本60。337种布局／2763字段／79条件由同源数字计划处理，保留u32 mask与12B记录头；容器、固定槽、动态Weapon、共享payload、条件及投影事实已接产品状态库。完整资源payload／用途允许集和整消息接受入口仍待第9步后续，10—14运行接线及第15步唯一验收尚未完成。

**2026-11-11 第9步公共合同，版本59**：[正式结构](9-public-wire-contract.md)。第9步第一项公共报文结构已定稿并同源生成，布局版本59。保留u32掩码／12B记录头；新增408—421及公共常量，更新304为8B、306为15B。world/body按Peer两序列、双分段共同提交、来源／目标分离、u32 GameFrame、结果／事件外层及10Mbps总上行／7.5Mbps持续预算已定。完整资源payload／允许集／接受规则和运行编解码仍待第9步后续，10—14接线及第15步验收未完成。

**2026-11-10 第8步正式源，版本58**：[实现与后续交接](implementation-8-presentation.md)。Presentation74＋Random24个新增布局登记310—407；地图复用300—303，D0／逐用途随机alpha／药丸／预览、各组HUD及屏幕／转场／音频固定结果同源生成，28条具名结果合同与141＋54行目录已登记。正式外层封装、正常来源缓存及原生adapter继续由9—14步完成，UNKNOWN／来源Gap保持；本步不运行测试。下方历史版本与进度保留原时点。

**2026-11-10 第8步显示结果批次与各端多角色HUD定稿**：[正式合同](presentation-result-batches.md)。用户确定“选 B；采用上述归属划分、明确贡献身份，并保持当前缺结果处理规则”：统一批次、按实际owner分组及依赖共同提交，保持world／body两序列；History／Stat等详细HUD默认跟随本端控制组，双角色语义与其他组公共多人HUD完整保留。Host原生两槽不作为全系统容量，各组合法来源仍须实施；缺必要结果不安装／不ACK。9—15步交接已同步，版本57未变，仅文档静态审阅，无构建或测试。

**2026-11-10 第8步来源同步职责复核（版本仍57）**：[资源／屏幕／转场和音频53项决定](presentation-source-synchronization-review.md)、[逐项机器表](presentation-source-synchronization-review.json)。必要当前结果／world事件／资源与Host生产、本地派生、禁止跨端内容分开；删除Nightmare路线／场景帧等与最终贡献的重复表达，删除音乐mask／混音树／ducking／全局pitch与每voice参数的重复表达。个人设备失败／抢占不变成world Stop，硬件游标非必需状态。字节账本修订5仍50视图且UNKNOWN范围不变，旧决定留历史，正式布局／运行接线未实施，无构建或测试。

**2026-11-10 第8步音频状态／事件来源补证（静态，版本仍57）**：[必要来源及未决](presentation-audio-sources.md)、[机器目录](presentation-audio-sources.json)。本地Wiki→官方SDK／源码→J273补证，落实实际sample／非随机leaf候选、并存voice更新／停止、环境声及sound460后续pan、两组音乐／六层与jingle有效分支。修订4共50类型／视图，纠正流volume／pitch／计数语义；MusicStream24→4B、MusicEntry72→2B、SoundSample16→4B、SoundEffect30→2B、SoundEffects4→0B直属未知，旧事实保存。Random另记BD0F50改号流，修订3共25视图。全部音乐逻辑包络writer、异步结束、seek／重连相位、视频音轨及其余孔仍未知；正式布局／运行接线未实施，无构建或测试。

**2026-11-10 第8步资源／屏幕／转场必要来源补齐（静态，版本仍57）**：[来源证据](presentation-resource-screen-sources.md)、[机器目录](presentation-resource-screen-sources.json)。按本地Wiki→官方SDK／源码→缺项静态逆向的顺序，补齐实际图像／RGBA8内容、内置shader固定格式、主要屏幕pass、前后房间surface、versus逐层临时变换、Nightmare条件队列／原版程序道具及BossOverlay来源。模式4核为原版Deathmatch并排除，Pause只拆出Online密码显示1B；账本修订3共49类型／视图，Nightmare直属UNKNOWN61→37B，旧未知及其他孔保留。正式Presentation布局、Hook／缓存、图形物化／adapter未实施；无构建或测试。

布局文档入口：[按总任务步骤归类的生成布局索引](layouts/README.md)。`layouts/02-base-types` 至 `08-run-presentation` 对应下方第 2—8 步，步骤内按布局编号排列；变体和附属类型随所属域，共享类型只保留一份。目录整理不改变布局版本、字节归属、未知项或各步完成状态。

**2026-11-10 第7步影子描述生成层完成，版本仍57**：[实现与边界](room-level-description-implementation.md)。`ClientDescribeRoomLevel`生成Room／Level／Descriptor／独立双配置的自有不可变描述；52个数字字段角色及L2帧界同源生成，完整显示投影保留，同world结构／占用／八门依赖核定一次。旧场景／描述／背景配置输入明确改名Legacy，未构造新旧兼容桥。描述库及产品Release／Win32纯编译／链接通过，无测试；UNKNOWN及Host来源／GPU／第12解码调度／第13原生安装保持未完成。

**2026-11-10 第7步生产布局源与生成完成，版本57**：[实现与边界](implementation-7-room-level.md)。正式`room.json`／`level.json`新增239—309共71个布局；Room13个world块，L0=22B／L1=53B／L2=1—12B／L3=1014B／D1=1B，15B配置与背景／FX／水光波纹／焦点及必要地图结果已登记。四域597行回映射、四份C++表和全部布局表同源生成；资源字段、复杂自有结果列表和u8计数进入schema。Room直属28B／静态内嵌递归613B、Level22B及外部UNKNOWN保持。Release／Win32状态库纯编译通过；正式生产缓存、GPU ABI、完整协议和reader／writer／adapter仍待后续，未运行测试。下列版本56及更早记录保留各自时点。

**2026-11-10 第7步必要显示／资源设计补齐**：[设计与来源](room-display-resource-design.md)、[机器表](room-display-resource-design.json)。15B配置显示投影与不可变typed资源替代完整配置／生成图；程序背景提供完整有序recipe或精确内容，RGBA8／SourceQuad坐标空间和图形状态保护明确。水光波纹／焦点及纯地图adapter、完整可显示事实／本地独立查看、资源依赖与预算已定；Home读StageId35而非Type35，现行说明已更正并保留原错误。生产布局／source／GPU ABI／writer待实施，版本56，未构建或测试。

**2026-11-10 第7步RoomSave／Exploration目录核对完成**：[职责与证据](room-save-exploration-synchronization-review.md)、[RoomSave98行](room-save-member-map.md)及[Exploration99行](exploration-member-map.md)。完整保存体／备份与揭图生产标志留Host；L0/L1探索事实、L2保存图标／楼梯／Portal、实际回访Entity/Grid及必要地图资源／最终震动承接。地图查看／长按留本地，最多7项有序道具／诅咒徽标另由Presentation的MapBadgeList草稿1—8B承接。补记Getter实际类型、Grid Clear不改变长度、ESSM原字段与公开语义不同及回调阶段；保存体74B等UNKNOWN保持。正式来源／编码／adapter仍待后续，版本56，无构建或测试。

**2026-11-10 第7步与Random交接静态核对完成**：[15项同步决定／原生补证](room-random-handoff.md)及[机器表](room-random-handoff.json)。生成流与全部私有游标留Host；实际背景／FX、地图选择上下文、音频sample／voice和逐用途随机alpha承接。D0由37／13B草稿改5B，仅Manager相位／paused；新核BD0F40选样本流不上线，挑战32含SafeGridIndex锚点／窗口而非孤立标记，Error沿用最新1B颜色谓词。Random账本修订2共24视图，54行目录保留。正式来源缓存／资源／adapter待9/10/12/13步，生产版本56，无构建或测试。

**2026-11-10 第7步RoomDescriptor专门核对完成**：[职责／证据](room-descriptor-synchronization-review.md)、[6类型／视图账本](room-descriptor-byte-ledger.md)及[44行目录](room-descriptor-member-map.md)。B8h原24B未知核为16B商店生成类别＋8B补货次数，均留Host；直属未知24→0，保存体／配置及SDK语义边界保持。纠正上轮两项遗漏：HasWater是Pit Light／重绘的直接输入，L1同53B改五位投影；当前Boss匹配普通TrapDoor也用，所有模式保留。当前Error颜色谓词D1仅1B，L2只取最终图标mask及必要帧，存在3..12B，楼梯/Portal为位，不发seed／全保存体。Level设计／目录已回写修订2；正式资源表／reader／draw未接，生产版本56，无构建或测试。以下旧24B／四位／清零条目保留其时点。

**2026-11-10 第7步Level静态字节核对完成**：[职责与J273证据](level-synchronization-review.md)、[7类型／视图账本](level-byte-ledger.md)、[131行回映射](level-member-map.md)。Game楼层前缀核到18910，SDK大小1890C单列；官方简化Level两个字段落入维表，真实flags／Greed连续无红心伤害波次为1839C／18374，错误API事实保留。只同步身份／有效诅咒／拓扑、双配置及地图／HUD／Door所需投影，生成／返回／全flags／保存历史留Host；保存图标及楼梯结果不能丢。根22B／描述24B未知保持，L0/L1/L3设计22／53每描述／1014B；地图批次／配置资源表及生产接线未完成。未改生产版本56，未构建或测试。以下记录保留各自时点。

**2026-11-10 第7步Room续核（修订2）**：[1290B与嵌套补证](room-residual-evidence.md)、[职责](room-synchronization-review.md)、[32类型／视图账本](room-byte-ledger.md)及[269行回映射](room-member-map.md)。Room7898h现91个直接范围，直属UNKNOWN1290→28B；EntityList884→3B、FXLayers39→3B、FX31→5B，其他嵌套未知另计。纠正456-DWORD为两张各448项的DWORD path／WORD占格表，真实slowdown为+722C。函数指针、生成队列／历史、碰撞／查询缓存和原版Online环不进入C/S；实际环境voice及+139反向水面方向必须承接。首轮26视图／85范围作为历史基线保留；纹理／最终结果来源、正式布局／接线仍待实施，生产版本56，无构建或测试。

**2026-11-10 范围调整：当前先跑通原版。** 第三方 Mod 适配已迁至目录外的[Mod 适配后续方案](../mod-adaptation.md)，包含自定义状态／角色／资源／绘制、StageAPI、Lua adapter、自描述编码及原 E31。它们不属于本次 0—15 步的实现和验收条件；当前正式同步不保留 Mod 域的 Lua 读取例外。官方 REPENTOGON 加载器、J273 目标和原版所需的官方扩展 owner 保持，原版全部必要内容、四参与者／多身体及性能目标保持；历史 Mod 事实和未决问题在后续方案与原记录中保留。

**2026-11-10 第8步Random静态核对完成**：[职责／J273证据](random-synchronization-review.md)、[23类型／字段视图字节账本](random-byte-ledger.md)及[54行成员回映射](random-member-map.md)。玩法流／池／运行库引擎留Host，药丸／预览等实际显示结果另承接；补核ItemPool+664饰品流、保存体+218实际vector<bool>和Game+182E4恶魔／天使流。两份绘制流仅发seed，固定shift与guard不上线，D0草稿37→13B；完整取样相位／消费序列与最终结果来源尚待接线。UNKNOWN和ABI模型边界保持，未改生产版本56，未构建或测试。以下记录保留各自时点。

**2026-11-10 第6步剩余描述编码完成**：[影子状态描述实现](grid-description-implementation.md)提供正式`ClientGridDescription`、17类同源数字索引与自有不可变投影，移出字符串字段；旧JSON链明确命名为`LegacyClientGrid*`等待后续换接。线布局未改，UNKNOWN与第10／12／13步运行边界保持，不运行测试。

**2026-11-10 第8步Presentation静态字节核对**：[同步职责与证据](presentation-synchronization-review.md)、[45类型／连续视图账本](presentation-byte-ledger.md)及[141行公开成员回映射](presentation-member-map.md)。最终心格／实际图像／动态HUD／History顺序和StatHUD三值保留，previous／threshold及生产器不上线；官方星象房行保留。地图本地查看与world事实分开，持续声音须状态、一次性声音走事件，Host音量／后端对象不复制。程序图／shader旁表ABI／Pause后缀／转场条件payload等UNKNOWN及具体来源缺口保持；正式Presentation布局／运行接线未实施，本轮未改生产布局版本、未构建或测试。以下记录保留各自时点。

**2026-11-10 第8步Run静态归属，版本56**：[Run职责核对](run-synchronization-review.md)及`run.json`新增230—238，回映射原目录全部191条run行。只保留具体显示消费者所需当前值、7项种子显示投影、ascent bit48及最终分数；生成／波次／历史／回溯与持久资格留Host。两黑暗浮点保留，ColorModifier只发current。GameFrame与TimeCounter、Game当前daily与Manager预告来源分别核定。Presentation实际owner／跨域委派与UNKNOWN明确保留，新handler与正式接线仍待后续；纯编译结果见Run核对，无测试。以下条目保留原时点。

**2026-11-10 第6步119行公开成员回映射完成，版本55**：[说明与证据](grid-member-map.md)、[逐行汇总表](layouts/06-grid/grid-catalog-members.md)。119个原始行全部进入正式布局源及代码生成，保留三个重复ID／原签名；16行字段、28行派生、74行Host权威、1行不适用。补清GetBigRockFrame无符号比较，ReducedSpawnRate明确已移除；各根UNKNOWN及线字段保持。第6步静态布局、Plus来源及成员目录工作均已登记；实际reader／writer与绘制adapter仍待后续。Release／Win32状态库纯编译成功，无测试。以下条目保留原时点。

**2026-11-10 第6步GridEntityPlus静态来源完成，版本54**：[来源／ABI／生命周期与同步决定](grid-plus-water-source-evidence.md)核定官方448×12B共享旁表，新增229本地ABI目录、零线字段。只发每实例最终u32，optional／地址／重置历史不上线；Factory／Init重置共享项，Free／换房不假定自动清。纠正Lock不进入官方六Hook、忽略旁表；现有C/S六wrapper也漏Lock，第13步补第七点。各根UNKNOWN／最小状态不变；全119项逐行回映射及正式reader／writer尚未完成，无测试。以下条目保留原时点。

**2026-11-10 第6步PressurePlate／Door完成，版本53**：[字节归属与同步职责](grid-pressure-door-evidence.md)追加224—228。TimerPlate／ExtraSprite、Door目标类型／描述索引／ExtraVisible及Light bit0x20保留；previous缓存、四动画名、NextGreedAnimation、GreedModeRNG与门玩法控制值留Host。PressurePlate主色派生；Door主色仅发alpha入口布尔，其余取同world墙色；两类专用描述不发无实际Light消费者的SpawnSeed。根UNKNOWN31／32B，最小212／223B。Release状态库纯编译通过，无测试。至此全部17派生均有字节／线格式生成表，完整目录、Plus合法来源及正式运行接线仍待完成。以下条目保留原时点。

**2026-11-10 第6步指定七类完成，版本52**：[字节归属与同步职责](grid-seven-subclasses-evidence.md)将Lock／Pit／Spikes／Statue／TNT／Poop／Rock接入217—223，214—216为专用视图。TNT+154爆炸计数、其余控制计时、Poop尾部40B伤害EntityRef及Statue实体裸指针留Host；Statue自身无Sprite负载。Rock保留碎块动画资源／有序偏移，state2主动画残留和layer0旧位置不读不发，真实overlay／原色保留。根UNKNOWN依次20／20／17／17／17／19／17B；Release状态库纯编译通过，无测试。PressurePlate／Door、全119项及正式运行接线仍待完成；以下条目保留原时点。

**2026-11-10 第6步指定八类完成，版本51**：[字节归属与同步职责](grid-eight-subclasses-evidence.md)将Decoration／Gravity／Stairs／Teleporter／Wall／Web／TrapDoor／Fire及四种专用视图接入202—213布局。五类主Sprite省去必覆写的44B颜色，TrapDoor计数及Fire计时／样式／粒子速度留Host；Fire两Sprite与16B有序粒子保留，第三Sprite主帧／选择器派生、播放插值开关保留。根UNKNOWN七类17B／Fire45B，最小98／142／372B。Release／Win32状态库纯编译通过，没有测试；其余九类、全目录、正式运行接线仍待完成。以下条目保留原时点。

**2026-11-10 第6步公共同步职责复核，版本50**：[具体消费者与纠错](grid-common-synchronization-review.md)撤销CollisionClass原值，改发按实例最终WaterClipFlags；主Sprite使用GridLayerState200／GridSprite201，移出默认动画名／待加载图层路径，并补Dataminer地址派生的最终两位纹理选择。G0仍24B、G1固定94B、公共最小状态142B，根UNKNOWN仍17B，资源／姿态与原证据等级保留。Release／Win32状态库纯编译通过；正式来源／writer／绘制adapter及子类仍待后续，无测试。以下条目保留原时点。 冠军shader只保留前四float16B，载体后七值28B留Host，普通ColorMod仍完整11值。

**2026-11-10 第6步公共字节归属，版本48**：[证据与职责](grid-common-evidence.md)、[公共生成表](layouts/06-grid/GridEntity.md)及[描述生成表](layouts/06-grid/GridEntityDesc.md)。新增Desc198／Grid199，公共340B逐字节登记，递归UNKNOWN17B（Desc3B＋ANM2 14B）；G0仅24B，公共最小状态171B。SpawnSeed因Light临时绘制RNG保留，SpawnCount／VarData／Initialized／VariableSeed／SpawnFrame与对象_rng留Host。Grid为根＋17个派生、共18个RTTI；本轮只处理公共前缀，子类、GridEntityPlus水面旁表／全119项回映射和实际接线仍待完成。0—14步无测试，第15步唯一统一验收。以下记录保留原时点。

**2026-11-09 第5步正式布局源与生成完成，版本47**：[实现说明](implementation-5-player.md)与[EntityPlayer生成表](layouts/05-player/EntityPlayer.md)。唯一源`mod/binding/state/player.json`新增150—197共48个布局，Player197共25块；条件效果、动态Weapon、固定槽、Plus投影及1265条应用行／97条补充成员已正式生成。Player／Plus未知仍3707B／9B，Plus ABI模型不升级证据。Release／Win32产品纯编译与链接通过，没有运行测试；第8—14步实际接线及第15步统一验收仍未完成。以下较早记录保留原时点。

**2026-11-09 第5步前两项设计工作完成（未生成／未实现）**：[统一原生字节账本](player-byte-ledger.md)覆盖Player 2EA0h、保存／移动箱／动态装备及PlayerPlus等58种类型；根对象239个直接范围，递归UNKNOWN仍3707B。Plus A8h及后半偏移明确为静态ABI模型，未知9B，不能称官方DLL实测。[成员逐行回映射](player-member-map.md)保留1265条旧应用行及重载，另列14条native、77条具名宏与6条结构，旧Entity目录逐行交叉引用。原版通信+268C及待移除效果队列不上线；无调用方的Captured通用字典退出设计，真实原生显示查询走具名最终决定，具体缓存／批次布局仍归第8/9步。剩余为唯一Player布局源／正式生成与采集、安装、退役接线；版本仍46，没有构建或测试。以下较早记录保留各自时点。

**2026-11-09 第5步P6—P8设计完成（未实现，版本仍46）**：[设计与J273证据](5-player-p6-p8.md)。P6为111＋4C B最终costume目录／map；P7为21B身体结构＋C/S组记录，独立激活资格保留，native sub／replaced／703四槽留Host；P8裁非704 cooldown、原始History／16索引／forms与未选回放节点，保留实际效果、残影、当前可绘记录／额外Sprite。最终图像列表及Render内分数重算的缓存交第8步，不能放行生产器补算。P0—P8九块设计完成，全Player字节账本、PlayerPlus、回映射／生成与运行接线仍待完成；没有构建或测试，第15步唯一统一验收。

**2026-11-09 第5步P3—P5设计完成（未实现，版本仍46）**：[设计、原生消费者与字节归属](5-player-p3-p5.md)。P3保留展示库存／四口袋／材料／法术与blocked＋有效innate投影，voided／管理时限等留Host；32根有界参数核对将三组特殊输入留Host，跨域计算查询最终显示来源未决照实列出；P4固定96B并规范空槽；P5为1—91B，按15种真实动态类裁五种恒零输入与非展示Modifiers位，保留Whip主引用。P0—P5设计已完成，P6—P8、全Player字节闭合、生成及运行接线仍待完成；没有构建或测试，第15步统一验收。

**2026-11-09 第5步P0—P2设计完成（未实现，布局版本仍46）**：[原生消费者、同步必要性与草稿布局](5-player-p0-p2.md)。P0核定真实闪隐、比例／headAnim及五项Render充能，按投影拆分Sprite并增加P6拓扑依赖；P1为1B CanFly，六项属性栏结果归Presentation；P2为77B，保留特殊健康显示原值及Jar／额外生命等直接输入，EternalHearts／BoneHeartsMask原值由最终逐心缓存承接。P3—P8、全Player字节表、布局生成及运行接线仍待完成。只改设计文档，没有构建或测试，第15步仍是唯一统一验收。

**2026-11-09 当前版本46：Delirium未决事项。** [静态证据与归属修订](delirium-residual-evidence.md)核定两个8B Host打包存储，7B移出独立未知；BD4—BD7的4B保持未知，Delirium根597→590B，NPC仍586B，最小线状态仍均710B且17／18空预留。池槽残留有静态可达路径但未实测；补齐4A13B0独立companion的创建／BE8登记，当前展示依赖闭合为真实成员和已有线上关系，BE8继续留Host。正式运行接线仍归第10／13／14步，测试只在第15步；旧版本条目保留原时点事实。

**2026-11-09 版本45：Projectile展示来源与Effect残余。** [J273静态证据与逐臂结论](projectile-effect-residual-evidence.md)闭合Projectile+440开关、+460/+464冻结绘制锚点的构造／Init／生产链，新增9B独立PR2，PR0保留4B Height、最小状态404B、根未知350B。Effect新增148号167臂与149号19/71/194内部flags臂，167的+430核为Host生成伤害快照；+434及后8B仍未知。119尾部确认一个callback的8B DamageFlags特例，通用臂仍保持未知。41C／44C／4BC没有语义或填充证明；此前五项不重做。普通Release／Win32产品纯编译与链接通过，未运行测试；正式接线仍按第10／13步。

**2026-11-09 版本44：Text／Dummy。** [字段职责与J273证据](text-dummy-evidence.md)及[实现说明](implementation-4-text-dummy.md)将Text145／variant2缩放146／Dummy147接入布局生成链。Text保留有效原始字节前缀与最终RGBA，counter仅146传输；duration与此固定字体不消费的颜色空间标签留Host，通用KColor20B保持。新增有界c-string读取／暂存和无尾部等大派生支持。Text339B／Dummy335B根未知、Dummy不可见标签历史缓存及自然来源缺口保持；普通Release／Win32产品纯编译与链接通过；第10／13步仍须接入实际窗口／owner，第15步前不运行测试。

**2026-11-09 版本43：NPC受控UI来源未决已闭合。** [静态证据与纠错](npc-control-ui-evidence.md)核定8E7550为原版NetplayManager::IsIdxLocalPlayer，撤销上轮“本地设备可用性”的误解。C/S只以NPC+F1C的-1哨兵生成0/2受控表现，不读取原版联机对象；新增ReadNpcControlledUi／DecodeNpcControlledUi，拒绝旧模式1。布局长度、根未知586／597B及其他未决保持，正式窗口／绘制接线仍按第10／13步；第15步之前不运行测试。

**2026-11-09 版本42：NPC同步职责修正。** [逐段复核](npc-synchronization-review.md)取消Delirium BossType／Variant的8B传输与无根据的shape换代；ControllerId本地设备键改为1B受控UI表现，禁止跨进程raw写入；OverlayEffect新增144号场景视图，8B Scale由当前主Sprite null-frame派生。五项原生绘制标量、真实引用、三deque与必要双Sprite保持。NPC／Delirium最小负载均710B，未知586／597B及Pathfinder SDK声明不变；实际控制UI来源／adapter仍待第10／13步，不运行测试。

**2026-11-09 版本41：Entity_NPC。** [实现说明](implementation-4-npc.md)与[静态证据](npc-evidence.md)将NPC142／Delirium143、双辅助Sprite、三组deque及44个唯一公开成员回映射接入生成链。保留真实绘制参数与8B变身shape，AI计数留Host；新增无分配deque读取接口。NPC本类223B未知、根586／597B未知与Pathfinder声明边界保留。普通Release纯编译／链接通过，没有运行测试；实际采集／安装仍待第10／13步。

**2026-11-09 版本40：Effect后续五项未决。** [静态证据与落地](effect-next-five-evidence.md)补入137—141号VarData具名臂，已核范围留Host／本地、其余UNKNOWN保留；Rotation／Scale移入FX0，Trail逻辑节点上限收紧为127。核到101可双减Timeout及141刷新常数，修正静止必不重发的验收表述；原生ExtraData仍为三种具体payload。正式reader／writer待第10／13步，测试只在第15步。

**2026-11-09 版本39：Player装备与Plus展示依赖。** [完整消费者与安装复核](player-equipment-plus-synchronization-review.md)将ActiveItemDesc来源池／定时充能冷却、Weapon五项非展示输入及KnifePlus.hitboxSource留Host；保留主动槽六值、Weapon充能三值与类型、TemporaryEffect三值及Plus公共nullable覆盖。新增135号UmbilicalWhip真实主引用、136号TemporaryEffects外层disabled／列表，并补Familiar+D76绘制抑制字节。原已排除Plus项不恢复，其他UNKNOWN保持。源、生成表、正式Plus读取接口和第5／8／10—15步已修订，未构建或测试，运行时仍待接线。

**2026-11-09 版本38（Effect合同继续适用）：Effect各臂与附属payload。** [Effect同步职责复核](effect-synchronization-review.md)撤销六项公共更新输入、TearFlags全128位与136描述八成员；State／Timeout／LifeSpan／IsFollowing／ParentOffset／Rotation／Scale及三种表现payload保留。Ghost只发0—4有效Sprite前缀，Offset／Scale派生、i32 count由投影计数重建；MrMe原16B未知核为Host内部状态，Trail找到64B分配与完整复制。六臂编号不变，FX2均预留；公共Effect最小444→420B。源、生成表和第6／10／12／13／14／15步合同已修订，未构建或测试，运行时仍待接线。

日期：2026-11-08；更新：2026-11-09（Asia/Singapore）。状态：**第 0—3 步已落地，第 4 步的 Familiar／Knife／Bomb、Tear／Pickup／Laser 和 Slot／Projectile／Effect 布局与专用表现已接入生成链，普通 Release 纯编译／链接通过**。入口为[第 0—2 步实现](implementation-0-2.md)、[第 3 步实现](implementation-3.md)、[Familiar／Knife／Bomb 实现](implementation-4-familiar-knife-bomb.md)、[Tear／Pickup／Laser 实现](implementation-4-tear-pickup-laser.md)、[Slot／Projectile／Effect 实现](implementation-4-slot-projectile-effect.md)和[生成布局表](layouts/README.md)。当前布局版本47（Player正式源见[第5步实现](implementation-5-player.md)；Delirium未决见[本轮处理](delirium-residual-evidence.md)；Projectile／Effect残余见[新证据](projectile-effect-residual-evidence.md)；Text／Dummy见[字段证据](text-dummy-evidence.md)；NPC受控UI来源见[闭合与纠错](npc-control-ui-evidence.md)，旧版本事实保留，未运行测试）；[第4步原未决前四项](implementation-4-first-four.md)已完成工作量估算与DepthOffset复核，补入Knife尾部六个f32、更正Slot／Projectile本地指针和Effect生成参数归属，续研核定Knife六值的分裂／碰撞用途与Effect参数的Host归属，六段空白的填充假设及证据边界仍保留。SDK 声明、未知字节、子类计时分支和跨域未决照实保留，不表示类型完整或同步已可运行；第4步NPC／Delirium已迁入布局源，仍有未知与后续接线；Text／Dummy布局及字段职责已接入版本44（见[text-dummy-evidence.md](text-dummy-evidence.md)），未知与Dummy历史屏幕缓存依赖保持；第5步正式布局源／生成已完成，第6—14步及Player实际运行接线仍待实施。0—14 步没有测试，第 15 步是唯一统一整体验收。

**2026-11-09 版本37（历史范围）：Laser／Homing采样、运动与计时。** [Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)保留optimized／unoptimized两组最终点列与当前角度、长度、完整效果位、原始采样模式、MaxDistance／GridHit、公开EndPoint和+4F0展示分支输入；19项Laser更新输入、六项Homing标量、indices中间点缓存及Bezier／系数留Host。indices原生类型纠正为8B双f32点对，旧i32步长记录错误。L2／L4／L7／L8及Homing子块0／3／4预留，父投影仅1／2；撤销两个Laser计时锚点。最小Laser542→439B，未知421B不变。源、生成器和第10／12／13／14／15步合同已修订，未构建或测试；运行时仍待接线。

本目录保存这次重构的总计划和逐步设计。按用户 2026-11-08 的要求，步骤**直接用数字编号，从 0 开始**，不再使用 `tx-0x-0x`；文件名为 `<编号>-<主题>.md`。旧 tx 文档中的事实和失败记录保持原样；与本目录冲突的设计以本目录为准，具体范围见第 6 节。

**2026-11-09 版本28同步职责修订（历史范围）。** [字段同步职责复核](synchronization-field-review.md)把 Knife K4 六值／命中父链选择字节、Knife／Bomb／Tear／Laser 四组命中历史、Projectile 伤害／三项延迟变更参数、Laser 掉落概率／伤害倍率共 17 个范围改归 Host 权威，并同步撤销线上字段、校验及公开成员的状态回映射。旧空块号保留。Familiar 充能／状态／硬币等已有真实展示消费者，继续同步；variant80 经 Weapon 查询充能的展示缺口已记录，未虚构已实现。其余候选仅在依赖闭合后调整，UNKNOWN 保持。没有测试，唯一功能整体验收仍是第 15 步。

**2026-11-09 版本29：24项内部关系展示复核（历史范围）。** [逐槽结论及证据](internal-entity-relations-review.md)：Knife4／Tear6／Pickup1／Laser12共23项留Host；Familiar+DAC仅variant240的绳索分段有展示依赖，新增布局130保留真实引用，其余Familiar臂不发送。Knife K3显式预留，原有块号不前移。240的UmbilicalBabyData绳索payload仍待补齐，不能声称展示完整。Room Light已证实读取Laser未优化采样点，继续保留该容器。布局源、回映射和第13／14／15步同步修订；没有测试。

**2026-11-09 版本30：Entity公共同步职责复核（历史范围，valid由版本31纠正）。** [逐项结论与静态依据](entity-common-synchronization-review.md)：33个公共范围撤销传输，其中32个为Host玩法状态，removedByFactory为本地工厂生命周期。包含毒／火伤害相位、20个公共计时器（含PauseTimer）、暂停保存／半帧历史位置及部分伤害／物理输入；每个Entity最小负载减少223B。valid／exists／dead有实际绘制消费者，继续保留；碰撞类别、尺寸及Effect使用的速度倍率同样保留。布局源、生成表、回映射及第5／10／13／14／15步同步修订，不改变UNKNOWN或历史证据等级，没有测试。

**2026-11-09 版本31：valid／exists／dead再复核（历史范围）。** [结论和证据](entity-lifecycle-synchronization-review.md)：撤回“Render读取valid就必须公共同步”的判断。valid移出Entity E2，由Client本地构造／发布／退休owner维护；Player活动／备用身体的结构资格仍须由第5步P7接管，现有证据不能证明valid与room_member等价，也不能删除旧Player结构源的独立资格信息。exists／dead继续同步：存在性与激活／成员分离已有历史反例，死亡状态有实际展示消费者，未证明可由HP等价替代。公共最小状态进一步由411B降为410B，UNKNOWN不变；没有测试。

**2026-11-09 版本32：动画／资源／颜色职责复核（历史范围）。** [动画／资源／颜色职责复核](animation-resource-synchronization-review.md)撤销两组AnimationState事件位、两个loaded原始布尔及完整ColorParams队列的独立传输；资源有无并入file_name可空资源号，路径与实际Image仍分开。官方InterpolV2绘制读取is_playing／anim_frame／playback_speed，继续保留；最终颜色保留全部11个ColorMod分量。Entity E7及原事件／颜色块保留空块号，公共最小状态410B→391B。唯一布局源、生成表及第2／3／8／9／10／13／14／15步同步修订，资源执行器和未核owner仍待接线，没有构建或测试。

**2026-11-09 版本33：Familiar参数、Incubus充能与Gello绳索。** [Familiar参数／充能／绳索复核](familiar-synchronization-review.md)将11个轨道／计数／方向范围共48B归Host；FireCooldown／State／Coins等真实展示输入保留。新增variant80布局131的最终充能amount（不重建玩法Weapon），variant240布局130补入133号UmbilicalBabyData及132号XYZ节点，四投影同版本提交。基础Familiar最小661B→613B，80为617B，240缺失payload时623B。新payload仍有58B未知、每外部节点另108B未知；具名采集／展示／owner尚待第10／13步，不表示可玩或验收通过。本轮没有构建或测试。

**2026-11-09 版本34：Knife／Bomb／Tear攻击、运动与计时。** [Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)将42个范围归Host，撤销Charge／MaxDistance、爆炸计时／伤害／范围、BaseDamage／KnockbackMultiplier／WaitFrames等输入及三个计时锚点。Knife保留拖影距离／速度、旋转偏移、飞行状态和效果位，Bomb保留最终五份costume及绘制开关并清pending，Tear保留效果位／DeadEye强度／三份辅助Sprite。最小状态分别450→422B、1181→1127B、953→854B；UNKNOWN不变。唯一源、生成表与第10／12／13／14／15步已修订；运行时仍待接线，没有构建或测试。

**2026-11-09 版本35：Pickup／Slot选择、交互与奖励。** [Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)将Pickup12／Slot8个原生范围留Host，撤销19项原线上字段／容器：选择组、触碰、自动价格、商店标识、机器状态／捐款／触发／奖品输入及未来循环列表等；Pickup Timeout／Wait两个锚点撤销。Price有WaterClip消费者、DropDelay直接控制Render，继续同步；Slot保留主／奖品Sprite，精确回映射其动画指针与is_playing。最小Pickup744→703B、Slot564→538B，另省循环列表元素；未知不变。源、生成表和第10／12／13／14／15步已联动，未构建或测试，运行时仍待接线。

**2026-11-09 版本36：Projectile（历史范围）运动与展示参数。** [Projectile运动与展示参数复核](projectile-synchronization-review.md)将追踪／曲线／加速／下落速度及加速度／摆动六项输入与Scale、ProjectileFlags共八项留Host，最小状态431→395B，PR1预留。Height因Render临时偏移有真实消费者保留；公共EntityFlags、最终位置／绘制偏移、尺寸和Sprite保持，DepthOffset仍只继承一次。私有+440及+460／+464的展示来源仍UNKNOWN，375B未知不变，不能称画面完整。源、生成表及第10／12／13／14／15步已修订，未构建或测试，运行时仍待接线。

## 1. 目标与用户决定

用户 2026-11-08 确定的方向：

1. **原生采集**：Host 在 C++ 侧直接读取游戏对象内存，正式同步路径不再调用 Lua getter，也不再构建 JSON 树。原文“不采用 isaac-lan 那种‘白名单字段＋兼容补丁’的适配方式”已由 2026-11-12 的决定修订：字段改为按需选择，见本节末尾。
2. **二进制状态**：每个类型的状态按固定布局连续排布：头部是类型标识，后面依次是父类字段和子类字段。字段在原生内存中不必连续，Host 构建状态时把采集到的字段连续排好，Client 按同一映射规则还原。
3. **合并读取与比较**：原生内存中相邻的已知字段合并成一次复制。Host 先把字段 memcpy 到自有缓冲区，再用 memcmp 判断变化；Client 按对应规则解码、校验并写入。
4. **完整性按字节归属**：每个类型从偏移 0 到对象大小的每个字节都必须有归属，见第 3 节。2026-11-12 起，完成条件改为需求清单，字节归属只作参照，见本节末尾。
5. **Baseline 统一为二进制**：Baseline 与增量使用同一格式，Baseline 就是“已确认版本为 0 的状态帧”。
6. **布局用单行表格画出**：每个类型都要画出状态二进制布局，每一格写清类型和语义，见第 5 节。
7. **新目录与数字编号**：即本目录。

2026-11-10 补充决定：本轮先完成原版内容的完整同步闭环；第三方 Mod 适配独立保存，后续另行推进。范围调整不表示游戏已经跑通，本次只修订方案。

2026-11-11 补充决定：[资源本地生成与当前显示范围](local-media-presentation.md)采用文字／有限样式与本地资源引用，Client本地排版、直接切房、本端hallucination合成和本地音频播放。尽量不传原始媒体，当前不启用通用像素／音频blob。仅放宽当前Client切房平滑与反馈像素一致性要求，不放宽完整世界事务、原版必要内容、其他动态显示或下列性能指标；原生来源／UNKNOWN及既有版本60实现事实保持。

性能和网络目标沿用 [tx-04-21 E2E 方案第 3.1 节](../tx-04-21/tx-04-21-e2e-plan.md)，本次不放宽：

- **Host 帧率**：不低于 50 FPS，即 20 ms 整帧。
- **Host 采集**：主线程每帧 p95 ≤2 ms、p99 ≤4 ms。
- **发布频率**：普通动态约 30 Hz，合格身体约 60 Hz。
- **带宽**：Host 三端总上行 10 Mbps，模组持续 7.5 Mbps。
- **首次准备**：不超过 1 MiB，从发起到 Client 画面就绪不超过 10 s。
- **Client 安装**：动态安装 p95 ≤2 ms、p99 ≤4 ms。

2026-11-12 方向调整，用户确认，见[修改方案](client-display-delegation.md)：

1. **按需采集**：只采集 Client 需要的字段，即满足下列任一条件的字段：
   - 原生显示要读、Client 又算不出；
   - 原生对象结构需要；
   - 玩家可见的规则结果；
   - 事件。

   正式路径仍是 C++ 复制计划和二进制格式，不改回 Lua 白名单。
2. **完整性改为需求清单（Q1）**：
   - 每个类型列出 Client 需要的字段及其消费方证据。
   - 字节归属账本保留，作为定位和语义参照，不再是完成条件。
   - UNKNOWN 不再阻塞，第 15 步发现差异时再补查。
3. **显示委托**：
   - Client 包装原生 `Game::Render`，由原生代码绘制。
   - `Game::Update` 继续拦截，改由 Client 的“显示推进”根调用已核实的显示子更新。
   - 显示推进根挂在 Manager 边界、world 安装之后（RE-1／RE-3）：本机 Manager 偶数帧运行约 30Hz 的显示批次，每个 Manager 帧运行相机阶段（本机关闭插值时跳过奇数帧，与原生一致）；本机 Manager 帧由显示推进 owner 推进。
   - Host 不再发送 HUD 生产、光照、地图、D0、随机 alpha 等显示结果。
4. **不可算的值与事件**：
   - 不可算的值同步后注入：优先原生 setter 或 REPENTOGON API，其次直接写内存，最后才挂 getter（Q3）。
   - 瞬时事件走有界串行事件队列。
5. **帧号方案B**：
   - 每装好一个 world 帧，就把 Client 的 `Game+0x25A70` 写成帧头的 game_frame。
   - 锚点按 Host 原值安装。
   - 取消每个绘制栈的帧号借用和 D0。
6. **外观随机差异（Q4）**：
   - 接受绘制随机、粒子等纯外观差异。
   - Host 选出、影响布局或规则的结果仍然同步。
7. **切房走原生过渡（Q5，同日追加）**：
   - Client 用 Host 已决定的参数回放原生转场。
   - 换房点改由 Client 构建 Host 的新房间目标；目标未就绪时，转场停在换房点。
   - 换层另行决定。
8. **放行回调分派（Q6，同日追加）**：
   - Client 放行路径上的 Lua 回调分派一律放行。
   - 前提是当前不含第三方 Mod。
9. **音效按来源去重（Q7，同日追加）**：
   - Client 放行的原生显示代码发出的音效在本地播放，不进事件队列。
   - Host 按显示作用域排除这些音效，只发玩法路径的音效。
   - 音乐请求、循环音效和全屏演出按状态同步，其余瞬时内容走事件队列。
10. **内联显示标量同步当前值（Q8，同日追加）**：
    - 原生 `Game::Update`／`Room::Update` 内联推进的显示标量（震屏、颜色、闪电、暗度、眩晕、fade、水色、冲击波等）同步 Host 当前值，Client 不本地推进。
    - 原生在阈值处对 Client 本地字段的清理由安装点补做。
    - 相机、ANM2 动画、FXLayers 和 HUD／Minimap 自身的动画仍在本地，见修改方案 4.6。
11. **同步当前房间描述的 SpawnSeed（Q9，同日 RE-4 审阅后追加）**：原生 Render 在房间年龄 30 时读它，为 0 即崩溃。只同步这一个 u32，按变化更新；其他 seed 和全局 RNG 游标仍留 Host，写入不授权 Client 运行生成路径。D1 谓词在布局收缩时评估退役。
12. **保留 Projectile 恢复作用域（Q10，同日 RE-5 审阅后追加）**：`client_entity_draw_projectile` 不随绘制 hook 删除，改为保存并精确恢复已同步的 PositionOffset `+0x350`。
13. **转场状态同步当前值（Q11，同日 RE-1 补遗与 RE-7—RE-9 审阅后追加）**：RoomTransition 的 mode、counter、duration、progress、direction、roomIndex、dimension、animation 和 Boss 标记同步 Host 当前值，随 world 原子安装；Client 不运行 Start／Update，也不写自有 FSM 驱动，只在本地捕获旧／新 surface、算滚动补偿、推进 versus 动画并调用原生 Render。取代 RE-9 的自有显示驱动建议。
14. **paused 位与终结分支枚举（Q12，同日追加）**：保留 `client_entity_draw_paused`，向 `Game::IsPaused` 的原生调用者返回 Host 同步的 paused 位；Host 每个 world 帧发送按原生优先级算出的终结分支枚举，Client 据此选显示批次序列；Pα 由已安装 primary alpha 判定；与本地菜单的合并由第 13 步决定。
    - 同次审阅新开 RE-10（Manager 尾部非 Game 调用分类，至少音乐／SFX tick 须在显示推进根运行），并定为：HUD 可见性 `Game+0x22E90` 按 Q8 同步、诅咒同步 raw 字段、SFX 回放 FrameDelay=0。

上面的性能和网络目标不变。2026-11-11“资源本地生成”和 2026-11-12“HUD本地化”两项决定，改由原生显示委托实现，见修改方案第 8、11 节。

## 2. 为什么重构

第 73 轮 `p2-E02.cold-56` 的实际记录（[双人 E2E 进度](../tx-04-21/tx-04-21-e2e-progress.md)）：

| 项目 | 实测 | 目标 |
| --- | --- | --- |
| Host 主线程采集（stage12），596 样本 | 平均 184 ms，最大 460 ms | p95 ≤2 ms |
| 帧间隔（stage13），596 样本 | 平均 195 ms，最大 1.16 s | ≤20 ms |
| 普通窗口 Entity 域读取 | 平均 113.5 ms | — |
| 后台构建（stage11），259 窗 | 平均 231 ms | 每窗 ≤33 ms 才跟得上 30 Hz |
| Client 应用 | 90 秒内 `world_applied=0` | — |

结构性原因（静态核对）：

- **逐项调用 getter**：每个成员都经 Lua getter 读取，再组织成嵌套 Lua 表。
- **多次复制整树**：同一份数据至少完整生成三次：
  - 主线程 `CopyLuaSource` 复制成纯值（`mod/src/game/world_sources/lua_entity.cpp:662`）；
  - 后台 `PartitionSource` 分层（`mod/src/replica/stream_capture.cpp:234`）；
  - 身份投影（`stream_capture.cpp:276,285`）。
- **先构建后比较**：内容比较放在全部构建之后（`stream_capture.cpp:176`）。场景静止时版本虽然不变，读取和构建成本一分不少。
- **Client 按字符串处理**：Client 要从 JSON 树还原目标，再按字符串成员名逐项匹配写入（`mod/src/game/client_entity_common_native.cpp:43-50`）。

这些问题来自数据形态本身，逐项优化 Lua 读取无法达到上面的预算。

## 3. 完整性：字节归属

> **2026-11-12 修订（Q1）**：完成条件改为需求清单，见[修改方案第 2.1 节](client-display-delegation.md#21-按需采集)。本节的归属类别、证据规则和已有账本继续用于定位字段和说明语义，但“未知字节数为 0 或已探测”不再是完成条件；未知字节默认不采集，也不阻塞完成。

### 3.1 归属类别

每个原生类型从 0 到对象大小的每个字节，都必须恰好归入下列一类。类别名也是布局表格中使用的标记。

| 类别 | 含义 | 是否进入状态 | 例子 |
| --- | --- | --- | --- |
| 状态 | 有语义的值，直接复制 | 是 | `Entity._pos`、`EntityEffect.Timeout` |
| 换算 | 需要 Host 换算后才有跨进程意义 | 是，发送换算结果 | `EntityEffect.Timeout` 换算为到期帧 = 全局帧 + Timeout（第 4 步 4.1.5） |
| 引用 | 指向另一个实体／格子的指针 | 是，转成成员序号 | `Entity._parent` 等 `EntityPtr`（`+0x3BC—0x3C8`） |
| 资源 | 指向资源或配置的指针 | 是，转成稳定标识 | `ANM2._animData` 转成动画序号；房间配置指针转成配置标识 |
| 容器 | `std::vector`、`std::string` 等容器头 | 是，发送元素数和元素 | `Entity._colorParams`（`+0x1AC`） |
| 本地 | 只在本进程有意义：虚表、缓存指针、锁、Lua 引用、自有对象回指 | 否 | `Entity._luaBridgeRef`；NPC `+0xF40` 拥有的玩法对象指针 |
| 派生 | 可由已发送状态重新得到 | 否 | 只有确认 Client 写入路径或原生逻辑会重建时才可归入此类；未证明同步必要性时保留证据边界，不因无法派生就自动归状态 |
| 权威 | 只属于 Host 的玩法状态，Client 不应拥有 | 否 | RNG 种子与游标（沿用 Random 热路径 AuthorityOnly 先例，`mod/src/game/world_sources/observation.cpp:120-122`） |
| 填充 | 已证实没有语义的对齐字节 | 否 | 必须有证据，不能凭“看起来是填充”认定 |
| 未知 | 尚未确定 | 否，但必须列出 | 每段都要记录位置和长度 |

### 3.2 证据与“完整”的标准

- **证据来源**：每一段归属都要注明证据，可以是 SDK 声明（`libzhl/functions/*.zhl`）、J273 静态反编译、REPENTOGON 官方源码、Lua 绑定或 E2E 观察。
- **SDK 名称不等于公开语义**：SDK 的 `_depthOffset`（`+0x2B0`）并不是公开的 `DepthOffset`，后者实际在 `+0x358`（`mod/src/game/client_entity_fields.inc:5`）。只有 SDK 声明、尚未核对 J273 的条目，状态必须标为“SDK 声明”。
- **“完整”的定义**（2026-11-12 起由需求清单取代，原文保留）：一个类型的未知字节数为 0；或者每段未知字节都已在 E2E 未知区探测（第 15 步）中确认，在覆盖场景里不发生变化，并写明理由。未知字节既不复制，也不能宣称完整。
- **公开成员回映射**：旧目录中的公开成员改为核对表。每个 `ReplicatedState` 成员都要映射到某个布局字段、某组字段的派生、Host 权威或明确的不适用，用来检查字节归属是否有遗漏（第 1 步生成）。
- **同步必要性**：用途已核、公开 getter 存在、旧目录曾发送或不能由其他字段推导，均不单独构成同步理由。按 Client 的结构、原生绘制／展示维护／HUD 实际依赖决定传输；纯玩法输入／历史归权威，本地 owner／地址归本地。排除传输不删除字节归属和证据。具体调整、保留理由及未决见[字段职责复核](synchronization-field-review.md)。
- **范围边界**：第三方 Mod 自定义状态的 schema、Lua 来源和完整性要求已迁至[后续方案](../mod-adaptation.md)，不计入当前字节归属与原版完成条件。

## 4. 新数据流

```text
Host 主线程，在合法窗口内（ordinary／PlayerPost，沿用已核窗口规则）
  → 用已核来源枚举成员，按虚表确定布局编号
  → 按生成的复制计划：合并区间 memcpy、换算、引用记录身份、容器复制元素
  → 写入预分配的自有缓冲区，非阻塞交给后台
Host 后台构建线程
  → 身份映射为成员序号；按块 memcmp，变化的块记录变化版本
  → 按 Peer 已确认版本组装状态帧，确认版本相同的 Peer 共用一份编码
网络线程（沿用现有通道、分片、补片和 zstd）
Client
  → 有界解码与校验 → 更新影子状态 → 只写入变化块 → 原生安装成功后回 AppliedAck
  → 同一安装点把 Game+0x25A70 写成帧头 game_frame（帧号方案B），写入 Q8 显示标量，注入不可算的值
Client 主线程（2026-11-12 起）
  → Manager 边界内、world 安装之后运行“显示推进”根：只调用已核实的显示子更新
     （本机偶数 Manager 帧跑 30Hz 批次，每帧跑相机阶段；Game::Update 不会被调用）
  → 包装原生 Game::Render：原生代码绘制房间、实体、HUD、地图和菜单
  → 按帧号播放事件队列中的音效、音乐和文字
```

增量规则：每个块记录“最后变化版本”，每个 Peer 记录“已确认版本”，状态帧携带两者之间所有变化过的块。丢包后下一帧仍会带上这些块，两端都不需要保存历史快照。“变了又变回”的块也不会漏，因为判断依据是变化版本，不是与旧快照逐字节比较。普通世界和逐身体是两条独立的版本序列，详见第 9、11 步。

**帧锚点规则**（2026-11-08）：随游戏帧递增或递减的计数，不发送计数本身，而是发送不随帧变化的锚点，否则所在块每帧都会变化。

- **锚点的含义**：锚点的值处于 Host 的帧空间。2026-11-12 起采用帧号方案B：Client 每装好一个 world 帧，就把 `Game+0x25A70` 写成帧头的 game_frame，原生 getter 直接用它和 Host 原值锚点还原计数，不再有独立的展示时钟和逐绘制栈借用（第 13 步 13.6，[修改方案第 3 节](client-display-delegation.md#3-帧号方案b已定)）。
- **已确定的锚点**：
  - `Entity+0x328` 帧基准：FrameCount = 时钟 − 基准，基准原值发送（第 3 步）；
  - `Room+0x11F0` 帧基准：`Room:GetFrameCount()` = 时钟 − 基准，基准原值发送（第 7 步）；
  - Effect Timeout：换算为到期帧 = 全局帧 + Timeout（第 4 步 4.1.5）。
- **其他字段**：其余帧计数类字段在第 3—8 步按同一规则逐个核对。
- **Run TimeCounter（版本56）**：Game+25A74是可修改i32，6F7889／6F7894／6F789D证实它与GameFrame同分支递增；发送u32基准`game_frame - bit_cast<u32>(TimeCounter)`。Client按已应用world帧头减基准再还原i32，按模2^32保留原符号；改时／回溯自然改变基准，不用本地时钟或把它等同GameFrame。见[Run核对](run-synchronization-review.md)。
- **空帧**：静止场景中虽然没有块变化，Host 仍要发送空帧，用来推进 Client 帧号（第 11 步）。

## 5. 布局表格要求

每个产生状态的类型都要画两张**单行表格**：

1. **原生字节归属表**：覆盖从 0 到对象大小的全部字节，按偏移排列，不留空隙。
2. **状态二进制布局表**：按线上顺序列出每个字段在状态中的位置。

格式规则：

- **只有一行**：表头行就是唯一的数据行，下面只放分隔行，不再加其他行。
- **单元格内容**：每一格写偏移、类型、名称和语义。
  - 原生表的格式是 `偏移 类型 名称【类别】语义`。
  - 状态表的格式是 `+偏移 类型 名称：语义`。
  - 未知或填充区也单独占一格，写明长度和现有判断。
- **嵌套类型**：如果嵌套类型有自己的表，可以占一格并注明“见某某表”，例如 `Vector`、`ColorMod`。变长容器写成 `u16 count + count×元素类型`，元素类型另有表。
- **头部和帧结构也要画**：记录头、状态帧头、成员表项等线格式结构同样画单行表（第 9 步）。
- **生成方式**：第 1 步的生成器从布局源产出表格，放在 `layouts/<步骤目录>/<类型>.md`。各步设计稿中手写的表要标“草稿”，生成器建成后以生成结果为准，避免文档与代码不一致。

### 5.1 示例：EntityEffect（草稿）

**已被取代**：EntityEffect 的实际字节归属和块划分，以[第 4 步 4.1](4-entity-classes.md#41-entityeffect-块划分修订稿2026-11-08) 为准。本示例按 SDK 声明的 0x490 和单个状态块写成，对象大小（J273 为 0x4C0）、未知字节数、0x420 变体复用区和块划分均已过时，现在只作表格格式示例。

偏移来自 `entity-application.json` 的 `class_native`（`EntityEffect.*` 13 项和 `NativeEffect` 0x420），语义取自本地 Wiki `IsaacDocs/docs/EntityEffect.md`。未知区来自 SDK `EntityEffect.zhl` 的对象大小 0x490，尚未经 J273 反编译核对。

**EntityEffect 原生字节归属（子类部分 0x410—0x48F）**

| 0x000—0x40F Entity【—】公共部分，见 Entity 表 | 0x410 f32 m_Height【状态】离地高度，粒子作纵向速度 | 0x414 f32 FallingSpeed【状态】下落速度 | 0x418 f32 FallingAcceleration【状态】下落加速度 | 0x41C—0x41F 4B【未知】尚无证据 | 0x420 BitSet128 VarData【状态】按效果变体解释的 128 位数据 | 0x430—0x44F 32B【未知】尚无证据 | 0x450 i32 State【状态】效果自定状态，初始为 0 | 0x454 f32 MinRadius【状态】冲击波最小半径 | 0x458 f32 MaxRadius【状态】冲击波最大半径 | 0x45C i32 Timeout【状态】剩余时间，每帧递减 | 0x460 i32 LifeSpan【状态】寿命 | 0x464 bool IsFollowing【状态】是否跟随父实体 | 0x465—0x467 3B【未知】疑似对齐填充，待证实 | 0x468 Vector ParentOffset【状态】相对父实体的偏移，见 Vector 表 | 0x470 i32 DamageSource【状态】伤害来源的实体类型 | 0x474 f32 Rotation【状态】旋转 | 0x478 f32 Scale【状态】缩放 | 0x47C—0x48F 20B【未知】尚无证据 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

**EntityEffect 子类状态块（69 字节，草稿）**

| +0x00 f32 m_Height：离地高度 | +0x04 f32 FallingSpeed：下落速度 | +0x08 f32 FallingAcceleration：下落加速度 | +0x0C BitSet128 VarData：按变体解释的 128 位原样字节 | +0x1C i32 State：效果自定状态 | +0x20 f32 MinRadius：冲击波最小半径 | +0x24 f32 MaxRadius：冲击波最大半径 | +0x28 i32 Timeout：剩余时间 | +0x2C i32 LifeSpan：寿命 | +0x30 u8 IsFollowing：是否跟随父实体，0／1 | +0x31 Vector ParentOffset：相对父实体的偏移 | +0x39 i32 DamageSource：伤害来源实体类型 | +0x3D f32 Rotation：旋转 | +0x41 f32 Scale：缩放 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

从示例可以得出三点：

- **复制次数**：这 13＋1 个字段只需 4 次 memcpy，区间是 `[0x410,0x41C)`、`[0x420,0x430)`、`[0x450,0x465)`、`[0x468,0x47C)`。0x465—0x467 在证实为填充前不能并入区间，否则 memcmp 可能因无意义字节误判变化。
- **未知字节**：子类部分 128 字节中有 59 字节未知，需要第 4 步核对。
- **布局待调整**：状态块不保留原生对齐，解码一律用 memcpy 读取。字段按什么频率变化、如何拆块，由第 3、4 步决定，这里只演示表格格式。

## 6. 取代与保留

| 旧内容 | 位置 | 处理 |
| --- | --- | --- |
| 公开 Lua 优先、自有只读观测 Lua 作为正式采集来源 | [规范第 15 节](../../refactor/project-conventions.md)、[观测扩展总览](../observation-expansion/README.md)、[REPENTOGON 适配设计](../repentogon-refactor.md) | 当前正式同步改为原生二进制。Lua 观测用于目录／语义核对和 E2E 对照；Mod 域正式来源迁至[独立后续方案](../mod-adaptation.md) |
| Host 采集性能修复方案 | [tx-04-21 采集性能方案](../tx-04-21/tx-04-21-capture-performance-plan.md) | 整体取代。成本口径 stage11—18 和有界交接的思路继续沿用 |
| 逐成员实时目录、`field_ids`、按字段名分层 | [tx-04-21 基础](../tx-04-21/tx-04-21-foundation.md)、`stream-catalog.json`、`mod/tools/gen_stream_catalog.py`、`mod/src/replica/stream_partition.cpp` | 由第 1 步的布局源取代；成员目录改为回映射核对表 |
| `Field`／`Atom`／`Layout` 范围模型，“永不隐式继承旧动态字段” | `mod/src/replica/stream_model.h:30-58,88`、[tx-04-21 网络合同](../tx-04-21/tx-04-21-network-contract.md) | 改为状态帧、块变化版本和显式继承已确认版本（第 9、11 步） |
| JSON 目标与计划：`Parse{Entity,RoomGrid,Player}Target`、`DomainModel`、`SubjectValues` | `mod/src/replica/*_target.*`、`*_plan.*`、`stream_materialize.*` | 由影子状态的类型化视图取代（第 12、13 步） |
| Client 按字符串成员名写入 | `client_entity_common_native.cpp:43-50` 等 | 改为生成的索引分派表（第 13 步） |
| 18—20 应用目录 | `tx-04-18/room-grid-application.json`、`tx-04-19/entity-application.json`、`tx-04-20/player-application.json` | 保留，作为应用职责和回映射的输入；不再驱动采集 |
| tx-04-21 的 E01—E26 | [tx-04-21 E2E 方案](../tx-04-21/tx-04-21-e2e-plan.md) | 修订后作为第 15 步的验收清单；tx-04-21 的暂停进度不再按原方式续跑 |
| 字节完整作为完成条件（2026-11-12） | 本文第 3 节、各类型字节账本 | 完成条件改为需求清单；账本保留作参照 |
| Client 自协调绘制：`MenuManager::Render` 边界、`RenderPublished`、`DrawImpl`、`FrameBorrow` 等（2026-11-12） | `client.cpp:817-831`、`client_scene.cpp:760`、`mod/src/game/client/presentation/*` | 改为包装原生 `Game::Render`；第 14 步退役，见[修改方案第 9 节](client-display-delegation.md#9-hook-与代码退役) |
| 显示结果同步：显示结果 310—407、D0 356、随机 alpha 357／358、显示包络 414—417、文字 422—424、资源 recipe（2026-11-12） | 第 8、9 步及其合同文档 | 退役或并入事件队列和注入值；资源协议收窄为资源引用 |

保留并复用：

- **身份**：`IdentityTable`、`Registry`、key／use／shape 的语义。热路径改用数值接口，见第 10、11 步。
- **采集窗口**：已核的 ordinary／PlayerPost 采集窗口和规范第 11 节的读取规则。
- **Hook**：统一 Hook 清单，常驻并按会话逻辑启停。
- **传输**：通道、`ReliableSender`、`Reassembler`、合包、zstd、`AppliedAck`、Resync，以及 512／128／128 MiB 三池。
- **Client 原生写入层**：原生写入执行器和场景结构事务。描述结构中依赖 JSON 的部分需要替换。

历史文档的正文不改写。第 0 步只在受影响文档顶部加一行说明，指向本目录。

## 7. 步骤

整个重构是一个推进目标：0—14 是实现步骤，期间不运行任何测试；第 15 步是唯一的统一整体验收。

| 步 | 文档 | 内容 | 依赖 |
| --- | --- | --- | --- |
| 0 | [起点、取代范围与规范同步](0-baseline.md) | 代码起点、旧文档标注、术语 | — |
| 1 | [布局源、字节归属模型与生成器](1-layout-source.md) | 唯一布局源，生成复制计划、编解码表和文档表格 | 0 |
| 2 | [基础值类型与嵌套规则](2-base-types.md) | Vector、ColorMod、ANM2 等基础类型，以及容器、引用、资源规则 | 1 |
| 3 | [Entity 公共与主 Sprite](3-entity-common.md) | Entity 0x410 与内嵌 ANM2 的字节归属、块划分 | 2 |
| 4 | [Entity 子类](4-entity-classes.md) | NPC、Tear、Projectile、Bomb、Laser、Knife、Pickup、Familiar、Effect、Slot 及专用形态 | 3 |
| 5 | [EntityPlayer](5-player.md) | Player 0x2EA0、内嵌 Sprite、容器、身体快源块 | 3 |
| 6 | [GridEntity](6-grid.md) | Grid 公共部分和各子类 | 2 |
| 7 | [Room、Level、RoomSave 与 Exploration](7-room-level.md) | 房间、楼层、描述表和保存集合 | 2、6 |
| 8 | [Run、Presentation 与 Random](8-run-presentation.md) | 原版整局、HUD／地图及 RNG 权威／必要绘制输入 | 2 |
| 9 | [线格式](9-wire-format.md) | 状态帧、记录、块、成员表、资源表和校验 | 1 |
| 10 | [Host 采集器](10-host-capture.md) | 主线程复制、换算、引用、容器和交接 | 1—9 |
| 11 | [Host 变化检测、增量与发布](11-host-publish.md) | 块比较、变化版本、按 Peer 组帧和 Baseline | 9、10 |
| 12 | [Client 解码、校验与影子状态](12-client-decode.md) | 校验、版本接受规则和影子状态 | 9 |
| 13 | [Client 原生应用](13-client-apply.md) | 冷热路径写入、描述结构替换和漂移校正 | 12 |
| 14 | [退役旧路径与产品边界](14-retire-old-path.md) | 移除 Lua／JSON 热路径，划清产品与 E2E 构建边界 | 10—13 |
| 15 | [E2E 适配与统一整体验收](15-e2e-acceptance.md) | 原版 Lua 对照、差异驱动的未知区补查、E01—E26 修订及 E27—E30、E32—E34、E36—E38；原 E31 已迁出，E35 于 2026-11-12 删除 | 0—14 |

步骤 3—8 是字节归属研究，需要对 J273 做静态反编译，可以交叉进行；第 9 步只依赖布局源格式，可以与 3—8 并行。

2026-11-12 方向调整不另开编号，修改落在原 0—14 步内，第 15 步仍是唯一的统一整体验收。各步的改动见[修改方案第 7 节](client-display-delegation.md#7-各步修改)。新增的逆向任务 RE-1—RE-8 归入第 13 步（Client 边界与注入）和第 8 步（事件源），推进顺序见修改方案第 14 节。

## 8. 推进规则

这些规则遵循[现行项目规范](../../refactor/project-conventions.md)：

- **不设 gate，不校验哈希**：布局兼容靠整数布局版本号，不计算任何哈希。
- **只在第 15 步测试**：0—14 期间只做资料分析、静态反编译、设计、实现和纯编译。不运行游戏、E2E、校验脚本或带测试的构建。第 15 步进入后连续修复、续测，不另开编号。
- **不另写单元测试**：实现后不补写单元测试，以 E2E 为唯一测试手段。必须隔离验证的系统，先写出全部失败方式再写代码；各步文档的“失败方式”一节即用于此。
- **注释**：代码为功能、核心函数和数据结构写中文注释。
- **实机测试前沟通**：每次启动前告知用户需要配合的操作和停止时机，需要手动配合时先确认用户已准备好。
- **记录如实保留**：失败和 UNKNOWN 照实记录，不留 artifact，只在中途停下需要恢复时保留必要材料。
- **隔离约束不变**：保持 C/S 与原版 Online、Isaac Sync 的隔离，线程与生命周期约束、Hook 常驻和实际操作授权范围不变。
- **提交**：只在用户要求时提交。

每步文档统一包含：目标、输入事实、设计、改动范围、失败方式、完成内容、未决问题。

## 9. 未决问题与需要用户决定的事项

1. **代码起点（2026-11-08 已定）**：用户已提交为 `87bae3e`（157 个文件，+6265／−532），含第 70—75 轮修复和本计划文档，作为重构起点，见第 0 步。
2. **Mod 适配（2026-11-10 已迁出）**：设计与未决问题见[独立后续方案](../mod-adaptation.md)，不再作为本轮未完成项或验收条件。当前先跑通原版。
3. **tx-04-21 验收（2026-11-08 已定）**：tx-04-21 未完成的统一验收并入第 15 步，不再按原 tx 续跑；[进度文档](../tx-04-21/tx-04-21-e2e-progress.md)顶部已标注。
4. **依赖其他状态的计算结果**：例如 `GetCollectibleNum` 的结果依赖库存、效果等多处状态。它们能否全部由已发送的字节在 Client 重建，要在第 5 步和回映射中逐项确认；不能重建的，按“换算”或“权威”归类。
5. **未知字节**：第 3—8 步的工作量取决于各类型的未知字节数量。EntityPlayer 0x2EA0 字节、Game 0x680E8 字节的完整核对规模最大，第 5、8 步需要先给出范围划分。2026-11-12 起未知字节不再阻塞完成（Q1），本项只在第 15 步发现差异时再补查。
6. **显示委托 Q1—Q4（2026-11-12 已定）**：全部按[修改方案第 13 节](client-display-delegation.md#13-未决问题)的推荐。
   - Q1：完整性按需求清单。
   - Q2：保留 C++ 执行器，只收缩复制计划。
   - Q3：优先原生 setter 或 REPENTOGON API。
   - Q4：接受外观随机差异。
   - Q5（同日追加）：切房走原生过渡。
   - Q6（同日追加）：放行回调分派。
   - Q7（同日追加）：音效按来源去重。
   - Q8（同日追加）：`Game::Update`／`Room::Update` 内联显示标量同步 Host 当前值，Client 不本地推进。
   - Q9（同日追加）：同步当前房间描述的 SpawnSeed。
   - Q10（同日追加）：保留 `client_entity_draw_projectile` 作 PositionOffset 恢复作用域。
   - Q11（同日追加）：转场显示字段同步 Host 当前值，Client 不运行转场 FSM。
   - Q12（同日追加）：保留 `client_entity_draw_paused`；Host 发送 paused 位与终结分支枚举。
   - RE-10 静态分类已完成，音频状态需求与本地维护接线待第 8／10／13 步实施。
   - Q13（同日追加）：地图拾取图标由 Client 在地图缓存构建 `0x9635B0` 作用域内临时镜像 Host 摘要合成的最小条目，调用后恢复。

   不为 Client 边界改造单设验收点。改动前的快照已提交为 `3977b46e`。
