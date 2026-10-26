# tx-04-07 E2E

Result: **FAIL**

Single launch; no automatic retry; no save restoration.

- V1: NOT_OBSERVED
- V2: NOT_RUN
- V3: NOT_RUN
- V4: FAIL
- V5: NOT_RUN
- V6: NOT_RUN
- V7: NOT_RUN
- V8: NOT_RUN
- V9: NOT_RUN
- V10: NOT_RUN
- V11: NOT_RUN
- V12: NOT_RUN
- V13: NOT_RUN
- V14: NOT_RUN
- V15: NOT_RUN
- V16: NOT_RUN

Failure: "runtime failure: {\"a_bound\": \"0\", \"a_generation\": \"2\", \"armed\": \"0\", \"b_awaiting_rebind\": \"0\", \"b_bound\": \"0\", \"b_character\": \"0\", \"b_conflict_reason\": \"\", \"b_conflicts\": \"0\", \"b_death_event\": \"0\", \"b_deaths\": \"0\", \"b_generation\": \"2\", \"b_life\": \"None\", \"b_lost_loop\": \"0\", \"b_lost_tick\": \"0\", \"b_ownership_confirmed\": \"0\", \"b_ownership_generation\": \"0\", \"b_pending_reason\": \"lifecycle_leave\", \"b_rebinds\": \"0\", \"b_revived_loop\": \"0\", \"b_revived_tick\": \"0\", \"b_revives\": \"0\", \"b_unreadable\": \"0\", \"blocks\": \"0\", \"calls\": \"4676\", \"capability.control\": \"1\", \"capability.device\": \"1\", \"capability.input\": \"1\", \"capability.join\": \"1\", \"capability.loop\": \"1\", \"capability.observe\": \"1\", \"capability.player\": \"1\", \"cleanup_failed\": \"0\", \"completed_tick\": \"0\", \"context_faults\": \"0\", \"current_tick\": \"0\", \"device_k_id\": \"0\", \"device_k_state\": \"Empty\", \"foreign\": \"0\", \"frame\": \"594\", \"g_game\": \"1\", \"generation\": \"1\", \"host_patches_installed\": \"1\", \"join_baby_overrides\": \"0\", \"join_creations\": \"0\", \"join_last\": \"None\", \"join_mode_calls\": \"0\", \"join_passes\": \"0\", \"join_state\": \"None\", \"last_error\": \"\", \"last_tick_decision\": \"Cancelled\", \"loading.a.generation_end\": \"2\", \"loading.a.generation_start\": \"1\", \"loading.a.queue_end\": \"0\", \"loading.a.queue_start\": \"0\", \"loading.a.resolved_tick_end\": \"0\", \"loading.a.resolved_tick_start\": \"0\", \"loading.a.submit_end\": \"0\", \"loading.a.submit_start\": \"0\", \"loading.b.generation_end\": \"2\", \"loading.b.generation_start\": \"1\", \"loading.b.queue_end\": \"0\", \"loading.b.queue_start\": \"0\", \"loading.b.resolved_tick_end\": \"0\", \"loading.b.resolved_tick_start\": \"0\", \"loading.b.submit_end\": \"0\", \"loading.b.submit_start\": \"0\", \"loading_active\": \"0\", \"loading_admitted_current\": \"0\", \"loading_admitted_end\": \"0\", \"loading_admitted_start\": \"0\", \"loading_completed_end\": \"0\", \"loading_completed_start\": \"0\", \"loading_end_known\": \"1\", \"loading_end_loop\": \"553\", \"loading_end_reason\": \"session_retired\", \"loading_episode\": \"1\", \"loading_episodes\": \"1\", \"loading_first_menu\": \"1\", \"loading_invalidations\": \"2\", \"loading_owners\": \"1\", \"loading_queries\": \"6\", \"loading_queries_nonzero\": \"0\", \"loading_queries_unknown\": \"1\", \"loading_reasons\": \"1\", \"loading_source\": \"starting_enter\", \"loading_start_loop\": \"553\", \"manager_starting\": \"0\", \"menu_game_selection\": \"-1\", \"menu_id\": \"-1\", \"net_bye_attempts\": \"3\", \"net_bye_sent\": \"3\", \"net_dropped\": \"0\", \"net_duplicate\": \"0\", \"net_error.Configuration\": \"0\", \"net_error.Crc\": \"0\", \"net_error.Duplicate\": \"0\", \"net_error.Flags\": \"0\", \"net_error.Handshake\": \"0\", \"net_error.Header\": \"0\", \"net_error.Identity\": \"0\", \"net_error.Incompatible\": \"0\", \"net_error.Kind\": \"0\", \"net_error.Length\": \"0\", \"net_error.Magic\": \"0\", \"net_error.None\": \"0\", \"net_error.Payload\": \"0\", \"net_error.Pending\": \"0\", \"net_error.PendingMismatch\": \"0\", \"net_error.Phase\": \"0\", \"net_error.Reserved\": \"0\", \"net_error.Role\": \"0\", \"net_error.Sequence\": \"0\", \"net_error.Stale\": \"0\", \"net_error.Terminal\": \"0\", \"net_error.Version\": \"0\", \"net_foreign\": \"0\", \"net_last_send_error\": \"0\", \"net_native_error\": \"0\", \"net_rx\": \"36\", \"net_rx_bytes\": \"5208\", \"net_rx_dropped\": \"0\", \"net_rx_errors\": \"0\", \"net_stale\": \"0\", \"net_tx\": \"517\", \"net_tx_bytes\": \"127712\", \"net_tx_dropped\": \"0\", \"net_tx_errors\": \"0\", \"net_valid\": \"36\", \"odd_patch_installed\": \"1\", \"ok\": \"1\", \"pause_selection\": \"1\", \"pause_state\": \"0\", \"peer_accepted_commands\": \"0\", \"peer_accepted_sequence\": \"0\", \"peer_ack_dropped\": \"0\", \"peer_ack_unknown\": \"0\", \"peer_address\": \"127.0.0.1:52958\", \"peer_admitted_commands\": \"0\", \"peer_advertised_generation\": \"0\", \"peer_discard.inactive\": \"35\", \"peer_discard.life_lost\": \"0\", \"peer_discard.loading_or_blocked\": \"0\", \"peer_discard.old_generation\": \"0\", \"peer_discard.old_window\": \"0\", \"peer_discard.sequence_floor\": \"0\", \"peer_discard.unbound\": \"0\", \"peer_discarded_commands\": \"35\", \"peer_duplicate_commands\": \"0\", \"peer_locked\": \"1\", \"peer_processed_sequence\": \"0\", \"peer_rejected_commands\": \"0\", \"peer_rtt_ms\": \"16\", \"peer_running\": \"0\", \"peer_state\": \"Failed\", \"peer_stop_finished\": \"1\", \"peer_terminal\": \"InternalFailure\", \"persist.b_death.abnormal\": \"0\", \"persist.b_death.close\": \"0\", \"persist.b_death.open\": \"0\", \"persist.b_revive.abnormal\": \"0\", \"persist.b_revive.close\": \"0\", \"persist.b_revive.open\": \"0\", \"persist.continue.abnormal\": \"1\", \"persist.continue.close\": \"1\", \"persist.continue.open\": \"1\", \"persist.run_exit.abnormal\": \"0\", \"persist.run_exit.close\": \"0\", \"persist.run_exit.open\": \"0\", \"persist.stage_change.abnormal\": \"0\", \"persist.stage_change.close\": \"0\", \"persist.stage_change.open\": \"0\", \"persist_current_call\": \"0\", \"persist_entries\": \"1989\", \"persist_last_load_slot\": \"2\", \"persist_load_reads\": \"7\", \"persist_load_slot_known\": \"1\", \"persist_open_windows\": \"0\", \"persist_return_errors\": \"0\", \"persist_returns\": \"1989\", \"persist_window_overflow\": \"0\", \"players\": \"1\", \"pointers\": \"8\", \"query_epoch\": \"4\", \"query_keys_observed\": \"0\", \"query_keys_repeated\": \"0\", \"query_mismatch_known\": \"0\", \"query_mismatches\": \"0\", \"query_nonfinite\": \"0\", \"query_overflow\": \"0\", \"query_overrides\": \"5\", \"query_reads\": \"0\", \"query_repeats\": \"0\", \"query_snapshots\": \"4\", \"queue_depth_a\": \"0\", \"queue_depth_b\": \"0\", \"queue_dropped\": \"0\", \"reason.control\": \"\", \"reason.device\": \"\", \"reason.input\": \"\", \"reason.join\": \"\", \"reason.loop\": \"\", \"reason.observe\": \"\", \"reason.player\": \"\", \"registered\": \"41\", \"retirement_cause\": \"RuntimeFailure\", \"run_active\": \"1\", \"selected_save\": \"-1\", \"session_error\": \"trace_event_lost\", \"session_generation\": \"1\", \"session_mode\": \"Host\", \"session_state\": \"Idle\", \"shooting_suppressed\": \"1\", \"start_qualified\": \"0\", \"state\": \"Idle\", \"status_version\": \"1\", \"ticks_odd\": \"0\", \"ticks_standard\": \"0\", \"trace\": \"isaac-cs/traces/boot-35640.jsonl\", \"trace_dropped\": \"63\", \"trace_truncated\": \"0\"}"

Artifacts actually generated:

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
