# tx-04-18 I13—I18：地图、Camera 与 Grid 应用 owner

日期：2026-11-02。实现范围为 I13—I18（含 G01—G27）；实机结果只在 I29 统一验收后填写。

## 实现前失败模型

- 同格不同 Key、同 Key 换类型/实例修订、槽位交换、多处门别名、跨 Room 复用地址，不能继承旧身份、资源或官方 index 副表。
- 不完整成员、重复占用、未知 cast、非法格索引、越界门槽、-19 Online 目标、未知路由、缺失 required 字段必须在原生写入前拒绝。
- 保存描述与活动 Getter 共用实际存储的字段若互相矛盾，不能先后覆盖后冒称两者均已安装；零种子、Null、未初始化来源事实分别保留。
- 工厂成功不等于 Init 完成。派生 Init/PostInit 内的随机、生成、回调、持久与状态裁决不能通过最终字段覆盖抵消；Client 采用经研究的构造与最终值安装路径。
- 工厂已分配但尚未发布、已发布但字段/资源失败、官方副表重置后失败，分别保留真实前缀和单一退休 owner；不绘制、不提交半套目标。
- Fire 的原生实例可能大于 SDK 声明；Pillar 的实际派生转换、可空 TimerPlate、门额外 Sprite 和专用私有资源不能按外观推测或替代。
- 清除门时先撤全部 Room/Grid 别名，再释放实例；旧对象晚到清理不能清除同格新对象的官方副表。删除不能调用 Destroy/Hurt/TryUnlock/爆炸或奖励。
- 普通地图、Lost、不同维度、部分集合与当前入口象限分别处理。缺探索来源不推导全图可见；Camera 复制不得调用 Update 或本地过门。
- 变更、取消、失败、重复应用和回到旧值使用同一 A→T/Step/Commit/Retire owner；写后失败不继续使用历史 A。资源/跨域引用/缓存/绘制依赖由 I19—I25 闭合，不把这些后续项冒报为本轮实机完成。

## 验证安排

本轮先静态主审和纯构建，不运行游戏、probe、分项/单元测试。I28 按以上失败模型接 E04—E10/E13—E18；全 tx 功能完成后 I29 从正式来源连续执行统一 E2E。

## 实现与接入责任

| 项目 | 当前实现 | 后续整场景责任 |
| --- | --- | --- |
| I13 | `client_map_native.*`、`client_scene.cpp:MapState`、`client_presentation*`：Camera 具名数值/最新 focus、真实 DisplayFlags、Minimap 配置和保存记录图标投影；原生 Mirror Surface/水平翻转与专用 shader；取消强制 Lost 限制 | 24 完整探索/保存、25 挑战规则、27 完整 HUD；April's Fool challenge 32 的 Refresh 会随机选标记，当前由既有 challenge/HUD 不支持结果写前报告，不重抽 |
| I14 | `client_grids.*`：复用 I05 计划，稳定 Key 与本地单调实例分离，完整成员差分，同格新 Key/同 Key 换代先退休旧实例；单 owner 记录真实前缀 | I25 完整 Scene 提交；Room 重建先退休 Grid owner |
| I15 | `client_grid_native.cpp`：官方工厂及 Hook 链/真实派生 ctor，不调用含玩法的 Init/PostInit；Plus 重置后按实例供应有效 water flags | I20 碰撞/PathTrail/邻接；I21 分层 draw；I23 统一失败协调 |
| I16 | Desc 八项/公共状态、Position 几何与来源屏幕投影核对、具名派生字段及 Getter 回读；Source Initialized 与本地 constructed 分开 | I19 跨域引用；J273 common State/VarData 与 Desc 是同地址，矛盾来源写前拒绝 |
| I17 | 27 type、17 官方 cast、真实对象/公开字段/资源；Fire 三 ANM/Ember、Rock 碎屑、TimerPlate 可空内嵌 ANM、已加载但无当前动画的资源状态 | G 项共享的缓存/draw/事件/整目标提交继续 I19—I25；类别实机结果归 I29，当前不报告画面通过 |
| I18 | Door字段、只读 Slot、主/额外 Sprite、八个别名、形状允许、descriptor 位/候选及稳定目标引用分别处理；未初始化离图目标保留事实，路由哨兵不寻址 | I20 缓存、I21 draw、23 网络过门；不调用解锁/爆破/过门 FSM |

`ClientGrids::PrepareTarget/Step/Retire/Visit` 复用同一原生世界，不建立第二个运行世界。`ClientScene` 仍写前报告 `native_grid_integration_pending_I19_I25`，避免非空 Grid 被17代表 draw 误提交；I25 完整接线后替换该依赖。Entity/Player 的17代表 profile 独立限制19/20范围。

Room application_source=3、Grid application_source=2、Map/Camera runtime=1、Grid runtime=1、能力 `room-grid-target-v3`；观测 schema、15 codec、RoomPresentation 版本不变。录制关闭时同一正式 consumer 仍提供新增来源。

## 静态研究与主审纠错

先读本地原版/REPENTOGON Wiki及官方源码，再由 GPT-6 Luna/max 补[静态逆向](tx-04-18-grid-native-research.md)。主智能体直接复读工厂 `706100`、Fire Free `7067E0`/Render `67B620`、BigRock setter `715550`、TimerPlate getter `8A7C10`、Grid位置 `8087A0`/投影 `705110`、Camera ctor `91C730`/Snap `91EC20`/可见性 `91E920`/Focus `91EBF0` 和 Minimap `965D60/9635B0` 调用集；静态结论不替代实机。

- 工厂经官方 wrapper 采用 ECX/EDX 合同，不按旧元数据 cdecl 强转。Pillar=24、Gold=27，真实工厂给 Pillar Rock vtable。
- Fire/Poop/Door 实际分配 `0x390/0x19c/0x2f0`，与 SDK 简化声明不同；仅派生 Free 释放，不用 SDK sizeof 分配。
- Rock `+184` 是碎屑 float-pair vector；Fire Ember 为24字节位置/速度/age/maxLife；本方缓冲 Free 前撤空 header。
- TimerPlate getter 按值返回 Sprite，不能修改弹栈后的副本；安装真实 `+154` 内嵌 ANM。Door.Slot 只有 getter，安装 `+158` 后回读。
- `SetBigRockFrame(-1)` 会随机重选，禁止调用；非负值必须与来源 variant 编码一致。`NextGreedAnimation` 按原版资源的13个实际动画写前检查；const-char* setter 之前先持有 Lua registry 根，失败也保留到退休。
- `GetRenderPosition` 依赖视口和像素比例；新增真实源输入并写前核对，Client 按本地分辨率投影，不把 Source 屏幕坐标当世界位置。
- 同 index 的主格/door-only 对象可共存，双主格占用拒绝。官方 Plus 仅448个 index 状态；新增六条常驻 wrapper 统一登记，官方原链各一次，Client 返回后按实例安装有效裁切；非 Client 透传。
- Pit MakeBridge 不保留父指针，I19 不寻找不存在的持久父对象。Door `+154` 完整名称仍 UNKNOWN，只复制实际 bool，不调用玩法消费者。Poop 尾部 EntityRef 不供渲染采用，不补零冒充未知来源。

Minimap 缓存读取 SavedEntities 的 type/variant/subtype/intStorage3/targetPosition；正式投影复制这些值，仅 cache 调用栈借给容器，离栈恢复真实 owner，不创建 Entity/保存状态表。原生 surface 构建由 RenderState 保持 target/depth/shader 顺序，不运行 Game/Room/Camera Update。

## 结果

`pwsh -NoProfile -File scripts/build.ps1` 最终纯构建成功：复制/网络静态库、Win32 Release `zhlIsaacCS.dll`、既有工具与 i686 Rust harness 均编译/链接完成，新增正式 Lua 与目录随产品整理。未运行 I29 或任何游戏测试。

沙箱内第一次 MSBuild 停在启动阶段后中断，获准的沙箱外构建进入编译；首次新增 map adapter 缺少 `bridge.h` 声明导致编译失败，已修复，后续构建及镜像补齐后的最终构建均成功。既有 SDK/RapidJSON 和 Lua 采集局部变量遮蔽警告保留；未修改 SDK 或屏蔽项目警告。I19—I30及全组合联合覆盖保持各自责任，不因本轮领域 adapter 完成而提前报通过。不留 tx artifact。

镜像后续主审纠正了 Luna 把 Game 首字段称作 game mode 的表述：实际是 Stage。UV/48字节 native shader 参数与真实资源文件已复读，见研究笔记主审补证；时间为正式 Source 帧的既定相位，不使用 Client 墙钟。各 G 类的原生 Render 会修改临时动画选择器（例如 Fire Ember），I21 接 pass 时须恢复源姿态，不得通过游戏 Update 修复。

资源状态补齐：正式 SpriteRuntimePose 复制原生 loaded；旧已加载姿态沿用兼容值，新来源的未加载 ANM 必须显式带 loaded=false、空路径/空图层/无当前动画，原生 owner 安装其实际标量后仍保持未加载，不挑选替代图像。已加载资源但无当前动画单独处理。Door 额外 Sprite 直接取官方 SDK 内嵌成员，不借用 Lua 属性可能返回的副本。

逐类 Render 主审另复读工厂 vtable slot3、719270/71B4E0/4291D0：余下常规类型没有新增必需派生尾部输入；通用路径临时覆盖的是 Room._floorColor → ANM2._color；Statue slot3 为空，I21 必须接 Room 专用路径，不能只做通用虚调用。
