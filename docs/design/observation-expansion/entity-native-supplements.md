# tx-04-10 未公开类别的只读补充

目标为 J273，IDA imagebase 为 `0x400000`。这是静态来源说明，不代替实机覆盖。

| 类别 | 核定位置 | 采集内容与限制 |
| --- | --- | --- |
| Entity_Text | TD RVA `0x7D2030`，COL `0x7434E0`，vtable `0x71410C`；构造 `0x1CC1F0`，Init `0x1CC260`，更新 `0x1CC340`，绘制 `0x1CC580` | Entity 公共公开状态；创建 owner 正常完成后读取 `+0x410` 更新计数、`+0x414` 生命周期长度，`+0x418` 的至多 256 字节以零结尾文本。逐字节读到终止符即停止，不读取终止符后的未初始化空间。无终止符报告 Truncated。文本专用颜色的私有表示不推测，由公共颜色与实际可读外观分别表达。 |
| Entity_Dummy | TD RVA `0x7D2164`，COL `0x7439B4`，vtable `0x7181B8`；构造 `0x2A3570`，更新 `0x2A35E0`，绘制 `0x2A3630` | 单独标识 NativeDummy，采集 Entity 公共公开状态；没有把它识别为 NPC。构造只调用公共基类构造，已读更新使用公共速度及死亡状态，没有添加猜测的派生字段。 |
| HomingLaser | SDK `IsaacRepentance.h` 的 HomingLaser，EntityLaser Wiki 暴露不透明成员 | 不调用 CalculateSamples；曲线使用公开 GetSamples/GetNonOptimizedSamples 的当前集合，并记录不透明 owner-qualified 关联。未公开私有 Bezier 内存不解释为 Lua 可观测字段。 |

两类 RTTI 的 CHD 均为非虚单继承、两个 BCD：本体及 Entity，PMD 为 `0/-1/0`，attributes 为 `0x40`。EntityFactory::Create `0x2EC540` 的 Type 9001 分支使用 Text 池，步长 1328 字节；不能因它大于 1000 就当作 Effect。Dummy 是否在具体场景出现保留实际观察结果，不制造出生历史。

点位统一保存在 `mod/binding/world_layout.json`，由既有生成器提供 Constants。Lua 仅通过主线程窄适配取得复制值；FIRST_OBSERVED Text 没有专用字段读取资格，记录 NotCaptured，不能借分类成功补造 Init。

## Sprite 名字与真实返回值

J273 `AnimationState::GetNullFrame` VA `0x406620` 使用 ANM2 的 `+0x84/+0x88` 名字表；`IsEventTriggered` VA `0x406710`、`WasEventTriggered` VA `0x4067B0` 使用 `+0x8C/+0x90`。名字为 24 字节 MSVC string（长度 +16、容量 +20，小串阈值 16）。适配只复制已有名字、复核描述符，再调用公开当前姿态 Getter；无 AnimationData 时不调用会进入原生 fatal logger 的接口。表与字符串均有预算，名字缺口不宣称完整。

LootList Getter 会实际生成掉落，即使 false 恢复 RNG 也可能执行回调，故不能主动轮询。三个统一 Hook 仅在真实调用返回后复制自有返回值，super 各一次：Pickup GetLootList RVA `0x2D8700`、NPC fireplace_get_loot `0x2CD630`、shopkeeper_get_loot `0x2BDC70`。使用官方 SDK 的返回 ABI 与注册项；记录 Type/Variant/SubType/Seed，RNG 仅给 tx-04-14 的显式关联缺口。来源为官方 ZHL 签名、LuaLootList.cpp、CustomCallbacks.cpp 与目标静态反编译。

第二次 E2E 的类型缺口进一步核定两项 J273 差异：GetBombFlags 的 Lua wrapper VA `0x8942E0` 返回 bitset128，实际函数 VA `0x779880` 在 Fruit Cake 路径即使 false 也调用随机函数，因此从普通/显式只读查询移除，目录标为真实玩法调用；不靠 Wiki 的 int 类型或 false 参数推断纯读。SampleLaser 属性 getter VA `0x8A6470` 返回 signed-char userdata，公开 IsSampleLaser VA `0x7DF700` 明确读取同一 +1117 字节并比较非零，因此二者合为 `EntityLaser.SampleLaser` 布尔规范字段，通过 IsSampleLaser 读取，不解释不透明 userdata。
