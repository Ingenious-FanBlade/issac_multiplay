//! tx-04-06 的一次单启动 E2E；S0–S11 顺序执行，失败保留现场且不重跑。
mod evidence;
use crate::{
    Control, Result, Run, read,
    remote::{Remote, Step, protocol::State},
    win32,
};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    thread,
    time::{Duration, Instant},
};
type Status = BTreeMap<String, String>;
fn step(after: u64, movement: [i16; 2], aim: [i16; 2], edge: u32) -> Step {
    Step {
        after,
        state: State {
            held: if aim == [0, 0] { 0 } else { 1 },
            edge,
            movement,
            aim,
        },
    }
}
impl Run {
    // 远端错误立即进入本次失败收尾；等待不创建新会话，不重发物理键。
    fn remote_watch(
        &mut self,
        game: &win32::Game,
        c: &mut Control,
        r: &Remote,
        ms: u64,
    ) -> Result<Status> {
        let end = Instant::now() + Duration::from_millis(ms);
        loop {
            let remote = r.snapshot();
            if let Some(e) = remote.error {
                return Err(format!("remote: {e}"));
            }
            let status = self.watch_tx05(game, c, 0)?;
            if Instant::now() >= end {
                return Ok(status);
            }
            thread::sleep(Duration::from_millis(25));
        }
    }
    fn remote_start(&self, round: u64) -> Result<Remote> {
        let port = self.scenario["host_port"].as_u64().ok_or("host_port")?;
        if port == 0 || port > 65535 {
            return Err("host_port_range".into());
        }
        Remote::start(
            format!("127.0.0.1:{port}")
                .parse()
                .map_err(|e| format!("{e}"))?,
            &self.out.join(format!("remote-{round}.jsonl")),
            self.scenario["peer_timeout_ms"].as_u64().unwrap_or(3000),
            self.scenario["remote_lead"].as_u64().unwrap_or(2),
        )
    }
    // 脚本从远端实际收到的 Ack.tick 起算；保留安装点，避免用请求前快照猜目标。
    fn play_tx06(
        &mut self,
        game: &win32::Game,
        c: &mut Control,
        r: &Remote,
        label: &str,
        steps: Vec<Step>,
        duration: u64,
    ) -> Result<()> {
        self.checkpoint_tx05(c, &format!("{label}.begin"))?;
        r.script(label, steps)?;
        let deadline = Instant::now() + Duration::from_secs(15);
        loop {
            self.remote_watch(game, c, r, 40)?;
            let snapshot = r.snapshot();
            if snapshot.script_label == label {
                if snapshot
                    .ack
                    .as_ref()
                    .is_some_and(|a| a.generation != snapshot.script_generation)
                {
                    return Err(format!("script_generation_invalidated: {label}"));
                }
                let base = snapshot.script_base;
                self.report["scripts"][label] =
                    json!({"base":base,"generation":snapshot.script_generation});
                if snapshot
                    .ack
                    .as_ref()
                    .is_some_and(|a| a.tick >= base + duration)
                {
                    break;
                }
            }
            if Instant::now() >= deadline {
                return Err(format!("script_timeout: {label}"));
            }
        }
        self.checkpoint_tx05(c, &format!("{label}.end"))?;
        Ok(())
    }
    fn remote_end_tx06(
        &mut self,
        game: &win32::Game,
        c: &mut Control,
        r: &Remote,
        round: u64,
    ) -> Result<()> {
        let deadline = Instant::now() + Duration::from_secs(5);
        loop {
            self.remote_watch(game, c, r, 30)?;
            let s = r.snapshot();
            if s.finished {
                self.report["remote_rounds"][round.to_string()] = json!({"tx":s.tx,"rx":s.rx,"duplicate":s.duplicates,
                    "stale":s.stale,"establishments":s.establishments,"bye":s.bye,"error":s.error});
                return if s.bye.is_some() {
                    Ok(())
                } else {
                    Err("remote_finished_without_bye".into())
                };
            }
            if Instant::now() >= deadline {
                return Err("remote_bye_timeout".into());
            }
        }
    }
    fn host_again_tx06(&mut self, c: &mut Control, label: &str) -> Result<()> {
        let port = self.scenario["host_port"].as_u64().ok_or("host_port")?;
        self.step(label, json!(c.command(&format!("cs host {port} 0"))?))?;
        c.wait("session_state", "Hosting", 5)?;
        Ok(())
    }
    fn scenes_tx06(&mut self, game: &win32::Game) -> Result<()> {
        let timeout = self.scenario["startup_timeout_seconds"]
            .as_u64()
            .unwrap_or(90);
        let w = game.wait_window(timeout)?;
        let focused = win32::focus(w);
        self.step(
            "startup_focus",
            json!({"ok":focused.is_ok(),"error":focused.err()}),
        )?;
        let mut c = Control::connect(
            self.scenario["control_port"].as_u64().unwrap_or(27091) as u16,
            game,
            timeout,
            &self.out.join("control.jsonl"),
        )?;
        c.wait_startup(game, w, timeout)?;
        // S1 首轮 host 只能由菜单 Lua 夹具发起；控制口只观察。
        c.wait("session_state", "Hosting", 10)?;
        self.checkpoint_tx05(&mut c, "S1.begin")?;
        self.watch_tx05(game, &mut c, 500)?;
        self.checkpoint_tx05(&mut c, "S1.end")?;
        win32::screenshot(w, &self.out.join("S1-hosting.bmp"))?;
        let remote = self.remote_start(1)?;
        c.wait("session_state", "Warmup", 5)?;
        self.checkpoint_tx05(&mut c, "S2.end")?;
        self.enter_run_tx05(game, w, &mut c)?;
        c.wait("session_state", "Active", 15)?;
        c.wait("b_bound", "1", 5)?;
        self.remote_watch(game, &mut c, &remote, 400)?;
        self.checkpoint_tx05(&mut c, "S3.end")?;
        win32::screenshot(w, &self.out.join("S3-joined.bmp"))?;
        let mut keys = win32::HeldKeys::new(w);
        self.play_tx06(
            game,
            &mut c,
            &remote,
            "S4.right",
            vec![step(0, [32767, 0], [0, 0], 0), step(24, [0, 0], [0, 0], 0)],
            30,
        )?;
        self.play_tx06(
            game,
            &mut c,
            &remote,
            "S4.up",
            vec![step(0, [0, -32767], [0, 0], 0), step(24, [0, 0], [0, 0], 0)],
            30,
        )?;
        win32::screenshot(w, &self.out.join("S4-movement.bmp"))?;
        // S5 同一游戏窗口，A 的物理 D 与 B 的射击经各自 ordinal 汇合到同一 tick。
        keys.press("D")?;
        self.play_tx06(
            game,
            &mut c,
            &remote,
            "S5",
            vec![step(0, [0, 0], [32767, 0], 0), step(24, [0, 0], [0, 0], 0)],
            30,
        )?;
        keys.release("D")?;
        self.play_tx06(
            game,
            &mut c,
            &remote,
            "S6",
            vec![step(0, [0, 0], [0, 0], 1), step(1, [0, 0], [0, 0], 0)],
            8,
        )?;
        self.play_tx06(
            game,
            &mut c,
            &remote,
            "S6.safety",
            vec![
                step(0, [-32767, 0], [0, 0], 0),
                step(18, [0, 32767], [0, 0], 0),
                step(36, [0, 0], [0, 0], 0),
            ],
            42,
        )?;
        self.checkpoint_tx05(&mut c, "S7.begin")?;
        remote.script("S7.blocked", vec![step(0, [32767, 0], [32767, 0], 0)])?;
        win32::key(w, "CONSOLE")?;
        self.remote_watch(game, &mut c, &remote, 350)?;
        self.checkpoint_tx05(&mut c, "S7.block.begin")?;
        self.remote_watch(game, &mut c, &remote, 1000)?;
        self.checkpoint_tx05(&mut c, "S7.block.end")?;
        win32::screenshot(w, &self.out.join("S7-overlay.bmp"))?;
        win32::key(w, "ESC")?;
        self.remote_watch(game, &mut c, &remote, 400)?;
        self.checkpoint_tx05(&mut c, "S7.end")?;
        self.checkpoint_tx05(&mut c, "S8.begin")?;
        win32::key(w, "F8")?;
        self.wait_fixture_tx05(game, &mut c, "skip-update", "end")?;
        self.wait_ticks_tx05(game, &mut c, 8)?;
        self.checkpoint_tx05(&mut c, "S8.before-room")?;
        win32::key(w, "F8")?;
        self.wait_fixture_tx05(game, &mut c, "room-transition", "arrived")?;
        c.wait("a_bound", "1", 10)?;
        c.wait("b_bound", "1", 10)?;
        self.remote_watch(game, &mut c, &remote, 300)?;
        self.checkpoint_tx05(&mut c, "S8.arrived")?;
        let latest = self
            .live_fixture()?
            .into_iter()
            .rev()
            .find(|v| v["event"] == "observation")
            .ok_or("missing_room_observation")?;
        let sign = if latest["players"][1]["x"]
            .as_f64()
            .ok_or("missing_b_position")?
            > 320.0
        {
            -32767
        } else {
            32767
        };
        self.report["post_room_sign"] = json!(if sign < 0 { -1 } else { 1 });
        self.play_tx06(
            game,
            &mut c,
            &remote,
            "S8.move",
            vec![step(0, [sign, 0], [0, 0], 0), step(18, [0, 0], [0, 0], 0)],
            24,
        )?;
        self.checkpoint_tx05(&mut c, "S8.end")?;
        self.step("S9_stop", json!(c.command("cs stop")?))?;
        c.wait("session_state", "Idle", 5)?;
        self.remote_end_tx06(game, &mut c, &remote, 1)?;
        self.checkpoint_tx05(&mut c, "S9.retired")?;
        drop(remote);
        // 已由 S5 走过的水平路径反向走回，方向显式记录，供 V2 判定。
        self.checkpoint_tx05(&mut c, "S9.raw.begin")?;
        keys.press("A")?;
        self.watch_tx05(game, &mut c, 700)?;
        keys.release("A")?;
        self.checkpoint_tx05(&mut c, "S9.raw.end")?;
        self.host_again_tx06(&mut c, "S10_host")?;
        self.checkpoint_tx05(&mut c, "S10.raw.begin")?;
        keys.press("D")?;
        self.watch_tx05(game, &mut c, 700)?;
        keys.release("D")?;
        self.checkpoint_tx05(&mut c, "S10.raw.end")?;
        let remote = self.remote_start(2)?;
        c.wait("session_state", "Active", 15)?;
        self.remote_watch(game, &mut c, &remote, 400)?;
        self.checkpoint_tx05(&mut c, "S10.active")?;
        remote.silence()?;
        c.wait("session_state", "Idle", 10)?;
        self.remote_end_tx06(game, &mut c, &remote, 2)?;
        self.checkpoint_tx05(&mut c, "S10.end")?;
        drop(remote);
        self.host_again_tx06(&mut c, "S11_host")?;
        let remote = self.remote_start(3)?;
        c.wait("session_state", "Active", 15)?;
        self.remote_watch(game, &mut c, &remote, 400)?;
        self.checkpoint_tx05(&mut c, "S11.active")?;
        self.return_to_menu(w, &mut c)?;
        c.wait("session_state", "Idle", 5)?;
        self.remote_end_tx06(game, &mut c, &remote, 3)?;
        self.checkpoint_tx05(&mut c, "S11.end")?;
        drop(remote);
        self.report["final_status"] = json!(c.command("cs status")?);
        crate::write_json(
            &self.out.join("counters.json"),
            &json!(c.command("cs counters")?),
        )?;
        drop(keys);
        drop(c);
        let code = game.close()?;
        self.report["enabled_exit"] = json!(code);
        if code != 0 {
            return Err(format!("game_exit={code}"));
        }
        Ok(())
    }
    pub(super) fn execute_tx06(&mut self) -> Result<()> {
        self.step("S0",json!({"launches":1,"manual_input":"SkipIntro=1; stop manual operation before launch; harness owns keyboard and loopback remote"}))?;
        self.environment()?;
        self.input_environment_tx05()?;
        self.capture_sources()?;
        self.report["crash_before"] = self.crash_files()?;
        self.snapshot("save_before")?;
        self.deploy()?;
        let game = self.launch("enabled")?;
        self.report["launches"] = json!(1);
        let pid = game.pid();
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| self.scenes_tx06(&game)))
                .unwrap_or_else(|_| Err("tx-04-06 scenario panic".into()));
        if let Err(e) = &result {
            self.capture_failure(&game, "enabled", e);
            let closed = game.close();
            self.report["failure_close"] =
                json!({"exit":closed.as_ref().ok(),"error":closed.as_ref().err()});
        }
        drop(game);
        thread::sleep(Duration::from_millis(300));
        if let Err(e) = self.collect(pid, "enabled") {
            self.report["collection_error"] = json!(e);
        }
        if let Err(e) = self.snapshot("save_mid") {
            self.report["save_mid_error"] = json!(e);
        }
        result
    }
    pub(super) fn finish_tx06(&mut self, result: Result<()>) -> Result<()> {
        self.finish_tx05(result)
    }
}
