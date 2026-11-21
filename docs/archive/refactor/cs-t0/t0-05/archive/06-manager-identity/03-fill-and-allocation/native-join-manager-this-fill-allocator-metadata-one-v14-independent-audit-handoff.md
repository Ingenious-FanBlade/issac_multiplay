> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# allocator-metadata exact1 v14 独立审计交接

## 结论

- 审计结论：`UNKNOWN`。
- 不接受候选的 `VERIFIED`，不计数；copy 计数保持 `315/405`。
- 唯一首缺口：`AEBE30_AEA9B0_ALTERNATE_BACKING_PRODUCER_NOT_CLOSED`。
- 本审计只做磁盘 PE 静态一致性复核；未运行游戏、未附加调试器、未执行网络或运行时测试。
- IDA 状态只记录为 `TIMEOUT_NO_PASS`，不计为通过。

## 候选绑定与不变性

候选三文件在 create-new、两次 verify-existing 的每次运行前后均保持下列 SHA-256：

- `out/cs-t0/analyze-manager-this-fill-allocator-metadata-one-v14.py`
  `10BBB35A226A006363F7740A94933824D2F9E618F0F1BC36F4AE52C3A8F0098D`
- `out/cs-t0/native-join-manager-this-fill-allocator-metadata-one-v14.json`
  `0EF108E9B68EEA47F543B25AAA7BD1707BA7404E39BF4063D6A5B805352172C1`
- `docs/archive/refactor/cs-t0/t0-05/native-join-manager-this-fill-allocator-metadata-one-v14-handoff.md`
  `19F1396BE7245015A39AB8BDAB6CB63662B9052B758534E1B0941BA31CE8E7D1`

候选文件没有被修改。

## 独立重建的反例路径

冻结目标为 `isaac-ng.exe` SHA-256
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。

独立 analyzer 从 PE 字节重建并校验：

1. `0x00AECB5B` 是 `sub_AEBE30` 的直接 caller；调用前最后三个 cdecl push 均为零，因此 `AEBE30 arg1=arg2=arg3=0`。
2. `AEBE30 arg3==0` 经 `0x00AEBF49` 到达 `0x00AEBFCF`。
3. `0x00AEBFF6` 调用 `sub_AEA9B0`，随后 `0x00AEBFFB` 执行 `EBX=EAX`。
4. 若返回非空，`0x00AEC014` 的成功分支直接跳到公共汇合点 `0x00AEC08E`，绕过 `sub_AE59B0`。
5. 同一 EBX 后续在 `0x00AEC161` 形成 `EBX+0x44`，并由 `0x00AEC167` 的 exact row 清零 `48*ESI+0x58` 字节。

因此，`sub_AEA9B0` 是 exact-row destination backing 的可达 alternate producer。候选只闭合了
`sub_AE59B0` 的 cache/fallback backing；没有闭合该 alternate producer 的 allocator-owned extent、
metadata/header 与 capacity 不等式。该缺口足以阻止 universal `VERIFIED`。

## 可复现包

- 独立 analyzer：
  `out/cs-t0/audit-manager-this-fill-allocator-metadata-one-v14-independent.py`
  SHA-256 `9A3E836AF157C2F8888716CC651BB4379F5908960163BB373B8E7CFFDE5E6C17`
- machine JSON：
  `out/cs-t0/native-join-manager-this-fill-allocator-metadata-one-v14-independent-audit.json`
  SHA-256 `45AEF9007B2988066278CA1CD2BB7A0ACE5A0A6FB2AC41F4D8BCC1608DD3BD6E`

create-new：

```text
python out/cs-t0/audit-manager-this-fill-allocator-metadata-one-v14-independent.py
ok=true action=created-new verdict=UNKNOWN mutationsRejected=6 candidateUnchanged=true
```

连续 verify-existing #1 与 #2：

```text
python out/cs-t0/audit-manager-this-fill-allocator-metadata-one-v14-independent.py --verify-existing
ok=true action=verified-existing outputSha256=45AEF900... mutationsRejected=6 candidateUnchanged=true
python out/cs-t0/audit-manager-this-fill-allocator-metadata-one-v14-independent.py --verify-existing
ok=true action=verified-existing outputSha256=45AEF900... mutationsRejected=6 candidateUnchanged=true
```

6 项语义 mutation 均被拒绝：alternate callsite、success join、exact destination、reachable caller
argument、verdict promotion、permission promotion。

## 权限与晋级

- active test：`false`
- run game：`false`
- attach debugger：`false`
- network/runtime test：`false`
- promotion：`false`
- matrix advance：`false`

