# 第8步 Presentation：公开成员逐行回映射

**2026-11-10修订2**：[来源同步职责](presentation-source-synchronization-review.md)收紧六行音乐／SFX映射：最终voice参数替代独立mask、全局pitch、Getter ID；个人后端状态不下行。旧决定保存在JSON，仍141行，无生产版本变更或测试。

2026-11-10；源：[presentation-member-map.json](presentation-member-map.json)。结论见[同步职责核对](presentation-synchronization-review.md)，字节见[账本](presentation-byte-ledger.md)。

原始stream目录共141行；旧标签：ReplicatedState 55、AuthorityOnly 70、NotReplicated 16。保留field、全局ordinal、签名、重载和null/空白；不按ID去重，不回写历史目录。决定列是当前同步必要性，不是公开API已可执行的声明。

| ordinal / field | 原始成员与参数 | 旧归属 | 当前决定与去向 | 依据/限制 |
|---|---|---|---|---|
| 3179 / 3180 | ColorModifier.A (null) | ReplicatedState | field → GameColorView.current.strength | 仅最终current实例；FXParams配置/target/lerp不是相同实例，不为保持任意ColorModifier对象相等复制它们 |
| 3180 / 3181 | ColorModifier.B (null) | ReplicatedState | field → GameColorView.current.b | 仅最终current实例；FXParams配置/target/lerp不是相同实例，不为保持任意ColorModifier对象相等复制它们 |
| 3181 / 3182 | ColorModifier.Brightness (null) | ReplicatedState | field → GameColorView.current.brightness | 仅最终current实例；FXParams配置/target/lerp不是相同实例，不为保持任意ColorModifier对象相等复制它们 |
| 3182 / 3183 | ColorModifier.ColorModifier ( float R = 1, float G = 1, float B = 1, float A = 0, float Brightness = 0, float Contrast = 1 ) | AuthorityOnly | not_applicable → 本地纯值构造／运算 | 不是网络命令或全局实例状态；实际结果由所属owner承接 |
| 3183 / 3184 | ColorModifier.Contrast (null) | ReplicatedState | field → GameColorView.current.contrast | 仅最终current实例；FXParams配置/target/lerp不是相同实例，不为保持任意ColorModifier对象相等复制它们 |
| 3184 / 3185 | ColorModifier.G (null) | ReplicatedState | field → GameColorView.current.g | 仅最终current实例；FXParams配置/target/lerp不是相同实例，不为保持任意ColorModifier对象相等复制它们 |
| 3185 / 3186 | ColorModifier.R (null) | ReplicatedState | field → GameColorView.current.r | 仅最终current实例；FXParams配置/target/lerp不是相同实例，不为保持任意ColorModifier对象相等复制它们 |
| 3186 / 3187 | ColorModifier.__add ( ColorModifier right ) | AuthorityOnly | not_applicable → 本地纯值构造／运算 | 不是网络命令或全局实例状态；实际结果由所属owner承接 |
| 3187 / 3188 | ColorModifier.__div ( ColorModifier right ) | AuthorityOnly | not_applicable → 本地纯值构造／运算 | 不是网络命令或全局实例状态；实际结果由所属owner承接 |
| 3188 / 3189 | ColorModifier.__eq ( ColorModifier right ) | AuthorityOnly | not_applicable → 本地纯值构造／运算 | 不是网络命令或全局实例状态；实际结果由所属owner承接 |
| 3189 / 3190 | ColorModifier.__mul ( ColorModifier right ) | AuthorityOnly | not_applicable → 本地纯值构造／运算 | 不是网络命令或全局实例状态；实际结果由所属owner承接 |
| 3190 / 3191 | ColorModifier.__sub ( ColorModifier right ) | AuthorityOnly | not_applicable → 本地纯值构造／运算 | 不是网络命令或全局实例状态；实际结果由所属owner承接 |
| 3196 / 3197 | Font.DrawString ( string String, float PositionX, float PositionY, KColor RenderColor, int BoxWidth = 0, boolean Center = false ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3197 / 3198 | Font.DrawString ( string String, float PositionX, float PositionY, float sizeX, float sizeY, KColor RenderColor, FontRenderSettings settings ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3198 / 3199 | Font.DrawStringScaled ( string String, float PositionX, float PositionY, float ScaleX, float ScaleY, KColor RenderColor, int BoxWidth = 0, boolean Center = false ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3199 / 3200 | Font.DrawStringScaledUTF8 ( string String, float PositionX, float PositionY, float ScaleX, float ScaleY, KColor RenderColor, int BoxWidth = 0, boolean Center = false ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3200 / 3201 | Font.DrawStringUTF8 ( string String, float PositionX, float PositionY, KColor RenderColor, int BoxWidth = 0, boolean Center = false ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3201 / 3202 | Font.Font ( ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3202 / 3203 | Font.Font ( string FontPath ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3203 / 3204 | Font.GetBaselineHeight ( ) | ReplicatedState | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3204 / 3205 | Font.GetCharacterWidth ( char Character ) | NotReplicated | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3205 / 3206 | Font.GetLineHeight ( ) | ReplicatedState | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3206 / 3207 | Font.GetStringWidth ( string String ) | NotReplicated | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3207 / 3208 | Font.GetStringWidthUTF8 ( string String ) | NotReplicated | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3208 / 3209 | Font.IsLoaded ( ) | ReplicatedState | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3209 / 3210 | Font.Load ( string FilePath ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3210 / 3211 | Font.SetMissingCharacter ( char MissingCharacter ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3211 / 3212 | Font.Unload ( ) | AuthorityOnly | not_applicable → 本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| 3334 / 3335 | GenericPrompt.GenericPrompt ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3335 / 3336 | GenericPrompt.GetCurrentSelection ( ) | ReplicatedState | projection → 共享Prompt最终选择外观／本地Prompt本地 | Yes/No高亮与提交命令分开；不能把Host选择写入访客本地确认框 |
| 3336 / 3337 | GenericPrompt.GetSprite ( ) | ReplicatedState | projection → GenericPrompt.paper | 共享world Prompt最终资源/姿态；本地菜单Prompt不上线，不能跨owner串用 |
| 3337 / 3338 | GenericPrompt.GetSubmittedSelection ( ) | ReplicatedState | authority → Host Prompt提交结果 | 0/1/2是决策生产输入，Client状态安装不执行提交，也不承诺此Getter跨端原值相等 |
| 3338 / 3339 | GenericPrompt.Initialize ( boolean SmallPrompt = false ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3339 / 3340 | GenericPrompt.IsActive ( ) | ReplicatedState | derived → 共享Prompt实际存在性/最终paper姿态，或本地Prompt | 82CB90读主动画与is_playing；不为生成它执行Update(ProcessInput=true) |
| 3340 / 3341 | GenericPrompt.Render ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3341 / 3342 | GenericPrompt.SetImageToVictoryRun ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3342 / 3343 | GenericPrompt.SetText ( string Text1 = "", string Text2 = "", string Text3 = "", string Text4 = "", string Text5 = "", ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3343 / 3344 | GenericPrompt.Show ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3344 / 3345 | GenericPrompt.Update ( boolean ProcessInput ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3345 / 3346 | HUD.AssignPlayerHUDs ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3346 / 3347 | HUD.FlashChargeBar ( EntityPlayer Player, ActiveSlot ActiveSlot ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3347 / 3348 | HUD.FlashRedHearts ( EntityPlayer Player ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3348 / 3349 | HUD.GetBossHPBarFill ( ) | ReplicatedState | field → HUD.boss_fill | 97F7A0实际读取，负值表示隐藏，不能仅从HP或把值夹到0..1重建 |
| 3349 / 3350 | HUD.GetCardsPillsSprite ( ) | ReplicatedState | projection → HUD.cards_pills | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3350 / 3351 | HUD.GetChargeBarSprite ( ) | ReplicatedState | projection → HUD.charge_bar | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3351 / 3352 | HUD.GetCoopMenuSprite ( ) | ReplicatedState | not_applicable → C/S本地参与者面板 | 原版Join/Online选择面板不进入C/S；若复用美术仅本地加载，不复制其选择/设备状态 |
| 3352 / 3353 | HUD.GetCraftingSprite ( ) | ReplicatedState | projection → HUD.crafting | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3353 / 3354 | HUD.GetFortuneSprite ( ) | ReplicatedState | projection → HUD.fortune | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3354 / 3355 | HUD.GetHeartsSprite ( ) | ReplicatedState | projection → HUD.hearts | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3355 / 3356 | HUD.GetHistoryHUD ( ) | ReplicatedState | projection → HUD.history_hud | 本地root，内容为两列实际owner及有序最终图像，不代表完整原始History |
| 3356 / 3357 | HUD.GetInventorySprite ( ) | ReplicatedState | projection → HUD.inventory | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3357 / 3358 | HUD.GetPickupsHUDSprite ( ) | ReplicatedState | projection → HUD.pickup | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3358 / 3359 | HUD.GetPlayerHUD ( int Index = 0 ) | NotReplicated | projection → HUD.players[index0..7] | 按真实C/S身体与显示槽查本地承载；原版设备/Online索引不复制 |
| 3359 / 3360 | HUD.GetPlayerStreakSprite ( int Index = 0 ) | NotReplicated | projection → HUD.message_players[index0..3].streak | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3360 / 3361 | HUD.GetPoopSpellSprite ( ) | ReplicatedState | projection → HUD.poop_spells | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3361 / 3362 | HUD.GetStackedStreakSprite ( int Index = 0 ) | NotReplicated | projection → HUD.message_stack[index0..5].streak | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3362 / 3363 | HUD.GetStreakSprite ( ) | ReplicatedState | projection → HUD.message_main.streak | 实际Sprite资源/外观及独立姿态；必覆写帧/比例按具体绘制pass派生，原生指针/事件位/loaded不传 |
| 3363 / 3364 | HUD.InvalidateActiveItem ( EntityPlayer Player, ActiveSlot ActiveSlot ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3364 / 3365 | HUD.InvalidateCraftingItem ( EntityPlayer Player ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3365 / 3366 | HUD.IsVisible ( ) | ReplicatedState | field → HUD.visible＋同world NO_HUD | Host world可见资格与本地显示偏好分开 |
| 3366 / 3367 | HUD.PostUpdate ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3367 / 3368 | HUD.Render ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3368 / 3369 | HUD.SetBossHPBarFill ( float percent ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3369 / 3370 | HUD.SetVisible ( boolean Visible = false ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3370 / 3371 | HUD.ShowFortuneText ( string MainString, string SecondaryString, ... ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3371 / 3372 | HUD.ShowItemText ( EntityPlayer Player, ItemConfigItem Item) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3372 / 3373 | HUD.ShowItemText ( EntityPlayer Player, ItemConfigItem Item, boolean StackUpText ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3373 / 3374 | HUD.ShowItemText ( string MainString, string SecondaryString, boolean IsCurseDisplay = false ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3374 / 3375 | HUD.ShowItemText ( string MainString, string SecondaryString, boolean IsCurseDisplay = false, boolean StackUpText ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3375 / 3376 | HUD.Update ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3376 / 3377 | HistoryHUD.GetCollectibleOffsets ( int PlayerIndex, CollectibleType[] IDs = nil, boolean IncludeNotVisible = false ) | NotReplicated | not_applicable → 最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| 3377 / 3378 | HistoryHUD.GetCollectibles ( int PlayerIndex, CollectibleType[] IDs = nil, boolean IncludeNotVisible = false ) | NotReplicated | not_applicable → 最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| 3378 / 3379 | HistoryHUD.GetItems ( int PlayerIndex, boolean IncludeNotVisible = false ) | NotReplicated | not_applicable → 最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| 3379 / 3380 | HistoryHUD.GetPlayer ( int PlayerIndex ) | NotReplicated | reference → HistoryHUD.players[index0..1].player | 实际两个原生显示列的StateRef；不能按参与者序号推断 |
| 3380 / 3381 | HistoryHUD.GetPosition ( ) | ReplicatedState | derived → 本地History布局 | 官方GetPosition按本地屏幕/HUDOffset/ExtraHUDStyle计算，不发Host屏幕坐标 |
| 3381 / 3382 | HistoryHUD.GetTrinketOffsets ( int PlayerIndex, TrinketType[] IDs = nil, boolean IncludeNotVisible = false ) | NotReplicated | not_applicable → 最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| 3382 / 3383 | HistoryHUD.GetTrinkets ( int PlayerIndex, TrinketType[] IDs = nil, boolean IncludeNotVisible = false ) | NotReplicated | not_applicable → 最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| 3383 / 3384 | HistoryHUDItem.GetHistoryItem ( ) | ReplicatedState | authority → HistoryHUDItem.history_origin（不上线） | 公开Getter读原始History_HistoryItem；Render只需visual_id@0和Image@4，不能把最终visual_id冒充原API原始itemID/trinket/time或完整HistoryItem |
| 3384 / 3385 | HistoryHUDItem.GetItemID ( ) | ReplicatedState | authority → HistoryHUDItem.history_origin（不上线） | 公开Getter读原始History_HistoryItem；Render只需visual_id@0和Image@4，不能把最终visual_id冒充原API原始itemID/trinket/time或完整HistoryItem |
| 3385 / 3386 | HistoryHUDItem.GetRenderOffset ( ) | ReplicatedState | derived → 实际列表index＋本地History布局 | 官方LuaHistoryHUD.cpp按本地columns/iconSize/visibleCount计算，不传Host屏幕坐标 |
| 3386 / 3387 | HistoryHUDItem.GetTime ( ) | ReplicatedState | authority → HistoryHUDItem.history_origin（不上线） | 公开Getter读原始History_HistoryItem；Render只需visual_id@0和Image@4，不能把最终visual_id冒充原API原始itemID/trinket/time或完整HistoryItem |
| 3387 / 3388 | HistoryHUDItem.IsTrinket ( ) | ReplicatedState | authority → HistoryHUDItem.history_origin（不上线） | 公开Getter读原始History_HistoryItem；Render只需visual_id@0和Image@4，不能把最终visual_id冒充原API原始itemID/trinket/time或完整HistoryItem |
| 3388 / 3389 | HistoryHUDItem.IsVisible ( ) | ReplicatedState | derived → 实际列表index＋本地History布局 | 官方LuaHistoryHUD.cpp按本地columns/iconSize/visibleCount计算，不传Host屏幕坐标 |
| 3389 / 3390 | ItemOverlay.GetDelay ( ) | ReplicatedState | authority → ItemOverlay.delay（不上线） | WAITING倒计时只生产未来可见性；直接发送当前可见性 |
| 3390 / 3391 | ItemOverlay.GetMegaMushPlayerSprite ( ) | ReplicatedState | projection → ItemOverlay.mega_mush_player | 实际独立Sprite |
| 3391 / 3392 | ItemOverlay.GetOverlayID ( ) | ReplicatedState | field → ItemOverlay.overlay_id | 最终类型影响Render分支，不回放Show |
| 3392 / 3393 | ItemOverlay.GetPlayer ( ) | ReplicatedState | reference → ItemOverlay.player | MegaMush锚点实际StateRef |
| 3393 / 3394 | ItemOverlay.GetSprite ( ) | ReplicatedState | projection → ItemOverlay.sprite | 实际主Sprite |
| 3394 / 3395 | ItemOverlay.Show ( Giantbook GiantbookID, int Delay = 3, EntityPlayer = nil ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3440 / 3441 | MusicManager.Crossfade ( Music ID, float FadeRate = 0.08 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3441 / 3442 | MusicManager.Crossfade ( Music MusicId, float FadeRate = 0.08 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3442 / 3443 | MusicManager.Disable ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3443 / 3444 | MusicManager.DisableLayer ( int LayerId = 0 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3444 / 3445 | MusicManager.Enable ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3445 / 3446 | MusicManager.EnableLayer ( int LayerId = 0, boolean Instant = false ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3446 / 3447 | MusicManager.Fadein ( Music ID, float Volume = 1, float FadeRate = 0.08 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3447 / 3448 | MusicManager.Fadein ( Music MusicId, float Volume = 1, float Volume = 0.08 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3448 / 3449 | MusicManager.Fadeout ( float FadeRate = 0.08 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3449 / 3450 | MusicManager.GetCurrentJingleID ( ) | ReplicatedState | projection → 实际world jingle资源/逻辑实例；无独立getter字段 | 官方ID可能由未生效调用更新；最终voice资源/事件承接，不单独发ID或倒计时 |
| 3450 / 3451 | MusicManager.GetCurrentMusicID ( ) | ReplicatedState | projection → 实际并存逻辑voice的资源引用 | 两个crossfade组可能并存；voice已有资源时不另发currentId Getter |
| 3451 / 3452 | MusicManager.GetCurrentPitch ( ) | ReplicatedState | authority → Host生产输入；最终每voice pitch承接 | 每voice最终pitch已有时不另发Music+314，不能重复全局/声部两套参数 |
| 3452 / 3453 | MusicManager.GetQueuedMusicID ( ) | ReplicatedState | authority → Music.queued_id（不作下行状态） | 未来续播控制由Host实际决策/事件接管，不放行Client Gameplay选曲 |
| 3453 / 3454 | MusicManager.IsEnabled ( ) | ReplicatedState | projection → world voice最终gain/pause/存续；本地Mute独立 | 不复制混合enabled字段；没有单独网络getter或覆写个人静音 |
| 3454 / 3455 | MusicManager.IsLayerEnabled ( int LayerId = 0 ) | NotReplicated | authority → Host分层混音输入；无独立下行mask | 最终每voice gain已承接分层权重/模式，撤销旧六位mask必发决定 |
| 3455 / 3456 | MusicManager.MusicManager ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3456 / 3457 | MusicManager.Pause ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3457 / 3458 | MusicManager.PitchSlide ( float TargetPitch ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3458 / 3459 | MusicManager.Play ( Music ID, float Volume = 1 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3459 / 3460 | MusicManager.Play ( Music MusicId, int Volume = -1 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3460 / 3461 | MusicManager.PlayJingle ( Music MusicId, int Duration = 140 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3461 / 3462 | MusicManager.Queue ( Music ID ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3462 / 3463 | MusicManager.ResetPitch ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3463 / 3464 | MusicManager.Resume ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3464 / 3465 | MusicManager.SetCurrentPitch ( float Pitch ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3465 / 3466 | MusicManager.StopJingle ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3466 / 3467 | MusicManager.UpdateVolume ( ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3467 / 3468 | MusicManager.VolumeSlide ( float TargetVolume, float FadeRate = 0.08 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3468 / 3469 | PlayerHUD.GetHUD ( ) | ReplicatedState | derived → 本地所属HUD root | 本地回指，不发送HUD* |
| 3469 / 3470 | PlayerHUD.GetHeartByIndex ( int Index ) | NotReplicated | projection → PlayerHUD.hearts[index0..23] | 固定物理槽的最终心格，不重跑UpdateHearts |
| 3470 / 3471 | PlayerHUD.GetHearts ( ) | ReplicatedState | projection → PlayerHUD.hearts[24] | 全部24个最终心格 |
| 3471 / 3472 | PlayerHUD.GetIndex ( ) | ReplicatedState | projection → C/S显示槽关系 | 不把原版联机index或设备键当网络身份 |
| 3472 / 3473 | PlayerHUD.GetLayout ( ) | ReplicatedState | derived → C/S身体显示分组＋本地排版 | 97E4A0含Online依赖，不放行原GetPlayerHUDLayout；采用具名C/S布局决定 |
| 3473 / 3474 | PlayerHUD.GetPlayer ( ) | ReplicatedState | reference → PlayerHUD.player | 实际身体StateRef |
| 3474 / 3475 | PlayerHUD.RenderActiveItem ( ActiveSlot Slot, Vector Position, float Alpha = 1.0, float Scale = 1.0 ) | AuthorityOnly | authority → Host正常生产／命令，不在Client重放 | 命令副作用不随状态重演；同步其已核最终显示结果，由具体Presentation owner负责 |
| 3475 / 3476 | PlayerHUDHeart.GetFlashType ( ) | ReplicatedState | field → PlayerHUDHeart.flash_type | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| 3476 / 3477 | PlayerHUDHeart.GetHeartAnim ( ) | ReplicatedState | field → PlayerHUDHeart.animation | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| 3477 / 3478 | PlayerHUDHeart.GetHeartOverlayAnim ( ) | ReplicatedState | field → PlayerHUDHeart.overlay_animation | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| 3478 / 3479 | PlayerHUDHeart.IsEternalHeartOverlayVisible ( ) | ReplicatedState | field → PlayerHUDHeart.eternal | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| 3479 / 3480 | PlayerHUDHeart.IsFadingHeart ( ) | ReplicatedState | field → PlayerHUDHeart.fading | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| 3480 / 3481 | PlayerHUDHeart.IsGoldenHeartOverlayVisible ( ) | ReplicatedState | field → PlayerHUDHeart.golden | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| 3481 / 3482 | PlayerHUDHeart.IsVisible ( ) | ReplicatedState | field → PlayerHUDHeart.visible | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| 3499 / 3500 | SFXManager.AdjustPitch ( SoundEffect ID, float Pitch ) | AuthorityOnly | authority → Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| 3500 / 3501 | SFXManager.AdjustVolume ( SoundEffect ID, float Volume ) | AuthorityOnly | authority → Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| 3501 / 3502 | SFXManager.GetAmbientSoundVolume ( SoundEffect ID ) | NotReplicated | not_applicable → 本地音频后端／具名持续voice目标视图 | 后端是否播放/音量含本地设置，不要求Host原始查询结果相等；持续声音目标另同步 |
| 3502 / 3503 | SFXManager.IsPlaying ( SoundEffect ID ) | NotReplicated | not_applicable → 本地后端结果；world逻辑存续另有来源 | 不传播后端pending/设备失效/偷占成全体Stop；world用途与退休来源待核 |
| 3503 / 3504 | SFXManager.Play ( SoundEffect ID, float Volume = 1, int FrameDelay = 2, boolean Loop = false, float Pitch = 1, float Pan = 0 ) | AuthorityOnly | authority → Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| 3504 / 3505 | SFXManager.Preload ( SoundEffect ID ) | AuthorityOnly | authority → Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| 3505 / 3506 | SFXManager.SFXManager ( ) | AuthorityOnly | authority → Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| 3506 / 3507 | SFXManager.SetAmbientSound ( SoundEffect ID, float Volume, float Pitch ) | AuthorityOnly | authority → Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| 3507 / 3508 | SFXManager.Stop ( SoundEffect ID ) | AuthorityOnly | authority → Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| 3508 / 3509 | SFXManager.StopLoopingSounds ( ) | AuthorityOnly | authority → Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |

## 旧141行以外的具体补充

| owner/来源 | 成员/范围 | 去向 |
|---|---|---|
| HUDMessage；官方LuaHUD.cpp::RegisterHUDMessage | GetSprite、IsShowing、Hide、Show、GetMainText、SetMainText、GetSubText、SetSubText | 当前showing/streak/text_image；文本语义按声明UI独立承载，命令不上线 |
| Minimap；官方LuaMinimap.cpp/Wiki Minimap.md | GetDisplayedSize、GetHoldTime、GetIconsSprite、GetItemIconsSprite、GetShakeDuration、GetShakeOffset、GetState、Refresh、SetHoldTime、SetShakeDuration、SetShakeOffset、SetState | 最终world标记/震动offset/资源；state/holdTime/尺寸本地，shakeDuration生产器留Host，Refresh命令不回放 |
| NightmareScene；官方LuaNightmareScene.cpp | GetBackgroundSprite、GetBubbleSprite、GetPlayerExtraPortraitSprite、GetProgressBarMap、GetProgressBarSprite、IsDogmaNightmare | 当前资源/14项有序路线/场景/最终pass，未知payload不静默省略 |
| PlayerHUD；官方LuaPlayerHUD.cpp | GetFlashRedHearts、GetFlashSoulHearts、GetFlashGoldHearts、SetFlashRedHearts、SetFlashSoulHearts、SetFlashGoldHearts | 三色当前显示计时；setter命令不回放，额外以实际1.1.3注册名为准 |
| Native；本文与原生字节账本 | D0、secondary_charge_bar、dynamic_PlayerHUD、holy_mantle_visible、drop_display_scale、boss_images、inventory_visuals、crafting_image、fortune_image、game_screen_passes、audio_active_voices、shader_extras | 具名原生/自有最终值owner；不是把旧目录数量当完整边界 |

补充组不冒充新的全量API目录；官方未注册的LuaMinimapConfig表不作公开接口承诺。原生字段／Root与最终pass另按字节账本和职责说明核对。没有执行Getter、测试或实机操作。
