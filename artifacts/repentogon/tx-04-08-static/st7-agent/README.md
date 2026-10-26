# ST7 IDB evidence index

Target: official REPENTOGON-compatible game binary `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`, IDA imagebase `0x400000`. All entries below were read through IDA MCP in read-only operations; no IDB edits or hash checks were performed.

- `idb-restore-player-decomp.json`: complete decompilation output for Game RestoreState VA `0x6F43A0` / RVA `0x2F43A0`, PlayerManager Restore VA `0x993A90` / RVA `0x593A90`, Player Restore VA `0x7A5660` / RVA `0x3A5660`, and RevivePlayerGhost VA `0x7CFC60` / RVA `0x3CFC60`.
- `idb-entrypoints-decomp.json`: complete decompilation output for ExecuteStartGame VA `0x932A20` / RVA `0x532A20`, Game Exit VA `0x6F6270` / RVA `0x2F6270`, and PlayerManager PostLevelInit Restore VA `0x993FA0` / RVA `0x593FA0`.
- `idb-player-manager-restore-helper.json`: raw IDA decompilation result for PlayerManager Restore helper VA `0x993780` / RVA `0x593780`.
- `idb-bytes.json`: raw bytes at selected entry points and call sites, including the Continue-to-Restore call.

These are static artifacts, not runtime observations. RVA values are VA minus imagebase.

