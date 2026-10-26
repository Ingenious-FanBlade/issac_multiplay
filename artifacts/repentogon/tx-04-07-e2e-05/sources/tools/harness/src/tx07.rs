//! tx-04-07 单启动四轮：Stop、PeerBye、PeerTimeout、RunTeardown；失败保留现场且不重跑。
mod evidence;
mod files;
use crate::{
    Control, Result, Run,
    relay::{Profile, Relay},
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
fn number(s: &Status, key: &str) -> Result<u64> {
    s.get(key)
        .ok_or_else(|| format!("missing {key}"))?
        .parse()
        .map_err(|e| format!("{key}: {e}"))
}
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
    pub(super) fn fixture_config_tx07(&self) -> Result<String> {
        let actions = self.scenario["actions"].as_array().ok_or("actions")?;
        if actions.iter().filter_map(Value::as_str).collect::<Vec<_>>()
            != ["skip", "room", "kill_b", "revive_b", "stage", "room"]
        {
            return Err("tx07_action_order".into());
        }
        let timeout = self.scenario["action_timeout_ms"]
            .as_u64()
            .ok_or("action_timeout_ms")?;
        let actions = actions
            .iter()
            .map(|v| {
                format!(
                    "{{ name = \"{}\", timeout_ms = {timeout} }}",
                    v.as_str().unwrap()
                )
            })
            .collect::<Vec<_>>()
            .join(", ");
        Ok(format!(
            "-- 本次临时 tx07 配置，不写回源码。\nreturn {{ tx = \"tx-04-07\", token = \"{}\", host_port = {}, menu_host = true, expected_b_player_type = 0, actions = {{ {} }}, observation_heartbeat = {}, laser_event_cap = {}, animation_timeout_ms = {} }}\n",
            self.report["token"].as_str().ok_or("token")?,
            self.scenario["host_port"],
            actions,
            self.scenario["observation_heartbeat"],
            self.scenario["laser_event_cap"],
            self.scenario["animation_timeout_ms"]
        ))
    }
    // 场景检查点同时发布分键累计与主线程状态，Idle 尚无 runtime 时明确记录未发布。
    fn checkpoint_tx07(&mut self, c: &mut Control, label: &str) -> Result<Status> {
        if let Some(scene) = label
            .split('.')
            .next()
            .filter(|s| s.starts_with('S') && s[1..].bytes().all(|b| b.is_ascii_digit()))
        {
            if self.report["scenes"][scene] != "COMPLETE" {
                self.report["scenes"][scene] = json!("RUNNING");
            }
        }
        let status = self.checkpoint_tx05(c, label)?;
        let result = c.request(&format!("cs query_checkpoint {label}"))?;
        if result.get("ok").map(String::as_str) != Some("1")
            && result.get("error").map(String::as_str) != Some("no_session")
        {
            return Err(format!("query_checkpoint:{result:?}"));
        }
        self.report["checkpoints"][label]["query"] = json!(result);
        Ok(status)
    }
    fn watch_tx07(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: &Remote,
        ms: u64,
    ) -> Result<Status> {
        let end = Instant::now() + Duration::from_millis(ms);
        loop {
            let remote = r.snapshot();
            if let Some(error) = &remote.error {
                if !(remote.expected_timeout
                    && remote.terminal_cause.as_deref() == Some("peer_timeout")
                    && error == "peer_timeout")
                {
                    return Err(format!("remote:{error}"));
                }
            }
            let records = self.live_fixture()?;
            if records
                .iter()
                .any(|v| v["fixture_error"] == true || v["event"] == "fixture-error")
            {
                return Err("tx07_fixture_error".into());
            }
            let status = self.watch_tx05(g, c, 0)?;
            if Instant::now() >= end {
                return Ok(status);
            }
            thread::sleep(Duration::from_millis(30));
        }
    }
    // 需要持续模拟的场景：维护焦点后仅给原生状态同步 500ms，不发 ESC、不延长窗口。
    // S6 的预期暂停/控制台、加载动作和退休仍用普通 watch，不能被此处自动恢复。
    fn watch_running_tx07(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: &Remote,
        ms: u64,
    ) -> Result<Status> {
        let end = Instant::now() + Duration::from_millis(ms);
        loop {
            let status = self.watch_tx07(g, c, r, 0)?;
            if status.get("pause_state").map(String::as_str) != Some("0") {
                if Instant::now() >= end
                    || !c
                        .focus_restored_at
                        .is_some_and(|t| t.elapsed() < Duration::from_millis(500))
                {
                    self.step("unexpected_pause", json!({"status":status}))?;
                    return Err("unexpected_pause_in_running_scene".into());
                }
            } else if Instant::now() >= end {
                return Ok(status);
            }
            thread::sleep(Duration::from_millis(30));
        }
    }
    fn relay_tx07(&self, round: u64, initial: Profile) -> Result<(Relay, Remote)> {
        let host = self.scenario["host_port"]
            .as_u64()
            .filter(|p| (1..=65535).contains(p))
            .ok_or("host_port")?;
        let seed = self.scenario["relay_seed"]
            .as_u64()
            .ok_or("relay_seed")?
            .checked_add(round)
            .ok_or("relay_seed_overflow")?;
        let relay = Relay::start(
            format!("127.0.0.1:{host}")
                .parse()
                .map_err(|e| format!("{e}"))?,
            &self.out.join(format!("relay-{round}.jsonl")),
            seed,
            usize::try_from(
                self.scenario["relay_capacity"]
                    .as_u64()
                    .ok_or("relay_capacity")?,
            )
            .map_err(|e| e.to_string())?,
            initial,
        )?;
        let remote = Remote::start_locked(
            relay.address,
            &self.out.join(format!("remote-{round}.jsonl")),
            self.scenario["peer_timeout_ms"]
                .as_u64()
                .ok_or("peer_timeout_ms")?,
            2,
            |address| relay.lock_remote(address),
        )?;
        Ok((relay, remote))
    }
    // 新代次必须由 DLL 和远端 Ack 同时确认；不靠旧 b_bound 快照提前结束等待。
    fn stable_tx07(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: &Remote,
        old: Option<u64>,
    ) -> Result<Status> {
        let end = Instant::now() + Duration::from_secs(45);
        loop {
            let status = self.watch_running_tx07(g, c, r, 30)?;
            let snapshot = r.snapshot();
            if status.get("session_state").map(String::as_str) == Some("Active")
                && status.get("blocks").map(String::as_str) == Some("0")
                && status.get("a_bound").map(String::as_str) == Some("1")
                && status.get("b_bound").map(String::as_str) == Some("1")
            {
                let generation = number(&status, "b_generation")?;
                if old.is_none_or(|old| generation > old)
                    && snapshot
                        .ack
                        .as_ref()
                        .is_some_and(|a| a.state == 3 && a.generation == generation)
                {
                    return Ok(status);
                }
            }
            if Instant::now() >= end {
                return Err(format!("stable_binding_timeout:{status:?}"));
            }
        }
    }
    fn play_tx07(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: &Remote,
        label: &str,
        steps: Vec<Step>,
        ticks: u64,
    ) -> Result<()> {
        self.checkpoint_tx07(c, &format!("{label}.begin"))?;
        r.script(label, steps)?;
        self.script_end_tx07(g, c, r, label, ticks)?;
        self.checkpoint_tx07(c, &format!("{label}.end"))?;
        Ok(())
    }
    fn script_end_tx07(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: &Remote,
        label: &str,
        ticks: u64,
    ) -> Result<()> {
        let end = Instant::now() + Duration::from_secs(30);
        loop {
            self.watch_running_tx07(g, c, r, 30)?;
            let snapshot = r.snapshot();
            if snapshot.script_label == label {
                if snapshot
                    .ack
                    .as_ref()
                    .is_some_and(|a| a.generation != snapshot.script_generation)
                {
                    return Err(format!("script_generation_invalidated:{label}"));
                }
                self.report["scripts"][label] =
                    json!({"base":snapshot.script_base,"generation":snapshot.script_generation});
                if snapshot
                    .ack
                    .as_ref()
                    .is_some_and(|a| a.tick >= snapshot.script_base + ticks)
                {
                    return Ok(());
                }
            }
            if Instant::now() >= end {
                return Err(format!("script_timeout:{label}"));
            }
        }
    }
    // 每项 F8 只发送一次；按配置 step 匹配唯一终态，不复用前一个 room 的 arrived。
    fn action_tx07(
        &mut self,
        g: &win32::Game,
        w: usize,
        c: &mut Control,
        r: &Remote,
        index: u64,
        name: &str,
    ) -> Result<()> {
        self.step("fixture_action", json!({"step":index,"action":name}))?;
        win32::key(w, "F8")?;
        let end = Instant::now()
            + Duration::from_millis(
                self.scenario["action_timeout_ms"]
                    .as_u64()
                    .ok_or("action_timeout_ms")?
                    + 5000,
            );
        loop {
            let records = self.live_fixture()?;
            let events = records
                .iter()
                .filter(|v| v["event"] == "action" && v["step"] == index && v["action"] == name)
                .collect::<Vec<_>>();
            if events
                .iter()
                .any(|v| matches!(v["phase"].as_str(), Some("rejected" | "timeout")))
            {
                return Err(format!("fixture_action_failed:{name}"));
            }
            if events.iter().any(|v| v["phase"] == "arrived") {
                return Ok(());
            }
            if Instant::now() >= end {
                return Err(format!("fixture_action_timeout:{name}"));
            }
            self.watch_tx07(g, c, r, 50)?;
        }
    }
    // 复活健康/稳定绑定与 Appear 完成分开等候，射击场景不能在动画中提前断言。
    fn animation_tx07(&mut self, g: &win32::Game, c: &mut Control, r: &Remote) -> Result<()> {
        let end = Instant::now()
            + Duration::from_millis(
                self.scenario["animation_timeout_ms"]
                    .as_u64()
                    .ok_or("animation_timeout_ms")?,
            );
        loop {
            self.watch_running_tx07(g, c, r, 50)?;
            if self
                .live_fixture()?
                .iter()
                .rev()
                .find(|v| v["event"] == "observation")
                .is_some_and(|v| v["players"][1]["animation_finished"] == true)
            {
                return Ok(());
            }
            if Instant::now() >= end {
                return Err("b_animation_timeout".into());
            }
        }
    }
    fn direction_tx07(&mut self, label: &str) -> Result<[i16; 2]> {
        let observation = self
            .live_fixture()?
            .into_iter()
            .rev()
            .find(|v| v["event"] == "observation")
            .ok_or("missing_observation")?;
        let player = &observation["players"][1];
        let options = player["movement_options"]
            .as_array()
            .ok_or("missing_movement_options")?;
        let (x, y) = (
            player["x"].as_f64().ok_or("b_x")?,
            player["y"].as_f64().ok_or("b_y")?,
        );
        let preferred = if x > 320.0 { [-32767, 0] } else { [32767, 0] };
        let candidates = [
            preferred,
            [0, if y > 280.0 { -32767 } else { 32767 }],
            [-preferred[0], 0],
            [0, if y > 280.0 { 32767 } else { -32767 }],
        ];
        let chosen = candidates
            .into_iter()
            .find(|v| options.iter().any(|o| o[0] == v[0] && o[1] == v[1]))
            .ok_or("no_observed_free_direction")?;
        self.report["movement_directions"][label] = json!(chosen);
        Ok(chosen)
    }
    fn move_tx07(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: &Remote,
        label: &str,
    ) -> Result<()> {
        let direction = self.direction_tx07(label)?;
        self.play_tx07(
            g,
            c,
            r,
            label,
            vec![step(0, direction, [0, 0], 0), step(18, [0, 0], [0, 0], 0)],
            24,
        )
    }
    fn host_tx07(&mut self, c: &mut Control, label: &str, character: u64) -> Result<()> {
        let port = self.scenario["host_port"].as_u64().ok_or("host_port")?;
        self.step(
            label,
            json!(c.command(&format!("cs host {port} {character}"))?),
        )?;
        c.wait("session_state", "Hosting", 5)?;
        Ok(())
    }
    fn end_round_tx07(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: Remote,
        relay: Relay,
        round: u64,
    ) -> Result<()> {
        let end = Instant::now() + Duration::from_secs(12);
        loop {
            self.watch_tx07(g, c, &r, 30)?;
            if r.snapshot().finished {
                break;
            }
            if Instant::now() >= end {
                return Err("remote_terminal_timeout".into());
            }
        }
        let snapshot = r.snapshot();
        self.report["remote_rounds"][round.to_string()] = json!({"tx":snapshot.tx,"rx":snapshot.rx,
            "establishments":snapshot.establishments,"bye":snapshot.bye,"error":snapshot.error,
            "terminal_cause":snapshot.terminal_cause,"expected_timeout":snapshot.expected_timeout});
        drop(r);
        self.report["relay_rounds"][round.to_string()] = relay.finish()?;
        self.checkpoint_tx07(c, &format!("round{round}.retired"))?;
        Ok(())
    }
    // Continue 根据实际菜单 ID/选项推进，只在选中 Continue(1) 后发确认键。
    fn continue_tx07(&mut self, g: &win32::Game, w: usize, c: &mut Control) -> Result<()> {
        let s = c.command("cs status")?;
        match s.get("menu_id").map(String::as_str) {
            Some("1") => {
                self.keys(w, &["ENTER".into()])?;
                c.wait("menu_id", "2", 15)?;
            }
            Some("2" | "3") => (),
            _ => return Err(format!("continue_unknown_menu:{s:?}")),
        }
        if c.command("cs status")?.get("menu_id").map(String::as_str) == Some("2") {
            c.wait("selected_save", "1", 5)?;
            self.keys(w, &["ENTER".into()])?;
        }
        c.wait("menu_id", "3", 15)?;
        for _ in 0..8 {
            let s = c.command("cs status")?;
            let selected = number(&s, "menu_game_selection")?;
            if selected == 1 {
                break;
            }
            self.keys(w, &[if selected > 1 { "UP" } else { "DOWN" }.into()])?;
        }
        let selected = c.wait("menu_game_selection", "1", 5)?;
        self.step("S20_continue_selected", json!(selected))?;
        win32::screenshot(w, &self.out.join("S20-continue-menu.bmp"))?;
        win32::key(w, "ENTER")?;
        let end = Instant::now() + Duration::from_secs(45);
        loop {
            let s = self.watch_tx05(g, c, 50)?;
            if s.get("run_active").map(String::as_str) == Some("1") {
                return Ok(());
            }
            if Instant::now() >= end {
                return Err("continue_run_timeout".into());
            }
        }
    }
    fn scenes_tx07(&mut self, g: &win32::Game) -> Result<()> {
        let timeout = self.scenario["startup_timeout_seconds"]
            .as_u64()
            .ok_or("startup_timeout_seconds")?;
        let w = g.wait_window(timeout)?;
        let mut c = Control::connect(
            self.scenario["control_port"]
                .as_u64()
                .ok_or("control_port")? as u16,
            g,
            timeout,
            &self.out.join("control.jsonl"),
        )?;
        // w 来自本次 Game 的已核对 GLFW 窗口；控制请求前保持键盘接管目标在前台。
        c.focus_window = Some(w);
        c.wait_startup(g, w, timeout)?;
        c.wait("session_state", "Hosting", 10)?;
        self.checkpoint_tx07(&mut c, "S1.begin")?;
        self.watch_tx05(g, &mut c, 500)?;
        self.checkpoint_tx07(&mut c, "S1.end")?;
        self.report["scenes"]["S1"] = json!("COMPLETE");
        let (relay, r) = self.relay_tx07(1, Profile::Clean)?;
        r.arm_next_generation(
            "S3.startup",
            vec![step(0, [32767, 0], [0, 0], 0), step(6, [0, 0], [0, 0], 0)],
            60000,
        )?;
        c.wait("session_state", "Warmup", 5)?;
        self.checkpoint_tx07(&mut c, "S2.end")?;
        self.report["scenes"]["S2"] = json!("COMPLETE");
        self.enter_run_tx05(g, w, &mut c)?;
        self.stable_tx07(g, &mut c, &r, None)?;
        self.checkpoint_tx07(&mut c, "S3.end")?;
        self.report["scenes"]["S3"] = json!("COMPLETE");
        let mut keys = win32::HeldKeys::new(w);
        self.play_tx07(
            g,
            &mut c,
            &r,
            "S4.right",
            vec![step(0, [32767, 0], [0, 0], 0), step(24, [0, 0], [0, 0], 0)],
            30,
        )?;
        self.play_tx07(
            g,
            &mut c,
            &r,
            "S4.up",
            vec![step(0, [0, -32767], [0, 0], 0), step(24, [0, 0], [0, 0], 0)],
            30,
        )?;
        keys.press("D")?;
        self.play_tx07(
            g,
            &mut c,
            &r,
            "S4.simultaneous",
            vec![step(0, [0, 0], [32767, 0], 0), step(24, [0, 0], [0, 0], 0)],
            30,
        )?;
        keys.release("D")?;
        self.checkpoint_tx07(&mut c, "S4.priority.begin")?;
        keys.press("RIGHT")?;
        self.watch_tx07(g, &mut c, &r, 1000)?;
        self.checkpoint_tx07(&mut c, "S4.priority.right")?;
        keys.press("LEFT")?;
        self.watch_tx07(g, &mut c, &r, 500)?;
        self.checkpoint_tx07(&mut c, "S4.priority.left")?;
        keys.release("LEFT")?;
        self.watch_tx07(g, &mut c, &r, 500)?;
        self.checkpoint_tx07(&mut c, "S4.priority.fallback")?;
        keys.release("RIGHT")?;
        self.watch_tx07(g, &mut c, &r, 300)?;
        self.checkpoint_tx07(&mut c, "S4.priority.end")?;
        self.report["scenes"]["S4"] = json!("COMPLETE");
        self.checkpoint_tx07(&mut c, "S5.begin")?;
        // 两次边沿分别采样资源窗口，避免共享炸弹库存把两次消耗合并成各减二。
        // 不补发边沿、不增补资源；每个 ordinal 仍须恰好一次 Accepted/Resolve 和减一。
        self.checkpoint_tx07(&mut c, "S5.A.begin")?;
        win32::key(w, "E")?;
        self.watch_tx07(g, &mut c, &r, 250)?;
        self.checkpoint_tx07(&mut c, "S5.A.end")?;
        self.play_tx07(
            g,
            &mut c,
            &r,
            "S5.bomb",
            vec![step(0, [0, 0], [0, 0], 1), step(1, [0, 0], [0, 0], 0)],
            8,
        )?;
        self.checkpoint_tx07(&mut c, "S5.end")?;
        self.report["scenes"]["S5"] = json!("COMPLETE");
        self.checkpoint_tx07(&mut c, "S6.begin")?;
        self.action_tx07(g, w, &mut c, &r, 1, "skip")?;
        self.checkpoint_tx07(&mut c, "S6.console.before")?;
        win32::key(w, "CONSOLE")?;
        self.watch_tx07(g, &mut c, &r, 300)?;
        self.checkpoint_tx07(&mut c, "S6.console.begin")?;
        let token = format!("tx07-echo-{}", std::process::id());
        self.report["console_token"] = json!(token);
        win32::text(w, &format!("cs echo {token}"))?;
        win32::key(w, "ENTER")?;
        self.watch_tx07(g, &mut c, &r, 1000)?;
        self.checkpoint_tx07(&mut c, "S6.console.end")?;
        win32::key(w, "ESC")?;
        self.stable_tx07(g, &mut c, &r, None)?;
        win32::key(w, "ESC")?;
        c.wait_pause(None)?;
        keys.press("D")?;
        self.checkpoint_tx07(&mut c, "S6.pause.begin")?;
        self.watch_tx07(g, &mut c, &r, 1000)?;
        self.checkpoint_tx07(&mut c, "S6.pause.end")?;
        win32::key(w, "ESC")?;
        self.stable_tx07(g, &mut c, &r, None)?;
        self.watch_tx07(g, &mut c, &r, 400)?;
        keys.release("D")?;
        self.checkpoint_tx07(&mut c, "S6.end")?;
        self.report["scenes"]["S6"] = json!("COMPLETE");
        // clean 与故障窗口等长，不根据实际命中率延长；推进率使用 DLL trace 时间轴归约。
        r.script("S7.clean", vec![step(0, [0, 0], [32767, 0], 0)])?;
        self.checkpoint_tx07(&mut c, "S7.clean.begin")?;
        self.watch_running_tx07(
            g,
            &mut c,
            &r,
            self.scenario["clean_baseline_ms"]
                .as_u64()
                .ok_or("clean_baseline_ms")?,
        )?;
        self.checkpoint_tx07(&mut c, "S7.clean.end")?;
        self.step("relay_moderate", relay.switch(Profile::Moderate)?)?;
        r.script(
            "S7.fault",
            vec![
                step(0, [-32767, 0], [32767, 0], 1),
                step(1, [-32767, 0], [32767, 0], 0),
                step(12, [0, 32767], [32767, 0], 0),
                step(24, [32767, 0], [32767, 0], 0),
                step(36, [0, -32767], [32767, 0], 0),
                step(48, [0, 0], [32767, 0], 0),
            ],
        )?;
        self.checkpoint_tx07(&mut c, "S7.begin")?;
        self.watch_running_tx07(
            g,
            &mut c,
            &r,
            self.scenario["moderate_ms"].as_u64().ok_or("moderate_ms")?,
        )?;
        self.checkpoint_tx07(&mut c, "S7.end")?;
        self.step("relay_clean", relay.switch(Profile::Clean)?)?;
        self.step("relay_drained", relay.drain()?)?;
        self.report["scenes"]["S7"] = json!("COMPLETE");
        self.checkpoint_tx07(&mut c, "S8.begin")?;
        self.step("relay_short", relay.switch(Profile::OutageShort)?)?;
        self.watch_running_tx07(g, &mut c, &r, 1200)?;
        self.step("relay_clean", relay.switch(Profile::Clean)?)?;
        self.step("relay_drained", relay.drain()?)?;
        self.play_tx07(
            g,
            &mut c,
            &r,
            "S8.recovered",
            vec![step(0, [0, 0], [32767, 0], 0), step(12, [0, 0], [0, 0], 0)],
            18,
        )?;
        self.checkpoint_tx07(&mut c, "S8.end")?;
        self.report["scenes"]["S8"] = json!("COMPLETE");
        let before = self.checkpoint_tx07(&mut c, "S9.begin")?;
        r.arm_next_generation("S9.fresh", vec![step(0, [0, 0], [32767, 0], 0)], 30000)?;
        keys.press("RIGHT")?;
        self.action_tx07(g, w, &mut c, &r, 2, "room")?;
        self.stable_tx07(g, &mut c, &r, Some(number(&before, "b_generation")?))?;
        keys.release("RIGHT")?;
        self.move_tx07(g, &mut c, &r, "S9.move")?;
        self.checkpoint_tx07(&mut c, "S9.end")?;
        self.report["scenes"]["S9"] = json!("COMPLETE");
        self.files_tx07("S10.before")?;
        self.checkpoint_tx07(&mut c, "S10.begin")?;
        r.script("S10.lost", vec![step(0, [32767, 0], [32767, 0], 0)])?;
        self.action_tx07(g, w, &mut c, &r, 3, "kill_b")?;
        c.wait("b_life", "Lost", 5)?;
        self.checkpoint_tx07(&mut c, "S10.lost.begin")?;
        keys.press("A")?;
        self.wait_ticks_tx05(g, &mut c, 8)?;
        keys.release("A")?;
        self.checkpoint_tx07(&mut c, "S10.lost.end")?;
        self.checkpoint_tx07(&mut c, "S10.end")?;
        win32::screenshot(w, &self.out.join("S10-ghost.bmp"))?;
        self.files_tx07("S10.after")?;
        self.report["scenes"]["S10"] = json!("COMPLETE");
        self.files_tx07("S11.before")?;
        self.checkpoint_tx07(&mut c, "S11.begin")?;
        self.watch_tx07(g, &mut c, &r, 80)?;
        let old = r.snapshot();
        let old_generation = old.ack.as_ref().ok_or("lost_ack")?.generation;
        self.report["lost_ack"] = json!({"sequence":old.last_ack,"generation":old_generation});
        let movement = self.direction_tx07("S11.fresh")?;
        r.arm_next_generation(
            "S11.fresh",
            vec![step(0, movement, [0, 0], 0), step(18, [0, 0], [0, 0], 0)],
            30000,
        )?;
        self.action_tx07(g, w, &mut c, &r, 4, "revive_b")?;
        self.stable_tx07(g, &mut c, &r, Some(old_generation))?;
        r.old_generation_sample(old.last_ack, old_generation)?;
        self.script_end_tx07(g, &mut c, &r, "S11.fresh", 24)?;
        self.animation_tx07(g, &mut c, &r)?;
        self.play_tx07(
            g,
            &mut c,
            &r,
            "S11.shoot",
            vec![step(0, [0, 0], [32767, 0], 0), step(12, [0, 0], [0, 0], 0)],
            18,
        )?;
        self.checkpoint_tx07(&mut c, "S11.end")?;
        win32::screenshot(w, &self.out.join("S11-revived.bmp"))?;
        self.files_tx07("S11.after")?;
        self.report["scenes"]["S11"] = json!("COMPLETE");
        self.files_tx07("S12.before")?;
        let before = self.checkpoint_tx07(&mut c, "S12.begin")?;
        r.arm_next_generation(
            "S12.fresh",
            vec![step(0, [32767, 0], [0, 0], 0), step(6, [0, 0], [0, 0], 0)],
            60000,
        )?;
        self.action_tx07(g, w, &mut c, &r, 5, "stage")?;
        self.stable_tx07(g, &mut c, &r, Some(number(&before, "b_generation")?))?;
        self.move_tx07(g, &mut c, &r, "S12.move")?;
        self.checkpoint_tx07(&mut c, "S12.end")?;
        win32::screenshot(w, &self.out.join("S12-stage.bmp"))?;
        self.files_tx07("S12.after")?;
        self.report["scenes"]["S12"] = json!("COMPLETE");
        self.checkpoint_tx07(&mut c, "S13.begin")?;
        c.command("cs stop")?;
        c.wait("session_state", "Idle", 5)?;
        self.end_round_tx07(g, &mut c, r, relay, 1)?;
        self.checkpoint_tx07(&mut c, "S13.raw.begin")?;
        keys.press("A")?;
        self.watch_tx05(g, &mut c, 700)?;
        keys.release("A")?;
        self.checkpoint_tx07(&mut c, "S13.raw.end")?;
        self.report["scenes"]["S13"] = json!("COMPLETE");
        self.host_tx07(&mut c, "S14.host", 7)?;
        self.checkpoint_tx07(&mut c, "S14.raw.begin")?;
        keys.press("D")?;
        self.watch_tx05(g, &mut c, 700)?;
        keys.release("D")?;
        self.checkpoint_tx07(&mut c, "S14.raw.end")?;
        let (relay, r) = self.relay_tx07(2, Profile::Clean)?;
        self.stable_tx07(g, &mut c, &r, None)?;
        self.checkpoint_tx07(&mut c, "S14.active")?;
        self.report["scenes"]["S14"] = json!("COMPLETE");
        self.animation_tx07(g, &mut c, &r)?;
        self.checkpoint_tx07(&mut c, "S15.begin")?;
        r.script("S15.held", vec![step(0, [0, 0], [32767, 0], 0)])?;
        self.script_end_tx07(
            g,
            &mut c,
            &r,
            "S15.held",
            self.scenario["charge_ticks"]
                .as_u64()
                .ok_or("charge_ticks")?,
        )?;
        let before = self.checkpoint_tx07(&mut c, "S15.charged")?;
        r.arm_next_generation("S15.reheld", vec![step(0, [0, 0], [32767, 0], 0)], 30000)?;
        self.action_tx07(g, w, &mut c, &r, 6, "room")?;
        self.stable_tx07(g, &mut c, &r, Some(number(&before, "b_generation")?))?;
        self.script_end_tx07(
            g,
            &mut c,
            &r,
            "S15.reheld",
            self.scenario["charge_ticks"]
                .as_u64()
                .ok_or("charge_ticks")?,
        )?;
        self.checkpoint_tx07(&mut c, "S15.release")?;
        self.play_tx07(
            g,
            &mut c,
            &r,
            "S15.release",
            vec![step(0, [0, 0], [0, 0], 0)],
            self.scenario["laser_release_ticks"]
                .as_u64()
                .ok_or("laser_release_ticks")?,
        )?;
        self.checkpoint_tx07(&mut c, "S15.end")?;
        self.report["scenes"]["S15"] = json!("COMPLETE");
        r.bye()?;
        c.wait("session_state", "Idle", 8)?;
        self.end_round_tx07(g, &mut c, r, relay, 2)?;
        self.report["scenes"]["S16"] = json!("COMPLETE");
        self.host_tx07(&mut c, "S17.host", 0)?;
        self.checkpoint_tx07(&mut c, "S17.begin")?;
        let (relay, r) = self.relay_tx07(3, Profile::HandshakeLoss)?;
        self.stable_tx07(g, &mut c, &r, None)?;
        self.step("relay_clean", relay.switch(Profile::Clean)?)?;
        self.step("relay_drained", relay.drain()?)?;
        self.move_tx07(g, &mut c, &r, "S17.move")?;
        self.checkpoint_tx07(&mut c, "S17.end")?;
        self.report["scenes"]["S17"] = json!("COMPLETE");
        self.checkpoint_tx07(&mut c, "S18.begin")?;
        r.expect_timeout()?;
        self.step("relay_long", relay.switch(Profile::OutageLong)?)?;
        c.wait("session_state", "Idle", 10)?;
        self.end_round_tx07(g, &mut c, r, relay, 3)?;
        self.checkpoint_tx07(&mut c, "S18.end")?;
        self.report["scenes"]["S18"] = json!("COMPLETE");
        self.files_tx07("S19.before")?;
        let idle = self.checkpoint_tx07(&mut c, "S19.begin")?;
        if idle.get("session_state").map(String::as_str) != Some("Idle")
            || idle.get("players").map(String::as_str) != Some("1")
        {
            return Err("S19_requires_idle_solo_A".into());
        }
        self.return_to_menu(w, &mut c)?;
        self.checkpoint_tx07(&mut c, "S19.end")?;
        self.files_tx07("S19.after")?;
        self.report["scenes"]["S19"] = json!("COMPLETE");
        self.files_tx07("S20.before")?;
        self.host_tx07(&mut c, "S20.host", 0)?;
        self.checkpoint_tx07(&mut c, "S20.hosting.begin")?;
        self.watch_tx05(g, &mut c, 500)?;
        self.checkpoint_tx07(&mut c, "S20.hosting.end")?;
        let (relay, r) = self.relay_tx07(4, Profile::Clean)?;
        r.arm_next_generation(
            "S20.fresh",
            vec![step(0, [32767, 0], [0, 0], 0), step(6, [0, 0], [0, 0], 0)],
            60000,
        )?;
        c.wait("session_state", "Warmup", 5)?;
        self.checkpoint_tx07(&mut c, "S20.begin")?;
        self.continue_tx07(g, w, &mut c)?;
        self.stable_tx07(g, &mut c, &r, None)?;
        self.checkpoint_tx07(&mut c, "S20.active")?;
        self.move_tx07(g, &mut c, &r, "S20.move")?;
        self.checkpoint_tx07(&mut c, "S20.end")?;
        win32::screenshot(w, &self.out.join("S20-continued.bmp"))?;
        self.files_tx07("S20.after")?;
        self.report["scenes"]["S20"] = json!("COMPLETE");
        self.files_tx07("S21.before")?;
        self.checkpoint_tx07(&mut c, "S21.begin")?;
        self.return_to_menu(w, &mut c)?;
        c.wait("session_state", "Idle", 5)?;
        self.end_round_tx07(g, &mut c, r, relay, 4)?;
        self.checkpoint_tx07(&mut c, "S21.end")?;
        self.files_tx07("S21.after")?;
        self.report["scenes"]["S21"] = json!("COMPLETE");
        self.report["final_status"] = json!(c.command("cs status")?);
        crate::write_json(
            &self.out.join("counters.json"),
            &json!(c.command("cs counters")?),
        )?;
        drop(keys);
        drop(c);
        let code = g.close()?;
        self.report["enabled_exit"] = json!(code);
        if code != 0 {
            return Err(format!("game_exit={code}"));
        }
        Ok(())
    }
    pub(super) fn execute_tx07(&mut self) -> Result<()> {
        self.report["invocation"] = json!(std::env::args().collect::<Vec<_>>());
        // 参数检查不启动任何场景；所有阈值在本次进程启动前冻结到 artifact。
        if self.scenario["remote_lead"] != 2
            || self.scenario["clean_baseline_ms"] != 10000
            || self.scenario["moderate_ms"] != 10000
            || self.scenario["fault_rate_min"] != 0.8
            || self.scenario["fault_rate_max"] != 1.2
        {
            return Err("tx07_fixed_fault_window_and_tolerance".into());
        }
        let defaults: Value = serde_json::from_str(include_str!("../scenarios/tx-04-07.json"))
            .map_err(|e| e.to_string())?;
        if self.scenario["profiles"] != defaults["profiles"]
            || self.scenario["peer_timeout_ms"] != 3000
        {
            return Err("tx07_profile_contract".into());
        }
        for (key, min, max) in [
            ("action_timeout_ms", 100, 60000),
            ("animation_timeout_ms", 100, 60000),
            ("observation_heartbeat", 1, 60),
            ("laser_event_cap", 1, 65536),
            ("charge_ticks", 30, 120),
            ("laser_release_ticks", 1, 120),
            ("control_port", 1, 65535),
            ("host_port", 1, 65535),
            ("relay_capacity", 64, 65536),
            ("startup_timeout_seconds", 10, 120),
        ] {
            if !self.scenario[key]
                .as_u64()
                .is_some_and(|n| (min..=max).contains(&n))
            {
                return Err(format!("invalid_tx07_{key}"));
            }
        }
        self.fixture_config_tx07()?;
        for n in 0..=21 {
            self.report["scenes"][format!("S{n}")] = json!("NOT_RUN");
        }
        self.step("S0",json!({"launches":1,"manual_input":"SkipIntro=1; user stops manual operation before launch"}))?;
        self.environment()?;
        self.input_environment_tx05()?;
        self.capture_sources()?;
        self.report["crash_before"] = self.crash_files()?;
        self.snapshot("save_before")?;
        let saves = self.saves.as_ref().ok_or("save_roots")?;
        let mut roots = vec![saves.documents.clone()];
        roots.extend(saves.steam_remote.iter().map(|(_, p)| p.clone()));
        let monitor = files::Monitor::start(roots, self.out.join("file-notifications.jsonl"))?;
        self.deploy()?;
        let game = self.launch("enabled")?;
        let pid = game.pid();
        self.report["launches"] = json!(1);
        self.report["scenes"]["S0"] = json!("COMPLETE");
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| self.scenes_tx07(&game)))
                .unwrap_or_else(|_| Err("tx07_scenario_panic".into()));
        if let Err(error) = &result {
            self.capture_failure(&game, "enabled", error);
            let close = game.close();
            self.report["failure_close"] =
                json!({"exit":close.as_ref().ok(),"error":close.as_ref().err()});
        }
        drop(game);
        if let Err(e) = monitor.finish() {
            self.report["file_monitor_error"] = json!(e);
        }
        if let Err(e) = self.collect(pid, "enabled") {
            self.report["collection_error"] = json!(e);
        }
        if let Err(e) = self.snapshot("save_mid") {
            self.report["save_mid_error"] = json!(e);
        }
        result
    }
    pub(super) fn finish_tx07(&mut self, result: Result<()>) -> Result<()> {
        if result.is_err() {
            if let Some(scenes) = self.report["scenes"].as_object_mut() {
                for value in scenes.values_mut() {
                    if *value == "RUNNING" {
                        *value = json!("FAILED");
                    }
                }
            }
        }
        self.finish_tx05(result)
    }
}
