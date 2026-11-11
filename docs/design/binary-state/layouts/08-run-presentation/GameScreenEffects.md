# GameScreenEffects

由唯一布局源生成。布局编号 346；布局版本 60；原生大小 0x24；未知 6B；完整：否。

独立连续视图，真实基址Game+25A98；不是新原生类。剩余UNKNOWN不能由此缩小为零。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 Image* previous_image.image【权威】实际Image与本地引用计数control；具体下行职责见具名结果，不直接复制该范围。；由GameScreenEffectsResult固定最终结果承接，不与生产输入重复发送。：实际选中图像由第8步资源身份承接（4B；已核） | 0x4 Control* previous_image.strong_control【本地】实际Image与本地引用计数control；具体下行职责见具名结果，不直接复制该范围。；由GameScreenEffectsResult固定最终结果承接，不与生产输入重复发送。：本地引用计数／析构控制块（4B；已核） | 0x8 i32[2] bloom_timers【权威】Game+25AA0/AA4 bloom倒计时/时长生产输入（8B；已核） | 0x10 f32 bloom_strength【派生】Game+25AA8 bloom设置强度；具体下行职责见具名结果，不直接复制该范围。；由GameScreenEffectsResult固定最终结果承接，不与生产输入重复发送。（4B；已核） | 0x14 bool resize_request【本地】Game+25AAC窗口resize请求（1B；已核） | 0x15 bytes[3] unknown_15_17【未知】未取得字段用途或填充证明（3B；未知） | 0x18 i32[2] hallucination_inputs【权威】Game+25AB0/AB4剩余/总时间（8B；已核） | 0x20 u8 render_feedback【本地】Game+25AB8 Host Render反馈标记（1B；已核） | 0x21 bytes[3] unknown_21_23【未知】未取得字段用途或填充证明（3B；未知） |
|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `previous_image.image`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `previous_image.strong_control`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `bloom_timers`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明
- `bloom_strength`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明
- `resize_request`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明
- `unknown_15_17`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明
- `hallucination_inputs`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明
- `render_feedback`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明
- `unknown_21_23`：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明
