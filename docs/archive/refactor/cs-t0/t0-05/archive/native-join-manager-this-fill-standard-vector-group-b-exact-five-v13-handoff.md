> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 standard-vector group B exact5 v13 negative handoff

## Scope and decision

Exact scope only, in this order:

`0x00331054, 0x003310C4, 0x0033112F, 0x003350C6, 0x0033685C`.

Result: **0 VERIFIED / 5 UNKNOWN**. No other v11 row is adjudicated. All permissions,
row/whole-image/manager-this promotions, and `overallAccepted` are `false`.

Frozen bindings: standard-vector v11 analyzer/JSON, corrected whole-image v2, accepted
manager-tuple global v20, and target SHA256
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
The v20 result is identity-specific to `DWORD[0x00C71678]+0x1BAA8`; it is not
transferred to any of these five receivers.

## Per-row first gaps

- `0x00331054`: `ROOT_2A404_PLUS30_OLD_BACKING_ALIAS_WRITER_FIXED_POINT_OPEN`.
  `sub_7252A0` reset and `sub_72CFF0` parser carry an already-live `object+0x30`
  tuple; copy/move/assignment/swap/adoption and return/stack-liveout closure is open.
- `0x003310C4`: `ROOT_2A404_PLUS24_OLD_BACKING_ALIAS_WRITER_FIXED_POINT_OPEN`.
  The corresponding `object+0x24` old-backing routes through `sub_7252A0` and
  `sub_72CFF0` remain open.
- `0x0033112F`:
  `EXACT18_MIXED_RECEIVER_AND_ADDRESS_TAKEN_755470_WRITER_UNIVERSE_OPEN`.
  All 18 direct callers are frozen, but caller-supplied tuples, mixed member
  lifetimes, and indirect owner receivers of address-taken `sub_755470`
  (`0x00B6BDD4`) are not closed.
- `0x003350C6`: `GAMEROOT_PLUS1A740_MEMBER794_ALIAS_WRITER_FIXED_POINT_OPEN`.
  All seven callers form `GameRoot+0x1A740`; the tuple is receiver `+0x794`
  (`GameRoot+0x1AED4`). Its full constructor/copy/move/assignment/swap/adoption and
  already-offset alias universe, including the `sub_426980` destructor escape, is open.
- `0x0033685C`: `SUB_6F9000_CALLER_OBJECT_PLUS1E3CC_TUPLE_PRODUCER_OPEN`.
  The unique `0x006F9271` caller supplies `caller object+0x1E3CC`; this is not the
  v20 manager tuple, and its complete producer/lifetime/liveout universe is open.

For every row, the in-place inequality remains conditional on a well-formed tuple.
Fresh growth is separately closed: stride-4 capacity is bounded by `0x3FFFFFFF`
(`4*capacity <= 0xFFFFFFFC`); stride-8 capacity is bounded by `0x1FFFFFFF`
(`8*capacity <= 0xFFFFFFF8`). Allocation failure precedes tuple publication.

## Replay and mutation status

PE create plus two consecutive verifies all returned the same machine SHA:

```text
PASS mode=PE ... sha256=305CC2421A4336DC3560391A9E4847CA902D4BC7D9380D878ADB294E9C83062D verified=0 unknown=5 mutations=28/28 permissions=false promotions=false overallAccepted=false
```

IDA read-only replay was attempted after the PE checks, but the IDA MCP endpoint
timed out repeatedly. Final status is **`TIMEOUT_NO_PASS`**. No IDA PASS is claimed,
and the timeout does not promote any row. Independent audit must rerun the same
analyzer in the frozen IDB and require byte-for-byte equality with the existing JSON.

## Artifacts

- `out/cs-t0/analyze-manager-this-fill-standard-vector-group-b-exact-five-v13.py`
  - SHA256 `0AC70297C30E7A2EC2E0BD377F21DE87CF04ACC4FBC7957B8BCDB247169A0BEF`
- `out/cs-t0/native-join-manager-this-fill-standard-vector-group-b-exact-five-v13.json`
  - SHA256 `305CC2421A4336DC3560391A9E4847CA902D4BC7D9380D878ADB294E9C83062D`

No game, debugger, process open/read, runtime test, IDB mutation, production/test
change, prior-evidence edit, or docs-index change was performed.
