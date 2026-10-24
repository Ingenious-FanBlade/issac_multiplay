#![cfg(windows)]

use std::error::Error;
use std::fs;
use std::process::{Child, Command};
use std::sync::atomic::{AtomicU64, Ordering};

use isaac_cs_t0::coop_passive_ipc::{
    PASSIVE_FLAG_ASSIGNMENT_MATCH, PASSIVE_FLAG_BINDING_UNIQUE, PASSIVE_FLAG_ENDPOINT_B,
    PASSIVE_FLAG_HUD_MATCH, PASSIVE_FLAG_LUA_VECTOR_EQUIVALENT, PASSIVE_FLAG_MAIN_THREAD,
    PASSIVE_FLAG_ORIGINAL_CALLED, PASSIVE_FLAG_TOPOLOGY_COHERENT, PassiveEventKind, PassiveMapping,
    PassiveMappingIdentity, PassiveScenario,
};
use sha2::{Digest, Sha256};
use windows::Win32::Foundation::{CloseHandle, HANDLE, WAIT_OBJECT_0};
use windows::Win32::System::Threading::{CreateEventW, SetEvent, WaitForSingleObject};
use windows::core::PCWSTR;

static IDENTITY: AtomicU64 = AtomicU64::new(1);

const TOPOLOGY_DIAGNOSTIC_DOMAIN: &[u8] = b"IsaacCsT0-CoopPassive-TopologyDiagnostic-v1";
const TOPOLOGY_DIAGNOSTIC_VECTORS: [&str; 25] = [
    "2C09A4A5D95FDC19252D9302FDD8A5F99C023B562F55843CA8D9417E533742B3",
    "728E8A073D0071DC0EF19F528D657309EB1770ED76EBE742F94DC1E664D767BB",
    "070AC84FE78800A813F68B14CD53837B09982983191DC61A6C914C8C4BF78760",
    "ED59D11B1A98A10AD89E85BD6E65C3EDA88BADA575D34BFD031E570CA7015EEC",
    "3B8BD7F299CD381A18EA52B7706903D883521A53C890A1D3F10DF52E68D82F09",
    "9C2D14B4B02274CB0A50EF80B30C6D93C1ECDB4AAD28D8679FD346CCB9378768",
    "F7C5EE9AFEA9C3658CABFA7B28DDF96FF16C8A28516BDBF94023F84A98BAE394",
    "1511132C1115BEB0F04ACB8C5B4B48D06D45C589597126AFE1A1DD822140085E",
    "A96D8FDAFCCBDE83CD8FA6058D9B304818C995ACD4A701338CD956C0762DE488",
    "37B1DFBD7503D2E49F1AE3881DEC25BB78CCA6F976CB6135BCCD5C973D5769D8",
    "254C7D2E5DBBF343EC0F4AC56D69CC39D87F9AF1D4C178B2F7F8C0B0D7054280",
    "6C459BCB675893F849B885D8BB6444E9E629F34E9A95C1C159C8BF20F13D1304",
    "25AEE064704A69C204B634763CF6B5444EAC7959B0B4D08BE6DD982DB9F28C3E",
    "FA613608761287D335DC9092B733E7894BB443440E2633565130EBC0E116EA8E",
    "FD1CBBFE33694ED0672BD15CAA917DB7320C4BD94F30AE7705C0AB89EC8CCD54",
    "813746922991DE8E14F3E96DF8C18A4F5F9508D8CAE818E7FFC310226C4BBFF8",
    "5D3A66832B42B3B2EAE787D16E0C12AF3EA5687243CF75F19AF089D13F0E0FCC",
    "C6DF94CEF7B3E5AAAE10863234C1B073222503ABB5DAA8043FD1723BA658CE05",
    "A0B07D872F01EEDB072D66CC5A96E6021287CC0C513223E794446960FD8CC13B",
    "8A266195D6DD253260A8D126308BE1ECA8D3C69492B9E070BE672F18F7A2446C",
    "65DD1A7725708E1FBFB979D7BDB559FE508FD6A049069037DD3F6E7A74A4341E",
    "10278C2713B0C0D108C26743516C9A86EA30A92EC87892FC08955FD09904B4B9",
    "5FDB056A88B410B512BA0B1340F945428E878B90628D103F566C7DA70757B5D8",
    "B09DA408BB01A5C08F5037D934B8F9D278578E200A05F8AA300E6A3A4BA9260C",
    "B00A9DDA5DDD97B3D2E499213E975B2907EA8C2327A775D3F0BD64B846DE923C",
];

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
    Ok(OwnedEvent(unsafe {
        CreateEventW(None, false, false, PCWSTR(wide.as_ptr()))?
    }))
}

fn wait_ready(ready: &OwnedEvent) -> Result<(), Box<dyn Error>> {
    let wait = unsafe { WaitForSingleObject(ready.0, 30_000) };
    if wait != WAIT_OBJECT_0 {
        return Err(format!("passive Native publisher wait returned 0x{:08X}", wait.0).into());
    }
    Ok(())
}

fn acknowledge(event: &OwnedEvent) -> Result<(), Box<dyn Error>> {
    unsafe { SetEvent(event.0) }?;
    Ok(())
}

fn mapping_identity() -> PassiveMappingIdentity {
    PassiveMappingIdentity {
        target_pid: std::process::id(),
        scenario: PassiveScenario::NativeJoin,
        process_creation_time_100ns: 1,
        nonce: [1; 32],
        canonical_target_sha256: [2; 32],
        observed_target_sha256: [3; 32],
        target_equivalence_sha256: [4; 32],
        catalog_sha256: [5; 32],
        graph_sha256: [6; 32],
        plan_sha256: [7; 32],
        verification_sha256: [8; 32],
        loop_mapping_name: "Local\\IsaacCsT0.Loop.v5.PassiveCrossLanguage".to_owned(),
    }
}

fn run_round_trip(native_harness: &std::ffi::OsStr) -> Result<(), Box<dyn Error>> {
    let serial = IDENTITY.fetch_add(1, Ordering::Relaxed);
    let identity = format!("{}.{}", std::process::id(), serial);
    let mapping_name = format!("Local\\IsaacCsT0.CoopPassive.NativePublisher.{identity}");
    let ready_name = format!("Local\\IsaacCsT0.CoopPassive.Ready.{identity}");
    let acknowledge_name = format!("Local\\IsaacCsT0.CoopPassive.Acknowledge.{identity}");
    let mapping = PassiveMapping::create(mapping_name, mapping_identity())?;
    let ready = create_event(&ready_name)?;
    let acknowledge_event = create_event(&acknowledge_name)?;
    let mut child = ChildGuard {
        child: Command::new(native_harness)
            .arg("--publish-coop-passive-cross-language")
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
    assert_eq!(records[0].sequence, 0);
    let event = records[0].event;
    assert_eq!(event.published_sequence, 1);
    assert_eq!(event.qpc, 11);
    assert_eq!(event.loop_iteration, 12);
    assert_eq!(event.owner_generation, 13);
    assert_eq!(event.thread_id, 14);
    assert_eq!(event.main_thread_id, 14);
    assert_eq!(event.kind, PassiveEventKind::BindingBound as u32);
    assert_eq!(event.slot_ordinal, 45);
    assert_eq!(event.phase, 2);
    assert_eq!(event.depth, 1);
    assert_eq!(event.local_ordinal, 1);
    assert_eq!(event.endpoint, 2);
    assert_eq!(event.original_calls, 1);
    assert_eq!(event.topology_digest, [0xA1; 32]);
    assert_eq!(event.player_digest, [0xB2; 32]);
    assert_eq!(event.device_digest, [0xC3; 32]);
    assert_eq!(event.assignment_digest, [0xD4; 32]);
    assert_eq!(
        event.flags,
        PASSIVE_FLAG_MAIN_THREAD
            | PASSIVE_FLAG_ORIGINAL_CALLED
            | PASSIVE_FLAG_TOPOLOGY_COHERENT
            | PASSIVE_FLAG_ENDPOINT_B
            | PASSIVE_FLAG_BINDING_UNIQUE
            | PASSIVE_FLAG_HUD_MATCH
            | PASSIVE_FLAG_ASSIGNMENT_MATCH
            | PASSIVE_FLAG_LUA_VECTOR_EQUIVALENT
    );
    acknowledge(&acknowledge_event)?;

    // The producer has advanced next_sequence, but the slot is not committed.
    wait_ready(&ready)?;
    assert!(mapping.drain(&mut next)?.is_empty());
    assert_eq!(next, 1);
    acknowledge(&acknowledge_event)?;

    wait_ready(&ready)?;
    let records = mapping.drain(&mut next)?;
    assert_eq!(records.len(), 1);
    assert_eq!(records[0].event.qpc, 22);
    assert_eq!(next, 2);
    acknowledge(&acknowledge_event)?;

    // Unknown flags must never be projected into a persisted event.
    wait_ready(&ready)?;
    assert!(mapping.drain(&mut next).is_err());
    assert_eq!(next, 2);
    acknowledge(&acknowledge_event)?;

    // An explicit producer overwrite is fatal even if no sequence gap is visible.
    wait_ready(&ready)?;
    assert!(mapping.overwritten_events() != 0);
    assert!(mapping.drain(&mut next).is_err());
    acknowledge(&acknowledge_event)?;

    // A produced distance beyond the fixed ring capacity is independently fatal.
    wait_ready(&ready)?;
    assert!(mapping.drain(&mut next).is_err());
    acknowledge(&acknowledge_event)?;

    let status = child.child.wait()?;
    child.completed = true;
    assert!(status.success());
    Ok(())
}

fn rust_topology_diagnostic_digest(code: u32) -> [u8; 32] {
    if !(1..=25).contains(&code) {
        return [0; 32];
    }
    let mut hasher = Sha256::new();
    hasher.update(TOPOLOGY_DIAGNOSTIC_DOMAIN);
    hasher.update(code.to_le_bytes());
    hasher.finalize().into()
}

fn file_sha256(path: &std::path::Path) -> String {
    hex::encode_upper(Sha256::digest(
        fs::read(path).expect("read frozen evidence file"),
    ))
}

#[test]
fn backend_screen_decision_is_exact_and_live04_remains_hash_bound() {
    let cs_t0 = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let repository = cs_t0.parent().expect("cs-t0 has a repository parent");
    let decision_path = cs_t0.join("coop-passive-backend-screen-v1.json");
    let decision: serde_json::Value =
        serde_json::from_slice(&fs::read(&decision_path).expect("read backend screen decision"))
            .expect("parse backend screen decision");
    let object = decision.as_object().expect("backend screen object");
    assert_eq!(object.len(), 13);
    assert_eq!(decision["schemaVersion"], 1);
    assert_eq!(
        decision["decisionIdentity"],
        "IsaacCsT0-CoopPassiveBackendScreen-v1"
    );
    assert_eq!(decision["workPackage"], "CS-T0-05-03");
    assert_eq!(decision["status"], "PASS_SCOPED_REPAIR");
    assert_eq!(decision["createNewOnly"], true);
    assert_eq!(
        decision["scope"],
        "COOP_PASSIVE_NONZERO_BACKEND_LOCAL_ROUTE"
    );

    let predecessor = &decision["predecessorLayout"];
    assert_eq!(predecessor["identity"], "IsaacCsT0-CoopC0LayoutEvidence-v1");
    assert_eq!(predecessor["unchanged"], true);
    assert_eq!(
        predecessor["sha256"],
        file_sha256(&repository.join(predecessor["path"].as_str().expect("predecessor path")))
    );
    let static_evidence = &decision["staticEvidence"];
    assert_eq!(
        static_evidence["graphIdentity"],
        "IsaacCsT0-CoopStaticGraph-v1"
    );
    assert_eq!(
        static_evidence["graphSha256"],
        file_sha256(
            &repository.join(
                static_evidence["graphPath"]
                    .as_str()
                    .expect("static graph path"),
            ),
        )
    );
    assert_eq!(
        static_evidence["ordinaryRouteSlots"],
        serde_json::json!([46, 49, 51])
    );
    assert_eq!(static_evidence["steamBackendVtableRva"], "0x007A2440");

    let rule = decision["scopedRule"]
        .as_object()
        .expect("scoped backend rule");
    assert_eq!(rule.len(), 17);
    assert_eq!(rule["nullBackendAllowedWithoutRouteWitness"], true);
    assert_eq!(
        rule["nonzeroBackendPresenceAloneIsPositiveOnlineEvidence"],
        false
    );
    for field in [
        "nonzeroBackendRequiresAlignedReadablePointer",
        "nonzeroBackendRequiresExactSteamVtable",
        "onlineManagerRosterBeginMustBeZero",
        "onlineManagerRosterEndMustBeZero",
        "onlineManagerRosterCapacityEndMustBeZero",
        "onlineFrameMustBeZero",
        "ordinaryRouteMustBeInsideOneManagerTransaction",
        "ordinaryRouteMustMatchCurrentGeneration",
        "nextConsecutiveManagerPassMayConfirmSameCandidate",
        "possessorOrExitCannotAuthorizeOrdinaryRoute",
        "staleGenerationRouteRejected",
        "unknownBackendOrAnyOnlineSignalKeepsUnbound",
    ] {
        assert_eq!(rule[field], true, "scoped rule widened {field}");
    }
    assert_eq!(rule["newHookAllowed"], false);
    assert_eq!(rule["analyzerRelaxationAllowed"], false);
    assert_eq!(
        rule["ordinaryRouteMustBeExact"],
        serde_json::json!([
            "ORDINARY_SPAWN_BEGIN",
            "ORDINARY_SPAWN_END",
            "ORDINARY_ASSIGNMENT_BEGIN",
            "ORDINARY_ASSIGNMENT_END",
            "JOIN_HUD_BEGIN",
            "JOIN_HUD_END"
        ])
    );
    let generation = decision["generationRule"]
        .as_object()
        .expect("generation rule");
    assert_eq!(generation.len(), 3);
    assert!(generation.values().all(|value| value == true));
    let permissions = decision["permissions"].as_object().expect("permissions");
    assert_eq!(permissions.len(), 13);
    assert!(permissions.values().all(|value| value == false));

    let live = &decision["liveEvidence"];
    let source = repository.join(live["sourceDirectory"].as_str().expect("live source path"));
    assert_eq!(live["analysisResult"], "BLOCKED_BY_EVIDENCE");
    assert_eq!(live["managerEndCount"], 8_177);
    assert_eq!(live["managerDiagnosticCode"], 3);
    assert_eq!(live["ordinaryRouteObserved"], true);
    assert_eq!(live["oldSourceResultImmutable"], true);
    assert_eq!(
        live["manifestSha256"],
        file_sha256(&source.join("manifest.json"))
    );
    assert_eq!(
        live["eventsSha256"],
        file_sha256(&source.join("events.jsonl"))
    );
    assert_eq!(
        live["analysisSha256"],
        file_sha256(&source.join("analysis.json"))
    );
    let frozen_analysis: serde_json::Value = serde_json::from_slice(
        &fs::read(source.join("analysis.json")).expect("read live-04 analysis"),
    )
    .expect("parse live-04 analysis");
    assert_eq!(frozen_analysis["result"], "BLOCKED_BY_EVIDENCE");
}

#[test]
fn topology_diagnostic_digest_codebook_is_cross_language_stable() {
    assert_eq!(rust_topology_diagnostic_digest(0), [0; 32]);
    assert_eq!(rust_topology_diagnostic_digest(26), [0; 32]);
    assert_eq!(rust_topology_diagnostic_digest(u32::MAX), [0; 32]);

    let mut unique = std::collections::BTreeSet::new();
    for (index, expected) in TOPOLOGY_DIAGNOSTIC_VECTORS.iter().enumerate() {
        let code = u32::try_from(index + 1).expect("diagnostic code fits u32");
        let digest = rust_topology_diagnostic_digest(code);
        assert_ne!(digest, [0; 32]);
        assert_eq!(hex::encode_upper(digest), *expected);
        assert!(unique.insert(digest), "duplicate digest for code {code}");
    }
    assert_eq!(unique.len(), 25);
}

#[test]
fn cpp_publisher_to_rust_mapping_is_exact_and_fail_closed() -> Result<(), Box<dyn Error>> {
    let native_harness =
        std::env::var_os("ISAAC_CS_T0_COOP_PASSIVE_NATIVE_CROSS_TEST").or_else(|| {
            let fallback = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
                .parent()?
                .join(
                    "out/cs-t0/obj/coop-passive-tests/Debug/IsaacCsT0.CoopPassiveNativeTests.exe",
                );
            fallback.is_file().then(|| fallback.into_os_string())
        });
    let Some(native_harness) = native_harness else {
        eprintln!("passive Native publisher is built by the CS-T0-05-03 verifier subgate");
        return Ok(());
    };
    run_round_trip(&native_harness)
}
