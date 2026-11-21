# RunChallengeParam

由唯一布局源生成。布局编号 232；布局版本 90；原生大小 0xA4；未知 19B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

只描述参数存储，不产生状态。name的实际显示由@current_run_title承接。6D..6F、75..77、7F、98..A3仍未知；不得把旧字段注释解释成独立bool。

原生字节归属：

| 0x0 std::string name【权威】挑战名称源，不等于全部最终HUD文字；当前run标题归Presentation缓存；不复制生产输入或调用初始化（24B；SDK声明） | 0x18 std::set<i32> room_filter【权威】Host房间类型过滤树；不复制生产输入或调用初始化（8B；SDK声明） | 0x20 std::vector<i32> collectibles【权威】Host起始道具列表；不复制生产输入或调用初始化（12B；SDK声明） | 0x2C std::vector<i32> trinkets【权威】Host起始饰品列表；不复制生产输入或调用初始化（12B；SDK声明） | 0x38 i32 starting_pill【权威】Host起始药丸；不复制生产输入或调用初始化（4B；SDK声明） | 0x3C i32 starting_card【权威】Host起始卡牌；不复制生产输入或调用初始化（4B；SDK声明） | 0x40 i32 end_stage【权威】Host终止楼层规则；不复制生产输入或调用初始化（4B；SDK声明） | 0x44 i32 player_type【权威】Host初始角色；不复制生产输入或调用初始化（4B；SDK声明） | 0x48 std::vector<i32> achievements【权威】Host成就列表；不得在Client记录完成；不复制生产输入或调用初始化（12B；SDK声明） | 0x54 i32 soul_hearts【权威】Host初始魂心；不复制生产输入或调用初始化（4B；SDK声明） | 0x58 i32 black_hearts【权威】Host初始黑心；不复制生产输入或调用初始化（4B；SDK声明） | 0x5C i32 hearts【权威】Host初始红心；不复制生产输入或调用初始化（4B；SDK声明） | 0x60 i32 max_hearts【权威】Host初始心容器；不复制生产输入或调用初始化（4B；SDK声明） | 0x64 i32 coins【权威】Host初始金币；不复制生产输入或调用初始化（4B；SDK声明） | 0x68 f32 add_damage【权威】Host初始伤害增量；不复制生产输入或调用初始化（4B；SDK声明） | 0x6C bool can_shoot【权威】Host射击规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x6D bytes[3] unknown_6d【未知】尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（3B；未知） | 0x70 i32 difficulty【权威】Host开局难度输入；当前实际难度另由Game字段承接；不复制生产输入或调用初始化（4B；SDK声明） | 0x74 bool mega_satan【权威】HostMegaSatan路线规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x75 bytes[3] unknown_75【未知】尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（3B；未知） | 0x78 f32 min_fire_rate【权威】Host最低射速规则；不复制生产输入或调用初始化（4B；SDK声明） | 0x7C bool max_damage【权威】Host伤害规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x7D bool min_shot_speed【权威】Host弹速规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x7E bool big_range【权威】Host射程规则；不复制生产输入或调用初始化（1B；SDK声明） | 0x7F bytes[1] unknown_7f【未知】尚无具体语义／所属嵌套owner内字段证据；不读取不发送，不以构造初值或对齐猜填充（1B；未知） | 0x80 i32 path_type【权威】0冥路／1天堂／2支线／3Home；旧isAltPath/isSecretPath不是独立字节；不复制生产输入或调用初始化（4B；SDK声明） | 0x84 u32 curse【权威】Host挑战curse输入；不复制生产输入或调用初始化（4B；SDK声明） | 0x88 u32 curse_filter【权威】Host挑战curse过滤；不复制生产输入或调用初始化（4B；SDK声明） | 0x8C std::vector<i32> twin_collectibles【权威】Host双子起始道具列表；不复制生产输入或调用初始化（12B；SDK声明） | 0x98 bytes[12] unknown_98【未知】对象尾部尚未解释；明确列出，不读取不发送，不假定为填充（12B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `name`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `room_filter`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `collectibles`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `trinkets`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `starting_pill`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `starting_card`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `end_stage`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `player_type`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `achievements`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `soul_hearts`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `black_hearts`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `hearts`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `max_hearts`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `coins`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `add_damage`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `can_shoot`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `unknown_6d`：docs/archive/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `difficulty`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `mega_satan`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `unknown_75`：docs/archive/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `min_fire_rate`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `max_damage`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `min_shot_speed`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `big_range`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `unknown_7f`：docs/archive/design/binary-state/run-synchronization-review.md；官方SDK未具名或本轮未接管的明确孔
- `path_type`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `curse`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `curse_filter`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `twin_collectibles`：ChallengeParam.zhl；LuaChallengeParam.cpp；Game::GetChallengeParams702320
- `unknown_98`：docs/archive/design/binary-state/run-synchronization-review.md
