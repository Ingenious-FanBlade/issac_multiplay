> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02/R03/R05 exact5 sealed independent TEST audit v2 handoff

## Outcome

Independent TEST accepts the physical REVERSE candidate as an exact-five-row, all-`UNKNOWN` result. No row is eligible for frontier promotion:

| Task | RVA | Verdict | First open obligation | Retained first gap |
|---|---|---|---|---|
| MT-R02 | `0x00008A62` | `UNKNOWN` | `backingExtent` | `R02_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN` |
| MT-R02 | `0x00008B2A` | `UNKNOWN` | `backingExtent` | `R02_SECOND_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN` |
| MT-R03 | `0x0001513A` | `UNKNOWN` | `backingExtent` | `R03_EMITTED_COUNT_LE_5000_OPEN_84` |
| MT-R03 | `0x000155A4` | `UNKNOWN` | `backingExtent` | `R03_EMITTED_COUNT_LE_5000_OPEN_108` |
| MT-R05 | `0x0033570B` | `UNKNOWN` | `countAndNoWrap` | `R05_AUX_NONEMPTY_FILTERED_EMPTY_CONJUNCTION_OPEN` |

For every row, TEST independently recomputed the frozen-PE call bytes/helper target and checked the candidate proof-atom bytes and digests. The precise sealed countermodel is retained for each first-open obligation. Classification is 0 positive / 5 unknown; `frontierCompositionEligible`, manager-this promotion, and whole-image promotion are all false.

No REVERSE builder was read or imported. The candidate, frontier, indexes, checkpoint, sealed specification, sealed validator, preparation, production code, historical evidence, and IDB were not modified. No game/process/debugger/runtime activity occurred.

## Physical bindings and outputs

| File | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-mt-r02-r03-r05-exact5-reverse-candidate-v1.json` | `47B1E320B329C5571D2C7F6CD35D48EBDF89D69E5EE6B5BC7C659E509689F694` |
| `out/cs-t0/native-join-manager-this-mt-r02-r03-r05-exact5-sealed-acceptance-spec-v1.json` | `B8EA5449E90F398D2B2514D9176810E4B91986783AE3E0482ECF4B07F79D6A93` |
| `out/cs-t0/audit-manager-this-mt-r02-r03-r05-exact5-sealed-v1.py` | `E70C507AFCA086435F1DC939BDD86DDC3DFA18E882877F7FF46C034E60B97027` |
| `out/cs-t0/native-join-manager-this-mt-r02-r03-r05-exact5-sealed-preparation-v1.json` | `36231A55B099685A06F00D4F7FE2F315EFCDB11CFCF5266FD0EFE65558593F4C` |
| `out/cs-t0/audit-manager-this-mt-r02-r03-r05-exact5-post-candidate-v2.py` | `9F65707DC077323D28C742151EDEB79950F4CF40D80041F91E6EB6442EA142F9` |
| `out/cs-t0/native-join-manager-this-mt-r02-r03-r05-exact5-sealed-independent-audit-v2.json` | `C17A2589256FEF6152F5878FE0FADDE7219C68D8B5ED5F852CE9704A85E32ECE` |

The v2 wrapper is append-only. It imports the sealed TEST validator only after verifying its physical hash. It does not supersede the sealed acceptance specification, preparation, or admission contract.

## Harness-only and implementation corrections

Two candidate-mode mutations in the original sealed validator were inapplicable to this all-`UNKNOWN` candidate:

1. Clearing evidence on an open UNKNOWN obligation is not forbidden by the sealed contract. v2 instead deletes a proof atom that the obligation references, which decisively breaks the reference.
2. Removing a positive target-interval field is a no-op when there are no positive rows. v2 uses `baseline-countermodel-drift` for an all-UNKNOWN candidate and retains `positive-target-interval-incomplete` for any future candidate containing a positive row.

A second pre-write run exposed a separate validator implementation bug: the sealed v1 function reused the final `exact` loop variable when applying the UNKNOWN baseline ID/countermodel equality. This accidentally skipped that equality for R02/R03 because their first-open obligation differs from the final R05 row's baseline obligation. The sealed rule itself was already correct and remains unchanged. v2 calls the base validator and then reapplies that same equality with an explicit lookup and loop for every exact5 RVA; all custom mutations must pass both checks.

The v2 artifact therefore supersedes only the post-candidate validator implementation/mutation harness. `admissionContractChanged` is false. The sealed spec, validator, preparation, and their hashes remain byte-identical.

Two exploratory candidate-mode runs failed before audit creation:

- `candidate mutation accepted: obligation-evidence-omission`;
- `corrected candidate mutation accepted: baseline-countermodel-drift`.

Both failures occurred before the exclusive create-new write. No audit file was produced by either failed run. They are retained as explained failures rather than overwritten evidence.

## Compressed verification

Create-new:

```text
python out/cs-t0/audit-manager-this-mt-r02-r03-r05-exact5-post-candidate-v2.py --candidate out/cs-t0/native-join-manager-this-mt-r02-r03-r05-exact5-reverse-candidate-v1.json --candidate-sha256 47B1E320B329C5571D2C7F6CD35D48EBDF89D69E5EE6B5BC7C659E509689F694 --output out/cs-t0/native-join-manager-this-mt-r02-r03-r05-exact5-sealed-independent-audit-v2.json
```

Result: PASS, created SHA-256 `C17A2589256FEF6152F5878FE0FADDE7219C68D8B5ED5F852CE9704A85E32ECE`, exact=5, positive=0, unknown=5, mutations rejected=6.

Exactly one verify-existing run used the same command plus `--verify-existing`. Result: PASS and byte-identical with the same SHA-256.

The six decision-relevant mutations rejected were:

1. exact-row omission;
2. fake positive without closed universal obligations;
3. referenced proof-atom omission;
4. UNKNOWN first gap not first open;
5. baseline countermodel drift;
6. permission enabled.

One wrong-candidate-hash probe supplied 64 zeroes. It failed with `candidate requested hash drift`, exit code 1; the probe output was absent before and after, proving fail-before-write.

No Cargo test, full T0 build, broad suite, or repeated self-test was run.

## Final decision

`PASS_SEALED_EXACT5_ALL_UNKNOWN_HARNESS_V2` is the final independent TEST verdict for this candidate. The five rows remain in the successor remaining set, and this audit alone authorizes no frontier or index edit.
