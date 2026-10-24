# tx-04-04 E2E

Result: **FAIL**

- E1: NOT_RUN
- E2: NOT_RUN
- E3: NOT_RUN
- E4: NOT_RUN
- E5: NOT_RUN
- E6: NOT_RUN
- E7: FAIL

Failure: "startup menu timeout after 90s: {\"calls\": \"25640\", \"capability.control\": \"1\", \"capability.device\": \"1\", \"capability.input\": \"1\", \"capability.join\": \"1\", \"capability.loop\": \"1\", \"capability.player\": \"1\", \"context_faults\": \"0\", \"foreign\": \"0\", \"frame\": \"5128\", \"g_game\": \"1\", \"last_error\": \"\", \"manager_starting\": \"0\", \"menu_id\": \"-1\", \"ok\": \"1\", \"pointers\": \"8\", \"queue_dropped\": \"0\", \"reason.control\": \"\", \"reason.device\": \"\", \"reason.input\": \"\", \"reason.join\": \"\", \"reason.loop\": \"\", \"reason.player\": \"\", \"registered\": \"27\", \"run_active\": \"0\", \"selected_save\": \"-1\", \"state\": \"Idle\", \"status_version\": \"1\", \"trace\": \"isaac-cs/traces/boot-35116.jsonl\", \"trace_dropped\": \"0\"}"

Artifacts actually generated:

- `build-revision.log`
- `control.jsonl`
- `deployment.json`
- `deployment_before`
- `enabled`
- `enabled-failure`
- `enabled-launch`
- `hook-manifest.json`
- `hook-manifest.md`
- `report.json`
- `save-changes.json`
- `save_after`
- `save_before`
- `save_mid`
- `scenario.json`
- `sources`
- `steps.jsonl`

No automatic retry. No save restoration.
