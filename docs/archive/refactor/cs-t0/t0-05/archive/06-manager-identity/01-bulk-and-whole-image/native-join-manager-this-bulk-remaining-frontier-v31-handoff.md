> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 positive successor frontier v31 handoff

## Decision

Wave A independently accepted only copy/move RVA `0x001CBEB5` from `MT-R01`.
The two `MT-R09` RVAs `0x0001B072` and `0x0001B3E3` remain `UNKNOWN` and are
not promoted.  The v31 successor therefore advances copy/move from `319/405`
to `320/405`; memset remains `225/242`.  The combined ledger is `545/647`
with exact remaining total `102`.

This is an intermediate positive frontier.  Whole-image and manager-this
promotion remain forbidden, all runtime/process/debugger/IDB/production
permissions remain false, and no v7 artifact is created.

## Main-role composition

- Builder: `out/cs-t0/build-manager-this-bulk-remaining-frontier-v31.py`
  - SHA-256 `61FE8C53B9E7B37B0988FD24A21B73B8B4AD1A5D40E25D84C2EAEB123ABCC7A1`
- Candidate: `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v31.json`
  - SHA-256 `37C23D80BF45F4162305EA174A053E0D294BD9D32B0494D4639B516C2D7551EB`

The builder binds the v30 frontier and the accepted Wave A audit, removes only
`0x001CBEB5` from `callerHeap` and `remainingRvas`, updates negative copy
coverage from `86/86` to `85/85`, and records the one-row positive delta.
Create-new and two consecutive verify-existing runs were byte-stable; the
builder rejected `20/20` focused mutations.

## Independent MT-T02

- Auditor: `out/cs-t0/audit-manager-this-bulk-remaining-frontier-successor-v31-independent-v1.py`
  - SHA-256 `C58CB60F5A5117847ED90062EA84146138B97AA461FD7DC9C8B88FCA0450B994`
- Audit JSON: `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v31-independent-audit-v1.json`
  - SHA-256 `244459B2B5C47A74E1D806E237895A9C7DB896CC748E0710885CFBFAC46F314D`
- Audit handoff:
  `docs/archive/refactor/cs-t0/t0-05/06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v31-independent-audit-v1-handoff.md`
  - SHA-256 `B6B068EF978B7F8C5CD682B5AA485FA288CAB7E21F1551FB4EC10A781FA9EC38`

The TEST role did not read or import the main builder.  It independently
recomputed the exact one-row difference, the group cardinalities
`16/7/23/25/14`, both ledgers, audit coverage and all permission gates.
Create-new plus two verify-existing runs were byte-stable and `59/59`
mutations were rejected.  The larger Wave A suite predates the user's later
request to compress subsequent test scope; future waves use focused tests.
