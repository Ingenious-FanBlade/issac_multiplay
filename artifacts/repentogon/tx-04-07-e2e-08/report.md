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
- V8: PASS
- V9: PASS
- V10: NOT_OBSERVED
- V11: PASS
- V12: FAIL
- V13: FAIL
- V14: FAIL
- V15: PASS
- V16: FAIL

Failure: "status timeout menu_game_selection=1: {\"a_bound\": \"0\", \"a_generation\": \"1\", \"armed\": \"1\", \"b_awaiting_rebind\": \"0\", \"b_bound\": \"0\", \"b_character\": \"0\", \"b_conflict_reason\": \"\", \"b_conflicts\": \"0\", \"b_death_event\": \"0\", \"b_deaths\": \"0\", \"b_generation\": \"1\", \"b_life\": \"None\", \"b_lost_loop\": \"0\", \"b_lost_tick\": \"0\", \"b_ownership_confirmed\": \"0\", \"b_ownership_generation\": \"0\", \"b_pending_reason\": \"\", \"b_rebinds\": \"0\", \"b_revived_loop\": \"0\", \"b_revived_tick\": \"0\", \"b_revives\": \"0\", \"b_unreadable\": \"0\", \"blocks\": \"4\", \"calls\": \"112026\", \"capability.control\": \"1\", \"capability.device\": \"1\", \"capability.input\": \"1\", \"capability.join\": \"1\", \"capability.loop\": \"1\", \"capability.observe\": \"1\", \"capability.player\": \"1\", \"cleanup_failed\": \"0\", \"completed_tick\": \"0\", \"context_faults\": \"0\", \"current_tick\": \"0\", \"device_k_id\": \"0\", \"device_k_state\": \"Empty\", \"foreign\": \"0\", \"frame\": \"6427\", \"g_game\": \"1\", \"generation\": \"4\", \"host_patches_installed\": \"1\", \"join_baby_overrides\": \"0\", \"join_creations\": \"0\", \"join_last\": \"None\", \"join_mode_calls\": \"0\", \"join_passes\": \"0\", \"join_state\": \"Waiting\", \"last_error\": \"\", \"last_tick_decision\": \"Idle\", \"loading.a.generation_end\": \"0\", \"loading.a.generation_start\": \"0\", \"loading.a.queue_end\": \"0\", \"loading.a.queue_start\": \"0\", \"loading.a.resolved_tick_end\": \"0\", \"loading.a.resolved_tick_start\": \"0\", \"loading.a.submit_end\": \"0\", \"loading.a.submit_start\": \"0\", \"loading.b.generation_end\": \"0\", \"loading.b.generation_start\": \"0\", \"loading.b.queue_end\": \"0\", \"loading.b.queue_start\": \"0\", \"loading.b.resolved_tick_end\": \"0\", \"loading.b.resolved_tick_start\": \"0\", \"loading.b.submit_end\": \"0\", \"loading.b.submit_start\": \"0\", \"loading_active\": \"0\", \"loading_admitted_current\": \"0\", \"loading_admitted_end\": \"0\", \"loading_admitted_start\": \"0\", \"loading_completed_end\": \"0\", \"loading_completed_start\": \"0\", \"loading_end_known\": \"0\", \"loading_end_loop\": \"0\", \"loading_end_reason\": \"open\", \"loading_episode\": \"0\", \"loading_episodes\": \"0\", \"loading_first_menu\": \"0\", \"loading_invalidations\": \"0\", \"loading_owners\": \"0\", \"loading_queries\": \"0\", \"loading_queries_nonzero\": \"0\", \"loading_queries_unknown\": \"0\", \"loading_reasons\": \"0\", \"loading_source\": \"loading_enter\", \"loading_start_loop\": \"0\", \"manager_starting\": \"0\", \"menu_game_selection\": \"0\", \"menu_id\": \"3\", \"net_bye_attempts\": \"0\", \"net_bye_sent\": \"0\", \"net_dropped\": \"0\", \"net_duplicate\": \"0\", \"net_error.Configuration\": \"0\", \"net_error.Crc\": \"0\", \"net_error.Duplicate\": \"0\", \"net_error.Flags\": \"0\", \"net_error.Handshake\": \"0\", \"net_error.Header\": \"0\", \"net_error.Identity\": \"0\", \"net_error.Incompatible\": \"0\", \"net_error.Kind\": \"0\", \"net_error.Length\": \"0\", \"net_error.Magic\": \"0\", \"net_error.None\": \"0\", \"net_error.Payload\": \"0\", \"net_error.Pending\": \"0\", \"net_error.PendingMismatch\": \"0\", \"net_error.Phase\": \"0\", \"net_error.Reserved\": \"0\", \"net_error.Role\": \"0\", \"net_error.Sequence\": \"0\", \"net_error.Stale\": \"0\", \"net_error.Terminal\": \"0\", \"net_error.Version\": \"0\", \"net_foreign\": \"0\", \"net_last_send_error\": \"0\", \"net_native_error\": \"0\", \"net_rx\": \"71\", \"net_rx_bytes\": \"10248\", \"net_rx_dropped\": \"0\", \"net_rx_errors\": \"0\", \"net_stale\": \"0\", \"net_tx\": \"1045\", \"net_tx_bytes\": \"259160\", \"net_tx_dropped\": \"0\", \"net_tx_errors\": \"0\", \"net_valid\": \"71\", \"odd_patch_installed\": \"1\", \"ok\": \"1\", \"pause_selection\": \"-1\", \"pause_state\": \"-1\", \"peer_accepted_commands\": \"0\", \"peer_accepted_sequence\": \"0\", \"peer_ack_dropped\": \"0\", \"peer_ack_unknown\": \"0\", \"peer_address\": \"127.0.0.1:55960\", \"peer_admitted_commands\": \"0\", \"peer_advertised_generation\": \"0\", \"peer_discard.inactive\": \"70\", \"peer_discard.life_lost\": \"0\", \"peer_discard.loading_or_blocked\": \"0\", \"peer_discard.old_generation\": \"0\", \"peer_discard.old_window\": \"0\", \"peer_discard.sequence_floor\": \"0\", \"peer_discard.unbound\": \"0\", \"peer_discarded_commands\": \"70\", \"peer_duplicate_commands\": \"0\", \"peer_locked\": \"1\", \"peer_processed_sequence\": \"0\", \"peer_rejected_commands\": \"0\", \"peer_rtt_ms\": \"15\", \"peer_running\": \"1\", \"peer_state\": \"Established\", \"peer_stop_finished\": \"0\", \"peer_terminal\": \"None\", \"persist.b_death.abnormal\": \"0\", \"persist.b_death.close\": \"1\", \"persist.b_death.open\": \"1\", \"persist.b_revive.abnormal\": \"0\", \"persist.b_revive.close\": \"1\", \"persist.b_revive.open\": \"1\", \"persist.continue.abnormal\": \"0\", \"persist.continue.close\": \"1\", \"persist.continue.open\": \"1\", \"persist.run_exit.abnormal\": \"0\", \"persist.run_exit.close\": \"0\", \"persist.run_exit.open\": \"0\", \"persist.stage_change.abnormal\": \"0\", \"persist.stage_change.close\": \"1\", \"persist.stage_change.open\": \"1\", \"persist_current_call\": \"0\", \"persist_entries\": \"2797\", \"persist_last_load_slot\": \"2\", \"persist_load_reads\": \"14\", \"persist_load_slot_known\": \"1\", \"persist_open_windows\": \"0\", \"persist_return_errors\": \"0\", \"persist_returns\": \"2797\", \"persist_window_overflow\": \"0\", \"players\": \"0\", \"pointers\": \"8\", \"query_epoch\": \"1\", \"query_keys_observed\": \"0\", \"query_keys_repeated\": \"0\", \"query_mismatch_known\": \"0\", \"query_mismatches\": \"0\", \"query_nonfinite\": \"0\", \"query_overflow\": \"0\", \"query_overrides\": \"0\", \"query_reads\": \"0\", \"query_repeats\": \"0\", \"query_snapshots\": \"3\", \"queue_depth_a\": \"0\", \"queue_depth_b\": \"0\", \"queue_dropped\": \"0\", \"reason.control\": \"\", \"reason.device\": \"\", \"reason.input\": \"\", \"reason.join\": \"\", \"reason.loop\": \"\", \"reason.observe\": \"\", \"reason.player\": \"\", \"registered\": \"41\", \"retirement_cause\": \"None\", \"run_active\": \"0\", \"selected_save\": \"1\", \"session_error\": \"\", \"session_generation\": \"4\", \"session_mode\": \"Host\", \"session_state\": \"Warmup\", \"shooting_suppressed\": \"0\", \"start_qualified\": \"0\", \"state\": \"Warmup\", \"status_version\": \"1\", \"ticks_odd\": \"0\", \"ticks_standard\": \"0\", \"trace\": \"isaac-cs/traces/boot-880.jsonl\", \"trace_capacity\": \"4096\", \"trace_dropped\": \"0\", \"trace_native_error\": \"0\", \"trace_queue_dropped\": \"0\", \"trace_truncated\": \"0\", \"trace_wake_errors\": \"0\", \"trace_write_errors\": \"0\"}"

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
- `laser-evidence.json`
- `life-evidence.json`
- `loading-query-evidence.json`
- `lua-observations.json`
- `network-ledger.json`
- `pause-exit-selected.bmp`
- `persistent-evidence.json`
- `relay-1.jsonl`
- `relay-2.jsonl`
- `relay-3.jsonl`
- `relay-4.jsonl`
- `remote-1.jsonl`
- `remote-2.jsonl`
- `remote-3.jsonl`
- `remote-4.jsonl`
- `report.json`
- `retirement-evidence.json`
- `returned-menu.bmp`
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
