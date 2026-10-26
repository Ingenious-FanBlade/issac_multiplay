# tx-04-07 E2E

Result: **FAIL**

Single launch; no automatic retry; no save restoration.

- V1: PASS
- V2: FAIL
- V3: FAIL
- V4: FAIL
- V5: FAIL
- V6: FAIL
- V7: PASS
- V8: FAIL
- V9: FAIL
- V10: NOT_RUN
- V11: PASS
- V12: NOT_RUN
- V13: NOT_RUN
- V14: FAIL
- V15: PASS
- V16: FAIL

Failure: "远程主机强迫关闭了一个现有的连接。 (os error 10054)"

Failure cause: {"dumps":["documents/crash_dumps/isaacv1.9.7.12.J273-20261025-112429-33856-31632.dmp"],"kind":"game_crash"}

Artifacts actually generated:

- `S0-run.bmp`
- `S10-ghost.bmp`
- `S11-revived.bmp`
- `S12-stage.bmp`
- `build-revision.log`
- `control.jsonl`
- `deployment.json`
- `deployment_before`
- `enabled`
- `enabled-failure`
- `enabled-launch`
- `fault-evidence.json`
- `file-notifications.jsonl`
- `file-window-changes.json`
- `file-windows`
- `hook-manifest.json`
- `hook-manifest.md`
- `inputconfigs.dat`
- `inputconfigs_rgon.dat`
- `life-evidence.json`
- `loading-query-evidence.json`
- `lua-observations.json`
- `network-ledger.json`
- `persistent-evidence.json`
- `relay-1.jsonl`
- `remote-1.jsonl`
- `report.json`
- `retirement-evidence.json`
- `save-changes.json`
- `save-slot-2.bmp`
- `save_after`
- `save_before`
- `save_mid`
- `scenario.json`
- `sources`
- `steps.jsonl`
- `tick-evidence.json`
- `tick-integrity.json`
