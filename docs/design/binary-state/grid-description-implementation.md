# 第6步：由影子状态生成ClientGridDescription

日期：2026-11-10。范围为第6步描述生成代码；不接第10步Host采集、第12步网络解码调度或第13步原生安装／Hook／绘制。测试只在第15步。本轮不改变线字段和整数布局版本。

## 输入与职责

正式描述以已经完成第12步递归校验的**完整影子记录视图**为输入，而不是单次增量包、JSON或原生对象。视图包含成员／房间身份、use／shape、布局版本、成员／资源epoch、目标world版本和各块当前字节。未变化块可以有较早的变化版本；它们必须属于同一个成员形态。资源和嵌套ANM2的完整递归校验仍由第12步负责，不把本次描述生成器冒充网络解码器。

`ClientGridDescription`生成时按可信布局计划检查根结构并取得自有不可变字节。公共描述、index、最终water_clip_flags和Door展示标量提供类型化访问；ANM2外观／动画／图层保留真实nested_layout和只读投影，容器提供有界按项访问。不创建字符串字段字典，不复制Host控制字段，不调用Lua或原生函数。实际消费这些投影、准备资源和修改游戏对象属于第13步。

原运行路径的DTO明确改名为`LegacyClientGridDescription`等，仅供尚未退役的旧JSON链继续使用；它不能隐式转换为新描述。`ClientNativeGridsExecute`／`ClientNativeGridsBindDoors`及调用顺序保持，第13／14步再换接与退役。

## 先列失败方式

- 布局版本不符、根前缀／辅助视图伪装成真实Grid类、Desc.Type与工厂RTTI类不符。
- 成员／房间run或身份空间错误，块属于另一key／use／shape／布局；把旧形态字节混进新记录。
- 必需块缺席、未变化旧块被误拒绝、reserved空块被误等候、未定义块／多余字节被无声丢弃。
- 总长度溢出、块长度越界、借用跨线程失效、描述复制后重复拷贝负载或重复收费。
- SpawnSeed在224被补零；Statue空Sprite被改成通用201；TimerPlate按variant强行判空；Rock条件视图／独立overlay丢失。
- 完整u32水面值被缩成0—255，Door负目标索引被当Null，布尔投影非法值被规范化。
- Fire16B粒子被当旧24B速度结构，Rock有序重复偏移被重排／截断，条件空列表被旧值污染。
- 将本次根描述检查误称为第12步递归解码验证、原生owner就绪或AppliedAck；把旧JSON兼容链伪装成新正式数据源。

## 后续接线

第12步在完整已接受记录上提供只读输入；第13步用生成的数字字段／投影安装到本地owner，并承接Room／Run上下文。第10／13步的Plus实际reader、Lock第七水面覆盖点、Dataminer／Rock／Fire绘制、Door目标解析及资源准备均保持待实施。第15步覆盖上述失败方式和实际生命周期，不在本次运行测试。

## 已实现的接口

- [`client_grid_description.h/.cpp`](../../../mod/src/game/client/grid/client_grid_description.h)提供`ClientDescribeGrid(ClientGridShadowRecordView, byte_limit)`。只接受17个真实派生布局；工厂27个type对应关系复用`world_layout.json.grid_types`，没有另写类型switch。`state_grid_view.inc`由现有正式生成器产出，包含25个语义角色到实际字段索引、Desc标量位置和必需块集合。
- 根结构检查覆盖布局版本、Grid／Room身份空间及run、块的key／use／shape／layout、版本范围、块长、缺失／未定义块和总预算。旧块变化版本允许小于当前目标版本；Statue的reserved三块不要求提供。最大字节上限只允许收紧现行32MiB world消息上限。
- 公共描述仅含type／variant／state和可选SpawnSeed；index与完整u32水面值分别读取。Door只给出target类型、带负值的i32目标索引、ExtraVisible及Light布尔投影。Host八项保存元数据、碰撞、spawn_frame、RNG、previous、控制动画名等不存在于新描述中。
- `Field(role)`返回带数字FieldIndex、真实NestedLayout／NestedBlock与块变化版本的只读投影；`Sprite(role)`组合该角色三个投影。不存在的角色为不适用，Statue216则为存在但无负载；不从空投影推导loaded、资源就绪或ACK。
- Fire粒子按16B线上元素读取，Rock偏移按8B有序Vector读取；保留空列表、顺序及重复元素，越界访问报Gap。不适用类别访问容器也报Gap，不返回假空列表。粒子寿命／有限位置、碎块状态与空资源规则在生成时再次核对；ANM2内部选择器、图层及条件投影仍依赖输入已完成的第12步递归校验。
- 自有负载只复制一次到连续缓冲，计入现有Values池；描述／投影复制共享同一不可变owner和租约，原输入指针不保留。`dto_memory`按owner去重提供统计，返回值不表示需要再次收费。
- 旧描述、字段／Sprite／ember与命令移至[`client_grid_legacy.h`](../../../mod/src/game/client/grid/client_grid_legacy.h)，均用`LegacyClientGrid*`命名。旧解析器、stream暂存和原生执行器只作对应类型更名；没有把旧JSON解析结果送入新描述，实际输入生产与原生消费仍待后续。

新增纯值静态库目标`isaac_cs_grid_description`，依赖`isaac_cs_state`并由产品DLL链接。SDK-facing文件仍为C++17；新描述头保持该边界可包含，生成实现用C++20。没有新增测试程序、单元测试或运行时接线。

## 纯编译记录

`ISAACCS_BUILD_E2E_TOOLS=OFF`。新增静态库及完整Release／Win32产品DLL均已纯编译／链接成功：

```text
cmake --build build/repentogon --config Release --target isaac_cs_grid_description --parallel 4
cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /nr:false /p:BuildInParallel=false
```

第一次产品`--parallel 4`构建停在`Checking File Globs`无后续输出，已中止；改用上述串行、不复用MSBuild节点的命令后完成。没有编译错误；构建仍输出已有RapidJSON枚举运算、SDK参数／名称遮蔽等警告，未修改SDK。没有运行游戏、测试、probe、smoke、harness或校验脚本，也没有部署或提交。实际画面、内存预算与生命周期行为仍留第15步统一验收。
