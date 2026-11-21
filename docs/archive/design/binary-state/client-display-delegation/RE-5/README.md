# RE-5／RE-6：原生 Render 覆盖与已知写集

日期：2026-11-12。状态：**按现行收窄范围完成静态研究，未实施、未测试**。目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273；当前 IDB imagebase 为 `0x400000`，本文及子文档明确区分 VA、RVA 和对象偏移。先查本地原版 Wiki、REPENTOGON Wiki、ZHL／SDK 与官方实现，再复读 J273 指令；没有启动游戏、调用游戏函数、修改目标内存、安装 Hook、构建或运行测试／校验脚本。

> **2026-11-12 审阅后标注**：用户决定保留 `client_entity_draw_projectile`（Q10），作为结论 4 与[写集表](write-set.md) Projectile 行所述 `+0x350` 保存／精确恢复的作用域，不新增 hook，见[修改方案 9.1](../../client-display-delegation.md#91-hook)。结论 6 中 `0x703E40` 由本机窗口尺寸请求触发，已从 RE-9 的转场前提中移出，完整写集按 E28 实测差异补查。

任务来源：[显示委托方案第 6 节](../../client-display-delegation.md#6-新增逆向任务j273-静态反编译)。RE-6 已并入 RE-5，本次统一交付；不另开 tx，归入二进制重构第 13 步，第 15 步仍是唯一整体验收。

## 成果

- [Render 树与调用位置](render-tree.md)：Minimap、ItemOverlay、HUD、Room 和其他已知显示根的 J273 调用点、对象与条件，以及是否需要独立绘制调度。
- [已知写集、本地归属与漂移校正](write-set.md)：现行方案 4.4 的已知项定稿，补正 Tear 的十进制偏移误记；交接布局收缩、Render 后置恢复和 E28 检查范围。

## 核定结论

1. **Minimap 已在树内**：`Game::Render 0x6F7A80 → 0x6F8108 HUD::Render 0x97C300 → 0x97C5D8 Minimap::Render 0x966680`。遵循 HUD 可见性及 `Game+0x25B8C<2` 等原生条件；被条件隐藏时不另补一次绘制。Minimap Update／Refresh 仍由显示推进按 RE-1 调度，不能因为 Render 已覆盖就删除它们。
2. **ItemOverlay 已在树内**：`0x6F8132 → ItemOverlay::Render 0x984660`，ECX=`Game+0x1C034`，位于 HUD 后。状态 0／1 直接返回；普通 Sprite 未加载且不是 id45 时不绘制。无需独立绘制挂点，也不因此放行含玩法的 `ItemOverlay::Update`；状态与 Sprite 来源继续交 RE-8。
3. **已知本地写入不进入漂移相等比较**：Entity 屏幕坐标 `+0x1A4/+0x1A8`、Room LightGradient Sprite 的 scale `+0x6EDC/+0x6EE0`、坑纹理 dirty `+0x11F6`、Game 本机 resize 请求 `+0x25AAC` 与 Render 反馈 `+0x25AB8`。其中 Room 两个浮点不是独立世界状态，而是 `Room+0x6E40` 内嵌 ANM2 的 `+0x9C` scale。
4. **临时写入不能一律改成本地字段**：Projectile `+0x350` 属于已同步 PositionOffset；Tear 的 `+0xF0..+0x11B` 属于已同步主 Sprite ColorMod。保留其同步职责；前者浮点减加的正常尾部不保证逐位恢复，后者正常路径保存并恢复 44B。恢复范围和异常／重入限制见写集表。
5. **纠正旧 Tear 偏移记法，并明确颜色位置**：伪代码十进制 `420/424` 是十六进制 `0x1A4/0x1A8`；旧报告“暂时更改颜色”没有具体定位，本次核定为 `Entity+0x48+0xA8=+0xF0`，不能外推为公共 `Entity.Color+0x1B8`。原报告保留，已在其开头追加纠正与补充。
6. **不穷尽 Render 闭包**：按方案 6.1，未重做光照尾部、所有派生 Render、资源 helper、Lua 或 RNG 的全图静态闭合。显示推进及维护 helper 上的其余已同步字段写入，由第 15 步 E28 按合法读回与影子目标实测；转场 `0x703E40／0x8273C0` 的完整条件与写集归 RE-9。

## 布局源与实现状态

本次只为已存在的本地／派生范围及 Tear 主 Sprite 范围补充职责说明和静态证据，见 [entity.json](../../../../../../mod/binding/state/entity.json)、[room.json](../../../../../../mod/binding/state/room.json)、[run.json](../../../../../../mod/binding/state/run.json)、[presentation.json](../../../../../../mod/binding/state/presentation.json)。没有改变类别、范围、块、wire、读写处理器或互通接受合同，没有生成或提升版本；现行源和生成代码仍是版本 71。

这些标记不表示版本 72 的按需收缩已实施。Camera／Room scroll、震屏偏移、HUD／Minimap／FX、屏幕结果等旧布局的迁移仍随后续布局收缩统一进行，不能把文档标记当成执行器已停止采集／安装。已知无 wire 的本地范围不新增复制字段；通用 ANM2 ColorMod 不因 Tear 的临时借用而被裁掉。

收尾采用静态文档与差异审阅，核对调用指令、ZHL 签名、对象偏移、当前源标记、引用和现行 Q4／Q6／Q8。没有新增运行时观察或 E2E PASS，也未声明漂移校正、原生 Render 包装或本地显示推进已接线。
