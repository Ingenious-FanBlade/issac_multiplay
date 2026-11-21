# tx-02-00：T0-08 操作5–9，实施前失败方式

2026-10-20（Asia/Shanghai）。用户要求完成剩余五项，编写E2E清单并进行实机测试。沿用无哈希路线和包CRC，使用GPT-6 Luna / max只读静态分析。完整目标包括输入冗余、逐lane最新消息、严格decoder、独立golden/负向/fuzz corpus、确定性有界FaultInjectionTransport，以及真实Authority主线程接入与一次统一验收。本文先于本轮生产修改登记。

分析/修改期间不测试、不查询游戏进程或提前输入；纯编译不执行产物。全部实现、清单与审阅完成，用户准备好全新安全游戏后，一个统一入口一次执行协议corpus/双端生产场景和真实Authority阶段。失败记录、不自动重跑。实机不进入原版Online/Isaac Sync，不实施Steam、Replica、snapshot apply或T0-09双游戏矩阵。

## 实现前失败方式

1. 扩展INPUT后仍使用旧wire版本，旧72字节载荷被误解释；HELLO版本/能力与实际codec不一致。
2. 冗余数量越界、长度不精确、未知reserved，或其中一条非法时已把前面的合法输入写入scheduler。
3. 冗余记录改变session/client/sequence/edge，或者不按input sequence从旧到新准入，使补偿包先变成stale。
4. 用packet sequence去重input，丢失不同packet里的同identity幂等性；同seq不同canonical产生多次世界边沿。
5. 乱序旧INPUT覆盖新包，或旧包被丢后仍推进输入/ACK水位；把丢包当作成功准入。
6. ACK和状态共用接收顺序，较新ACK使合法较旧tick的状态消息被跨lane拒绝；更高packet seq的ACK自身server_tick/accepted/processed却回退。
7. source=0的真实中性回执被误拒绝，或者把processed高水位当连续确认/最新输入来源。
8. 状态gap被写成完整/相等；未知schema/domain、非有限值或进程指针进入原始状态观察。
9. FaultTransport容量、时间计算、PRNG或序号溢出，无界分配/循环；queue full仍提交发送，或transport drop错误回滚消息序号。
10. loss/duplicate/reorder/outage不可复现、不同方向串线、未记录精确原始包/到达原因；重复包另造identity。
11. 在network/worker线程写游戏、Lua或Participant；新的B协议路径绕过现有pipeline预算、主线程、代际和统一codec。
12. room/death/rebind后旧transport包获得新generation，旧edge复活；协议Established替代exact pair并提前启动游戏Active。
13. Native使用新协议但controller仍创建旧尺寸mapping；旧v3/v4/v5前缀破坏，扩展配置未冻结或日志环越界。
14. 真实tick完成前发布wire ACK；receipt/gap不对应同一commit，原生调用、Hook事务或恢复被改变。
15. 停止后仍发送/准入，trace未drain，或日志溢出/控制器失联时盲目卸载常驻DLL。
16. golden从被测C++encoder生成导致自证，负向包只被CRC挡住而未真正覆盖语义，fuzz改了原始文件/哈希或自动重试。
17. 离线替身通过冒充真实网络/游戏通过；真实B移动/中性、A继续推进、edge次数与恢复缺少生产事件依据。
18. 使用旧错误PE、已驻留DLL或不确定游戏状态；输入使玩家穿门/死亡；测试窗口持久副作用超出已授权范围。
19. 实机准备前先跑独立probe/smoke/CLI检查，从而增加测试次数；失败后改编号无实质修改重复验收。

所有观察、错误、未覆盖项与唯一最终结果原样保存。文本build仅表示配置标识相同，CRC仅检测包损坏；不计算或比较任何文件/构建/世界哈希。
