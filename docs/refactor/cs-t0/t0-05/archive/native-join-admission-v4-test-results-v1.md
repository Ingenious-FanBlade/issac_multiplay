> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Native join admission v4 independent test results

Date: 2026-09-01  
Work package: `CS-T0-05-04`  
Scope: fresh-B publication identity from allocation return through Init dispatch

The final independent gate completed every required check. Local structural and integrity checks passed, while the bounded native semantic review retained 36 unresolved control items and all eight proof frontiers. The resulting status is `FAILED_PUBLICATION_IDENTITY_PROOF`, with `publicationIdentityStatus=UNKNOWN`, `freshBStatus=UNKNOWN`, `overallAccepted=false`, and exit code 1. This report does not authorize process access, injection, a production rebuild, active join, T0-05-05, T0-05, or T0-06.

## Sealed inputs and final outputs

- Init v6: `cs-t0/coop-native-join-init-closure-v6.json`, SHA-256 `281137C970DC39CE2DF325E817076E325FE37C02D98F1FC6B3A9EB0E57367847`.
- Init v6 explanation: `docs/refactor/cs-t0/t0-05/native-join-init-closure-v6.md`, SHA-256 `3B464889BAB72C978674514156636C62D3F8B7C5AC354EE0502D1E32738F7806`.
- Admission contract v4: `cs-t0/coop-native-join-admission-contract-v4.json`, SHA-256 `91787E4DFB406F5A27C3B03DBEC2BBA648793AA53DE2188DDBEF441866BEB180`.
- Public inspection: `out/cs-t0/native-join-admission-v4-inspection-v1.json`, SHA-256 `7A18C253971833ECC375D67F81C1E8D7BACD710D1928118E6F7F7345B24748CE`; it reports `BLOCKED_BY_EVIDENCE` and exits 1.
- Final independent verification: `cs-t0/coop-native-join-admission-verification-v4-attempt-v2.json`, SHA-256 `EA555C44AD6D47FD33C8D84AAEC8468A4665F6026381C943E71D4650D61EC1BA`.
- Final verifier log: `out/cs-t0/native-join-admission-v4-verifier-attempt-v2.log`, SHA-256 `2E98F0B8D3170A290967C45F4AE5B3B13396FDD68459717A3ECEA8FEF34218CC`.

The final verification binds 426 immutable inputs. It preserves the frozen v3 381-input gate, 174 existing sources, 164 snapshots, 149 original baselines, three production files, and all three mutable indices. The current publication entries remain plan v10 and progress v4.

## Independent results

- Contract counterexamples: 30 passed.
- Synthetic publication-identity counterexamples: 37 passed. Acceptance remains limited to `SYNTHETIC_PUBLICATION_IDENTITY_ONLY` and is not a native proof.
- Submitted publication-evidence counterexamples: 54 passed. These exercise allocation identity, object and path cross-references, publication order, receiver identity, failure paths, unresolved/status coupling, and chain ordering.
- Public CLI blackbox: six groups and 71 assertions passed, including spaced and compact `expectedBytes` spellings, malformed byte rejection, target/root/output aliases, create-new rules, junctions, tampering, and full report equivalence.
- Independent PE reconstruction: 177 document rows; 50,063 instructions, 5,438 calls, 20,320 writes, and 405 exits matched the disk target.
- v5-to-v6 merge: all 62 frozen function entries remained present; six functions were added. Chunks, instructions, blocks, and exits were identical. Call semantic signatures were identical with only canonical target metadata additions. Writer signatures and all fields were identical except 342 explicitly audited one-way alias-label tightenings: 313 persistent-owner labels and 29 source-owner labels were tightened to `STACK_SEMANTICS_NOT_FRESH_B_OR_PERSISTENT_OWNER_PROOF`.
- Explicit v4 dependency audit: four interpreted nodes and 100 explicit edges, no self-binding and no cycle. Historical predecessor contents were treated as opaque leaves rather than recursively reinterpreting snapshot-relative paths.
- Frozen v3 public behavior and the old production seven-gap preflight remained byte-for-byte blocked and exit 1.

The bounded native review independently rebuilt 17 publication functions, PE imports, the Player vtable slot, publication writers and dispatch bytes. It returned `COMPLETED_WITH_UNRESOLVED`; structural references, hashes, counts, and decoded bytes do not close allocation identity, global alias exposure, failure reachability, first exposure, reallocation identity, trap, or dispatch-receiver premises.

## Preserved failed attempts

- Inspector integration failure: `out/cs-t0/native-join-admission-v4-inspector-integration-failure-v1.json`, SHA-256 `0976E157B2854BBE2D371D4948F6AFFE9BC642A10BB3963EFAD223AAA5DCF59D`. It records rejection of the original parser's spaced-byte incompatibility.
- Development verification: `out/cs-t0/native-join-admission-v4-verification-development-v1.json`, SHA-256 `ED44E12CD2AF2917B01C6E058C87EC84683AB5649103B0DFB672D63E72417F66`; log SHA-256 `5F0B1D71D10634F4D27802ECCE150D10421B1DA0641A4F481422157553FF0A6D`.
- First formal attempt: `cs-t0/coop-native-join-admission-verification-v4.json`, SHA-256 `9C73B2903CC3F5D6B6643AE2392923D6C963E1A54A734C22ED24A72099BDFC63`; log SHA-256 `0AEA935D3FC6B8B0EC60F381E7B1022FB9518D2D54D6E176AE6C54909DC190F0`. It retained the initially over-strict writer comparison and the running-log self-mutation failure.

No v11 plan, v5 progress, production binary, sealed predecessor, or mutable index was created or changed. The next proof step remains blocked on native evidence that resolves every publication identity frontier and then the still-UNKNOWN fresh-B and remaining synchronous subproofs.
