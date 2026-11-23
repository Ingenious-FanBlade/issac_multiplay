# FX

由唯一布局源生成。布局编号 245；布局版本 94；原生大小 0x4C；未知 5B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。 仅有显示资格的完整有序层；每层有效pose受原count和vector逻辑长度共同约束；原生pass0／未知pass报Gap，不静默当空。

原生字节归属：

| 0x0 u32 id【权威】配置／生成身份；实际draw不按该值重新选；同步决定：不上线；保留实际pass／图像即可（4B；已核） | 0x4 Image* image.image【权威】已选图像与实际sampler/format；同步决定：有资格的资源引用，control本地：实际选中图像由第8步资源身份承接（4B；已核） | 0x8 Control* image.strong_control【本地】已选图像与实际sampler/format；同步决定：有资格的资源引用，control本地：本地引用计数／析构控制块（4B；已核） | 0xC vector<FXPose> poses【容器】实际有序pose；同步决定：只发有效prefix；不发头／容量（12B；已核） | 0x18 i32 count【派生】draw使用数量；同步决定：由集合长度表达，保留0的清空语义（4B；已核） | 0x1C f32[4] velocity_ranges【权威】XML xMin/xMax/yMin/yMax；生成每pose速度的四端点；同步决定：留Host；有序当前pose位置／flip发送，不重新抽速度（16B；已核） | 0x2C u32 pass【状态】实际pass1..6选择；同步决定：同步；未知pass不当空／不执行（4B；已核） | 0x30 i32 stage_filter【权威】配置stage经旧编号换算后保存，供967DB0筛选；同步决定：不发候选过滤条件；实际被采用层／资源发送（4B；已核） | 0x34 u32 alt_stage_mask【权威】XML altStages过滤mask；同步决定：不发；不在Client按楼层重新选层（4B；已核） | 0x38 bool only_default_backdrop【权威】配置onlyDefaultBackdrop筛选条件；同步决定：不发；实际选层已知（1B；已核） | 0x39 u8 parallax_mode【权威】XML parallax存在时置1；96BF40据此按平均Player位置生成pose位置；同步决定：模式留Host，实际当前pose发送；不复制跟随生产器（1B；已核） | 0x3A bytes[2] unknown_3A_3B【未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；同步决定：不读不发；窄字段读写跳过该范围，保留未决（2B；未知） | 0x3C f32 condition color_condition【换算】generic非mask且count>0分支比较==0；同步决定：只发is_zero条件；NaN/Inf都按原==0语义归非零，不让其进入坐标浮点；正式投影：仅generic非mask且有效count>0时投影原float==0；NaN/Inf为false，未消费规范0（4B；已核） | 0x40 u32 mask_backdrop【换算】mask且count>0判断是否35；同步决定：只发is_35布尔；不把非采用分支原值读上网；正式投影：仅generic mask分支取原value==35；其它分支规范0（4B；已核） | 0x44 u32 blend_mode【换算】pass1/2/3/6 BlendMode实际选择；同步决定：仅实际分支发；不按Image猜；正式投影：仅generic实际blend分支保留原值；其余规范0，不读旧值（4B；已核） | 0x48 bool mask【换算】generic分支shader选择；同步决定：仅适用pass发规范bool；余3B未知；正式投影：仅pass1/2/3/6消费；pass4/5规范0且不读旧mask（1B；已核） | 0x49 bytes[3] unknown_49_4B【未知】声明边界内未解释尾部，不能称填充；同步决定：不读不发；保留具名缺口（3B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 15—163855B，不含第 9 步记录头）：

块 0：FX当前结果（world，15—163855B）：

| +0x0 u32 pass：实际pass1..6选择；同步决定：同步；未知pass不当空／不执行 | +0x4 u16 image：已选图像与实际sampler/format；同步决定：有资格的资源引用，control本地 | +0x6 u16 count + count×FXPose（上限16384） poses：实际有序pose；同步决定：只发有效prefix；不发头／容量 | 顺序变长偏移 u8 mask：generic分支shader选择；同步决定：仅适用pass发规范bool；余3B未知；正式投影：仅pass1/2/3/6消费；pass4/5规范0且不读旧mask | 顺序变长偏移 u8 color_condition：generic非mask且count>0分支比较==0；同步决定：只发is_zero条件；NaN/Inf都按原==0语义归非零，不让其进入坐标浮点；正式投影：仅generic非mask且有效count>0时投影原float==0；NaN/Inf为false，未消费规范0 | 顺序变长偏移 u8 mask_backdrop：mask且count>0判断是否35；同步决定：只发is_35布尔；不把非采用分支原值读上网；正式投影：仅generic mask分支取原value==35；其它分支规范0 | 顺序变长偏移 u32 blend_mode：pass1/2/3/6 BlendMode实际选择；同步决定：仅实际分支发；不按Image猜；正式投影：仅generic实际blend分支保留原值；其余规范0，不读旧值 |
|---|---|---|---|---|---|---|

证据：

- `id`：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0
- `image.image`：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `image.strong_control`：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `poses`：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0
- `count`：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0
- `velocity_ranges`：968E10解析numLayers/xMin/xMax/yMin/yMax/layer/stage/altStages/parallax/onlyDefaultBackdrop/backdrop/blendMode/useBGShader；967D40；96BF40
- `pass`：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0
- `stage_filter`：968E10解析numLayers/xMin/xMax/yMin/yMax/layer/stage/altStages/parallax/onlyDefaultBackdrop/backdrop/blendMode/useBGShader；967D40；96BF40
- `alt_stage_mask`：968E10解析numLayers/xMin/xMax/yMin/yMax/layer/stage/altStages/parallax/onlyDefaultBackdrop/backdrop/blendMode/useBGShader；967D40；96BF40
- `only_default_backdrop`：968E10解析numLayers/xMin/xMax/yMin/yMax/layer/stage/altStages/parallax/onlyDefaultBackdrop/backdrop/blendMode/useBGShader；967D40；96BF40
- `parallax_mode`：968E10解析numLayers/xMin/xMax/yMin/yMax/layer/stage/altStages/parallax/onlyDefaultBackdrop/backdrop/blendMode/useBGShader；967D40；96BF40
- `unknown_3A_3B`：968E10解析numLayers/xMin/xMax/yMin/yMax/layer/stage/altStages/parallax/onlyDefaultBackdrop/backdrop/blendMode/useBGShader；967D40；96BF40
- `color_condition`：96C620：value==0 || lightingAlpha*0.1<=0.9
- `mask_backdrop`：96C620
- `blend_mode`：96CE20/96CC00/96CD10/96E2B0
- `mask`：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0
- `unknown_49_4B`：967D40构造；96C620；96CC00/96CD10/96CE20/96DB20/96DF80/96E2B0
