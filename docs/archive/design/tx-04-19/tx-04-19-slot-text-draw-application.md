# tx-04-19 I20—I25：剩余类别、形态与分层绘制

日期：2026-11-03，Asia/Singapore。本轮实现 I20—I25 的正式来源、纯值模型、原生安装与接线。没有执行构建、分项测试、E2E、游戏、注入或部署；统一运行验收仍归 I34，不能据此声明画面或全部异常恢复通过。I26—I35继续按原清单推进，没有设置完整 Entity 域提交位，也没有网络 ACK。

## 实现范围

| 项目 | 本轮交付 |
| --- | --- |
| I20 Slot | 注册真实 Slot adapter，覆盖现有1—18变体。State、PrizeType、PrizeCollectible、DonationValue、TriggerTimer、Touch、ShellGameAnimationIndex、Timeout按原生宽度安装／回读；主／奖品 ANM独立安装。采用 base Init 和 Source 最终结果，避免派生 Init 的随机、支付、奖品选择、Player外观选择及虚Remove。Wiki高位timeout说法与J273 signed16 Getter冲突，保留说明而不虚构高位字段。 |
| I21 NativeText | 当前use真实writer见证下传原始字节，最多255字节加终止符；独立counter／duration、完整KColor20含opaque32安装并回读。Source新增同一普通窗口的draw输入，Client用已加载的Font6窄Width／DrawString消费variant2缩放与其他variant文本，不运行Text Update。独立`native-text-render-v1`仍保留原scope，不拼进普通状态。 |
| I22 NativeDummy／未知分类 | 从Factory `+0x675650`的128个真实预构造槽按`0x410`扫描空use，核精确Dummy vtable、valid=false、exists=false；不动cursor、不改Type/vptr、不覆盖活槽。走真实动态Init、发布、绘制和退休；自有acquire补一次官方ESSM New通知，原ctor／base Init保留Plus维护链。generic `Create(964)`仍是NPC，允许具有真实NPC见证的964走NPC adapter。未知类明确失败，NativeDarkness不当别名。 |
| I23 形态／死亡／删除 | A→T保持同Key逻辑身份；shape revision、类别、Type／Variant／Subtype／seed变化仍退休旧use后取得新实例，旧资源／关系／checked资格失效。先撤全图关系再退休，NPC私有引用也维护反向登记。死亡但仍存在直接安装Dead／Exists后态，不调用Die／爆炸／奖励；明确成员缺席才删除，重复目标无重复退休。 |
| I24 Effect136／Grid | 从同一T的Grid18 lifted owner与Entity136双向核对，安装`+0x420`内32B descriptor的八个有名字段，不复制padding、不创建活动Grid或伪造source_grid。Grid引用收尾检查真实Entity owner／类型／描述后态。修复“Entity值未变化但Grid描述变化”的漏写：仍安排Fields，不按Entity子树相同跳过跨域描述。 |
| I25 派生draw／缓存 | 主Render按真实vtable槽5分派，覆盖原先错误Effect cast；独立Shadow、Room Light与无参Post保持各自consumer。原生水面间接分派也经过常驻类别Hook。Post按`8061C8`实际顺序，在全部主层／柱尾之后、Light合成之前，对已消费范围执行一次。沿用18的FBO／depth／水面／镜像／柱遮挡顺序。 |

主体仍共用 `ClientObjects`、原生实例表、资源服务、Grid owner和唯一Scene；没有第二套世界或身份。`ClientScene`把已解析RoomGrid T传给对象预检，避免Effect136另起身份。对象／Grid阶段的内部安装完成仍不等于Scene或整个Entity域提交，I29继续闭合其编排与准确范围。

## NPC与Player复合绘制

正式`application_npc`提供三份独立容器：BF0的EntityPtr deque，以及C04、C18两份Vector2 deque。Source复制完整顺序Key／Null和有序点，不发送proxy、map、block或原生地址。本地保留原ctor的proxy，用游戏allocator构建独立map／16B block；引用槽通过官方SetReference维护反向登记，退休逐槽解除后释放本方map／block，不连带Remove独立Key。

835 variant0的前两项引用及真正NPC路径、906／920的子体动画、921的C04曲线与FlipX、951:3的C18完整点列均有具名输入。835非空引用表须至少有两槽；其余variant不进入该路径分支。三点分组消费者先检查完整尾组；未消费的路径不额外施加三点限制。父体直接借子体ANM时，也保护子体屏幕缓存、变换、位置及frame-base，避免只恢复父Sprite。

Familiar68／78／243借用按当前Key、实例和roster证明就绪的Player，保护完整44B持续色、可见性、主／costume ANM及其layers。965:10额外从正式Player同窗口来源取得Minecart关系与`+205C/+2060`两f32偏移（第二个值同时是原生排序key，另传u32位模式保留负零等情况），仅在draw栈临时安装、绘制后归还。没有创建第二个Player，也没有把本轮范围扩成20的全Player／装备应用。

Siren `+F40`是独立heap Player玩法owner，当前NPC主绘制不消费它。本轮保留Authority存在性／查询事实，本地保持null，不把roster身体塞进拥有型字段，不执行其玩法构造／更新。

2026-11-03追查更正：初次实现曾把真正Player的`+C28`当作NPC路径长度，仅接受零值并拒绝整批非零目标。该解释错误：Player ctor `74DCF5..74DD07`在`+758`构造7份、步长`114`的ANM2；`+C28 = +BA8 +80`实际是数组第5项的`ANM2._layerCount`。因此零值也不能证明“空路径”。原生835 helper `55546F..555489`仅在`Game+25B8C >=2`的Deathmatch状态下把Type1纳入NPC数值分支；普通世界只接受Type10..999。已撤掉错误来源字段、模型和两处原生拒绝；真实Player仍按Key保留引用，不读写其NPC私有布局。普通来源、写前预检和整帧绘制共同核对`Game+25B8C <2`，只读模式而不进入／修改Deathmatch；真正NPC路径继续按完整来源安装。研究与主审见[835更正](entity-i20-i25-native.md)。此修复不等于20的完整Player形态／装备实现。

## 表现输入、借用与失败

新增必需合同`entity-class-application-v3`。普通`application_draw`含同窗口Manager／Game frame、pause／subframe及Pickup／Familiar两个inline xorshift的四字状态。整帧只安装一次并连续消费，结束时恢复本地值；不逐实体重置同seed，也不推进本地玩法随机源。相同Z按Authority Index作tie次序，该值不覆盖本地Index或身份。

统一draw保护深借用可能被原生SetFrame修改的ANM选择器，并保存有名显示标量／layers；字符串、Image strong和资源owner保持原所有者。CPU raw队列先在同FBO消费，保留连续depth，再归还选择器。Abyss `647310`含fade、Reset和Update，本用途替代为Source最终ANM的纯Render，原helper主动未调用，不计成原链完成。

常驻Hook全部进入同一清单，新增主／Shadow／Post、暂停查询和Abyss helper分派；Host／Off原样透传，生产不额外开启录制或逐帧日志。Native预检确认实际draw隔离已安装，不能写完才发现缺入口。未知draw／队列异常保留owner和Client隔离，停止展示；不盲目释放在途缓冲或切Off。完整故障收口、恢复及预算优化继续归I28／I30，不能把正常前缀实现外推为SEH／OOM安全。

## 静态研究与主审

按用户指定启用一个GPT-6 Luna max子智能体，形成[静态研究](entity-i20-i25-native.md)。主智能体先读两份本地Wiki和官方源码，再复读关键反编译／vtable／原始指令；采用审阅与修正记在研究文档末节。重点更正Text的Exists字节、LaserShadow的实际单offset ABI、Pickup随机与Bomb lazy重建的归属、921实际消费的C04，以及GridDesc.VarData的i32宽度。

本轮仅做代码／资料生成和人工静态审阅；没有测试结果或tx artifact包。运行覆盖、类别画面、实际成本、Source Text／Dummy自然业务可达性、失败恢复及18回归均留在I34统一验收。未知SourceGrid身份、未见Text／Dummy业务caller等事实原样保留，不用构造能力冒称真实场景已覆盖。
