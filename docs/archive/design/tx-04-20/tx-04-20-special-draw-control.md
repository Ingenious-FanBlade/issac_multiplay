# tx-04-20 I26—I30：备份、专用状态、生命周期、绘制和控制关系

日期：2026-11-04，Asia/Singapore。本轮完成 I26—I30 的实现和静态主审，按会话要求直接提交；没有构建、运行任何测试或启动游戏。整个 tx 仍有 I31—I40，唯一整体验收仍为 I39。`native_apply`、`native_source_complete` 和完整 Entity/Player 域提交声明继续保持 false，不能把本轮安装器接入记为运行通过或网络闭环。

## 本轮实现

| 项 | 实现及边界 |
| --- | --- |
| I26 | `ClientNativePlayers` 保存同 T 的四个具名备份 Key 和对应本地实例借用；703 每个主动 descriptor 的 VarData 0 表示未关联，1—4 对应槽0—3。全体结构取得后解析槽，检查实际实例，退休先撤镜像借用；不按 ordinal/PlayerIndex 猜槽，不写 Authority 的全局槽，不调用 Esau Jr 使用/恢复链。`ClientPlayers::Backup` 只公开已提交 A 的纯值引用。 |
| I27 | `application_player_special` v1 补齐16个库存 History 下标、8个 crafting 内容及已选 output/pool、6个 poop 队列槽；通用最终量补 SamsonBerserkCharge、IBSCharge、crafting 使用计时及三个充能条计时器。直接安装/原生回读，不重算配方、不随机补法术、不重播道具/变身/伤害。完整残影 deque、可选动画序列也按具名 draw 读集补源。 |
| I28 | `PlayerLifecycle` 随唯一 Scene 连接保存已提交 current/retired Key、最后 shape；原生整场景重建不清这份逻辑寿命。完整成员后态的删除及显式 Retired 才退休 Key；Partial 在写前拒绝，不把 dead/ghost/非活动/非 roster 当删除。旧 Key 不复活、shape 不倒退。角色/shape/实际 Room owner 改变沿已核定的完整安全重建；其他组在 T 中的逻辑身份及值仍保留，不调用 Die/递归玩法 Remove。render/update 成员恢复可以重新加入同实例的真实 EL。 |
| I29 | 内建 Player adapter 接入现有结构→最终量/资源→全体 Link→全体 Check；统一清单增加 Player Render、Player Shadow 及纯 palette 查询，继续使用18/19唯一分层分派、FBO、共享 shadow buffer、light、post、水面、镜像和柱遮挡。两份真实 Image 从同窗口传资源身份/格式/采样参数，在本机加载并交 Player strong 持有；不调用完整派生 Init。 |
| I30 | `ClientPlayerControlView` 只从 Scene 已提交 A 提供 participant/group/control_session/binding_revision 和成员/活动/consumer 数，不带 native 地址。Client 按已分配 ordinal 发一条意图流；视图的实际绑定代次须与 Authority progress generation 一致。真实控制身份变化撤待发边沿；shape/换装/roster 重排不制造新连接。构建/暂停/UI/失焦/停止保持中性；Authority 的统一 Submit/queue/dedupe/Resolve 和09多身体消费路径未改。 |

## 原版角色族的责任

| 族 | 最终量和对象 owner |
| --- | --- |
| C02 Lost/Keeper | 共用最终心量、护盾/资源/临时效果及原生姿态，不套普通 Isaac 的心量约束，不触发新奖励。 |
| C03 Lilith/Tainted Lilith | Player 身体/眼部资源、五槽 Weapon、现有 Familiar/胎儿/鞭的 Entity19 具名关系；不新建第二个 Player。 |
| C04 Apollyon | voided 库存、实际主动槽和 locust Entity/Familiar owner；不调用 Void/Abyss。 |
| C05 Bethany | 魂/血充能、主动槽、wisp map 和 Entity19 的真实 wisp 对象/Player owner；共享查询与每身体 backing 分开。 |
| C11 Tainted Isaac | 有序 History 与16个实际索引分开保留；适用的索引必须指向同身体 History。非本类型未消费的 backing 下标不假装当前库存次序。 |
| C12 Maggy/Judas/Azazel | 已有最终冷却、资源、效果、武器和完整身体 ANM；不在 Client 发动短期能力或攻击。 |
| C13 Cain | crafting8/output/pool/使用计时；不用有配方/HUD副作用的公开 setter。 |
| C14 Blue Baby | poop mana、6槽队列、IBSCharge 及已生成 Entity；空槽0与缺来源不同，不执行 RemovePoopSpell 的随机补充。 |
| C15 Eve | 最终心量、Sumptorium charge、效果/资源和 Entity19 的 clot owner；不消耗心量生 clot。 |
| C16 Samson/Eden | +2070 Berserk 最终充能、临时效果、骨武器及姿态；Eden 重掷后采用完整新结果，不重随机。 |
| C17 Jacob | 37/39 最终身体形态、现有 NPC/Effect 的具名关系/资源；Dark Esau 继续由 Entity19 唯一拥有。 |

C06—C10 的主/副/sub/Flip/backup 图继续使用 I23—I26 合同。模组角色保持既定27分工；不把0—40当作动态角色系统的上限。

## 绘制及寿命

`ClientPresentation` 通过正式20标记采用完整Player成员检查与稳定roster缓存，不再将每个角色/身体限制为首个Isaac；非roster render成员也必须通过Player当前owner见证。旧单Isaac profile保留原约束。合法空render集合不调用有非空前置的原生shadow批次；没有roster时不索引首Player或绘制旧HUD。现阶段HUD仍是已有来源的首身体展示，完整多身体HUD视图/绑定继续由I31及27完成，不能由多身体绘制外推HUD已全部完成。

主 Sprite、13个原有物理内嵌 ANM、完整 costume 和 Player+1C50 的可选序列 Sprite 分别持有资源。+1C44 是实际 list/count/cursor/Sprite 复合对象，+1C48 是 count，不能当指针或默认零忽略。新增来源复制完整有界记录顺序、当前 cursor 的纯值下标，以及 draw 消费的位置和两对 animation/frame；节点另两个word的推进语义未核定，当前采用的draw/析构不读取，不传输，Client 中性化并禁止 Update。用本机40字节 list 节点构建，保留真实 sentinel/cursor；空目标清旧列表，动画索引在实际本机资源上核对。原生 Player 析构唯一释放列表和 Sprite。

Lazarus 原生 draw 可以复制并 Swap 另一身体的 ANM，不能只保留 scalar/selector。完整20路径在整帧借用内用真实 ANM copy ctor 保存 strong，最外层 CPU 队列退尽后用 J273 fastcall Swap 恢复，再真实析构工作对象；Layer parent/self 由原生维护。形态/资源/实例变化不复用旧借用。未知构造/恢复/析构前缀保留 owner、首因和隔离，不盲重试；更完整的故障/恢复编排仍由 I33 收束。

身体 visibility/exists/dead、full-sprite flags、六个实际 scale、held/minecart/位置/颜色/frame-base 按原用途保留和恢复。Player/持物/非 roster 身体通过唯一当前实例见证解析；非 roster 的合法 valid=false 不再误套普通池 Entity 的 draw-cache 资格。held 的真实父代画继续保留自身 Key/shape，普通可见成员不另造第二个对象。

充能条 helper 758850/758D50 只采用其具名纯表现维护：读取最终充能，选择动画/设帧/Play，七个实际 ANM 的临时变化保留至外层消费结束，不调用 ANM Update 或动画事件。Hologram 的 BEF6A0 专用 RNG 只有783680使用，采用独立纯视觉噪声合同；普通 game/manager frame/subframe 仍消费既有 Authority 表现输入，不推进玩法 RNG。Source palette leaf 的最终 ColorMod 经窄 Hook供给原生绘制，Client 的 ControllerIndex 始终为 -1，不读取本地设备来决定外观。

## 后续与验证状态

I31 补 HUD/展示视图及必要最小依赖，I32 补新采用路径的完整用途隔离，I33 收束全前缀异常和退休，I34 才更新完整 Scene 范围谓词；I35—I38 继续预算、诊断、工程及统一编排，I39 统一实机，I40 如实记录结果。本轮新增模块均进入产品工程，SDK-facing 单元保持 C++17；没有测试桥、故障开关或录制依赖。

必要逆向由指定一个 GPT-6 Luna max 子智能体执行，主智能体复读并补证，见[原生研究与纠错](player-i26-i30-native.md)。没有运行时通过结论，没有 tx/E2E artifact。
