# I26—I30 必要静态研究及主审

日期：2026-11-04。目标：REPENTOGON1.1.3 / J273。一个 GPT-6 Luna max 子智能体只读研究，主智能体独立复读实际采用点位并补证；没有运行游戏/构建/测试，没有修改 IDB或校验哈希。以下 VA 以0x400000为基址，RVA为 VA减基址；不是运行观察。

## 资料顺序

先读现行规范及20的 C/P/W/R 清单。原版本地 Wiki 的 EntityPlayer、PlayerType、CollectibleType、PoopSpellType；REP本地 Wiki 的 EntityPlayer、History、BagOfCraftingPickup；再读官方 `EntityPlayer.zhl`、`AfterImageFrame.zhl`、`ANM2.zhl`、`AnimationData.zhl`、`KAGE_Graphics_Image.zhl` 与 `LuaEntityPlayer.cpp`。未公开的槽关联、私有最终量、draw caller/临时写才使用只读 IDA。

官方 wrappers 直接给出 crafting8/output/pool、poop6、16个 inventory History 下标。公开 crafting setter 会使 HUD 缓存失效并重算配方；RemovePoopSpell 会随机补队列；History mutation 会维护外部下标/HUD。因此采用最终存储安装，不重放这些动作。

## 实际采用

| 点位 | 子智能体结论及主审采用 |
| --- | --- |
| 7A3A40、7A3E73、7A3E8C | 703 descriptor VarData=0扫描 PlayerManager+80 四槽，写新备份并把slot+1写回。零合法，1—4为槽关联；不按参与者猜槽。不同 descriptor 不在此做跨槽去重，不能凭相同道具号推断共享。Client仅自有镜像槽，不写原生Authority槽或调用此创建链。 |
| 74DC70、74E5F1—74E606 | 16个 inventory History 下标有真实 ctor 初始化；+1E38残影 deque 真实构造。AfterImageFrame为 duration+Vector、12字节，不能复制 deque 环或地址。 |
| 74F6A0 / Init | 主审复读：+1F20八个 crafting字节归零，+1F28 output=0、pool=-1；+1F34六个poop槽归零，+1F54/+1F58独立归零。+14E8..14FC六个scale写入真实默认值，full-sprite+1398/+139A初始化。采用同use合法普通窗口当前量，不向未取得读取资格的备份借用另一身体见证。 |
| 84FC39—84FC4C、75187A/751884、7A68D8—7A68EA | 原生 Lua scalar descriptor type1分别注册 IBSCharge+2074、SamsonBerserkCharge+2070；Init归零，Restore分别从+5AC/+5B0复制。主审逐指令复读偏移与writer，按i32安装/回读并核对同窗口公开属性。 |
| 7CFAB0、769209—769212 | Samson+2070累加/阈值归零属于玩法，Client不调用；角色表现更新读取它。Source复制最终量及已产生的效果/武器/ANM。 |
| 758850、758D50、789E50 | 主审否决把+1F54称作Samson充能的初步猜测：ItemState710分支使用它作为crafting使用计时。充能条数组为+758起七个ANM，步长114；758D50只做动画选择/设帧/Play，没有采用Update/动画事件。另补18A8/1EC0/1EC4三个条计时器。 |
| 7518D2—7518E5、75193D—751949；74F260 | Init给+1FF8安装gfx/shadow.png、+2000安装gfx/ui/coop menu.png；ctor将两strong置空，Free真实释放。784C5E多人路径会解引用前者。实现从Source复制两图实际身份/格式/采样参数并本地加载，不能只放行ctor+base Init，也不调用完整Player Init。 |
| 429440、4294C5、784CE6 | 主审复读：ECX为Game+1BAA8；按其中四个controller值选palette并写11个float，ret8，无玩法/随机。Source保存最终ColorMod；Client Render窄查询覆写该颜色，ControllerIndex保持-1。 |
| B1BDB4 vtable+14/+18/+1C | 主审读取：Render=784A60；Shadow=7862A0，查HasCollectible433再返回true或进入base6AD450；Post=base6AC660。保留EntityList::RenderShadows caller共享buffer及18/19的light/post/水面/镜像/深度序列，不孤立调用Shadow。 |
| 783680、786460、786760、7868A0、786900、7862E0 | Body/Head/Glow/Top按真实layer map/costume/完整ANM及最终flags/scales绘制；783680的29/38备份分支会复制、Swap、Reset资源并临时修改另一身体，不能用轻量selector恢复假定闭包。 |
| 402CD0、403160、403790 | 主审纠正先前“thiscall copy assignment”猜测：403160实际fastcall Swap，ECX/EDX两对象、无栈参；双向交换资源/值，保留AnimationState self，4034CA/4034EF重绑两侧Layer parent。整帧真实copy ctor保留strong，CPU drain后Swap回原对象并析构工作副本。 |
| 783680、BEF6A0/A4/A8/AC | 子智能体查代码xref仅回783680，是hologram专用视觉RNG；采用独立纯表现噪声，不冒称普通采集等于某次Render随机结果。主审所读路径没有时钟API或ANM Update。412DD0为flags检查，不是随机。 |
| 82E1C0、82E340、82E740、786048—786066 | 子智能体纠正+1C48“资源指针”初读：Player+1C44为list/head/count/cursor/内嵌Sprite，+1C48为count。主审复读ctor分配40字节sentinel，Sprite在+C，cursor初始=head；Render读当前node位置及+18/+1C、+20/+24两对动画/帧。析构只沿next free节点及head。 |

## 主审补齐的可选序列合同

不能把+1C44归为“仅某角色/录像API”而跳过，也不能因为当前样本为空而将其永久清零。正式来源读取完整有界节点顺序，检查双向链/count/cursor，将cursor换成纯值下标；只复制原生draw使用的位置和两对动画/frame，另复制+1C50的完整独立ANM姿态。节点data+8/+C的推进语义没有核定，不传远端字节；Client停止玩法Update，这两个本机word显式中性化。+1B20的另一序列为Authority更新源，不由Client重做其copy/推进。

原生安装使用同布局的本机Release list节点（节点40字节，值32字节，list头8字节），swap完整列表并重建真实cursor，不伪造仅count非零的空链；空T清旧项。安装后从实际list/位置/动画/frame/cursor/ANM回读，动画索引还与本机实际资源核对。82E340的SetFrame/Render仍走原生消费者，Sprite临时变化纳入整帧强借用。原生Player析构仍为唯一释放owner。

此处是当前draw读集合同，不声称已理解每种历史record玩法的完整Update/save协议。那部分不在Client重演；如后续消费者新增对未解释字段的依赖，须补具名来源/处置，不能靠零值兜底。

## 保留的未知与范围

Authority Esau Jr退出后全局四槽的完整回收链仍没有由本轮证明；保留09既有边界，不清Authority槽。Client镜像对象已有自己的唯一owner/链接撤销/资源析构，不沿用“因此永久泄漏”的假设。原生异常前缀的全覆盖/可恢复性由I33实现和I39实际验收确认；本轮静态工作不报告其运行通过。完整HUD/Mod/独立事件仍保持既定I31及27责任。
