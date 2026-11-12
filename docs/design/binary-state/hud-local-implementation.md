# HUD本地化：布局70与纯值显示模块

**2026-11-12 已被取代**：原生 HUD 能产生的部分，在纯值模块中进入退役评估，只保留原生做不到的项。布局70的事实保留。原文、事实与 UNKNOWN 保留。见[修改方案](client-display-delegation.md)。

**2026-11-12 后续正常来源接线**：第10步已接832C10／92F610真实消费、历史P8与435驱动，并增加逐身体Host自有Stat／History显示生产；详细状态、概率／程序History及其它缺口见[第五项实施记录](10-run-presentation-capture.md)。以下“尚未接通”描述布局70初次模块交付时点；正式发布／候选／资源后端／本地绘制仍未实现，不能据本次采集接线宣称第13步完成。

日期：2026-11-12（Asia/Singapore）。合同：[HUD本地生成边界](hud-local-presentation.md)。本轮落实第8／9步的布局迁移，并为第10／13步提供可编译的纯值采集驱动和显示模块。**尚未接通正常HUD采集、正式发布、完整候选校验、本地资源后端和原生绘制**；不宣称八类HUD已在游戏中运行。第15步仍是唯一统一整体验收。

## 1. 唯一源与兼容

- `mod/binding/state/version.json` 从69递增为70，新域 `hud-local.json` 由同一生成器参与布局、正文、依赖、预算和文档生成。
- 315保留全部原生字节事实，将 `low_health_flash`、`holy_mantle_visible`、`drop_display_scale` 改为派生归属并退出线上块。心格、三色倒计时、四Pocket、库存、实际合成图像保持。
- 311／312新增 `icon_source`：0真实空槽、1已登记普通本地图标、2Host特殊结果。311仅分支2带 `visual_id`／`item_image`，`book_image`／`charge_flash` 始终保留。312仅分支2带 `visual_id`／`image`／`draw_center`，`tint_override_source` 始终保留。311为7—13B，312为2—16B；条件字段经具名处理器，不能进入无条件memcpy。
- 364继续保留group、body、roles、display_order。P7的控制活动／roster顺序不足以唯一推出显示身体、关联健康角色与显示顺序，故没有删除该必要投影。位置、缩放、行列、裁剪和History前缀不下行。
- 正式整数版本隔离旧布局；公共协议沿用world／body两条确认序列。没有另设HUD ACK、哈希或项目gate。

生成表和布局文档只由 `mod/tools/gen_state_layout.py` 更新，没有手改生成输出。既有旧协议／Lua运行路径尚未被新路径接管，按第14步保留，不在它们现有版本中删字节。

## 2. 独立435显示驱动

`PlayerHUDLocalClock` 是41B具名body结果，与315分开变化检测／发布；315不会因单纯的低血周期或Drop渐变变化而成为必发结果。

| 字段 | 线宽 | 来源与用途 |
|---|---|---|
| player | u16 | 同身体成员引用 |
| binding_revision | u64 | 已提交控制组绑定代次 |
| update_ordinal | u64 | 本HUD实例实际正常完成的显示更新次数，不是Authority tick、渲染帧或确认号 |
| sample_game_frame | u32 | 最后一次有效HUD更新的真实GameFrame，暂停／冻结时保持 |
| critical_health | u8 | 同次生产的red＋eternal＋soul＋bone原生32位和是否为1 |
| mantle_source_revision | u64 | 最近允许护盾查询时的完整P8效果封存修订 |
| drop_anchor_update | u64 | 最近实际held变化、初始化或纠正的更新后锚点 |
| drop_held | u8 | 832C10尾部92F610的实际持续状态返回值 |
| drop_anchor_phase | u8 | 该锚点更新完成后的0—4有限相位 |

`HudClockCapture::Observe` 只接受调用方已取得正常更新证言的自有值。相同完整实例下拒绝重复／倒序来源修订，按实际调用推进update；只有实际held变化、初建或已识别纠正才换Drop锚点。相位是更新后值，随后按 `update - drop_anchor` 有界推进，最多四步饱和。丢包或重复展示不依赖到包数恢复；切房、换形、绑定及HUD实例变化建立新实例，不能继承旧held。

`EncodeHudLocalClockInto`／`DecodeHudLocalClock` 使用通用正式正文编解码器和同源数字字段／宽度表，不复制C++结构，不维护另一份线上偏移。检查非Null身体、非零修订／更新、相位0—4和anchor≤update；通过正文检查不代表依赖ready或允许ACK。

护盾冻结需要额外注意：832C10并非所有分支都重算1696缓存。435引用最近允许查询时的P8修订；正常运行、冷连接及恢复必须保留／提供**那一份**完整纯值效果输入，不能用暂停后任意当前P8代替。没有该输入就等待／恢复，不默认无护盾。布局70同时登记条件历史输入 `HudMantleEffects`（436）：仅当当前／已提交自有P8无法满足435所需修订时，随同body或joint目标补完整disabled、有序条目与配置资源身份，`produced_revision`精确等于`mantle_source_revision`，不常规重复发送P8，也不直接传护盾bool。436保留180／150的count、条件cooldown和顺序语义；大小5—163845B、单快body≤65536B，超出走可靠joint，仍受整帧／准备额度限制。该历史输入必须继续计入实际缓存及准备预算；本轮登记了可传输布局，尚未实现Host保留／基线装配。

## 3. 已实现的纯显示算法

代码位于 `mod/src/state/hud_local.h`／`.cpp`，加入产品状态库。

- `HudCriticalHealth` 用原生32位回绕求和，包含永恒心／骨心；不从24心格猜缺失健康值。
- `HudHolyMantle` 要求完整、准确身体／修订的效果输入；disabled直接false，否则依原序查类别1／3／4且ID313的条目，count为0或负数仍是存在。不运行效果Update、护盾恢复或伤害逻辑。
- `HudLowHealthFlash` 用已提交采样GameFrame对45取余，前十相位执行整数式 `255*(9-phase)/9`，其余为0；不读Client墙钟或渲染帧。
- `HudDropScale` 从有限锚点计算0—1显示值，单次最多四步；不消费输入设备、不执行物品丢弃或任何玩法动作。
- `HudOpaqueCenter` 从本地实际32×32以内alpha有效范围计算整数中点；全透明／损坏图明确失败。缓存资源键含epoch、配置与内容代次，主体键含scope、body/use/shape、绑定、拓扑及HUD实例。
- 六项Stat文字仅对Host最终value执行有界fixed格式化，默认两位，允许本地精度0—6；不再次换算射速／射程，不推断delta、不执行概率重算。delta/countdown、概率、资格仍由原结果owner承接。
- 材料与法术数据使用P3实际u8[8]／u8[6]。生成本地绘制描述，不重排源数组、不按counts抽取。材料采用839A80的Idle／原值帧与四列两行局部位置；法术采用83A840的首槽Idle、其余IdleSmall、实际绘制顺序5→0，并根据P2 `poop_mana` 保留1／0.1 alpha。帧／动画／文件必须在本地资源owner精确ready，空值仍为真实帧0。
- `PlanHudLocalLayout` 使用364和已提交控制组的C/S ordinal，在本地视口排版，保留唯一／稠密display_order。每组最多32个必要显示身体、最多四组；重复身体／组冲突失败。本端组保留详细roles，其他组保留公共与关联健康roles；不将备用身体自动加进选择，不修改PlayerIndex、ControllerIndex、roster或Online本地性。NO_HUD与Host world可见性在布局入口生效；诅咒、完整心格等仍由原结果绘制owner消费。

队列输出是带精确资源键的绘制描述，排版输出是自有坐标，尚不是实际原生draw。字体owner、各类HUD原生writer及完整world隐藏／诅咒条件仍须按第13步接线。

## 4. 普通图标映射及特殊结果

`hud-local.json` 登记J273原版范围：主动候选1—732、饰品1—189，以及832C10的具名特殊排除项。主动排除290、326、352、422、434、474、479、483、489、523、640、685、710、711、720；710即使某槽走普通路径，本轮仍保守保留Host结果。饰品排除75与166。负ID、范围外、配置身份或实际图像不匹配均保留Host特殊分支。

`SelectHudIconSource` 还要求实际配置类别：主动3、饰品2，同槽P4/P3身份等于正常Host visual_id，且配置图与实际选中图像内容精确一致。**范围内正ID本身不构成本地派生资格**。只有真实空状态与真实空选择才能选0；本地资源缺失不能改成0或借Host特殊图绕过缺口。

Client从同目标P3两饰品及黄金位产生黄金外观，从P4四槽取得普通主动ID及当前充能六值。`book_image`、`charge_flash`、`tint_override_source`与特殊图像／中心继续来自Host；口袋主动不以ActiveSlot+1冒充道具ID。资源后端须按已登记类别、槽及精确代次返回已准备图像，不运行完整HUD Update或玩法查询，也不通过图像像素下行兜底。

## 5. 准备、提交与退休

`PrepareHudLocal` 要求完整同目标输入、准确主体／绑定与P8修订，普通／特殊图标、book与全部队列资源一起准备；失败不会产生半份输出。`HudLocalOwner` 保存独立候选与已提交帧：

- Prepare成功才生成本地票据，不发送ACK。相同实例拒绝旧target、倒序时钟与同更新冲突；同修订P8显示结果不可暗改。
- Commit核原票据、完整主体、目标与精确资源仍ready，并等待自有draw借用退出。成功原子替换本模块全部值，仍不代表整批原生写入完成。
- Cancel只撤销候选；Retire禁止新借用，已有借用归还前保留已提交存储。该借用只保护本模块纯值，不能代替原生writer／draw／GPU资源的真实排空。
- Host采集器与Client owner分别绑定构造线程。保留状态以实际 `sizeof` 在共享Values池冷准备收费；没有逐帧vector扩容。外层资源owner及完整目标仍按既有Peer与共享池收费，不能因本地生成免计。

第12／13步完整目标装配者负责核对P3/P4/P8来源、同身体use/shape、旧链接、组关系、拓扑与world/body窄依赖，并与其他必要结果共同提交。只有全部主线程安装及纯值缓存提升真实成功后才沿原接受状态机ACK。当前没有为缺失完整validator提供默认成功实现。

## 6. 预算与剩余接线

`wire.json` 新增八身体435完整消息的设计样例，按具名结果组头、41B正文、每组显式三依赖／两贡献及公共消息／分段／网络封装收费，不误套12B成员头。详见[生成预算](layouts/09-wire-format/budget-plan.md)。这是声明数量的静态成本，不包括完整315、其他资源、重传等，不能作为实测带宽或性能通过结论。实际发布仍须按全部真实依赖／贡献和资源计费。

| 环节 | 本轮状态／剩余工作 |
|---|---|
| 第8／9步 | 布局70、派生边界、条件分支、435编解码、条件历史输入436、依赖及静态预算已实现 |
| 第10步 | 纯值正常更新tracker已实现；832C10／92F610实际观察、全身体来源、P8历史输入保留及315特殊结果封存尚未接通，未增装猜测ABI的Hook |
| 第11步 | schema允许435独立变化发布，整份315不因本地派生必发；真实发布器与有限驱动／冷连接装配仍待实施 |
| 第12步 | 本模块强制同目标必要输入与精确资源；完整候选validator、历史P8解析及正式调度仍待实施 |
| 第13步 | 纯值派生／中心／格式／队列描述／排版及候选owner已实现；`HudLocalResources`真实后端、原生writer、绘制及整批安装接线仍待实施 |
| 第14步 | 新布局不再登记重复缓存；旧可运行Lua／legacy采集、写入与原值比较暂保留，等真实新用途接管后退役 |
| 第15步 | 尚未进入，不运行任何测试；验收范围沿[合同第8节](hud-local-presentation.md#8-第15步统一验收与本次完成范围) |

## 7. 静态证据与构建记录

本轮复读J273静态反编译：832C10正常生产及其92F610尾部消费、90A490护盾查询、839A80材料绘制与83A840法术绘制；官方 `PlayerHUD.zhl` 核对消费者名称，原版本地Wiki `ItemType.md` 核对配置类别。以上是静态事实，不是实机观察。保留原有UNKNOWN、原生字节和旧失败记录。

产品Release／Win32使用既有 `build/repentogon`，`ISAACCS_BUILD_E2E_TOOLS=OFF`。首次生成先后暴露“退出负载但仍标状态”和“条件字段仍为无条件Copy”两项声明错误，已分别改为派生归属与具名转换；最终状态库及产品DLL纯编译／链接成功。官方SDK既有警告未作本轮无关改动；本轮新增模块最终编译没有警告。没有部署、启动游戏、单元／隔离／smoke／E2E测试或哈希校验。
