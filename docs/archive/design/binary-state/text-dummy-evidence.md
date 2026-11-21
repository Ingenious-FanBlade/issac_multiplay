# 第4步：Text／Dummy字节归属与同步职责

日期：2026-11-09（Asia/Singapore）。布局版本44。只读本地Wiki／官方1.1.3 SDK及J273 IDB（ImageBase=0x400000）；下列VA减基址为RVA。没有修改IDB、运行游戏或测试。布局与基础读取接口的实现不表示第10／13步已接线或第15步已验收。

## 类型、边界和生命周期

| 类型 | vtable VA | TypeDescriptor VA／RVA | 原生范围依据 |
|---|---|---|---|
| Entity_Text | B1410C | BD2030／7D2030 | Factory 6EEE60预构造128槽、stride530；删除析构5CC220在5CC237传530给free |
| Entity_Dummy | B181B8 | BD2164／7D2164 | 同Factory预构造128槽、stride410；删除析构6A3590在6A35A7传410给free |

Text ctor `5CC1F0`调用公共ctor `6A3940`，安装专用vtable，构造`+518`的KColor；没有初始化`+418`文本缓冲。Init `5CC260`调用公共Init `6A4A20`，`5CC27F`清`+410` counter，构造／复制KColor20，按variant写`+414` duration：0为90、1为25、2为35、其余1000。Init仍未清缓冲，尾字`+52C—52F`也未取得语义证据。对象大小不证明尾字为填充。

Dummy ctor `6A3570`仅公共ctor加vtable；Init `6A35D0`于`6A35D4`尾跳公共Init。没有`+410`之后的成员。Update `6A35E0`调用公共Update，再衰减公共速度，dead时正常Remove；这些是Host模拟，Client不调用。等大派生只改变动态类型／分派，不增加字节。生成器允许等大父类且子类必须没有额外原生range。

本地Wiki `IsaacDocs/docs/enums/EntityType.md:342,349`分别列964／9001；它们不能代替RTTI。旧[自然来源研究](../tx-04-19/entity-i31-i33-native.md)仍适用：Factory `6EC540`的9001分支取Text，964落默认NPC。Dummy正式Client只可通过真实预构造空槽owner取得，不改vptr／Type、不覆盖活槽、不复活旧use，也不把E2E-only桥声明成自然业务caller。未发现自然Dummy acquisition的事实保持。

## Text逐项决定

| 原生范围 | 原生用途及消费者 | 当前传输 |
|---|---|---|
| 410—413 i32 counter | Update递增；Render仅variant2在5CC723读取参与缩放 | 仅146号Scaled臂TX2的4B；基础145号不发 |
| 414—417 i32 duration | Update中淡出／闪烁／到期Remove | Host；最终颜色与结构Remove分别供应，不做计时锚点 |
| 418—517 char[256] | 当前use writer写入，Font读取首个零以前的原始字节 | TX1：u16 count＋0—255B；不发NUL和其后旧槽内容 |
| 518—527 四f32 | Update／setter最终RGBA，Image真实绘制 | TX0：16B，逐分量有限值 |
| 528—52B u32 color_space | KColor通用颜色空间；此固定Text字体链只传递，不消费 | Host，不发；通用KColor仍完整20B |
| 52C—52F 四字节 | 用途未核 | UNKNOWN，不读、不补零、不发 |

Update `5CC340`在pre-counter=0且variant2时于`5CC38E`按`%d`写串，variant1时于`5CC420`按`%g`写串；随后计算最终颜色／偏移，`5CC514`递增counter，`5CC564`与duration比较后Remove。Render `5CC580`以exists `+172`控制整条Text路径；只读counter的variant2分支与Update边界不同，不能把duration一起发送。

当前use writer资格沿用已核两条来源：上述两个格式化分支正常返回，或`6FF270`完成`%s`写串后虚调Update，其原生返回点`6FF344`。旧`text_writes`的local/use/revision见证不能因切换二进制路径而丢失；第10步须将其移入与Recorder／Lua来源无关的正式owner，Init／复用／退休撤销。`read_requires=text-current-use-writer`是实际来源要求，不是生成器自行建立了资格。仅创建／RTTI／Render不能替代它；不得运行Update为采集补写字符串。

`ReadCStringInto`先逐字节读至NUL，再一次复读相同前缀含NUL，拒绝未终止、地址／缓冲越界、来源变化和失效窗口；不整段读取256B。`StageCStringInto`检查u16长度、实际负载、容量与内嵌NUL，完成全部校验后只向自有暂存区写前缀和一个NUL，支持重叠输入。第13步owner安装同一有效区；空串写一个NUL，缩短不会让旧尾巴复活。字节允许非UTF8，不能改用JSON／文本重编码。

## KColor末字的实际消费复核

旧[Text记录](../tx-04-19/entity-i20-i25-native.md)把“Font传入20B”解释成“末字被绘制消费”，旧opaque语义又已被[第2步证据](base-types-evidence.md)纠正为颜色空间标签。本轮继续追到最终consumer，当前决定仅限固定Text路径，不修改旧记录或通用KColor。

1. Text两分支在`5CC768／5CC841`将FontSettings callback槽写0；无第三方gameplay回调适配。
2. unscaled `9E9500`在`9E9583—9E958E`、scaled `9E9680`在`9E9701—9E970A`确实复制color+10，随后调用`9EA2A0`。这证明传递，不证明使用。
3. `9EA2A0`将RGBA与第五字继续传给`9E9AC0`。后者只有FontSettings callback非空时，才把第五字复制给四份可修改KColor；Text的固定settings不进入此分支。默认在`9E9DFE—9E9E0A`虚调Image槽3。
4. `ImageBase B32054／ImagePlatformBase B3E3E4／ImagePng B3BA80`及Pcx `B3B8EC`、RGBA `B3BC14`、ProceduralImagePlatform `B3BCAC`、ProceduralImageBase `B3E674`的槽3均为`9E2150`。`9E2158—9E2162`只把同一color指针压四次后调槽2 `9E2170`，进入`9E2460`。其颜色分支仅用+0/+4/+8/+C：非预乘在`9E2AF0—9E2B1B`每顶点复制16B；预乘在`9E29A5—9E2AB0`只读RGBA并写16B。没有使用颜色空间标签或执行颜色空间转换。

因此发送最终四分量已覆盖此Text字体消费者；Client用本地KColor包装器的正常标签初值即可，不把Host标签当未知字节／填充，也不增加raw标签writer或漂移比较。任意其他KColor consumer不受此结论影响。

## 绘制上下文与Dummy

Text固定字体是`Manager+4AB84`的Font6；Dummy是`Manager+4AA54`的Font2（SDK Manager.zhl:121），不是Font3。字体／Image本地owner及加载失败归第8／13步，不发送地址。Text Render据公共position／positionOffset和Client视口／渲染offset重建屏幕坐标`+1A4/+1A8`；这两个值不是新的Text尾部状态。

variant2在`5CC6FD`查pause，暂停取subframe=0，否则`5CC70F`读Manager+4A8B8并计算交替子帧；`5CC742`用原生32位`subframe+2*counter`，随后转f32，乘0.5再乘0.25并clamp，得到`size=4-2.5*clamp((subframe+2*counter)/8,0,1)`。第13步须按原生32位运算和浮点顺序复现，不能以C++有符号溢出表达式实现；正常Init／Update非负计数不授权非法来源。Manager帧不等于Game frame／Authority tick；复用同world展示上下文，不为每条Text重复发帧或混入旧独立Render样本。基础臂不用此缩放。

Dummy `6A3630`先调用公共Render `6AC4E0`，再在`6A3667／6A3670`压公共variant `+2C`和type `+28`，按`e%d.%d`生成栈上标签。字体量宽和固定颜色／settings均本地生成，没有持久字符串或额外颜色。只继承公共E0—E9和EntityPlus，不复制标签缓存，也不借Type964选择NPC字段。

**公共屏幕缓存的条件边界仍须处理。** `6AC4E0`只在exists／visible均真时刷新`+1A4/+1A8`；Dummy的后续标签没有自己的同等判断。`415350`主Render分派与`4150C0`排序也不据visible过滤。因此不能声称所有Dummy标签位置都能无条件由当前position派生。正常可见分支由公共Screen公式覆盖；跳过公共Render时的历史缓存依赖由第13步具名draw owner闭合，缺少实际来源时必须报Gap，不重画为当前位置、把本地旧cache冒充Host或宣称画面完整。此处是已定位的公共展示依赖，非虚构Dummy尾部字段；本轮未将未核跨视口／历史来源贸然加为8B状态。第15步须覆盖visible切换及Baseline进入不可见对象。Dummy自然来源与该分支运行覆盖均未取得。

## 交付与未决

唯一源为`mod/binding/state/entity-text-dummy.json`。145／146／147分别为Text、Text_Scaled、Dummy，继承公共块不重排；TX2在基础臂显式预留。旧目录NativeText五个唯一ReplicatedState成员有明确回映射；NativeDummy没有专用成员owner，不能伪造一个目录owner。

| 布局 | 公共最小状态 | 专用状态 | 最小—最大负载下界口径 | 根未知 |
|---|---|---|---|---|
| Text145 | 391B | RGBA16＋count2＋0—255字节 | 公共动态部分取最短时409—664B | 339B＝公共335＋尾字4 |
| Scaled146 | 391B | 上述内容＋counter4 | 同口径413—668B | 339B |
| Dummy147 | 391B | 无 | 391B最小，不是完整帧上界 | 335B |

上述不是带宽实测，也不是任意资源／图层数量下的全对象上界。未知字节、Dummy缓存依赖和自然来源缺口不因布局已生成而消失。正式采集／写入／展示／旧链退役仍分别在第10／12／13／14步；第15步为唯一测试。本轮生成时首次声明NativeDummy公开owner被生成器拒绝（旧目录无此owner），已改为空owner；没有修改旧目录去伪造成员。
