fn main() {
    if let Err(error) = isaac_cs_t0::t09::run_from_env() {
        eprintln!("IsaacCsT0T09: {error}");
        std::process::exit(error.kind.code());
    }
}
