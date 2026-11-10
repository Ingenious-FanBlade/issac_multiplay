# RunScoreSheet

由唯一布局源生成。布局编号 234；布局版本 57；原生大小 0x90；未知 3B；完整：否。

Game+25C5C；64B为14个最终分数组件/总分加完成楼层和真正完成时间。SDK把+40写作u32 runTime已经过时，实际+40为vector头、+4C才是时间。83EE80复制、83F670逐项绘制是保留这些值的正面证据；不要调用Show或Render内9BAC90重算/每日Init。+89..8B仍未知。结算显示owner可用同布局描述Leaderboard+30C已冻结副本，不能把新run当前缓存覆盖旧结算批次。处理器尚未实施。

原生字节归属：

| 0x0 i32 stage_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x4 i32 schwag_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x8 i32 blue_baby_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0xC i32 lamb_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x10 i32 mega_satan_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x14 i32 mother_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x18 i32 delirium_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x1C i32 beast_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x20 i32 rush_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x24 i32 exploration_bonus【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x28 i32 damage_penalty【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x2C i32 time_penalty【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x30 i32 item_penalty【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x34 i32 total_score【状态】Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate（4B；已核） | 0x38 i32 finished_stage【状态】最终已完成楼层，用于83F670的FINISH_TIME标签参数与显示存在性；不是Client生成楼层输入（4B；已核） | 0x3C i32 finished_stage_type【权威】Host已完成楼层类型输入；本轮实际结算Render不需要它，不为SDK字段可见发送（4B；已核） | 0x40 std::vector<HostScoreRecord> score_history_40【权威】真实vector三指针，不是SDK所称u32 runTime；属于Host分数生产历史，不能泄露地址（12B；已核） | 0x4C u32 finished_time【状态】真正的已完成时间帧数，9BA470写+4C；83F670以30Hz格式化小时分秒，不用GameFrame或Client墙钟替代（4B；已核） | 0x50 u32 producer_50【权威】Host分数生产计数，9BAC90读取v2[20]；不以未知私有名称臆造显示状态（4B；已核） | 0x54 u32 producer_54【权威】Host分数生产辅助缓存，9B9D20/9B9DC0维护；最终显示值已独立承接（4B；已核） | 0x58 i32 run_ending【权威】Host分数生产用run ending，9BAC90读取v2[22]；不作为结束／持久命令发送（4B；已核） | 0x5C std::vector<HostScoreRecord> score_history_5c【权威】真实Host分数生产历史vector；只保持本机所有权，不复制三指针或元素来补显示（12B；已核） | 0x68 std::vector<HostScoreRecord> score_history_68【权威】真实Host分数生产历史vector；只保持本机所有权，不复制三指针或元素来补显示（12B；已核） | 0x74 u32[2] producer_74【权威】Host分数生产辅助两值，9B9DC0清零，不在Client重新生产分数（8B；已核） | 0x7C std::vector<HostFinishedStage12> finished_stage_history【权威】每项12B的完成楼层／类型／repeatedStage记录，9BA470追加，9BAC90消费；只发最终结果，不发历史（12B；已核） | 0x88 u8 producer_latch【权威】Host分数生产锁存，具体私有名未知，不作为Client重算许可（1B；已核） | 0x89 bytes[3] unknown_89【未知】尚无具体语义，不读不发，不猜填充（3B；未知） | 0x8C u32 build_version【本地】游戏构建版本标识，9B9DC0用J273构造；不把Host整数缓存当本机版本（4B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 64—64B，不含第 9 步记录头）：

块 0：最终结算分数缓存（world，64—64B）：

| +0x0 i32 stage_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x4 i32 schwag_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x8 i32 blue_baby_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0xC i32 lamb_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x10 i32 mega_satan_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x14 i32 mother_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x18 i32 delirium_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x1C i32 beast_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x20 i32 rush_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x24 i32 exploration_bonus：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x28 i32 damage_penalty：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x2C i32 time_penalty：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x30 i32 item_penalty：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x34 i32 total_score：Host已生产完成的最终分数组件／总分；83F670实际逐项绘制，保留原符号，不能在Client Calculate | +0x38 i32 finished_stage：最终已完成楼层，用于83F670的FINISH_TIME标签参数与显示存在性；不是Client生成楼层输入 | +0x3C u32 finished_time：真正的已完成时间帧数，9BA470写+4C；83F670以30Hz格式化小时分秒，不用GameFrame或Client墙钟替代 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：

- `stage_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `schwag_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `blue_baby_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `lamb_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `mega_satan_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `mother_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `delirium_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `beast_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `rush_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `exploration_bonus`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `damage_penalty`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `time_penalty`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `item_penalty`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `total_score`：ScoreSheet.zhl；9BAC90；966680内9679FA读取Game+25C90总分；83EE80复制到Leaderboard+30C；83F670读取组件并渲染各项DAILY_RUN标签
- `finished_stage`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `finished_stage_type`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `score_history_40`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `finished_time`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `producer_50`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `producer_54`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `run_ending`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `score_history_5c`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `score_history_68`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `producer_74`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `finished_stage_history`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `producer_latch`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `unknown_89`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
- `build_version`：J273 83EE80将ScoreSheet复制到Leaderboard+30C；83F670实际绘制；9BA470；9B9D20/9B9DC0；SDK原40=runTime声明已过时
