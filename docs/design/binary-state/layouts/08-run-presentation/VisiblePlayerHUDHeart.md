# VisiblePlayerHUDHeart

由唯一布局源生成。布局编号 353；布局版本 60；原生大小 0x0；未知 0B；完整：是。

记录编码预算≤8B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 6—8B，不含第 9 步记录头）：

块 0：VisiblePlayerHUDHeart具名当前结果（world，6—8B）：

| +0x0 u8（C/S自有元数据，无原生偏移） eternal：可见心格当前最终eternal，从正常PlayerHUDHeart对应字段读取。 | +0x1 u8（C/S自有元数据，无原生偏移） golden：可见心格当前最终golden，从正常PlayerHUDHeart对应字段读取。 | +0x2 u8（C/S自有元数据，无原生偏移） fading：可见心格当前最终fading，从正常PlayerHUDHeart对应字段读取。 | +0x3 i8（C/S自有元数据，无原生偏移） flash_type：可见心格有符号flash原值。 | +0x4 u16（C/S自有元数据，无原生偏移） animation：可见心格实际动画名在所属hearts文件中解析。 | +0x6 u16（C/S自有元数据，无原生偏移）（仅visible_player_h_u_d_heart_eternal_eternal__1(eternal)成立时存在） overlay_animation：仅eternal分支读取实际overlay名称。 |
|---|---|---|---|---|---|

证据：



逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3476 | PlayerHUDHeart.GetFlashType | " " / "int" | field → flash_type；PlayerHUDHeart.flash_type | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| docs/design/tx-04-21/stream-catalog.json:3477 | PlayerHUDHeart.GetHeartAnim | " " / "string" | field → animation；PlayerHUDHeart.animation | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| docs/design/tx-04-21/stream-catalog.json:3478 | PlayerHUDHeart.GetHeartOverlayAnim | " " / "string" | field → overlay_animation；PlayerHUDHeart.overlay_animation | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| docs/design/tx-04-21/stream-catalog.json:3479 | PlayerHUDHeart.IsEternalHeartOverlayVisible | " " / "boolean" | field → eternal；PlayerHUDHeart.eternal | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| docs/design/tx-04-21/stream-catalog.json:3480 | PlayerHUDHeart.IsFadingHeart | " " / "boolean" | field → fading；PlayerHUDHeart.fading | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| docs/design/tx-04-21/stream-catalog.json:3481 | PlayerHUDHeart.IsGoldenHeartOverlayVisible | " " / "boolean" | field → golden；PlayerHUDHeart.golden | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
