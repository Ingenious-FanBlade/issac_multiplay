> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R03 assumption-waiver 独立审计交接 v1

日期：2026-09-06  
角色：独立 TEST  
结论：`PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`

## 1. 结果

预封印 validator/spec 已独立接受真实 candidate
`out/cs-t0/native-join-manager-this-mt-r03-assumption-waiver-v1.json`，candidate
SHA-256 为 `A3A659A77B0167E71E63D6D216CBDE15E77103194C490A1273E8914EEA2EF865`。

create-new audit：
`out/cs-t0/native-join-manager-this-mt-r03-assumption-waiver-sealed-independent-audit-v1.json`  
SHA-256：`758F92C60890DE425E19932231019A7270148CD7FA01D29DD2BB92C472EA63D2`

该 PASS 只接受 MT-R03 exact2 的用户风险 waiver。两行静态 verdict 均继续为
`UNKNOWN`，零 `STATIC_CONFIRMED_NO_OVERLAP`、零 runtime observation；v33 strict
frontier 和 strict remaining 99 不变。人工逆向 policy queue 从 98 降至 96
（copy/move 79、memset 17）。frontier composition、manager-this、whole-image、
runtime、active、T0-05 和 T0-06 权限均保持关闭。

TEST 未读取/import candidate builder，未修改 candidate、spec 或 validator。

## 2. 极简测试

- candidate audit create-new 一次：退出 0；
- verify-existing 一次：退出 0且字节一致；
- 封印三类 mutation 全拒绝；
- wrong-candidate-hash 一次：退出 1，`candidate physical/requested hash drift`，
  探针输出前后均不存在。

没有运行 Cargo、build、历史宽回归、游戏、进程访问、调试器或 IDB 操作。

create-new 命令：

```powershell
python out\cs-t0\audit-manager-this-mt-r03-assumption-waiver-sealed-v1.py `
  --spec out\cs-t0\native-join-manager-this-mt-r03-assumption-waiver-sealed-acceptance-spec-v1.json `
  --spec-sha256 2167D5368C43C52EF909AC16AD92C043E2BE77BBBD3F366BD647C5A991C19616 `
  --candidate out\cs-t0\native-join-manager-this-mt-r03-assumption-waiver-v1.json `
  --candidate-sha256 A3A659A77B0167E71E63D6D216CBDE15E77103194C490A1273E8914EEA2EF865 `
  --output out\cs-t0\native-join-manager-this-mt-r03-assumption-waiver-sealed-independent-audit-v1.json
```

verify-existing 使用同一命令仅追加 `--verify-existing`。
