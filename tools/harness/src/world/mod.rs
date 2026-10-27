//! tx-04-08 离线入口。依赖仅为自有文件模型，不触达 win32、网络或旧 mapping。
mod compare;
pub(crate) mod dataset;
mod io;
pub(crate) mod model;
mod profile;
mod replay;
mod validate;
type Result<T> = std::result::Result<T, String>;
use serde_json::json;
use std::{collections::BTreeMap, fs, path::Path};

pub const DOMAIN: &str = "isaac.cs.world-record";
pub const LEGACY: &str = "isaac.cs.t0.world-record";
pub const PHASE: &str = "POST_STANDARD_UPDATE_PRE_TOP_EXIT";

// 参数在任何建目录/文件操作之前检查；legacy 必须显式选择，绝不自动降级。
pub fn run_cli(args: &[String]) -> Result<()> {
    let command = args
        .first()
        .map(String::as_str)
        .ok_or("missing world command")?;
    if args.len() == 2 && args[1] == "--help" {
        println!(
            "harness world-replay --capture <world-dir> --artifact-dir <new-dir> [--legacy-t0]\nharness world-compare --left <world-dir> --right <world-dir> --alignment <json> --artifact-dir <new-dir> [--legacy-t0]"
        );
        return Ok(());
    }
    let required = match command {
        "world-replay" => vec!["--capture", "--artifact-dir"],
        "world-compare" => vec!["--left", "--right", "--alignment", "--artifact-dir"],
        _ => return Err("unknown world command".into()),
    };
    let mut options = BTreeMap::new();
    let mut legacy = false;
    let mut i = 1;
    while i < args.len() {
        let key = &args[i];
        if key == "--legacy-t0" {
            if legacy {
                return Err("duplicate --legacy-t0".into());
            }
            legacy = true;
            i += 1;
            continue;
        }
        if !required.contains(&key.as_str())
            || i + 1 == args.len()
            || args[i + 1].starts_with("--")
            || options.insert(key.clone(), args[i + 1].clone()).is_some()
        {
            return Err("expected unique world options; use --help".into());
        }
        i += 2;
    }
    if required.iter().any(|k| !options.contains_key(*k)) {
        return Err("missing world option; use --help".into());
    }
    let out = Path::new(&options["--artifact-dir"]);
    fs::create_dir(out)
        .map_err(|e| format!("create new artifact directory {}: {e}", out.display()))?;
    let result = (|| {
        io::save(
            &out.join("invocation.json"),
            &json!({"command":command,"arguments":options,"legacy_t0":legacy,"game_operations":false,"automatic_retry":false}),
        )?;
        if command == "world-replay" {
            replay::run(Path::new(&options["--capture"]), out, legacy)
        } else {
            compare::run(
                Path::new(&options["--left"]),
                Path::new(&options["--right"]),
                Path::new(&options["--alignment"]),
                out,
                legacy,
            )
        }
    })();
    if let Err(e) = &result {
        if let Err(report) = io::save(
            &out.join("failure.json"),
            &json!({"status":"ERROR","error":e,"automatic_retry":false}),
        ) {
            return Err(format!("{e}; failure report: {report}"));
        }
    }
    result
}
