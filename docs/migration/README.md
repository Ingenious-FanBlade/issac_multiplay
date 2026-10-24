# tx-03-09：独立工程复制与文档交付

日期：2026-10-22（Asia/Shanghai）。用户已确认：复制为独立工程、保留原目录；旧构建缓存和大体积测试产物保留原处；点位覆盖全部已知记录；状态同步采用完整基线＋增量状态＋表现事件，首版单房间无预测。

## 路径与范围

| 内容 | 源位置 | 新位置/处理 |
| --- | --- | --- |
| C/S源码、脚本、testdata、ledger、archive | `D:\projects\isaac_hook_reserach\cs-t0` | 新根目录`cs-t0/`，保持内部相对布局 |
| 项目规范与T0历史文档 | 原根`docs/refactor/` | `docs/refactor/`原样复制 |
| 隔离边界所引用的旧同步背景文档 | 原根`docs/isaac-sync/` | `docs/isaac-sync/`，只迁文档，不迁原版/Isaac Sync运行组件 |
| 静态逆向原始JSON与附属资料 | 原根`artifacts/cs-t0/*-static/` | 同相对路径复制，供新文档本地定位 |
| 根构建依赖 | `Directory.Build.props`、`VERSION` | 新根同名文件；VERSION不代表新状态同步已发布 |
| 当前规范入口 | 根`AGENTS.md` | 新根复制并增加迁移入口说明 |
| 构建缓存/输出 | `cs-t0/target`、原根`out/build`等 | 不复制；原处保留 |
| 大体积实机产物 | 原根`artifacts/cs-t0`中非static目录 | 不复制；历史证据仍在原D盘 |
| VM旧采集 | `cs-t0/vm_share`内容 | 不复制；新位置建立空目录 |
| Git元数据 | 两端`.git` | 原仓库不改，目标原有Git仓库保留；不复制/重置/提交 |

新根：`F:\projects_isaac\issac_multiplay`。不移动、删除源文件，不修改原生产源码。新根的`README.md`、`.gitignore`、`docs/README.md`和新文档为新增内容。

## 路径适配和独立性边界

保留`cs-t0/`而不是将源码铺到新根，可继续解析现有`../docs`、`ROOT/cs-t0`、Cargo include_bytes/include_str、vcxproj及testdata。新副本`cs-t0/accept-t09-b-live.py`的默认RDP共享路径更新为`\\tsclient\F\projects_isaac\issac_multiplay\cs-t0\vm_share`；实际VM重定向盘符不同可使用已有`--vm-guest-share`显式参数。

源码构建输入随副本提供，但机器工具链、Steam游戏EXE/IDA数据库和VM不属于工程文件，不迁移或操作。保留历史记录里的绝对D盘/F盘路径是为了保留事实；不批量把原artifact来源伪改成新路径。

历史accept/replay/test中有依赖旧运行artifact或旧DLL默认路径的入口，如tx-01-48、tx-01-58、tx-01-79、tx-01-83至86及早期passive来源。它们的原始参数/路径保持历史语义；本次没有携带其大体积输入，不能声称新根内可直接重放所有历史测试。需要时显式指定原artifact位置，或在后续任务中设计无旧流程的新验收。含旧哈希/gate逻辑的入口本次从未运行。

新文档自身的源代码/静态逆向依据使用新根内相对链接；历史文档中指向未复制实机产物的链接可以在原根查看，不伪造不存在的本地证据。原完整产物位置：`D:\projects\isaac_hook_reserach\artifacts\cs-t0`。迁移脚本和只读索引生成脚本在本次暂存目录保留，不混入生产运行流程。

## 新文档

- [Hook模块索引](../hooks/README.md)：24个功能模块，一模块一份文档；另提供机器可读索引。
- [状态同步方案](../design/state-synchronization.md)：数据所有权、网络身份、采样、明确基线delta重建、ACK/恢复、客户端apply、表现事件、线程、失败及实施顺序。
- [源复制清单](source-copy-manifest.json)：逐文件路径和字节数，不含哈希。
- [整体静态审阅记录](static-review.json)：仅在整个目标完成后生成一次；不代表构建、游戏或代码测试PASS。

## 验收边界

本次仅一次整体静态文档与迁移审阅，检查新文档/索引/相对链接、源码声明覆盖、字段表对应、源复制与目标文件状态、原目录及Git元数据保留策略。无哈希计算/校验，无构建、注入、输入、游戏、调试器或代码测试。文档生成过程中发生的脚本编写错误只修正工具，不冒充产品测试或实机结果。
