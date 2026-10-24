//! T0-09-B transport/control foundation.
//!
//! The modules in this directory deliberately contain no game-object access.
//! They bridge immutable ICT8 packets between a Wire IPC v1 mapping and a UDP
//! socket, provide the T9C1 control plane, and orchestrate the offline F0
//! two-process endpoint scenario. Real-game Native integration is a separate
//! boundary and must not be inferred from an offline PASS.

pub mod authority_game;
pub mod control;
pub mod endpoint;
pub mod manifest;
pub mod orchestrator;
pub mod relay;
pub mod wire;

use crate::error::{AppError, AppResult};

pub const RUNNER_IDENTITY: &str = "IsaacCsT0-T09B-v1";

pub fn run_from_env() -> AppResult<()> {
    let arguments: Vec<String> = std::env::args().skip(1).collect();
    if arguments.is_empty() || arguments == ["--help"] {
        print_help();
        return Ok(());
    }
    match arguments[0].as_str() {
        "f0" => orchestrator::run_f0(&arguments[1..]),
        "live-f0" => orchestrator::run_live_f0(&arguments[1..]),
        "endpoint" => endpoint::run(&arguments[1..]),
        "relay" => relay::run(&arguments[1..]),
        other => Err(AppError::usage(format!(
            "unknown T0-09-B command '{other}'; use f0, endpoint, or relay"
        ))),
    }
}

fn print_help() {
    println!("IsaacCsT0T09 <f0|endpoint|relay> ...");
    println!("  f0       --manifest <json> --artifact-dir <new-dir> --scenario <exe>");
    println!("  live-f0  --manifest <json> --artifact-dir <new-dir> --native-dll <dll>");
    println!("           --helper <exe> --vm-share <host-dir> [--game-exe <host-exe>] [--await-b-seconds N]");
    println!("  endpoint --role <A|B> --manifest <json> --artifact-dir <new-dir>");
    println!("           --control <host:port> --relay <host:port> --scenario <exe>");
    println!("  relay    --manifest <json> --artifact-dir <new-dir> --stop-file <new-file>");
}
