> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 native alias closure split rule-hit retry TEST results v12

## 1. Identity and fixed evidence

- Role: independent `TEST`.
- Scope: `CS-T0-05-05 / NATIVE-ALIAS-CLOSURE` only.
- Governing preseal: `virtual-device-native-alias-closure-test-plan-v12.md`, SHA-256 `26B69852F533D511508D0116EC009CCBA2B5A348B8B739940CC052DEE6557E79`.
- Failure result v11: SHA-256 `39425C9DC84C4CAE092CE6BA34B400E0C6222CC4A1D89EF592EA6661E9A03F13`.
- Independent split rule-hit diagnostic: SHA-256 `172BD3D0CE2418DB9183AC97B21B5C085C9D74BA5B8E79A7D5D3C22EFF8EF090`.
- Immutable manifest v1: SHA-256 `ECBB06126F85C7293BE816683C302E49F941518B7DF6F7D94C3996A57C495735`.
- Immutable REVERSE v1 handoff: SHA-256 `DE8B1168F07F147F166EFDFCC7A8C9E3A5AEC32FBBC8C3BC332AE46DA112F2C3`.
- Active manifest v2: `cs-t0/native-alias-universe-v2.json`, SHA-256 `1C1871DE6189AAD8A3D91BE7D60D226AD388EAC0B459F5FB624AA3E4B2F1EFE4`.
- Active REVERSE v2 handoff: SHA-256 `1845D7729D9490E16B66136F4045E25E9E48FBE4649FBA84FE4540E5C1142D48`.
- Objdump-width diagnostic: SHA-256 `D01912DA14DC524E320CDEE8C8BAF5456158CEB25AFB4C2C3A296FA01B36F188`.
- Verifier: SHA-256 `7363F8E3C191CBAA872536FA3B04FD96A311EEADB1FC51FBD6663D026075EBB2`.
- Decision: SHA-256 `E80C7873C0499F5FD920E7D10472E4C6CB7E82157F459DEB356A745E83987448`.
- Public wrapper: SHA-256 `5084EDD447DD3BF07FE67712BA260D66F0A0B212BE67630B6A587B1118068ED4`.
- Internal runner: SHA-256 `EB826B2D0CAD5BA0AF0693F5ED0B93DFEDFBBEFDCFECC20C213B676DFD0DC01F`.
- Two-method negative module: SHA-256 `27BE0B3072AD944C74669976DDF57012C9B1C779DFD84E46EFA1D5F495D30B57`.
- Fixed PE: Win32/x86 SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`.
- Bound `objdump`: SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`, banner `GNU objdump (GNU Binutils) 2.44`.

The decision contained exactly 37 ordered artifact bindings, and the pre-execution static review found every bound path and SHA-256 exact. All v1-v11 plan/result lineage remains immutable and consumed.

No game process, IDA, x32dbg, IPC, hook, injection, input mutation, or world mutation was used.

## 2. Mandatory static-review disposition

The read-only review authorized v12 formal execution:

- intermediate split state-machine diagnostics remained fixed at `{270,53,14,0,0}`;
- contractual `observedRuleHits` were independently derived after reconstruction from final candidate recognition, chain, and transfer state as `{270,0,0,0,0}`;
- all 270 split candidate rows, 489 base candidates, 759 core rows, 34,430 residual rows, and exact partition checks remained strict;
- verifier SHA-256 was `7363F8E3C191CBAA872536FA3B04FD96A311EEADB1FC51FBD6663D026075EBB2`;
- decision SHA-256 was `E80C7873C0499F5FD920E7D10472E4C6CB7E82157F459DEB356A745E83987448` with 37 bindings;
- `UNKNOWN`, no promotion, false permissions/unlocks, and absent retry-v11 output/result state were retained.

The static review did not run, import, or compile the verifier and did not invoke the wrapper, runner, tests, or `objdump`.

## 3. Exactly four compressed acceptance groups

Each sealed group was invoked exactly once, in order:

| Group | Result | Exit code | Summary |
|---|---|---:|---|
| 1. Internal fixed-PE / anchor / independent alias oracle | `PASS` | `0` | `verificationStatus=PASS`, manifest `IsaacCsT0-NativeAliasUniverse-v2`, `aliasVerdict=UNKNOWN`, `dynamicAliasUniverseClosed=false`, exact first edge at `0x00024037`, `outputWritten=false`. |
| 2. Public CLI create-new | `PASS` | `0` | All seven named checks PASS; `artifactBindingCount=37`, `outputWritten=true`, `formalDecision=null`, `BLOCKED_BY_EVIDENCE`, `ownerPromotionCount=0`, all permissions/unlocks false. |
| 3. Public CLI verify-existing and immutability | `PASS` | `0` | `verificationStatus=PASS`, `outputWritten=false`; all 39 measured evidence files retained identical path/length/UTC/SHA tuples. |
| 4. Parameterized negative module | `PASS` | `0` | Exactly two test methods ran once in `17.255s`: `test_semantic_mutations` and `test_write_boundaries`; result `OK`. |

No group was repeated.

## 4. Exact commands

Group 1:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\run_native_alias_oracle_v1.py --game-exe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" --objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" --repository-root "D:\projects\isaac_hook_reserach"
```

Group 2:

```powershell
pwsh -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v5.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -Objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" -RepositoryRoot "D:\projects\isaac_hook_reserach" -Output "out\cs-t0\virtual-device-native-alias-closure-verification-20260909-v5-retry-v11.json"
```

Group 3 public invocation, surrounded by the in-memory before/after metadata capture described below:

```powershell
pwsh -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v5.ps1 -GameExe "F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe" -Objdump "D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe" -RepositoryRoot "D:\projects\isaac_hook_reserach"
```

Group 4:

```powershell
python -B .\cs-t0\tests\virtual_device_ownership_lifecycle_decision_v5\test_virtual_device_ownership_lifecycle_decision_v5.py
```

## 5. Group 3 immutability evidence

The measured set was the 37 bound files plus the v5 decision and Group 2 output, for 39 unique files total. Each snapshot row contained the repository-relative path, byte length, `LastWriteTimeUtc` in round-trip format, and SHA-256.

| Measurement | Before | After |
|---|---|---|
| File count | `39` | `39` |
| Canonical snapshot SHA-256 | `83235E163186AF29C62A6F8EBA53902FE3E293BEDB7B2BD39087D59F10F0DE5E` | `83235E163186AF29C62A6F8EBA53902FE3E293BEDB7B2BD39087D59F10F0DE5E` |
| Changed paths | none | none |
| v12 result present | false | false |

The Group 2 output tuple was identical before and after:

- path: `out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v11.json`;
- length: `5268` bytes;
- last-write UTC: `2026-09-09T03:45:38.7150099Z`;
- SHA-256: `51191D57704C25E643688BB8698309D250E2B16A4717F9275CD275D4D25EB360`.

The metadata presentation expression emitted two non-terminating `Where-Object` parameter warnings after both snapshots and the equality comparison had completed. It did not invoke the wrapper again, change the wrapper exit code, or affect the measured set: the Group 2 output was already one of the 39 rows, `changedPaths=[]`, and the two complete snapshot hashes were identical. Its tuple above was read back without rerunning any formal entry point.

## 6. Conservative accepted verdict

The v12 verification passes as a conservative blocked decision, not as an alias-closure promotion:

- output SHA-256: `51191D57704C25E643688BB8698309D250E2B16A4717F9275CD275D4D25EB360`;
- `aliasVerdict=UNKNOWN`;
- `dynamicAliasUniverseClosed=false`;
- `allAliasesBoundedAndReleasedBeforeManagerRemove=false`;
- `nativeOwnerHeldThroughDrain=false`;
- `formalDecision=null`;
- `workPackageStatus=BLOCKED_BY_EVIDENCE`;
- `ownerPromotionCount=0`;
- `VIRTUAL_DEVICE_OBJECT_OWNER=UNKNOWN`;
- all runtime/implementation permissions and CS-T0-05-06/T0-05/T0-06 unlocks remain false.

The exact first unresolved edge remains:

- function RVA: `0x00023CC0`;
- instruction RVA: `0x00024037`;
- raw bytes: `FF D0`;
- edge kind: `POST_RELEASE_INDIRECT_CALLBACK_WITH_PAIR_ADDRESS`;
- source: callback slot RVA `0x0087E814` after control release at RVA `0x00024024`;
- prospective destination: unknown callback target / potential retained pair alias;
- reason: the PE has 101 decoded loads and no decoded store of the zero-filled `.data` callback slot, so its target and alias disposition cannot be closed statically.

This satisfies the plan's allowed conservative outcome: a complete, auditable blocked result with an exact unresolved frontier. It does not claim universe completeness, drainability, or owner-through-drain.

## 7. Explicitly omitted work

The v4 offline lifecycle harness, Cargo tests, full T0 build, Debug/Release Native matrices, historical broad regression, duplicate self-test, standalone `objdump`, game, IDA, x32dbg, process probes, canary, IPC, hooks, injection, input mutation, and world mutation were not run. No historical v1-v11 report or hashed evidence file was deleted or overwritten.
