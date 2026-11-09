# Entity valid／exists／dead 同步职责再复核

日期：2026-11-09（Asia/Singapore）。布局版本 **31**。本次纠正版本30将“绘制读取 valid”直接提升为“所有 Entity 必须发送 valid”的结论。只进行静态阅读、布局生成和普通产品编译，没有测试、游戏或部署。

## 结论

| 字段 | 当前决定 | 原因 |
|---|---|---|
| `valid +170` | 从 Entity 公共 E2 删除，原生字节归本地 | Client 的构造、列表发布、池占用和退休资格由自己的 owner 维护；Host 原生字节不应作为逐实体热状态覆盖它。Player 特殊身体的结构资格另见下文，不因此删除相关信息 |
| `exists +172` | 保留同步 | 它与成员在表、valid、visible 均不同；影响原生绘制及间接引用对象。备用身体可以 valid=false、没有任何房间列表成员资格，但 exists=true |
| `dead +173` | 保留同步 | 独立死亡状态会影响 Player 的角色进度条／旁体 HUD 和 Room Light；不是“已退休”，也没有证据可与 HP≤0 或某个 Sprite 动画建立恒等关系 |

本次仅减少 Entity 公共状态 **1B**，最小状态从版本30的411B改为410B，E2从36B改为35B；没有新增 Player 布局或宣称其结构 writer 已实现。此前保留的 exists／dead 原生 bool 校验和公开回映射不变，valid 的 wire、专用读取资格、bool validator 和 `entity_valid_presentation_owner` 声明均撤销。原生大小0x410、335B未知和证据等级不变。

## valid：本地原生资格与 Player 结构角色分开

J273 原始指令（VA，ImageBase 0x400000）：

- Entity ctor `6A39D4` 写 valid=0；`6A39DB` 另写 exists=0。二者并非同一个状态。
- `EntityList::AddEntity 413D00` 在 `413D0B` 检查 valid，首次发布分支在 `413D9C` 置1，并维护本进程的列表和查询缓存。现有 Client 发布器也要求调用前为false、调用后为true（`client_objects_native.cpp:247—258`）。
- 工厂 `6EC540` 用该字节选择可复用池项；当前 Client 的占用、地址唯一性、借用及退休检查同样依赖自己的 `_valid`。网络数据不得写true假装已经发布，也不得写false让还在绘制或被引用的对象成为可复用槽。
- Player RenderHead `783680` 的 `783707` **确实读取当前身体 valid**，但其语义是在角色29／38、`backup +1E50`存在等条件下选择递归复合绘制。被借画的备用体若也无条件置true，可能再次进入互惠backup递归，原来的终止条件不能保证；所以“所有已构造对象一律valid=true”不可取。
- Player 查询helper `7699D0` 在 `769D4C` 也以当前身体valid控制沿backup的递归查询。需要正确的是这份活动／备用角色关系，不是把Host池字节发给全部Entity。

**Player信息仍须保留的边界**：现有 `PlayerApplicationMembership` 分开采集 room/render/update 列表和 `native_membership.valid`；`player_target.cpp:106` 只验证 `room_member ⇒ native_valid`，没有验证反向等价。`client_players_native.cpp` 对已发布身体经AddEntity得到本地valid，对非房间身体则由结构owner处理资格。因此本轮不能声称 `valid == room_member`，也不能删除Player旧结构源里的独立资格字段或把“非roster”当作无效。

第5步须将房间／render／update成员、活动／备用／存储角色及其原生激活资格作为 **Player P7结构语义** 处理；沿用旧源时，`native_membership.valid`是该资格的见证，不能在E2再重复发送。若后续证实完整角色能唯一推导它，可在那时进一步消除这一结构信息；当前没有这种证明。资格变化沿用结构事务，现有 `player_plan.cpp:113` 已将room／native_valid变化列为重建原因；新路径的具体编码／writer仍在第5／9／13步落地。它不授予输入资格，也不覆盖本地分配／初始化完成事实。

所以“valid移出公共同步”**不等于**“所有情况下都不需要传达Player激活角色信息”，更不等于把Host字节改个名字继续塞回每个Entity的热块。本地owner和Player结构源是分开的职责。

## exists：成员存在不等于原生展示存在

- 公共 Render `6AC4ED` 先检查 `+172`；Tear、Bomb、Pickup和Text也有相同门控。只同步Visible不能替代Exists。
- 原生 Remove `6A8000` 在 `6A800C` 清exists、在 `6A8015` 置dead，但不清valid；实际撤表／池退休是另一阶段。不能用网络成员表的离开事件冒充所有中间状态，也不能为了得到false在Client调用Remove。
- 历史冷Client10已有直接反例：Esau非活动存储type38，`exists=true`、`native_valid=false`、非roster且room/render/update均0；旧实现额外要求Exists=false导致错误，修复后保留真实Exists。见[原验收记录](../tx-04-20/tx-04-20-e2e.md)。本次没有重跑或改写该历史。
- Player RenderHead会暂时把backup的visible／exists置true（`783723/783733`），递归绘制后恢复（`783DC7`附近）。这说明借画前后的原始exists与绘制借用不同；不能永久清零备用体，也不能永久强制true。

## dead：死亡展示独立于 HP 和退休

- 基类 Kill `6A7ED0` 在 `6A7EDA` 检查dead、`6A7EEA`置true，再进入死亡处理／回调；该入口没有把公共HP字段写成统一约定值，也没有清exists或valid。该入口及已读helper没有给出dead与HP／成员离表的等价公式；没有完成全部派生死亡路径证明，不能据HP或成员表删除dead。
- Player Render `785F3B` 用dead跳过角色进度条／旁体HUD；Room Light `7FF848` 的Effect分支也读该字节。主Sprite、HP、Visible和成员表没有提供等价替代证据。
- Client必须安装最终dead，不能重演Kill／伤害／死亡回调。角色的ghost、inactive、dead及retired保持各自语义。

## 后续采集、应用及统一验收

- 第10步按生成计划采集公共exists／dead，不采集E2 valid。Host身份／生命周期观测仍可按合法窗口读valid；Player结构源需要的激活资格不因本次排除消失。
- 第13步公共写入／漂移校正不再覆盖valid。对象owner按构造／发布／撤边／退休维护它；Player特殊身体由完整结构事务确定资格。exists／dead按最终状态安装，不调用Remove或Kill补算。
- 第14步移除版本30的公共valid handler和比较要求；旧Player结构源／角色信息在新P7真正接管前保留，不能一并删去。旧运行路径本轮不提前切换。
- 第15步沿用E09／E25／E28／E29覆盖：普通实体发布／退休，角色29／38互惠backup复合绘制不递归失控，非roster／非房间但exists=true的存储身体，死亡但尚未退休的展示，热块不得改变本地池资格。Player结构资格按新P7合同比较，公共valid不再做逐字节对照；exists／dead继续按当前回映射比较。没有新增验收编号，没有提前执行。

本轮版本31生成链和普通Release／Win32产品构建已完成，`ISAACCS_BUILD_E2E_TOOLS=OFF`；`isaac_cs_state`与完整`zhlIsaacCS`编译／链接成功。生成的公共E2已无valid及其handler，exists／dead仍各占1B；没有运行测试或部署。
