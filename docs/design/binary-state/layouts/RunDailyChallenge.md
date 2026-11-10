# RunDailyChallenge

由唯一布局源生成。布局编号 233；布局版本 56；原生大小 0xB4；未知 22B；完整：否。

Game+25BA8的副本，不能拿Manager::GetDailyChallenge()替代；只发送1B is_daily，不发完整id，其他已知字段Host。

原生字节归属：

| 0x0 u32 is_daily【换算】原生u32 id仅投影(id!=0)的1B显示布尔；97C300/98DDA0只消费非零判断。实际日期/名称若显示由Presentation当前run批次承接，不复制日期用于Init/排行榜（4B；已核） | 0x4 u32 seed【权威】Host每日世界生成种子；Client接收当前世界，不按日期Init（4B；SDK声明） | 0x8 bool practice【权威】每日练习／提交资格输入；最终标识若需显示由Presentation承接，不改变访客权限（1B；SDK声明） | 0x9 bytes[3] unknown_9【未知】尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（3B；未知） | 0xC u32 special_daily_id【权威】Host特殊每日生产分支；实际实体／资源／图像结果另承接，不在Client重放生成（4B；SDK声明） | 0x10 std::string params.name【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：挑战名称源，不等于全部最终HUD文字；当前run标题归Presentation缓存；不复制生产输入或调用初始化（24B；SDK声明） | 0x28 std::set<i32> params.room_filter【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host房间类型过滤树；不复制生产输入或调用初始化（8B；SDK声明） | 0x30 std::vector<i32> params.collectibles【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host起始道具列表；不复制生产输入或调用初始化（12B；SDK声明） | 0x3C std::vector<i32> params.trinkets【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host起始饰品列表；不复制生产输入或调用初始化（12B；SDK声明） | 0x48 i32 params.starting_pill【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host起始药丸；不复制生产输入或调用初始化（4B；SDK声明） | 0x4C i32 params.starting_card【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host起始卡牌；不复制生产输入或调用初始化（4B；SDK声明） | 0x50 i32 params.end_stage【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host终止楼层规则；不复制生产输入或调用初始化（4B；SDK声明） | 0x54 i32 params.player_type【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host初始角色；不复制生产输入或调用初始化（4B；SDK声明） | 0x58 std::vector<i32> params.achievements【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host成就列表；不得在Client记录完成；不复制生产输入或调用初始化（12B；SDK声明） | 0x64 i32 params.soul_hearts【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host初始魂心；不复制生产输入或调用初始化（4B；SDK声明） | 0x68 i32 params.black_hearts【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host初始黑心；不复制生产输入或调用初始化（4B；SDK声明） | 0x6C i32 params.hearts【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host初始红心；不复制生产输入或调用初始化（4B；SDK声明） | 0x70 i32 params.max_hearts【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host初始心容器；不复制生产输入或调用初始化（4B；SDK声明） | 0x74 i32 params.coins【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host初始金币；不复制生产输入或调用初始化（4B；SDK声明） | 0x78 f32 params.add_damage【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host初始伤害增量；不复制生产输入或调用初始化（4B；SDK声明） | 0x7C bool params.can_shoot【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host射击规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x7D bytes[3] params.unknown_6d【未知】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（3B；未知） | 0x80 i32 params.difficulty【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host开局难度输入；当前实际难度另由Game字段承接；不复制生产输入或调用初始化（4B；SDK声明） | 0x84 bool params.mega_satan【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：HostMegaSatan路线规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x85 bytes[3] params.unknown_75【未知】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（3B；未知） | 0x88 f32 params.min_fire_rate【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host最低射速规则；不复制生产输入或调用初始化（4B；SDK声明） | 0x8C bool params.max_damage【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host伤害规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x8D bool params.min_shot_speed【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host弹速规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x8E bool params.big_range【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host射程规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x8F bytes[1] params.unknown_7f【未知】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（1B；未知） | 0x90 i32 params.path_type【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：0冥路／1天堂／2支线／3Home；旧isAltPath/isSecretPath不是独立字节；不复制生产输入或调用初始化（4B；SDK声明） | 0x94 u32 params.curse【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host挑战curse输入；不复制生产输入或调用初始化（4B；SDK声明） | 0x98 u32 params.curse_filter【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host挑战curse过滤；不复制生产输入或调用初始化（4B；SDK声明） | 0x9C std::vector<i32> params.twin_collectibles【权威】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：Host双子起始道具列表；不复制生产输入或调用初始化（12B；SDK声明） | 0xA8 bytes[12] params.unknown_98【未知】当前run参数副本，区别Manager预告／菜单对象；不发送原生容器或启动／解锁规则：对象尾部尚未解释；明确列出，不读取不发送，不假定为填充（12B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 1—1B，不含第 9 步记录头）：

块 0：当前每日显示存在性（world，1—1B）：

| +0x0 u8 is_daily：原生u32 id仅投影(id!=0)的1B显示布尔；97C300/98DDA0只消费非零判断。实际日期/名称若显示由Presentation当前run批次承接，不复制日期用于Init/排行榜 |
|---|

证据：

- `is_daily`：DailyChallenge.zhl；98DDA0／97C300读取Game+25BA8
- `seed`：DailyChallenge.zhl；Game::StartDailyChallenge6F3140
- `practice`：DailyChallenge.zhl；6F3140
- `unknown_9`：docs/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `special_daily_id`：DailyChallenge.zhl；6F3140
- `params.name`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.room_filter`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.collectibles`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.trinkets`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.starting_pill`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.starting_card`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.end_stage`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.player_type`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.achievements`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.soul_hearts`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.black_hearts`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.hearts`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.max_hearts`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.coins`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.add_damage`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.can_shoot`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.unknown_6d`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：docs/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `params.difficulty`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.mega_satan`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.unknown_75`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：docs/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `params.min_fire_rate`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.max_damage`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.min_shot_speed`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.big_range`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.unknown_7f`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：docs/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `params.path_type`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.curse`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.curse_filter`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.twin_collectibles`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `params.unknown_98`：Game702320在daily非零时返回Game+25BB8；LuaDailyChallenge.cpp却取Manager对象；嵌套字段：docs/design/binary-state/run-synchronization-review.md
