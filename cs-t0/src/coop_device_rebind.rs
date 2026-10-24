//! Device v2: a native detach/attach transaction, never a topology-delta heuristic.
use crate::coop_passive::{PassiveEvent, PassiveManifest, SCENARIO_DEVICE_REBIND};
use sha2::{Digest, Sha256};

pub const TOOL_IDENTITY: &str = "IsaacCsT0-CoopPassiveObserver-v2";
pub const ANALYSIS_IDENTITY: &str = "IsaacCsT0-CoopPassiveAnalysis-v2";
pub const GATE_IDENTITY: &str = "IsaacCsT0-CoopPassiveDecision-v2";

pub fn is_v2(manifest: &PassiveManifest) -> bool {
    manifest.schema_version == 2
        && manifest.tool_identity == TOOL_IDENTITY
        && manifest.scenario == SCENARIO_DEVICE_REBIND
}

fn digest(value: &Option<String>) -> bool {
    value.as_ref().is_some_and(|s| {
        s.len() == 64 && s.bytes().all(|b| b.is_ascii_hexdigit()) && s.bytes().any(|b| b != b'0')
    })
}

fn full(event: &PassiveEvent) -> bool {
    event.topology_coherent
        && event.binding_unique
        && event.assignment_matches
        && event.hud_matches
        && event.lua_vector_equivalent
        && [
            &event.topology_digest,
            &event.player_digest,
            &event.device_digest,
            &event.assignment_digest,
        ]
        .into_iter()
        .all(digest)
}

fn same_identity(a: &PassiveEvent, b: &PassiveEvent) -> bool {
    a.local_ordinal == b.local_ordinal
        && a.endpoint == b.endpoint
        && a.player_digest == b.player_digest
}

fn same_relation(a: &PassiveEvent, b: &PassiveEvent) -> bool {
    same_identity(a, b)
        && a.device_digest == b.device_digest
        && a.assignment_digest == b.assignment_digest
}

fn ensure(ok: bool, detail: &str) -> Result<(), String> {
    if ok {
        Ok(())
    } else {
        Err(format!("device v2: {detail}"))
    }
}

pub fn validate(events: &[PassiveEvent], manifest: &PassiveManifest) -> Result<(), String> {
    ensure(
        is_v2(manifest),
        "legacy device evidence is not causal; a new v2 source is required",
    )?;
    let catalog: serde_json::Value = serde_json::from_str(include_str!("../coop-catalog-v4.json"))
        .expect("compiled device catalog");
    let slots = catalog["orderedHookTransaction"]["slots"]
        .as_array()
        .expect("compiled slots");
    ensure(
        manifest.target_slots.len() == slots.len(),
        "frozen target slot cardinality mismatch",
    )?;
    for (ordinal, (actual, expected)) in manifest.target_slots.iter().zip(slots).enumerate() {
        let bytes = expected
            .get("expectedProof")
            .or_else(|| expected.get("expectedBytes"))
            .and_then(serde_json::Value::as_str)
            .expect("compiled slot proof");
        let mut hasher = Sha256::new();
        hasher.update(b"slot");
        hasher.update([ordinal as u8; 32]);
        hasher.update(bytes.as_bytes());
        let proof = hex::encode_upper(hasher.finalize());
        ensure(
            actual.ordinal == ordinal as u32
                && actual.id == expected["id"]
                && actual.rva == expected["rva"]
                && actual.patch_kind == expected["patchKind"]
                && actual.expected_proof == proof
                && actual.before_proof == proof
                && actual.after_proof == proof,
            "target slot identity or frozen proof drift",
        )?;
    }
    for event in events {
        ensure(
            event.slot_ordinal.is_none_or(|slot| slot < 56),
            "slot outside frozen profile",
        )?;
        ensure(
            event.local_ordinal.is_none_or(|ordinal| ordinal < 2),
            "third ordinal",
        )?;
        for value in [
            &event.topology_digest,
            &event.player_digest,
            &event.device_digest,
            &event.assignment_digest,
        ] {
            ensure(
                value.is_none() || digest(value),
                "malformed relation digest",
            )?;
        }
        if matches!(event.kind.as_str(), "BINDING_CANDIDATE" | "BINDING_BOUND") {
            ensure(full(event), "binding lacks complete coherent relation")?;
        }
        ensure(
            event.kind != "UNCLASSIFIED_RELATION_CHANGE",
            "unclassified change cannot establish device causality",
        )?;
    }
    let causal = events
        .iter()
        .enumerate()
        .filter(|(_, e)| e.kind.starts_with("DEVICE_"))
        .collect::<Vec<_>>();
    let slots: &[u32] = match causal.first().and_then(|(_, e)| e.slot_ordinal) {
        Some(53) => &[53, 53, 54, 54, 55, 55],
        Some(54) => &[54, 54, 55, 55],
        _ => return Err("device v2: missing native unassign witness".to_owned()),
    };
    ensure(
        causal.len() == slots.len(),
        "exactly one closed native transaction is required",
    )?;
    let (start, first) = causal[0];
    ensure(start >= 2, "unassign lacks preceding invalidation")?;
    let generation = first.owner_generation;
    let ordinal = first
        .local_ordinal
        .ok_or("device v2: unassign has no ordinal")? as usize;
    ensure(
        ordinal < 2 && generation > 1 && full(first),
        "unassign must name a previously bound relation",
    )?;
    let mut baseline: [Option<&PassiveEvent>; 2] = [None, None];
    for event in &events[..start - 2] {
        if let Some(index @ 0..=1) = event.local_ordinal {
            match event.kind.as_str() {
                "BINDING_BOUND" => baseline[index as usize] = Some(event),
                "BINDING_UNBOUND" | "BINDING_RETIRED" => baseline[index as usize] = None,
                _ => {}
            }
        }
    }
    let baseline = [
        baseline[0].ok_or("device v2: A was not bound")?,
        baseline[1].ok_or("device v2: B was not bound")?,
    ];
    ensure(
        baseline
            .iter()
            .all(|e| e.owner_generation.checked_add(1) == Some(generation)),
        "unassign generation must advance once from both bound endpoints",
    )?;
    ensure(
        same_relation(first, baseline[ordinal])
            && first.topology_digest == baseline[ordinal].topology_digest,
        "unassign does not match existing Player relation",
    )?;
    for (index, event) in events[start - 2..start].iter().enumerate() {
        ensure(
            event.kind == "BINDING_UNBOUND"
                && event.local_ordinal == Some(index as u32)
                && event.owner_generation == generation
                && event.phase == 10,
            "primary unassign must invalidate A then B in its generation",
        )?;
    }
    let detached = &causal[1].1.assignment_digest;
    ensure(
        digest(detached) && *detached != first.assignment_digest,
        "unassign must read back a distinct detached assignment",
    )?;
    for (index, ((_, event), slot)) in causal.iter().zip(slots).enumerate() {
        let last_pair = index >= slots.len() - 2;
        let begin = index % 2 == 0;
        let expected = match (last_pair, begin) {
            (false, true) => "DEVICE_UNASSIGN_BEGIN",
            (false, false) => "DEVICE_UNASSIGN_END",
            (true, true) => "DEVICE_REASSIGN_BEGIN",
            (true, false) => "DEVICE_REASSIGN_END",
        };
        ensure(
            event.kind == expected
                && event.slot_ordinal == Some(*slot)
                && event.phase == 10
                && event.owner_generation == generation
                && same_identity(first, event)
                && event.main_thread
                && event.depth == 0
                && event.original_calls == u32::from(!begin)
                && event.original_called != begin
                && !event.stable_registry_retired
                && event.binding_state.is_none(),
            "assignment witness identity/order/ABI boundary mismatch",
        )?;
        if index > 0 {
            ensure(
                event.topology_digest.is_none()
                    && event.device_digest.is_none()
                    && digest(&event.assignment_digest)
                    && !event.topology_coherent
                    && !event.binding_unique
                    && !event.assignment_matches
                    && !event.hud_matches
                    && !event.lua_vector_equivalent
                    && !event.relation_changed,
                "partial assignment witness must not claim a complete topology",
            )?;
            if index < slots.len() - 1 {
                ensure(
                    event.assignment_digest == *detached,
                    "detached identity changed before native reassign returned",
                )?;
            } else {
                ensure(
                    event.assignment_digest != *detached,
                    "reassign did not leave detached state",
                )?;
            }
        }
    }
    let (reassign_end, attached) = causal[causal.len() - 1];
    let relations = events
        .iter()
        .enumerate()
        .filter(|(_, e)| e.kind == "RELATION_CHANGE")
        .collect::<Vec<_>>();
    ensure(
        relations.len() == 1,
        "exactly one consumable causal relation is required",
    )?;
    let (relation_index, relation) = relations[0];
    ensure(
        relation_index > reassign_end
            && full(relation)
            && same_identity(first, relation)
            && relation.assignment_digest == attached.assignment_digest
            && relation.owner_generation == generation
            && relation.phase == 10
            && relation.slot_ordinal == Some(55)
            && relation.relation_changed
            && relation.original_calls == 0
            && !relation.original_called,
        "relation change is not backed by the completed native reassign",
    )?;
    let confirmation = events[reassign_end + 1..relation_index]
        .iter()
        .filter(|e| e.kind == "MANAGER_PASS_END")
        .collect::<Vec<_>>();
    ensure(
        confirmation.len() == 1
            && confirmation[0].loop_iteration == relation.loop_iteration
            && confirmation[0].topology_digest == relation.topology_digest,
        "relation must follow a complete manager return",
    )?;
    let following = events
        .iter()
        .enumerate()
        .skip(relation_index + 1)
        .filter(|(_, e)| matches!(e.kind.as_str(), "BINDING_CANDIDATE" | "BINDING_BOUND"))
        .collect::<Vec<_>>();
    ensure(
        following.len() == 4,
        "transaction requires exactly two candidates then two bounds",
    )?;
    for (index, (_, event)) in following.iter().enumerate() {
        let endpoint = index % 2;
        let expected_kind = if index < 2 {
            "BINDING_CANDIDATE"
        } else {
            "BINDING_BOUND"
        };
        let expected_relation = if endpoint == ordinal {
            relation
        } else {
            baseline[endpoint]
        };
        ensure(
            event.kind == expected_kind
                && event.local_ordinal == Some(endpoint as u32)
                && event.owner_generation == generation
                && same_relation(event, expected_relation)
                && event.topology_digest == relation.topology_digest
                && full(event),
            "post-attach topology/endpoint binding mismatch",
        )?;
        if index < 2 {
            ensure(
                event.loop_iteration == relation.loop_iteration,
                "candidate not in causal manager pass",
            )?;
        } else {
            ensure(
                event.loop_iteration
                    == relation
                        .loop_iteration
                        .checked_add(1)
                        .ok_or("device v2: loop overflow")?,
                "bound not on next consecutive manager pass",
            )?;
        }
    }
    let final_bound_index = following[3].0;
    let candidate_end = following[1].0;
    let bound_start = following[2].0;
    let final_confirmation = events[candidate_end + 1..bound_start]
        .iter()
        .filter(|e| e.kind == "MANAGER_PASS_END")
        .collect::<Vec<_>>();
    ensure(
        final_confirmation.len() == 1
            && final_confirmation[0].topology_digest == relation.topology_digest
            && final_confirmation[0].loop_iteration == following[2].1.loop_iteration,
        "confirmation must contain exactly one manager return",
    )?;
    for event in &events[start..] {
        ensure(
            event.owner_generation == generation,
            "transaction generation was invalidated or resurrected",
        )?;
        ensure(
            !matches!(
                event.kind.as_str(),
                "LIFECYCLE_ENTER"
                    | "LIFECYCLE_EXIT"
                    | "EXIT_BEGIN"
                    | "EXIT_RETURN_STABLE"
                    | "ORDINARY_SPAWN_BEGIN"
                    | "ORDINARY_SPAWN_END"
                    | "ORDINARY_ASSIGNMENT_BEGIN"
                    | "ORDINARY_ASSIGNMENT_END"
                    | "JOIN_HUD_BEGIN"
                    | "JOIN_HUD_END"
                    | "EXIT_HUD_BEGIN"
                    | "EXIT_HUD_END"
                    | "BINDING_RETIRED"
                    | "BINDING_UNBOUND"
            ),
            "unrelated lifecycle/assignment invalidated device transaction",
        )?;
    }
    ensure(
        !events[start..relation_index]
            .iter()
            .any(|e| matches!(e.kind.as_str(), "BINDING_BOUND" | "BINDING_CANDIDATE")),
        "binding published before causal manager confirmation",
    )?;
    ensure(
        !events[..final_bound_index + 1]
            .iter()
            .any(|e| matches!(e.kind.as_str(), "HOOKS_RESTORED" | "PRODUCER_CLOSED")),
        "producer restored/closed before causal binding completed",
    )?;
    ensure(
        manifest.final_owner_generation == generation
            && events
                .first()
                .is_some_and(|e| e.kind == "NATIVE_INITIALIZED")
            && events
                .iter()
                .filter(|e| e.kind == "NATIVE_INITIALIZED")
                .count()
                == 1
            && events
                .iter()
                .filter(|e| e.kind == "HOOKS_INSTALLED")
                .count()
                == 1
            && events.iter().filter(|e| e.kind == "HOOKS_RESTORED").count() == 1
            && events
                .iter()
                .filter(|e| e.kind == "PRODUCER_CLOSED")
                .count()
                == 1,
        "terminal generation/publication sequence mismatch",
    )
}

/// Only these immutable, independently replayed pre-v2 live sources may cross
/// the producer-revision boundary. New/unknown source triples get no exemption.
pub fn approved_legacy(scenario: &str, manifest: &str, events: &str, analysis: &str) -> bool {
    const APPROVED: [(&str, &str, &str, &str); 3] = [
        (
            "native-join-passive",
            "E268ECE2E43FB2F0834E797E9986D0294A67E03FC347CF39D3801D8F5064B991",
            "BE9AEE8A6AE6C96F919563AFA71C0A60A10E9A0762D7E7C12927F3BFC5CBC6B6",
            "BD2A8C6E8EE18B7D2E0B684062193731B39750DE6835403358AE196827B1FD53",
        ),
        (
            "death-respawn-passive",
            "50394427D5217CC7CE7587D8C29AEBB377CA58F0DE7D7EEB5AC0B0BA16E006D2",
            "B7A479B17DE837EB662376220B17FB8207952D2E49A7DF112F98FF48A70154FB",
            "B4113180004E9F765EE88F117D84D095909D230155DB3BF809A45AF828711C78",
        ),
        (
            "room-rebind-passive",
            "01FE528ABC6BE9C401A1FA5A8785D37D6DED64482C214FE42B6FB9B777882F0E",
            "5F69A683587DCC63DB73CC0C641A76208D90369B620E45B40C3936BA4C093EA3",
            "FABD4445747902C73B73E7907AC2B9F6A290807F6CAC4182795FC62711E7DACA",
        ),
    ];
    APPROVED.contains(&(scenario, manifest, events, analysis))
}
