# 第4步：Text／Dummy实现说明

日期：2026-11-09（Asia/Singapore）。布局版本44。本轮完成两类的布局迁移、必要传输字段复核及有界文本基础接口。静态原始依据、纠错和未决见[text-dummy-evidence.md](text-dummy-evidence.md)；运行采集／解码／应用仍依次归第10／12／13步，不表示可玩同步已完成。

唯一源新增`mod/binding/state/entity-text-dummy.json`：Text145继承Entity E0—E9，TX0仅最终RGBA16B，TX1仅u16 count＋0—255原始字节，TX2空预留；variant2选择146号Scaled臂，在TX2增加真实draw消费的counter4B。duration只参与Host更新／移除，KColor末字在此固定字体的最终Image consumer中没有使用，均不发送／安装／漂移比较。通用KColor颜色空间合同仍是完整20B，不按Text特例改动。

Dummy147是与Entity等大的真实派生类。生成器允许等大继承且不允许该类声明额外字节，保持RTTI分派。没有专有尾部／标签容器，标签从公共Type／Variant生成；公共Sprite／Plus与生命周期保持。通用Spawn964仍是NPC，不能代替Dummy专用池来源。NativeText五个唯一目录成员有明确回映射，NativeDummy没有专有目录owner。

schema和生成器增加`storage=c-string`：内联容量≤256B，线上上限必须容量减一，元素为u8、无伪计数源、必须有真实writer读取资格。`state/nested.h/.cpp`新增`ReadCStringInto`及`StageCStringInto`，前者无堆分配地扫描至NUL并复读有效区，后者完整检查线上长度／count／内嵌NUL后暂存并补NUL；两者不猜编码，不读或传输未初始化尾部。资格由第10步正式owner提供，本函数与生成表都不伪造use/revision见证。SDK独立编译对象补入KColor20B／Alpha／末字ABI断言。

单行原生／线布局表由唯一源重生成：[EntityText](layouts/04-entity-classes/EntityText.md)、[EntityText_Scaled](layouts/04-entity-classes/EntityText_Scaled.md)、[EntityDummy](layouts/04-entity-classes/EntityDummy.md)。两Text根未知339B（公共335＋52C尾字4），Dummy公共335B未知；未抹为填充。公共动态部分取最短时，Text负载409—664B、Scaled413—668B，Dummy最小391B；不含外层帧，也不代表资源／图层任意数量下的完整上界或带宽实测。

Dummy不可见分支会跳过公共Render的cache刷新，随后标签仍读取历史屏幕cache。此已定位依赖的跨视口／历史来源须在第13步闭合，缺失报Gap，不能把它当无条件派生、发未经核定的8B缓存或用Client本地旧值伪装一致。正常可见路径已有公共Screen公式；自然Dummy caller和该特殊分支运行覆盖均未取得。Text Manager／pause子帧、真实writer独立owner、字体加载／窄draw及旧链退役也已写入第10／12／13／14步合同，第15步统一覆盖空／长／短／非UTF8、writer撤销、错包和绘制分支。本轮没有增加Hook、游戏操作或部署。

生成过程中首次声明NativeDummy公开owner被拒绝（旧目录无此专有owner），已改为空，不伪造目录。两次沙箱内构建停在CMake／File Globs阶段，无后续编译输出，已中断；改用配置所指D:/VisualStudio2022的MSBuild在沙箱外完成普通Release／Win32／v143产品纯编译与链接，退出码0。SDK ABI编译对象、isaac_cs_state及完整zhlIsaacCS均成功；ISAACCS_BUILD_E2E_TOOLS=OFF，SDK既有CMAKE_REPENTOGON_VERSION undefined提示保持。没有运行测试、游戏、注入或部署，唯一统一整体验收仍在第15步，没有创建artifact包。
