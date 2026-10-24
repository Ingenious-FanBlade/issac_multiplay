#![cfg(windows)]

use std::error::Error;
use std::process::{Child, Command};
use std::sync::atomic::{AtomicU64, Ordering};

use isaac_cs_t0::active_artifact::active_event_from_native;
use isaac_cs_t0::active_ipc::{ActiveMapping, ActiveScenario};
use windows::Win32::Foundation::{CloseHandle, HANDLE, WAIT_OBJECT_0};
use windows::Win32::System::Threading::{CreateEventW, SetEvent, WaitForSingleObject};
use windows::core::PCWSTR;

static IDENTITY: AtomicU64 = AtomicU64::new(1);

struct OwnedEvent(HANDLE);

impl Drop for OwnedEvent {
    fn drop(&mut self) {
        let _ = unsafe { CloseHandle(self.0) };
    }
}

struct ChildGuard {
    child: Child,
    completed: bool,
}

impl Drop for ChildGuard {
    fn drop(&mut self) {
        if !self.completed {
            let _ = self.child.kill();
            let _ = self.child.wait();
        }
    }
}

fn create_event(name: &str) -> Result<OwnedEvent, Box<dyn Error>> {
    let mut wide = name.encode_utf16().collect::<Vec<_>>();
    wide.push(0);
    let handle = unsafe { CreateEventW(None, false, false, PCWSTR(wide.as_ptr())) }?;
    Ok(OwnedEvent(handle))
}

fn wait_ready(ready: &OwnedEvent) -> Result<(), Box<dyn Error>> {
    let wait = unsafe { WaitForSingleObject(ready.0, 30_000) };
    if wait != WAIT_OBJECT_0 {
        return Err(format!(
            "Native cross-language publisher wait returned 0x{:08X}",
            wait.0
        )
        .into());
    }
    Ok(())
}

fn signal_acknowledge(acknowledge: &OwnedEvent) -> Result<(), Box<dyn Error>> {
    unsafe { SetEvent(acknowledge.0) }?;
    Ok(())
}

fn run_native_mapping_round_trip(native_harness: &std::ffi::OsStr) -> Result<(), Box<dyn Error>> {
    let serial = IDENTITY.fetch_add(1, Ordering::Relaxed);
    let identity = format!("{}.{}", std::process::id(), serial);
    let mapping_name = format!("Local\\IsaacCsT0.Active.NativePublisher.{identity}");
    let ready_name = format!("Local\\IsaacCsT0.Active.NativePublisher.Ready.{identity}");
    let acknowledge_name =
        format!("Local\\IsaacCsT0.Active.NativePublisher.Acknowledge.{identity}");
    let mapping = ActiveMapping::create(
        mapping_name,
        std::process::id(),
        ActiveScenario::Keyboard as u32,
        1,
        [1; 32],
        [2; 32],
        [3; 32],
        [4; 32],
        [5; 32],
        [6; 32],
        [7; 32],
        [8; 32],
        [9; 32],
        [10; 32],
        [11; 32],
        "Local\\IsaacCsT0.Loop.NativePublisher",
        [12; 16],
        13,
    )?;
    let ready = create_event(&ready_name)?;
    let acknowledge = create_event(&acknowledge_name)?;
    let mut child = ChildGuard {
        child: Command::new(native_harness)
            .arg("--publish-active-cross-language")
            .arg(mapping.name())
            .arg(&ready_name)
            .arg(&acknowledge_name)
            .spawn()?,
        completed: false,
    };

    let mut next = 0;
    wait_ready(&ready)?;
    let records = mapping.drain(&mut next)?;
    assert_eq!(records.len(), 1);
    assert_eq!(next, 1);
    let raw = records[0].event;
    assert_eq!(raw.qpc, 11);
    assert_eq!(raw.authority_tick, 12);
    assert_eq!(raw.loop_iteration, 13);
    assert_eq!(raw.thread_id, 14);
    assert_eq!(raw.attempt, 15);
    assert_eq!(raw.client_id, 10);
    assert_eq!(raw.command_sequence, 16);
    assert_eq!(raw.target_tick, 17);
    assert_eq!(raw.queue_depth, 19);
    assert_eq!(raw.apply_generation, 20);
    assert_eq!(raw.owner_generation, 21);
    assert_eq!(raw.witness_before, [0xB6; 32]);
    assert_eq!(raw.witness_after, [0xC7; 32]);
    let converted = active_event_from_native(&records[0], 1, &[9; 16])?;
    assert_eq!(converted.stage, "canonical");
    assert_eq!(converted.phase, "PHYSICAL_SAMPLE");
    assert_eq!(converted.source_device.as_deref(), Some("KEYBOARD"));
    assert_eq!(converted.client_id, Some(10));
    assert_eq!(converted.command_sequence, Some(16));
    assert_eq!(converted.target_tick, Some(17));
    assert_eq!(converted.apply_generation, Some(20));
    assert_eq!(converted.owner_generation, Some(21));
    assert_eq!(converted.action, None);
    signal_acknowledge(&acknowledge)?;

    wait_ready(&ready)?;
    assert!(mapping.drain(&mut next)?.is_empty());
    assert_eq!(next, 1);
    signal_acknowledge(&acknowledge)?;

    wait_ready(&ready)?;
    let records = mapping.drain(&mut next)?;
    assert_eq!(records.len(), 1);
    assert_eq!(records[0].event.qpc, 22);
    assert_eq!(next, 2);
    let converted = active_event_from_native(&records[0], 2, &[9; 16])?;
    assert_eq!(converted.qpc, 22);
    assert_eq!(converted.stage, "canonical");
    signal_acknowledge(&acknowledge)?;

    wait_ready(&ready)?;
    assert!(mapping.drain(&mut next).is_err());
    signal_acknowledge(&acknowledge)?;

    let status = child.child.wait()?;
    child.completed = true;
    assert!(status.success());
    Ok(())
}

#[test]
fn native_publisher_real_mapping_round_trip_is_fail_closed() -> Result<(), Box<dyn Error>> {
    let native_harness = std::env::var_os("ISAAC_CS_T0_ACTIVE_NATIVE_CROSS_TEST").or_else(|| {
        let fallback = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .parent()?
            .join("out/cs-t0/obj/active-tests/Debug/IsaacCsT0.ActiveNativeTests.exe");
        fallback.is_file().then(|| fallback.into_os_string())
    });
    let Some(native_harness) = native_harness else {
        eprintln!("cross-language Native publisher is built before the verification-v2 subgate");
        return Ok(());
    };
    run_native_mapping_round_trip(&native_harness)
}
