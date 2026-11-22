# 文档

isaac-cs 是基于官方 REPENTOGON 1.1.3 的以撒（Repentance+ v1.9.7.12.J273）C/S 联机模组：房主运行权威游戏，最多 3 个客机只上传输入、显示房主的世界。

## 阅读顺序

1. [总体架构](architecture.md)：部件、源码分层、线程模型、一帧之内的数据流、Hook 策略、版本与构建
2. [采样](capture.md)：布局源、需求清单、生成器，Host 读取原生对象的流程与安全规则
3. [同步](synchronization.md)：两条状态流、线格式、增量与确认、资源与事件、预算
4. [Host 端设计](host.md)：会话与大厅、权威 tick、统一输入、远端玩家、发布线程
5. [Client 端设计](client.md)：输入上行、接收与校验、原生安装、显示委托、插值与预测、副作用隔离
6. [Mod 联机接口方案](mod-api.md)：声明式 Mod API、握手协商、Mod 状态同步、客机回调放行与意图（草案，待审阅，未实施）

## 其他资料

- 生成的布局表：`mod/binding/layouts/`（由构建生成，不要手改）
- 源码目录说明：`mod/src/game/README.md`
- 项目工作约定：仓库根目录的 `AGENTS.md`
- 历史设计、逆向报告与验收记录：`docs/archive/`。只作查证用，其中的旧流程不再作为现行要求
