#![cfg(windows)]
#![recursion_limit = "256"]

#[path = "support/device_v2_fixture.rs"]
mod specimen;

use isaac_cs_t0::coop_passive_ipc::{
    NativePassiveEvent, PassiveEventKind, PassiveMapping, PassiveMappingIdentity,
    PassiveNativeState, PassiveScenario, SharedPassiveBlock,
};
use serde_json::{Value, json};
use std::error::Error;
use std::fs;
use std::mem::{align_of, offset_of, size_of};
use std::process::{Child, Command};
use tempfile::TempDir;
use windows::Win32::Foundation::{CloseHandle, HANDLE, WAIT_OBJECT_0};
use windows::Win32::System::Threading::{CreateEventW, SetEvent, WaitForSingleObject};
use windows::core::PCWSTR;

fn raw_event(bytes: &[u8]) -> Value {
    let u32_at = |n| u32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    let u64_at = |n| u64::from_le_bytes(bytes[n..n + 8].try_into().unwrap());
    let digest_at = |n| {
        if bytes[n..n + 32].iter().all(|b| *b == 0) {
            Value::Null
        } else {
            json!(hex::encode_upper(&bytes[n..n + 32]))
        }
    };
    let kind = u32_at(168);
    let flags = u32_at(196);
    let name = match kind {
        1 => "NATIVE_INITIALIZED",
        2 => "HOOKS_INSTALLED",
        3 => "MANAGER_PASS_BEGIN",
        4 => "MANAGER_PASS_END",
        5 => "ORDINARY_SPAWN_BEGIN",
        6 => "ORDINARY_SPAWN_END",
        10 => "ORDINARY_ASSIGNMENT_BEGIN",
        11 => "ORDINARY_ASSIGNMENT_END",
        13 => "JOIN_HUD_BEGIN",
        14 => "JOIN_HUD_END",
        17 => "BINDING_UNBOUND",
        18 => "BINDING_CANDIDATE",
        19 => "BINDING_BOUND",
        21 => "LUA_VECTOR_EQUIVALENT",
        24 => "RELATION_CHANGE",
        25 => "HOOKS_RESTORED",
        26 => "PRODUCER_CLOSED",
        28 => "DEVICE_UNASSIGN_BEGIN",
        29 => "DEVICE_UNASSIGN_END",
        30 => "DEVICE_REASSIGN_BEGIN",
        31 => "DEVICE_REASSIGN_END",
        _ => panic!("unexpected raw event kind {kind}"),
    };
    let mut e = specimen::event(name, u64_at(24), u64_at(16));
    e["sequence"] = json!(u64_at(0) - 1);
    e["qpc"] = json!(i64::from_le_bytes(bytes[8..16].try_into().unwrap()));
    e["slotOrdinal"] = if u32_at(172) == u32::MAX {
        Value::Null
    } else {
        json!(u32_at(172))
    };
    e["phase"] = json!(u32_at(176));
    e["depth"] = json!(u32_at(180));
    e["localOrdinal"] = if u32_at(184) == u32::MAX {
        Value::Null
    } else {
        json!(u32_at(184))
    };
    e["endpoint"] = match u32_at(188) {
        1 => json!("ENDPOINT_A"),
        2 => json!("ENDPOINT_B"),
        _ => Value::Null,
    };
    e["topologyDigest"] = digest_at(32);
    e["playerDigest"] = digest_at(64);
    e["deviceDigest"] = digest_at(96);
    e["assignmentDigest"] = digest_at(128);
    for (field, bit) in [
        ("mainThread", 0),
        ("originalCalled", 1),
        ("topologyCoherent", 2),
        ("bindingUnique", 5),
        ("hudMatches", 6),
        ("assignmentMatches", 7),
        ("luaVectorEquivalent", 8),
        ("relationChanged", 9),
        ("stableRegistryRetired", 10),
    ] {
        e[field] = json!(flags & (1 << bit) != 0);
    }
    e["originalCalls"] = json!(u32_at(192));
    e["bindingState"] = match kind {
        17 => json!("UNBOUND"),
        18 => json!("CANDIDATE"),
        19 => json!("BOUND"),
        _ => Value::Null,
    };
    e
}

#[test]
fn actual_native_shutdown_events_serialize_into_public_cli_contract() {
    let Some(exe) = std::env::var_os("ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST") else {
        return;
    };
    let root = TempDir::new().unwrap();
    for route in [
        "menu",
        "disconnect",
        "same-device",
        "removed-device",
        "incomplete-unassign",
        "incomplete-candidate",
    ] {
        let raw_path = root.path().join(format!("{route}.native-events"));
        let output = Command::new(&exe)
            .arg("--emit-device-causal-events")
            .arg(&raw_path)
            .arg(route)
            .output()
            .unwrap();
        assert!(
            output.status.success(),
            "native {route}: {}",
            String::from_utf8_lossy(&output.stderr)
        );
        let bytes = fs::read(&raw_path).unwrap();
        const HEADER: usize = 168;
        assert!(bytes.len() > HEADER);
        assert_eq!((bytes.len() - HEADER) % 200, 0);
        let metadata: Vec<_> = bytes[..104]
            .chunks_exact(8)
            .map(|x| u64::from_le_bytes(x.try_into().unwrap()))
            .collect();
        assert_eq!(metadata[0], 0x3154_5548_5356_4544);
        let events: Vec<_> = bytes[HEADER..].chunks_exact(200).map(raw_event).collect();
        assert_eq!(events.len() as u64, metadata[1]);
        assert_eq!(
            metadata[2], metadata[3],
            "observed and shared final generations"
        );
        assert_eq!(
            (metadata[4], metadata[5]),
            (0, 5),
            "real Shutdown result and Stopped state"
        );
        assert_eq!(
            (metadata[6], metadata[7], metadata[8]),
            (0x00FF_FFFF_FFFF_FFFF, 0x00FF_FFFF_FFFF_FFFF, 56)
        );
        assert_eq!(
            (metadata[9], metadata[10], metadata[11], metadata[12]),
            (1, 0, 0, 2)
        );
        assert_eq!(events[0]["kind"], "NATIVE_INITIALIZED");
        assert_eq!(events[1]["kind"], "HOOKS_INSTALLED");
        assert_eq!(events[events.len() - 2]["kind"], "HOOKS_RESTORED");
        assert_eq!(events[events.len() - 1]["kind"], "PRODUCER_CLOSED");
        for e in events.iter().rev().take(2) {
            assert_eq!(
                e["ownerGeneration"], metadata[3],
                "actual production terminal generation in {route}"
            );
        }
        // No event is manufactured, appended, renumbered or normalized here.
        // Only low-level embedded-observer dependencies are test substitutes;
        // native shutdown restores a real 56-slot test-memory transaction.
        let source = specimen::create(root.path(), route, false, true);
        specimen::write_events(&source, &events);
        let mut manifest = specimen::manifest(route);
        manifest["finalOwnerGeneration"] = json!(metadata[3]);
        manifest["installedHooksMask"] = json!(format!("0x{:016X}", metadata[6]));
        manifest["restoredHooksMask"] = json!(format!("0x{:016X}", metadata[7]));
        manifest["restoredSlotCount"] = json!(metadata[8]);
        manifest["producerClosed"] = json!(metadata[9] == 1);
        manifest["callbacksDrained"] = json!(metadata[12] == 2);
        manifest["slotSnapshotBeforeSha256"] = json!(hex::encode_upper(&bytes[104..136]));
        manifest["slotSnapshotAfterSha256"] = json!(hex::encode_upper(&bytes[136..168]));
        specimen::write_manifest(&source, &mut manifest);
        let analysis = source.join("analysis.json");
        let output = Command::new(env!("CARGO_BIN_EXE_IsaacCsT0"))
            .arg("coop-analyze")
            .arg(&source)
            .arg("--output")
            .arg(&analysis)
            .output()
            .unwrap();
        assert!(
            analysis.exists(),
            "public Native bridge {route}: {}",
            String::from_utf8_lossy(&output.stderr)
        );
        let result: Value = serde_json::from_slice(&fs::read(analysis).unwrap()).unwrap();
        assert_eq!(
            result["result"],
            if route.starts_with("incomplete-") {
                "BLOCKED_BY_EVIDENCE"
            } else {
                "PASS"
            },
            "Native/Rust field contract {route}: {result}"
        );
    }
}

struct Event(HANDLE);
impl Drop for Event {
    fn drop(&mut self) {
        let _ = unsafe { CloseHandle(self.0) };
    }
}
struct Publisher(Child, bool);
impl Drop for Publisher {
    fn drop(&mut self) {
        if !self.1 {
            let _ = self.0.kill();
            let _ = self.0.wait();
        }
    }
}

fn event(name: &str) -> Result<Event, Box<dyn Error>> {
    let mut wide = name.encode_utf16().collect::<Vec<_>>();
    wide.push(0);
    Ok(Event(unsafe {
        CreateEventW(None, false, false, PCWSTR(wide.as_ptr()))?
    }))
}

#[test]
fn frozen_device_codebook_has_unchanged_physical_abi() {
    assert_eq!(
        (
            size_of::<NativePassiveEvent>(),
            align_of::<NativePassiveEvent>()
        ),
        (200, 8)
    );
    assert_eq!(
        (
            size_of::<SharedPassiveBlock>(),
            align_of::<SharedPassiveBlock>()
        ),
        (3_277_472, 8)
    );
    assert_eq!(offset_of!(SharedPassiveBlock, events), 672);
    assert_eq!(offset_of!(SharedPassiveBlock, state), 480);
    assert_eq!(offset_of!(SharedPassiveBlock, loop_mapping_name), 288);
    assert_eq!(offset_of!(NativePassiveEvent, thread_id), 160);
    assert_eq!(offset_of!(NativePassiveEvent, flags), 196);
    assert_eq!(PassiveScenario::DeviceRebind as u32, 5);
    assert_eq!(PassiveScenario::DeviceRebind.ipc_version(), 2);
    assert_eq!(PassiveScenario::DeviceRebind.slot_count(), 56);
    for scenario in [
        PassiveScenario::NativeJoin,
        PassiveScenario::NativeExit,
        PassiveScenario::DeathRespawn,
        PassiveScenario::RoomRebind,
        PassiveScenario::MenuTeardown,
    ] {
        assert_eq!(scenario.ipc_version(), 1);
        assert_eq!(scenario.slot_count(), 53);
    }
    for (kind, value) in [
        (PassiveEventKind::DeviceUnassignBegin, 28),
        (PassiveEventKind::DeviceUnassignEnd, 29),
        (PassiveEventKind::DeviceReassignBegin, 30),
        (PassiveEventKind::DeviceReassignEnd, 31),
        (PassiveEventKind::UnclassifiedRelationChange, 32),
    ] {
        assert_eq!(kind as u32, value);
    }
    for (state, value) in [
        (PassiveNativeState::Empty, 0),
        (PassiveNativeState::Initializing, 1),
        (PassiveNativeState::Ready, 2),
        (PassiveNativeState::Failed, 3),
        (PassiveNativeState::ShuttingDown, 4),
        (PassiveNativeState::Stopped, 5),
        (PassiveNativeState::Unsafe, 6),
    ] {
        assert_eq!(state as i32, value);
    }
}

#[test]
fn independent_native_publisher_crosses_real_win32_mapping_and_rejects_drift()
-> Result<(), Box<dyn Error>> {
    let Some(exe) = std::env::var_os("ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST") else {
        eprintln!(
            "Native cross-language executable not configured; verify-coop-passive must set ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST for both configurations"
        );
        return Ok(());
    };
    let suffix = format!("{}.device-v2", std::process::id());
    let ready_name = format!("Local\\IsaacCsT0.Ready.{suffix}");
    let ack_name = format!("Local\\IsaacCsT0.Ack.{suffix}");
    let mapping = PassiveMapping::create(
        format!("Local\\IsaacCsT0.Mapping.{suffix}"),
        PassiveMappingIdentity {
            target_pid: std::process::id(),
            scenario: PassiveScenario::DeviceRebind,
            process_creation_time_100ns: 1,
            nonce: [1; 32],
            canonical_target_sha256: [2; 32],
            observed_target_sha256: [3; 32],
            target_equivalence_sha256: [4; 32],
            catalog_sha256: [5; 32],
            graph_sha256: [6; 32],
            plan_sha256: [7; 32],
            verification_sha256: [8; 32],
            loop_mapping_name: "Local\\IsaacCsT0.Loop.DeviceV2.OfflineOnly".to_owned(),
        },
    )?;
    let ready = event(&ready_name)?;
    let ack = event(&ack_name)?;
    let mut publisher = Publisher(
        Command::new(exe)
            .args([
                "--publish-coop-device-cross-language",
                mapping.name(),
                &ready_name,
                &ack_name,
            ])
            .spawn()?,
        false,
    );
    for stage in 0..16 {
        assert_eq!(
            unsafe { WaitForSingleObject(ready.0, 30_000) },
            WAIT_OBJECT_0,
            "publisher stage {stage}"
        );
        let mut next = 0;
        let result = mapping.drain(&mut next);
        match stage {
            0 => {
                let rows = result?;
                assert_eq!(rows.len(), 5);
                assert_eq!(next, 5);
                for (index, record) in rows.iter().enumerate() {
                    let e = &record.event;
                    assert_eq!(record.sequence, index as u64);
                    assert_eq!(e.published_sequence, index as i64 + 1);
                    assert_eq!(e.kind, index as u32 + 28);
                    assert_eq!(e.slot_ordinal, if index < 2 { 54 } else { 55 });
                    assert_eq!(e.phase, 10);
                    assert_eq!(e.depth, 0);
                    assert_eq!(e.owner_generation, 2);
                    assert_eq!(e.loop_iteration, 42);
                    assert_eq!(e.qpc, 100 + index as i64);
                    assert_eq!(e.thread_id, 17);
                    assert_eq!(e.main_thread_id, 17);
                    assert_eq!(e.local_ordinal, 1);
                    assert_eq!(e.endpoint, 2);
                    assert_eq!(e.player_digest, [0x12; 32]);
                    assert_eq!(e.assignment_digest, [0x34; 32]);
                    assert_eq!(e.device_digest, [0; 32]);
                    assert_eq!(e.topology_digest, [0; 32]);
                    assert_eq!(e.original_calls, index as u32 % 2);
                    assert_eq!(e.flags, 17 + if index % 2 == 1 { 2 } else { 0 });
                }
            }
            1 => {
                assert!(result?.is_empty());
                assert_eq!(next, 0);
            }
            2 => {
                assert_eq!(result?.len(), 1);
                assert_eq!(next, 1);
            }
            14 => {
                assert_eq!(result?[0].event.kind, 26);
                assert!(mapping.producer_closed());
                assert_eq!(mapping.state()?, PassiveNativeState::Stopped);
                assert_eq!(mapping.active_hooks(), 0);
                assert_eq!(mapping.installed_hooks_mask(), 0x00FF_FFFF_FFFF_FFFF);
                assert_eq!(mapping.restored_hooks_mask(), 0x00FF_FFFF_FFFF_FFFF);
                assert_eq!(mapping.restored_slot_count(), 56);
                assert_eq!(mapping.zero_capability_counts(), [0; 6]);
            }
            _ => {
                assert!(result.is_err(), "accepted malformed mapping stage {stage}");
                assert_eq!(next, 0);
            }
        }
        unsafe { SetEvent(ack.0) }?;
    }
    let status = publisher.0.wait()?;
    publisher.1 = true;
    assert!(status.success());
    Ok(())
}
