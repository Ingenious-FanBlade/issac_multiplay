> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R05 assumption-waiver 独立审计交接 v1

日期：2026-09-06  
角色：独立 TEST  
结论：`PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`

## 1. 验收结果

预封印 validator/spec 已对真实 candidate
`out/cs-t0/native-join-manager-this-mt-r05-assumption-waiver-v1.json` 完成独立验收。
candidate SHA-256 为
`04B713B932028873756C860057D138B2F1E63EFF481B812CF0B369479EBD629A`；
create-new audit 为
`out/cs-t0/native-join-manager-this-mt-r05-assumption-waiver-sealed-independent-audit-v1.json`，
SHA-256 为
`92FC838C9063514508D79BCE65174D7A57ACDB4BBEA01BF6E28048506E295C5A`。

本 PASS 只接受用户 assumption waiver：`MT-R05 / 0x0033570B` 从人工逆向队列移除。
静态 verdict 继续为 `UNKNOWN`，`staticConfirmedNoOverlap=false`，
`runtimeObserved=false`，v33 machine frontier 未修改且 remaining 99 保留。
manager-this、whole-image、runtime、active、T0-05、T0-06 权限均未授予。

TEST 未读取或 import candidate builder，未修改 candidate、sealed spec 或 validator。

## 2. 压缩测试

正式执行仅包含：

1. candidate audit create-new 一次，退出 0；
2. 同一 audit verify-existing 一次，退出 0且字节一致；
3. 封印的三类 mutation 全拒绝：`target-or-scope-drift`、
   `assumption-mislabeled-as-proof`、
   `waiver-effect-or-permission-escalation`；
4. wrong-candidate-hash 一次，退出 1并报告
   `candidate physical/requested hash drift`；探针输出前后均不存在。

create-new 命令为：

```powershell
python out\cs-t0\audit-manager-this-mt-r05-assumption-waiver-sealed-v1.py `
  --spec out\cs-t0\native-join-manager-this-mt-r05-assumption-waiver-sealed-acceptance-spec-v1.json `
  --spec-sha256 4977DBEE6D2DDAC78E5B25EF9BEE87B9D0D27DCE3DB8BF49FD595FED76D387AE `
  --candidate out\cs-t0\native-join-manager-this-mt-r05-assumption-waiver-v1.json `
  --candidate-sha256 04B713B932028873756C860057D138B2F1E63EFF481B812CF0B369479EBD629A `
  --output out\cs-t0\native-join-manager-this-mt-r05-assumption-waiver-sealed-independent-audit-v1.json
```

verify-existing 使用同一命令仅追加 `--verify-existing`。没有运行 Cargo、build、历史宽回归、
游戏、进程访问、调试器或 IDB 操作。
