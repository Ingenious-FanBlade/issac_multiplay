//! 固定 profile 声明：迁自原型，目标版本偏移只从 manifest 读取。
pub const PROFILE: &str = "min-read-v1";
pub const CLASSES: [&str; 9] = [
    "Player",
    "NPC",
    "Tear",
    "Projectile",
    "Pickup",
    "Familiar",
    "Effect",
    "Grid",
    "UnknownEntity",
];
pub const SOURCES: [&str; 15] = [
    "main",
    "persistent",
    "update",
    "render",
    "effect",
    "temp",
    "bufferTemplate",
    "wisp",
    "roster",
    "esau",
    "player_relations",
    "grid",
    "door",
    "reserved_13",
    "reserved_14",
];
pub const REASONS: [&str; 24] = [
    "NONE",
    "PHASE_INCOMPLETE",
    "ODD_PLAYER_ONLY",
    "LIFECYCLE",
    "READ_FAILED",
    "BUDGET_EXCEEDED",
    "RELATION_BUDGET_EXCEEDED",
    "BUFFER_BACKPRESSURE",
    "UNKNOWN_CONTINUITY",
    "WRONG_THREAD",
    "NONFINITE",
    "SEQUENCE_EXHAUSTED",
    "ROOT_CHANGED",
    "EVENT_OVERFLOW",
    "STOPPED",
    "UNKNOWN_CLASS",
    "NOT_READY",
    "SOURCE_CHANGED",
    "UNPAIRED_SCOPE",
    "FILE_ERROR",
    "NOTIFICATION_FAILED",
    "PREPARE_FAILED",
    "THREAD_FAILED",
    "INVALID_RECORD",
];
pub const HOOKS: [&str; 19] = [
    "Factory",
    "Init",
    "Publish",
    "Remove",
    "ClearReferences",
    "Destroy",
    "PlayerCtor",
    "PlayerCreate",
    "ManagerRestore",
    "Replace",
    "PlayerRestore",
    "NpcMorph",
    "PickupMorph",
    "GridFactory",
    "GridInit",
    "GridSpawn",
    "Door",
    "GridFree",
    "Spawn",
];
pub fn field_name(id: u16) -> &'static str {
    const COMMON: [&str; 30] = [
        "index",
        "type",
        "variant",
        "subtype",
        "flags",
        "valid",
        "visible",
        "exists",
        "dead",
        "removed_by_factory",
        "position.x",
        "position.y",
        "velocity.x",
        "velocity.y",
        "hp",
        "max_hp",
        "init_seed",
        "drop_rng.word0",
        "drop_rng.word1",
        "drop_rng.word2",
        "drop_rng.word3",
        "parent",
        "child",
        "target",
        "spawner",
        "spawner_type",
        "spawner_variant",
        "birth_counter",
        "game_frame_counter",
        "frame_count",
    ];
    const PLAYER: [&str; 16] = [
        "player_type",
        "player_index",
        "damage_cooldown",
        "max_hearts",
        "red_hearts",
        "eternal_hearts",
        "soul_hearts",
        "black_hearts",
        "bone_hearts",
        "golden_hearts",
        "broken_hearts",
        "rotten_hearts",
        "sub_player",
        "twin",
        "backup",
        "replaced",
    ];
    const NPC: [&str; 4] = [
        "state_frame",
        "state",
        "projectile_cooldown",
        "projectile_delay",
    ];
    const FAMILIAR: [&str; 3] = ["player", "fire_cooldown", "state"];
    const EFFECT: [&str; 3] = ["state", "timeout", "lifespan"];
    const PICKUP: [&str; 3] = ["timeout", "wait", "state"];
    const GRID: [&str; 17] = [
        "type",
        "variant",
        "state",
        "spawn_count",
        "spawn_seed",
        "var_data",
        "initialized",
        "variable_seed",
        "grid_index",
        "spawn_frame",
        "rng.word0",
        "rng.word1",
        "rng.word2",
        "rng.word3",
        "collision_class",
        "position.x",
        "position.y",
    ];
    match id {
        1..=0x1e => COMMON[(id - 1) as usize],
        0x101..=0x110 => PLAYER[(id - 0x101) as usize],
        0x201..=0x204 => NPC[(id - 0x201) as usize],
        0x301..=0x303 => FAMILIAR[(id - 0x301) as usize],
        0x401..=0x403 => EFFECT[(id - 0x401) as usize],
        0x501..=0x503 => PICKUP[(id - 0x501) as usize],
        0x601 => "tear_flags",
        0x701 => "projectile_flags",
        0x801..=0x811 => GRID[(id - 0x801) as usize],
        _ => "unknown_field",
    }
}
pub fn required(class: &str) -> Vec<u16> {
    let mut ids = if class == "Grid" {
        (0x801..=0x811).collect()
    } else if class == "UnknownEntity" {
        Vec::new()
    } else {
        (1..=0x1e).collect()
    };
    let (first, last) = match class {
        "Player" => (0x101, 0x110),
        "NPC" => (0x201, 0x204),
        "Familiar" => (0x301, 0x303),
        "Effect" => (0x401, 0x403),
        "Pickup" => (0x501, 0x503),
        "Tear" => (0x601, 0x601),
        "Projectile" => (0x701, 0x701),
        _ => (1, 0),
    };
    ids.extend(first..=last);
    ids
}
pub fn expected_field(id: u16) -> (u32, &'static str) {
    match id {
        0x0005 | 0x0701 => (8, "RAW_LE"),
        0x0601 => (16, "RAW_LE"),
        0x0006..=0x000a | 0x0807 => (1, "RAW_LE"),
        0x000b..=0x0010 | 0x0810..=0x0811 => (4, "F32_BITS"),
        0x0016..=0x0019 | 0x010d..=0x0110 | 0x0301 => (4, "REFERENCE"),
        0x001e => (4, "DERIVED_RAW32"),
        _ => (4, "RAW_LE"),
    }
}
