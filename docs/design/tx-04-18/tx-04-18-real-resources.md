# tx-04-18 原版资源与实景补全

> **已停止的实现路线（2026-11-01更新）**：现行[create／remove／change原生应用及统一验收](tx-04-18-native-state-plan.md#13-最终统一验收结果)已完成，最终 **29/29 PASS**，包括17的配套调整。下文“进行中”、RGBA／surface／顶点重放及续验安排属于旧路线历史，实际失败和旧源码保留，不再作为继续执行指令；本轮未删除预先存在的旧路线恢复材料。

2026-10-31，进行中。用户明确要求：此前受控模型／测试素材的10/10不构成原版房间和Grid完成。本轮仍为tx-04-18，全部实现后统一E2E，复用确实不受修改影响的已通过项；通过后直接提交。

## 完整目标与实现前失败模型

- Authority实际原版Room/Grid绘制产生的贴图、生成floor/wall/water surface、最终几何、颜色／shader属性、混合和顺序必须成为有界纯值。不得只换一组原版路径而仍由夹具构造pose，不以整屏视频替代分对象资源。
- Source只观察原调用，original exactly-once；Client不用Room/Grid Render、Init、生成、开门、破坏、奖励或本地RNG。真实原版创建／变化发生在Authority，客户端应用其结果。
- Image::Render返回后shader helper还会填vertex属性，不能立即复制半套顶点。所有native临时借用限制在已核定同主线程／同帧／同scene内，不越过队列reset、对象退休或mutation变更。
- Source readback必须在实际图形队列排空及OS context检查之后；GetTexelRegion不是无图形副作用的普通Getter，不在活动batch中递归调用。
- Client只创建自有图像；源RGBA有界压缩传值，上传保存恢复binding／UNPACK/PBO；不重调会转移buffer所有权并重建texture的SetTexelData。资源及Room/Grid状态同候选发布／退休，失败保持旧版本。
- 实景验收从真实Authority场景取得模型与资源，覆盖自然房间、实际Grid原版图、门与环境、坑桥／破坏／同格替换、形状及正常退出；GPU像素须与实际Authority来源核对。控制夹具仅编排真实操作，不提供替代贴图或模型。

旧受控用例按原事实保留；原版内容尚未验证的结论在本轮通过前不得写为完成。必要静态研究沿用同一个GPT-6 Luna/max，主智能体审阅[真实资源逆向](tx-04-18-real-resource-reverse.md)。

## 实现与主审

`game/room_grid_source.cpp`从正式观测consumer读取同tick、完整membership的Grid／Room模型，并维护规范身份Registry。`game/room_grid_original.cpp`在真实Authority绘制期间关联Room／Backdrop／Grid／RockTop／GridLight，复制原调用最终shader顶点、混合与逐Image资源；已核定Entity Render／Shadow／PostRender由独立owner排除。未分类绘制整候选拒绝，不能猜作实体丢弃。每帧native临时借用仅在同run／room／mutation与HGLRC安全点复制成owned值，随后清除。

`replica/room_grid_original.cpp`组合实际资源与15的源模型；客户端创建自有procedural Image、无损解码RGBA8、上传自有texture并重放最终顶点，不调用Room／Grid／Backdrop原生绘制。canonical状态、资源和图形提交一起发布，失败不替换旧场景。whole-world Applied及网络Baseline仍不在18内。

复杂度自查后的收敛：图元只观察八个Image重载共用的一个最终入口，替换先前不完整的六色入口；准备阶段复用官方`Room.PreRender`，替换水面专用观察点；Shadow／Light／Water和已核定的房间生成目标共用同一有界重建路径。FX按真实容器成员归属识别，不增加Light／Water专用Hook；实体阴影用官方`EntityList.RenderShadows`整体边界替换四个派生类边界，同时排除原版通用回退阴影。新增观察点由38项降为35项。不会复制含实体的原生混合目标像素。

主智能体回读官方ImageUtils／ShaderUtils和ZHL，并独立从IDA核对SetTexel对`+0x18`格式／`+0x78`主纹理的使用及PostRender零显式参数。官方`SetTexelData`会转移allocator所有权，因此没有复用它；上传仅针对本方新图像。GPU读回保存PACK状态，上传保存UNPACK和texture binding，存在外部PBO时拒绝。

实景入口默认启用原版模式。玩法夹具只执行真实开局、原版Grid创建与状态变化、换房换层，既不构造复制模型，也不提供贴图。暂存逐资源数据和Authority／Client GPU参考仅服务本次对照，成功收尾清理。顺序实景应用验收不等于跨进程网络Baseline端到端，也不外推所有原版内容组合均已覆盖。

## 复杂度自查与收敛（2026-10-31）

用户要求自查后，确认本轮有两处复杂化：资源应用逐步扩成较通用的渲染截获／目标生成路径；纯值适配为复用旧表示而多次编码、解析大份JSON。38项新增观察Hook及私有布局维护成本已经较高，不能继续遇到像素差异就扩大截获范围。

已把纯值适配改为同候选DOM共用，保留现有严格检查与Scene发布／退休；停止增加通用Hook和渲染状态复制。领域核对还发现实际Room模型已带`FXParams.ColorModifier`，而新绘制路径仅保留这些环境值，没有应用其显示语义。应先核对原版房间环境的必要应用步骤，再决定最小实现。

截至第20次启动，11组原版源已实际采集；应用实际色彩值并修正离屏提交后，首个自然房间的Authority／Client GPU匹配率为14.84%，仍失败，暗云／阴影存在明显差异。尚未取得原版实景验收PASS，也未提交。必要续验数据为`out/tx-04-18-real-e2e-20`，只用于恢复和诊断，完成后清理。此前启动中的绑定、日志、源预算、真实Null适配、水生成目标及离屏提交问题保留为真实失败，不以已采集或已显示代替验收通过。

自查后的实施约束：只修复由原版调用与实景差异共同支持的具体缺口；若多个原生重载收敛到同一已核定入口，应替换观察点而非继续叠加Hook；不新增通用shader/uniform/GL状态同步框架。不降低像素阈值，不用完整屏幕贴图代替Room/Grid资源。Client修复复用未受影响的真实源，只有采集语义改变才重新采集。

续验反例：第21次共同入口首次捕获旧路径遗漏的7-float纯色quad，旧9-float下限拒绝；第22次因此暴露shading、Shadow和Light Overlay三项漏捕。第23次补齐官方PreRender后，发现其FX type4光照贴图需要按真实容器归属识别；第24次自然、类型、状态来源已采集，破坏场景暴露实体专用阴影方法返回后的通用回退阴影。它来自`EntityList.RenderShadows`，不是Grid碎片，不能按资源名称接纳。第25次使用整体实体阴影边界续验，并复用第24次不受影响的三组已完成来源。所有这些均属于同一tx整体验收中的失败与修复，不是已通过的实景像素项目。
