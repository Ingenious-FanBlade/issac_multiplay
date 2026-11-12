# 第8步：资源、屏幕与转场的必要来源

**2026-11-11 当前网络职责修订**：[本地生成合同](local-media-presentation.md)替代本文旧文字图／前后surface／Host hallucination图下行设计：分别采用文字与样式、本阶段直接切房、本端历史合成。原生对象、读取点、RGBA8事实与UNKNOWN仍保留；发现原生Image不证明必须传像素。当前不为正式同步执行纹理读回／音频内容采集，特殊程序内容改核本地资源与有界描述；运行实现尚待迁移。

**2026-11-10同步职责复核**：[53项来源的当前同步决定](presentation-source-synchronization-review.md)及[逐项JSON](presentation-source-synchronization-review.json)优先解释本文“来源”的网络职责。采用最终有序贡献后，不再同时发Nightmare路线14值／场景帧／Dogma等生产输入；Run／D0与本地尺寸可派生的量不重复。资源按用途引用、按需声明，recipe／精确内容择一；原生后端、个人Options和隔离内容不上线。账本修订5仅改职责，50视图及UNKNOWN范围不变，生产版本57，未构建或测试。

2026-11-10；目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273 Win32。按本轮用户要求，先查本地 Wiki，再查官方 SDK／源码，只有具体来源、ABI或副作用未被公开资料解释时才只读 J273 IDB。本文补齐**正常 Host 已采用资源和最终显示结果的静态来源合同**；机器目录见[来源表](presentation-resource-screen-sources.json)，原生字节修订见[账本修订3](presentation-byte-ledger.md)。

本文不是生产布局源。生产 layout_version 保持57，`state/presentation.json`、Hook、结果缓存、完整屏幕 recipe、读回／上传执行器和Client adapter仍待第8—13步实施。本轮没有构建、游戏运行或测试，没有修改或保存IDB。下文地址均为VA，RVA=VA−0x400000；源码／指令核定不等于运行观察。

## 1. 公开资料先行：可以直接采用什么

| 本地资料 | 能确定的事实 | 本轮处理 |
|---|---|---|
| `IsaacDocs/docs/Game.md` 的 Fadein／Fadeout／ShowHallucination／StartStageTransition | Fadein有ShowIcon和颜色，Fadeout含流程target；StartStageTransition原Lua binder还有栈损坏警告 | 来源是已经产生的颜色、图像和pass；正式reader不调用这些命令或Lua binder |
| `REPENTOGON-1.1.3/docs/docs/StageTransition.md`、`NightmareScene.md`、相应ZHL与Lua接口 | StageTransition主要配置流程；画面内容属于Manager内嵌NightmareScene。公开接口给四Sprite、14项路线和Dogma判断，并未覆盖全部条件内容 | 从Manager+21498绑定真实Nightmare owner；四个公开Sprite不足以代表整张换层画面 |
| `RoomTransition.md`／`RoomTransition.zhl`／`LuaRoomTransition.cpp` | versus Sprite和按layerID的额外肖像map；StartBossIntro是生产命令 | 展示时读取当前资源／姿态及最终变换，Client不StartBossIntro或ChangeRoom |
| `renderer/Image.md`、`Renderer.md`、`SurfaceRenderController.md`、Image／ImageManager／GraphicsManager的ZHL | 逻辑尺寸与padded尺寸不同；RenderToImage的controller只在本次绘制内有效；CreateImage和surface可以作目标 | 保存自有内容或完整纯值recipe，不发Image*/GL/FBO/controller；资源操作在图形主线程边界完成 |
| `Sprite.md`／`LayerState.md`、`Anm2Extras.h/.cpp` | default／layer、普通／champion四类选择；逐层有效shader优先，失败时回退default，再回退内置shader | 采用实际生效的内置shader身份；第三方自定义shader旁表ABI仍留在Mod后续范围，不能由Has布尔反推路径 |
| `renderer/Shader.md`、`enums/ShaderType.md`、`Global.zhl`、`Utils/{ShaderUtils,ImageUtils}.hpp` | 内置shader有固定类型；顶点格式属于具体shader；SubmitQuadForShader会临时更换图像格式并恢复 | 后台只接固定语义投影；不发送任意stride／属性字典或把指针当shader编号 |
| `menus/PauseMenu.md`／`PauseScreen.zhl`／`LuaPauseMenu.cpp` | 本地菜单状态／选择和四个公开Sprite，不提供完整REP+尾部布局 | 仅使用已核的共享run显示数据；本地菜单、Online密码及访客进度保持各自职责 |
| `ProceduralItem{,Manager}.md`及对应ZHL | 程序道具是原版内容，CreateProceduralItem有生产行为 | Nightmare的高道具ID分支必须承接实际程序道具Sprite／图像，不能因迁出第三方Mod而删除 |

**Image Wiki与实现的格式差异必须保留。** 本地Image文档把GetTexelRegion返回值描述为32位float RGBA；但官方`LuaRender.cpp::Lua_Image_GetTexelRegion`实际只分配`width*height*4`字节，J273 `A4C863/A4C868/A4C879`分别提交`GL_UNSIGNED_BYTE(0x1401)`、`GL_RGBA(0x1908)`和glReadPixels。目标版这条surface读回路径为**RGBA8，每像素4B**，不按16B浮点像素解释。该结论不证明任意派生Image／浮点surface均可无损按RGBA8表示。

## 2. 图像身份、内容来源和时点

资源规则复用[第7步typed资源合同](room-display-resource-design.md#4-图像内容资源表和获取时点)，不建立第二套表或像素格式。

| 用途 | 合法原生来源 | 复制／物化时点与负载 |
|---|---|---|
| 文件图像 | 正常`931B30`加载调用的路径及返回strong pair；LayerState当前`+98`实际Image选择 | 记录正常成功加载的本地关联；传稳定文件资源及实际选择，路径元数据独立。路径先变而Image未变时保留旧图。已有对象缺少可证明文件来源时不能只用`_name`补身份 |
| 程序图／surface | 正常`9EF4F0`创建返回的pair，后续真正完成的目标写入；官方CreateProceduralImage_Internal是SDK封装入口 | producer／用途／代次／整数内容版本绑定自有内容。相同`.procedural`名字可对应不同内容；normal writer有更新便产生新不可变内容版本 |
| 11份HUD消息 | HUD+3CFC一份、+3EF0六份、+4AA8四份，每份步长1F4；各`HUDMessage+18`实际文字图像 | `831AD0`正常UpdateTextImage的`831D70 Present → 831D7A Restore`完成后封存。主／副文字、特殊首码、不同字体和未知样式不再靠重新排版猜等价；直接使用已完成图像内容 |
| fortune及其他最终HUD图 | HUD+538C实际fortune Image；PlayerHUD+6A4有序图像、+6B4合成图像；各用途仍由原HUD owner管理 | 正常生产已完成、图像绑定稳定时复制内容／资源引用；本轮不新增通用Getter字典。确切HUD批次、身体与槽位编排另由Presentation正式布局承接 |
| hallucination反馈 | Game+25A98当前Image strong、独立全局`BE72B4` Hallucination Snapshot | 这是两个不同用途。前者在`6F8C4F/6F8C88`被实际选取，后者在`6F7EC8/6F8408`被消费。各自封存实际已完成内容和版本，不以名字、当前world或另一次Render替代 |
| 切房前／后surface | RoomTransition+28／+30的实际strong pair，`827970—8279E3`／`827A81—827AA8`读取 | 正常Host完成对应目标写入并消费CPU队列后物化；两份分别保持内容版本和结构关联。只承接纯world用途内容，个人菜单／Online／画质混入和跨分辨率映射需按复核补齐；Client不为获得旧房图像重新加载／模拟旧房 |

`A4C840`只提供读回依据，不是可在任意Hook调用的承诺。图形主线程必须先完成正常生产及相关CPU提交，并保存／恢复FBO、viewport／scissor、projection／transform、shader／blend／sampler、pack/unpack／PBO、GraphicsManager镜像及strong引用；不能对Host主画面额外Clear／Present或改变正常调用次数。后台仅持有封存的自有字节。

已有完整已核recipe时优先复用；错过生产历史的冷连接可从当前仍有效内容物化。没有完整recipe或精确内容时报告具体来源Gap；明确Null、内容未准备、失败与缺来源分开。像素、表项和recipe均计入既有首次准备与持续带宽预算，不能以Context通道隐藏成本；本轮没有性能结论。

## 3. shader：本轮原版来源与仍未核的旁表

J273 `67DA3C`命中官方`g_AllShaders`签名，实际指针表VA **AACAB0**。本轮直接读该静态表，与官方ShaderType逐项对应：

| ID | 内置类型 | J273对象VA | 本轮必要消费者 |
|---|---|---|---|
| 0 | COLOR_OFFSET | BAB44C | 普通图像／Sprite；hallucination反馈叠加 |
| 1 | PIXELATION | BAB528 | 转场像素化；ScreenSize为逻辑＋padded四值 |
| 2 | BLOOM | BAB4FC | `6F848E—6F8645`，BloomAmount及Ratio |
| 3 | COLOR_CORRECTION | BAB3C8 | 内置资源身份，具体使用仍按具名pass |
| 4 | HQ4X | BAB39C | 内置资源身份，不因本地画质选择复制Host开关 |
| 5 | SHOCKWAVE | BAB3F4 | 内置资源身份；Room用途沿第7步 |
| 6 | OLDTV | BAB344 | `6F8DC1—6F8ECA`，Time及ScreenSize |
| 7 | WATER | BAB210 | Room唯一owner |
| 8 | HALLUCINATION | BAA7AC | `6F89A7—6F8AEA`，Amount及ScreenSize |
| 9 | COLOR_MOD | BAB2EC | `6F7F71—6F80BC`，最终RGB＋Amount／Brightness／Contrast |
| 10 | COLOR_OFFSET_CHAMPION | BAB554 | 官方Champion格式，仅四个champion tint值 |
| 11—19 | WATER_V2／BACKGROUND／WATER_OVERLAY／UNK／DOGMA／GOLD／DIZZY／HEAT_WAVE／MIRROR | 依次BAB318／BAB23C／BAB2C0／BAB4A4／BAB370／BAB268／BAB420／BAB478／BAB294 | 固定官方枚举身份；本轮不据此宣称所有用途adapter已完成 |

这是本地**对象→固定资源类型**映射，地址只用于当前进程的暂时核对，线上使用shader-table资源引用。Solid颜色提交`9E8A50`另选`BAE1D8` basic shader，不属于上面20项，不能硬塞为COLOR_OFFSET；其固定SolidColor pass直接由本地已核入口承接。

内置格式证据来自初始化`6EFB70`所用descriptor：`B88448`=Position／Color／TexCoord／BloomAmount／Ratio；`B88538`=Position／TexCoord／PixelationAmount／ScreenSize；`B88510`=Position／TexCoord／Time／ScreenSize；`B886A0`=Position／TexCoord／Amount／ScreenSize；`B885C8`=Position／Color／TexCoord／Amount／Brightness／Contrast。ScreenSize保留逻辑与padded尺寸的区别。ColorOffset额外的PixelationAmount与ClipPlane见官方ShaderUtils，属于具体提交时的上下文，不能只保存Image默认shader或在临时格式恢复后读一次就当最终结果。

官方源码中SetCustomShader的外部调用来自LuaSprite的Sprite／Layer setter，ShaderLoader另有Lua Renderer入口；它从Mod资源加载自定义shader，旁表无公开路径枚举API。**当前原版范围采用内置资源和实际固定pass；不要求反解第三方Mod的STL旁表才能推进。** 旁表物理ABI仍为UNKNOWN，不被改写成“已核”或无条件空。若受支持原版显示出现未能映射的实际shader，必须保留具名Gap，不能自动降级或以旧Lua路径tracker作为正式二进制来源。原版新增的固定shader使用仍须单独核定；任意Mod shader归后续适配。

## 4. 屏幕pass的具体结果来源

只捕获正常Host已经形成的具名值，或在已核分支上从同窗原生值作确定纯值投影；不额外执行Render／getter／Lua／随机／Update。外层保留实际pass顺序及资源依赖，固定数量小记录在第8/9步注册；本文不发送顶点缓存或GPU命令。

| 结果 | 正面来源／实际消费者 | 当前同步决定 |
|---|---|---|
| current ColorMod | Game+66920六值；`6F7F76—6F80BC`设置RGB、Amount／Brightness／Contrast | 复用Run既有六值和world版本，不重复传target／lerp |
| BloomAmount | `6F8589—6F85E4`从Game+25AA0／AA4算包络，再乘+25AA8；`6F85FF—6F8645`写四顶点最终强度与Ratio | 取最终f32强度，Ratio按Client资源padded尺寸派生；不下发倒计时。Host关闭画质而跳过提交时，仍可由这个已核纯算式取得world意图，不能编码为world关闭 |
| PixelationAmount | `6F86E4—6F8783`分别取得Room／Stage当前有效进度，`6F87F3—6F8820`形成`max(max(room_progress,stage_progress)*0.3,0.0001)` | 保存正常最终有效性／强度；不在Client执行两套转场FSM。ScreenSize按本端同批资源建立 |
| Hallucination Amount | `6F89DE—6F8A1E`正常包络：开始／结束10帧渐变，中段1；`6F8A33`写最终值 | 只发当前结果和必要实际图像依赖。反馈翻转由ManagerFrame%4选择，复用D0派生，不再传独立selector；`6F8C2A`的SourceQuad仍含pixel／UV空间 |
| Hallucination Snapshot叠加 | `6F7E92—6F7EF1`算正弦alpha，`6F7F00`提交BE72B4；另有`6F8425`白色完整叠加 | 这两次用途分别保留alpha／顺序；不合并成一个“最后alpha”，不额外执行ShowHallucination |
| OldTV | Run七项种子显示投影中的seed8资格；`6F8E18`写`GameFrame*0.05` | 资格复用Run显示投影，时间来自已应用world帧；不新增墙钟或每帧游标 |
| Retrovision | `80454F—8045E0`检查seed78或原生剩余／总时长，`8045EE`把最终f32写BE7704 | 在Room正常pass局部上下文中复制最终值；BE7704是临时显示输出，不作为会话全局常驻源。Client不运行Room整段Render或计时 |
| Glitch／clip上下文 | Room `8045F6—804612`的effect481结果参与本次glitch；官方ShaderUtils在提交时读clip／pixelation全局 | Room开关与逐Sprite最终选择沿第6/7步及结果批次交接；普通／冠军shader格式保持，不额外推进随机或按本地ANM2地址生成结果 |
| Fade primary／secondary | Game+25B04／25B1C两组RGBA；`6F8FFD`primary优先，否则`6F90B5`选secondary；`9E8A50`是solid颜色leaf | 发最终选中pass及颜色／顺序，不同时叠加两组；+25B18的后续调用另拆只读图标显示，不执行退出target |
| Fade加载插画 | `6F90A6 → 92B8B0`；后者按ManagerFrame%16从Manager+4A9F4／+4A9FC实际pair选择，在`92B914`取strong；新增92B5C0文件名补证见同步复核 | 仅共享world Fade承接当前实际图像＋alpha，中心位置本地派生；原生加载耗时／timeGetTime和个人等待UI不下行，没有图像与缺来源分开 |
| 白色整屏覆盖 | Game+669EC，`6F8F20—6F8FBE`形成`clamp(value/90,0,1)` | 发当前最终alpha，不运行其生产／转场逻辑 |

原生某处的zero-alpha cull和返回空vertex不代表world效果不存在；合法无可见输出与缺源分别表达。Host图形选项、Client图形选项、world意图各有职责。跨pass借用的颜色、shader、Sprite临时变换必须在CPU消费后按已核顺序恢复；不能把整个`6F7A80`当安全Client入口。

Bloom纯值投影的具体算式由`6F8589—6F85E4`给出：令`t=Game+25AA0`、`T=Game+25AA4`、`strength=Game+25AA8`，有效分支为`sin((1-abs(t-T*0.5)/(T*0.5))*pi*0.5)*strength`，按原f32顺序保留舍入；T无效时报告来源Gap，不执行除零或额外调用原生链。正常已产生的最终结果仍优先封存，算式不启动倒计时／Host生产，也不把画质Options变成world资格。

## 5. RoomTransition与Nightmare的内容来源

### 5.1 切房surface、versus与额外肖像

`8273D0`只处理mode2／3，`8273F0`分出boss intro。普通滑动在`8277D4—82780F`取+0C的s，形成`(3-2s)*s*s`，再由direction和+22C/+230及本地视口得到两个实际位移；`8279CE/8279E3`分别消费+28/+30图像与各自alpha。非滑动路径在`827A81/827A96`选其一，`827B30—827BB6`再产生遮罩色／alpha。当前最终贡献路线只传实际图像／遮罩、必要位移／alpha及顺序；原mode／direction／style／s等生产输入不再与结果同发。目标roomIndex／dimension由结构事务拥有，不变成显示命令。

boss intro先按versus当前动画的真实有序layer绘制。`8274F6—827548`确认+224 map中当前layer已有额外Sprite且loaded；随后`827618—8276E0`从该层当前帧复制完整颜色、rotation、scale和位置，`82771E—827741`提交主／叠加姿态。**在这些正常变换已完成、实际提交前复制本次结果**；事后只读map最后一个Sprite状态会漏同帧多层调用。cos／sin为确定几何，不新增随机源；Client不调用会隐式插入的map helper补成员。

### 5.2 Nightmare条件队列不再是无名payload

SDK给出的Manager+21498、大小5EC及四个公开Sprite保持。补证来源为正常Show `8FBE00`和Render `8FAC20`：

- `+360`为Host选中配置条目的索引，`8FBE7B—8FBE9A`实际用于从配置字符串集合取项，留Host。
- `+4C0..4D3`是目标版20B队列头：proxy／map／map_size／offset／size。`8FC0DA—8FC151`交换此完整头，`8FC8DE—8FC943`扩容并分配32B元素、复制24B字符串和末尾两指针；每个32B块只有一个元素。它是Host保存的显示生产输入，不复制原生队列／地址。
- Render `8FAF1A—8FAF87`只在对应bubble／内容／非空资格下取队首，读取末尾两个实际配置／实体来源。固定四个slot表VA BD0EF0：Item、Item2使用kind6、偏移(+84,-20)和索引0/1；Boss、Boss2使用kind3、偏移(-21,+30)和索引0/1。表是正常画面选择事实，不是四个网络实体。
- 普通配置结果在`8FB0E3—8FB178`安装实际layer状态／帧并提交；NPC特殊Sprite在`8FB093—8FB0D6`正常选动画、颜色、scale后提交；item ID≥733在`8FB01A`进入`8D5FD0`，其`8D6021—8D60E5`绘制实际程序道具Sprite，未loaded分支`8D6100—8D6189`选择既有Default占位层。原版程序道具也必须保留此分支。
- 因而正式结果是**本次真正可见的有序Sprite／图像贡献**，每项绑定Nightmare显示代次、slot／layer／主叠加身份、实际资源及完整姿态；不是发送两个配置指针，也不调用Show重抽。同步复核进一步明确：Connector／Levels／指示器等全部贡献已承接后，14项路线、Scene帧、Dogma／bubble标记留Host，不再重复下发；辅助Sprite实际贡献和通用ANM2仍有消费者的插值字段保持。Scene帧也不能用GameFrame伪造。

这解决必要消费者的来源，不宣称所有保存体／实体专用数据或队列全部历史语义已知。`+8..1F`等剩余UNKNOWN继续保留。冻结最终贡献是在正常Host绘制路径中观察，不能因某个slot未被绘制就提前执行它或给其补零。

### 5.3 新识别的BossOverlay

Game `[1D294,1D4C8)`的234B不是旧GenericPopup。`6F0F76`向它加载`bossoverlays.xml`，`41C3D0`读取`bossoverlay/anm2root/entry`；构造为`+0 active`、`+4 selected_entry`、`+8 ANM2`、`+11C`起五份38B定义。定义是id、两份24B字符串和persist布尔，末尾3B仍UNKNOWN。

`41CD30`选择0..4中的定义并加载当前Sprite；`41CC80`只在active且Sprite有效时按本地480×270比例绘制。Client承接**当前存在性及当前实际Sprite／图层资源／姿态**，本地重建视口比例，不复制五份生产配置或重放选择命令。`41CB40`的entry3完成分支会改进度／音乐并ChangeRoom，严禁作为Client显示维护放行。此处新增字节表不等于整个Boss流程已接线。

## 6. 两处旧UNKNOWN的范围纠正

1. **Game模式4虚拟owner是原版Deathmatch。** `6F7AE6/6F8110`按Game+25B8C==4调用+25B98的vtable+14／+18；构造中的`4257A0`按类型建立`IsaacRepentancePlus::DeathmatchType{Normal,Ipecac,Bomberman,Ludovico,...}`，析构`6EF88F—6EF8A1`退休它。这里只读静态来源；按既有C/S与原版Online隔离，此owner及其绘制不纳入本轮原版C/S必要显示，不新建死亡竞赛adapter，也不把它继续写成必须补齐的未知运行模式。具体派生对象内部ABI未核，不改成已知。
2. **PauseScreen+E04是Online密码显示切换。** `98DF32/98DF69`控制ShowPassword／ShowPasswordKey层，`98E23D/98E2B9`分派PasswordButton／PasswordPos；正常Update `98D1A3—98D1AD`在相应输入分支切换它。此1B留本地并排除原版Online来源。原SDK仍只声明DF4；不能由这个字节推导整个`Game[24430,25444)`均无显示需求。账本只拆出已核1B，其余段UNKNOWN照留；本地暂停菜单需要的Host seed／daily／challenge／stats由独立只读run显示结果承接。

## 7. 交付、失败方式与后续接线

本轮交付为本文、机器来源目录、账本修订3及第8/9/10/13步的引用交接。旧45类型账本、未知及来源缺口在修订历史中保存；新增BossOverlay／定义、Nightmare队列头／32B条目四类型，Nightmare直属UNKNOWN由61B降为37B（配置索引4B＋队列头20B）。BossOverlay直属0B、定义各3B未知；其他原生孔、派生Image大小和自定义shader STL ABI不由本轮消除。生产布局版本仍57。

后续实施须处理的失败方式：

- 源图名字重名、路径已变而实际Image仍旧、Null与Wait／Gap混淆；surface或程序道具没有内容仍ACK。
- 把RGBA8误读为float像素、漏padded范围、读回时批次未消费／PBO污染，或更改Host的正常图形状态。
- 仅取函数返回后的最后Sprite／alpha，漏多个layer、两次snapshot叠加、主／叠加或重复调用。
- world与presentation／资源代次错配，晚连接重放旧Show／Start或从Client重抽；Host画质跳过分支被误当world关闭。
- 把BossOverlay Update、完整Nightmare／Game Render、Fadeout target或StageTransition setter当显示应用，造成进度、切房或随机副作用。
- 把Deathmatch或Online密码混入C/S；以当前不支持Mod为由删除原版程序道具。

第8步正式布局承接固定结果投影；第9步登记结果批次与资源表编码、固定shader允许集及预算；第10步只在上述正常生产／消费点封存自有内容；第12/13步有界解码、图形物化与窄显示adapter；第14步退役旧Lua getter／shader tracker／JSON正式来源。未知shader、未完成内容、未核特殊格式继续报具体Gap。

唯一功能验收仍在第15步：覆盖消息／fortune程序图、前后房间surface和滑动／遮罩／versus多肖像、Nightmare普通／Dogma／条件Boss及程序道具、BossOverlay、bloom／pixelation／hallucination反馈／oldTV／retrovision／fade图标、各端独立视口／图形选项、冷连接内容恢复、借用与图形状态恢复以及资源预算。本轮没有执行这些测试，不据静态来源宣称已可运行。
