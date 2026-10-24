> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure objdump-width retry TEST preseal v9

## 1. Scope and consumed v8 boundary

This append-only preseal authorizes one minimal `NATIVE-ALIAS-CLOSURE` retry after the consumed v8 Group 1 failure. It does not authorize new alias-universe evidence, further REVERSE work, factory/rollback work, final-release/thread work, production implementation, runtime access, or wider testing.

The immediately governing lineage is:

| Artifact | SHA-256 |
|---|---|
| TEST preseal v8 | `C564E3FF93EC1C83BC881E78B7154E9D2364D50540896F5051B7E61309969A92` |
| v8 failed verifier | `FBC831EF48814BF6F7A9BE2F61D0814C480F7535B77A796F67D210855AE74B51` |
| v8 failed decision | `EDB3566785E6D02470BEFE7CC2FF47B857C34016EA60FA7176A78F7AD66C4DF3` |
| Failure result v8 | `784BBDC398A9A442E0106F74A4F1909865956D7B55A7BE280AC91DB71B726005` |
| Independent width diagnostic | `D01912DA14DC524E320CDEE8C8BAF5456158CEB25AFB4C2C3A296FA01B36F188` |

The diagnostic path is `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-objdump-width-reverse-v1.md`.

The v8 Group 1 invocation is permanently consumed. It returned outer exit code `1`, verifier `exitCode=2`, `verificationStatus=REJECTED`, `outputWritten=false`, and error `SERVICE_GLOBAL_REFERENCE_ROW:SERVICE-GLOBAL-REF-0001`. Groups 2-4 were not executed and the retry-v7 JSON remained absent.

All v1-v7 and v8 plan/result lineage already frozen by the preceding preseals remains immutable and is not reopened here.

## 2. Immutable evidence and test surfaces

These artifacts must remain byte-for-byte unchanged:

- `cs-t0/native-alias-universe-v1.json`, SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- `virtual-device-native-alias-closure-reverse-v1.md`, SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- `cs-t0/native-alias-universe-v2.json`, SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`.
- `virtual-device-native-alias-closure-reverse-v2.md`, SHA-256 `1845D7729D9490E16B66136F4045E25E9E48FBE4649FBA84FE4540E5C1142D48`.
- The independent width diagnostic named above.
- Public wrapper, SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal Group 1 runner, SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Compressed negative module, SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`, exactly two test methods.
- Fixed PE Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump` SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

No manifest, REVERSE document, diagnostic, wrapper, runner, negative test, production DLL, offline lifecycle source, build script, earlier result, or earlier verification output may be edited.

## 3. Sealed diagnostic

The independent diagnostic establishes the narrow cause of the v8 rejection:

- fixed `objdump` in its default 80-column mode prints the ten-byte instruction for `SERVICE-GLOBAL-REF-0001` as a seven-byte instruction row followed by a three-byte continuation line;
- the verifier's line-oriented census therefore does not receive the complete instruction row needed for the frozen manifest comparison;
- the instruction RVA, whole-image service-global reference count, canonical order, raw bytes, and decoded instruction are otherwise correct;
- `objdump -w` disables line wrapping and emits the ten-byte instruction as one parseable row.

This is a tool-output-width issue. It does not authorize changes to the manifest row, census counts/order, decoder regex, evidence graph, closure verdict, or lifecycle decision semantics.

## 4. Exact verifier repair budget

The only authorized verifier logic change is the disassembly subprocess argument list inside `run_objdump`:

```python
[str(resolved), "-d", str(game_exe.resolve())]
```

must become exactly:

```python
[str(resolved), "-d", "-w", str(game_exe.resolve())]
```

The version invocation, tool identity/path/banner/hash checks, timeout, return-code checks, parser, regular expressions, PE validation, all census and ordering rules, raw/decode comparisons, instruction-span mode from v8, graph/frontier reconstruction, nullable-range handling, path/write boundaries, and decision rules must remain unchanged.

No fallback invocation, retry, standalone `objdump`, output postprocessing, continuation-line merger, alternate width, or parser relaxation is authorized.

## 5. Main-integration and exact binding budget

After this preseal is hashed, the main integrator may:

- make only the exact one-argument `run_objdump` edit in Section 4;
- update the decision's verifier SHA-256 to the repaired verifier hash;
- add exactly three lineage bindings for the v8 result, independent width diagnostic, and this v9 preseal;
- mechanically update the decision binding count/order and resulting decision SHA-256.

The current 27 bindings remain in their existing relative order. Insert these three rows immediately after the existing v8-preseal row and before the immutable manifest-v1 row:

1. `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v8.md`, SHA-256 `784BBDC398A9A442E0106F74A4F1909865956D7B55A7BE280AC91DB71B726005`, role `V5_RETRY_V8_FAILED_TEST_RESULT`;
2. `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-objdump-width-reverse-v1.md`, SHA-256 `D01912DA14DC524E320CDEE8C8BAF5456158CEB25AFB4C2C3A296FA01B36F188`, role `INDEPENDENT_OBJDUMP_WIDTH_DIAGNOSTIC`;
3. `docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-plan-v9.md`, SHA-256 equal to this create-new preseal's final disk hash, role `TEST_RETRY_PRESEAL_V9`.

The final decision and verifier binding specification must each contain exactly 30 rows in that order. Manifest v2 remains the active alias evidence and is not rewritten merely to bind this retry.

Before formal execution, independent TEST must perform a read-only static review proving that the verifier diff is exactly the Section 4 argument plus the three binding constants/specifications, the decision diff is purely mechanical, all 30 paths/roles/disk hashes and their order match, and an in-memory removal of only the v9 changes restores the v8 verifier and decision hashes. This review must not run, import, or compile the verifier and must not invoke the wrapper, runner, tests, or `objdump`; it does not consume a formal group.

## 6. Acceptance semantics remain unchanged

- Manifest identity remains `IsaacCsT0-NativeAliasUniverse-v2` and evidence verdict remains `UNKNOWN`.
- Receiver dispatch and returned `{object,control}` pair transfer remain separate fixed points.
- Universe completeness remains separate from drainability and native owner through drain.
- `dynamicAliasUniverseClosed=false`, `formalDecision=null`, and `workPackageStatus=BLOCKED_BY_EVIDENCE` remain mandatory.
- `VIRTUAL_DEVICE_OBJECT_OWNER=UNKNOWN`; no promotion is authorized.
- All factory, rollback, reference-count, final-release, and thread conclusions remain inherited unchanged.
- Every CS-T0-05-06/T0-05/T0-06 unlock and every implementation/runtime/scheduler/transport/mutation permission remains false.

## 7. Distinct v9 publication paths

The only permitted create-new formal output is:

`out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v8.json`

The v9 TEST result path is:

`docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v9.md`

Both paths must be absent before Group 1. Every earlier retry output remains retired and must not be created or reused.

## 8. Exactly four v9 groups, once each, fail-fast

Only after the mandatory static review passes:

1. **Internal oracle — one invocation.** Run the unchanged internal runner once with the fixed PE, bound `objdump`, and repository root. Require exit `0`, PASS, manifest v2 identity, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, the exact first unresolved edge, `outputWritten=false`, and no publication.
2. **Public CLI create-new — one invocation.** Run the unchanged public wrapper once with fixed PE, bound `objdump`, repository root, and retry-v8 output. Require exit `0`, PASS, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, no promotion, and all permissions/unlocks false.
3. **Public CLI verify-existing — one invocation.** Run the wrapper once without `-Output`. Require exit `0`, PASS, `outputWritten=false`, and no change to repository/bound/generated evidence or Group 2 output, measured by path, length, UTC time, and SHA-256 before/after.
4. **Parameterized negative module — one invocation.** Run the single unchanged unittest module once; it must contain only `test_semantic_mutations` and `test_write_boundaries`, with table-driven subtests sharing one fixture.

Any failure stops all later groups. No invocation may be repeated for diagnosis or confirmation under this seal.

## 9. Result record and prohibitions

After the attempt, create `virtual-device-native-alias-closure-test-results-v9.md` with create-new semantics. On success record all four exact commands, exit codes/summaries, static-review disposition, 30 binding rows, manifest/REVERSE/diagnostic/verifier/decision/output hashes, Group 3 immutability measurements, negative method count, `UNKNOWN` verdict/first edge, no promotion/unlock, lineage, and omitted work. On failure record the consumed group and mark every later group `NOT_EXECUTED`.

Do not run the v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, game, IDA, x32dbg, process probes, canary, IPC, hook, injection, input mutation, or world mutation. Do not invoke standalone `objdump`; it may be used only through the four sealed verifier entry points.

## 10. Pre-execution state

At creation of this preseal no v9 formal group has run. The retry-v8 JSON and v9 TEST result are absent. The only formal executions on record remain the eight consumed Group 1 attempts documented by results v1-v8.
