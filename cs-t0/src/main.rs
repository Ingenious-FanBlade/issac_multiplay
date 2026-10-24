fn main() {
    let result=if std::env::args_os().nth(1).as_deref()==Some(std::ffi::OsStr::new("coop-active")) {
        isaac_cs_t0::coop_active::run_from_env()
    } else if std::env::args_os().nth(1).as_deref()==Some(std::ffi::OsStr::new("coop-input-config")) {
        isaac_cs_t0::coop_active::run_input_config_from_env()
    } else if matches!(std::env::args().nth(1).as_deref(),Some("world-replay"|"world-compare")) {
        isaac_cs_t0::world_record::run_from_env()
    } else {isaac_cs_t0::commands::run_from_env()};
    if let Err(error) = result {
        eprintln!("IsaacCsT0: {error}");
        std::process::exit(error.kind.code());
    }
}
