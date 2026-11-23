# VisiblePlayerHUDHeart

由唯一布局源生成。布局编号 353；布局版本 94；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

记录编码预算≤8B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：VisiblePlayerHUDHeart具名当前结果（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：



逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| mod/binding/catalog/stream-catalog.json:3476 | PlayerHUDHeart.GetFlashType | " " / "int" | field → flash_type；PlayerHUDHeart.flash_type | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| mod/binding/catalog/stream-catalog.json:3477 | PlayerHUDHeart.GetHeartAnim | " " / "string" | field → animation；PlayerHUDHeart.animation | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| mod/binding/catalog/stream-catalog.json:3478 | PlayerHUDHeart.GetHeartOverlayAnim | " " / "string" | field → overlay_animation；PlayerHUDHeart.overlay_animation | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| mod/binding/catalog/stream-catalog.json:3479 | PlayerHUDHeart.IsEternalHeartOverlayVisible | " " / "boolean" | field → eternal；PlayerHUDHeart.eternal | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| mod/binding/catalog/stream-catalog.json:3480 | PlayerHUDHeart.IsFadingHeart | " " / "boolean" | field → fading；PlayerHUDHeart.fading | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
| mod/binding/catalog/stream-catalog.json:3481 | PlayerHUDHeart.IsGoldenHeartOverlayVisible | " " / "boolean" | field → golden；PlayerHUDHeart.golden | 最终心格实际Render消费者；动画指针转所属资源ID/有界名称，本地正常持有 |
