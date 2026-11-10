# PlayerHUDHeart

由唯一布局源生成。布局编号 310；布局版本 58；原生大小 0x10；未知 3B；完整：否。

834930先检查visible；隐藏心格只传存在性，不读其余可能残留的名字。淡出色的412AC0是sin，不是RNG；按同world相位派生。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

记录编码预算≤4194304B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 0x0 bool visible【状态】心格可见（1B；已核） | 0x1 bool eternal【派生】永恒心叠加；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：VisiblePlayerHUDHeart可见分支；隐藏时不读取该字段（1B；已核） | 0x2 bool golden【派生】金心叠加；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：VisiblePlayerHUDHeart可见分支；隐藏时不读取该字段（1B；已核） | 0x3 bool fading【派生】淡出心；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：VisiblePlayerHUDHeart可见分支；隐藏时不读取该字段（1B；已核） | 0x4 i8 flash_type【派生】最终心格闪烁类别；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：VisiblePlayerHUDHeart可见分支；隐藏时不读取该字段（1B；已核） | 0x5 bytes[3] unknown_5_7【未知】未取得字段用途或填充证明（3B；未知） | 0x8 char* animation【派生】最终主动画名指针；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：VisiblePlayerHUDHeart可见分支；隐藏时不读取该字段（4B；已核） | 0xC char* overlay_animation【派生】最终叠加动画名指针；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：VisiblePlayerHUDHeart可见分支；隐藏时不读取该字段（4B；已核） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 1—9B，不含第 9 步记录头）：

块 0：PlayerHUDHeart具名当前结果（world，1—9B）：

| +0x0 u8 visible：心格可见 | +0x1 VisiblePlayerHUDHeart（见该类型状态表）（伴随源，无根对象偏移）（仅player_h_u_d_heart_visible_visible__1(visible)成立时存在） visible_payload：只在visible==1时读取正常可见心格payload。 |
|---|---|

证据：

- `visible`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800
- `eternal`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800
- `golden`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800
- `fading`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800
- `flash_type`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800
- `unknown_5_7`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800
- `animation`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800
- `overlay_animation`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800

伴随来源（不属于上述原生对象字节）：

- `visible_payload` → `VisiblePlayerHUDHeart`：只在visible==1时读取正常可见心格payload。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3482 | PlayerHUDHeart.IsVisible | " " / "boolean" | field → visible；PlayerHUDHeart.visible | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
