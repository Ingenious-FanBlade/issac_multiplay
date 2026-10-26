//! tx-04-05 单进程 S0–S11 编排；所有判定只在这次整体 E2E 收尾执行。
pub(super) mod evidence;
use crate::{Control, Result, Run, read, saves, win32, write_json};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    fs, thread,
    time::{Duration, Instant},
};

type Status = BTreeMap<String, String>;
const CASES: [&str; 10] = ["V1", "V2", "V3", "V4", "V5", "V6", "V7", "V8", "V9", "V10"];
fn number(s: &Status, key: &str) -> Result<u64> {
    s.get(key)
        .ok_or_else(|| format!("missing status {key}"))?
        .parse()
        .map_err(|e| format!("invalid {key}: {e}"))
}

// 严格解析同一次部署的 Lua 记录；缺失、损坏和重复序号不会变成空集合通过。
fn fixture_records(text: &str, token: &str) -> Result<Vec<Value>> {
    let mut out = vec![];
    for line in text.lines() {
        if let Some((_, payload)) = line.split_once("[isaac-cs-test] tx05 ").or_else(||line.split_once("[isaac-cs-test] tx06 ")).or_else(||line.split_once("[isaac-cs-test] tx07 ")) {
            let v: Value = serde_json::from_str(payload)
                .map_err(|e| format!("malformed fixture record: {e}"))?;
            if v["token"].as_str() == Some(token) {
                out.push(v);
            }
        }
    }
    Ok(out)
}

impl Run {
    pub(super) fn live_fixture(&self) -> Result<Vec<Value>> {
        let text = read(&self.save.join("log.txt"))?;
        // 游戏可能正追加最后一行；实时等待只读完整行，退出后的归约仍严格读取原始日志。
        let complete = text.rfind('\n').map(|n| &text[..=n]).unwrap_or("");
        fixture_records(
            complete,
            self.report["token"].as_str().ok_or("missing token")?,
        )
    }
    // 场景在控制口等待时仍检查子进程与失败状态，物理输入从不重发。
    pub(super) fn watch_tx05(&mut self, game: &win32::Game, control: &mut Control, ms: u64) -> Result<Status> {
        let end = Instant::now() + Duration::from_millis(ms);
        loop {
            if let Some(code) = game.exited() {
                return Err(format!("game exited during scenario: {code}"));
            }
            let s = control.command("cs status")?;
            // 提前失败也保留最后一次真实主线程快照，不把运行时错误变成“未观察”。
            self.report["last_observed_status"]=json!(s);
            if s.get("session_error").is_some_and(|v| !v.is_empty())
                || s.get("last_error").is_some_and(|v| !v.is_empty())
            {
                return Err(format!("runtime failure: {s:?}"));
            }
            if Instant::now() >= end {
                return Ok(s);
            }
            thread::sleep(Duration::from_millis(80));
        }
    }
    // 状态字段来自上一 Manager 出口，保存原值和 Lua 当前记录以供事后对齐。
    pub(super) fn checkpoint_tx05(&mut self, control: &mut Control, label: &str) -> Result<Status> {
        let s = control.command("cs status")?;
        let observations = self.live_fixture()?;
        if observations.iter().any(|v| v["event"] == "fixture-error") {
            return Err("Lua fixture reported an error".into());
        }
        let latest = observations
            .iter()
            .rev()
            .find(|v| v["event"] == "observation");
        self.report["checkpoints"][label] = json!({"status":s,"observation":latest});
        self.step(
            "checkpoint",
            json!({"label":label,"status":s,"observation":latest}),
        )?;
        Ok(s)
    }
    pub(super) fn wait_ticks_tx05(
        &mut self,
        game: &win32::Game,
        control: &mut Control,
        count: u64,
    ) -> Result<Status> {
        let initial = control.command("cs status")?;
        let generation = number(&initial, "generation")?;
        let target = number(&initial, "completed_tick")? + count;
        let deadline = Instant::now() + Duration::from_secs(12);
        loop {
            let s = self.watch_tx05(game, control, 80)?;
            if number(&s, "generation")? != generation {
                return Err("unexpected session replacement".into());
            }
            if number(&s, "completed_tick")? >= target {
                return Ok(s);
            }
            if Instant::now() >= deadline {
                return Err(format!("Authority tick timeout target={target}: {s:?}"));
            }
        }
    }
    pub(super) fn wait_fixture_tx05(
        &mut self,
        game: &win32::Game,
        control: &mut Control,
        event: &str,
        phase: &str,
    ) -> Result<()> {
        let deadline = Instant::now() + Duration::from_secs(15);
        loop {
            let records = self.live_fixture()?;
            if records.iter().any(|v| {
                v["event"] == "fixture-error"
                    || (v["event"] == "room-transition"
                        && v["phase"] == "arrived"
                        && v["target_matched"] != true)
                    || (v["event"] == event
                        && matches!(v["phase"].as_str(), Some("rejected" | "timeout")))
            }) {
                return Err(format!(
                    "fixture failed while waiting for {event}/{phase}: {records:?}"
                ));
            }
            if records
                .iter()
                .any(|v| v["event"] == event && v["phase"] == phase)
            {
                return Ok(());
            }
            if Instant::now() >= deadline {
                return Err(format!("fixture timeout: {event}/{phase}"));
            }
            self.watch_tx05(game, control, 100)?;
        }
    }
    fn hold_tx05(
        &mut self,
        game: &win32::Game,
        control: &mut Control,
        keys: &mut win32::HeldKeys,
        label: &str,
        key: &str,
        ms: u64,
        active: bool,
    ) -> Result<()> {
        self.checkpoint_tx05(control, &format!("{label}.begin"))?;
        self.step(
            "key_down",
            json!({"label":label,"key":key,"duration_ms":ms}),
        )?;
        keys.press(key)?;
        self.watch_tx05(game, control, ms)?;
        self.checkpoint_tx05(control, &format!("{label}.held"))?;
        keys.release(key)?;
        self.step("key_up", json!({"label":label,"key":key}))?;
        self.checkpoint_tx05(control, &format!("{label}.release"))?;
        if active {
            self.wait_ticks_tx05(game, control, 8)?;
        } else {
            self.watch_tx05(game, control, 350)?;
        }
        self.checkpoint_tx05(control, &format!("{label}.end"))?;
        Ok(())
    }
    // S0 在任何部署前检查默认键位和原始配置，避免输入脚本在未知映射下改变世界。
    pub(super) fn input_environment_tx05(&mut self) -> Result<()> {
        let expected = [
            ("Left-0", "65"),
            ("Right-0", "68"),
            ("Up-0", "87"),
            ("Down-0", "83"),
            ("ShootLeft-0", "263"),
            ("ShootRight-0", "262"),
            ("ShootUp-0", "265"),
            ("ShootDown-0", "264"),
            ("Bomb-0", "69"),
            ("MenuConfirm-1", "257"),
            ("MenuBack-0", "256"),
        ];
        let mut configurations = vec![];
        for name in ["inputconfigs.dat", "inputconfigs_rgon.dat"] {
            let path = self.save.join(name);
            let content = read(&path)?;
            fs::copy(&path, self.out.join(name)).map_err(|e| e.to_string())?;
            let keyboard = content
                .split("<Controller ")
                .find(|part| part.starts_with("Name=\"Keyboard\""))
                .and_then(|part| part.split_once("/>").map(|p| p.0))
                .ok_or("missing Keyboard configuration")?;
            let attributes = keyboard
                .split_whitespace()
                .filter_map(|part| part.split_once('='))
                .map(|(k, v)| (k, v.trim_matches('"')))
                .collect::<BTreeMap<_, _>>();
            let mut mismatches = expected
                .iter()
                .filter(|(k, v)| attributes.get(k) != Some(v))
                .map(|(k, v)| json!({"key":k,"expected":v,"actual":attributes.get(k)}))
                .collect::<Vec<_>>();
            for action in [
                "Left",
                "Right",
                "Up",
                "Down",
                "ShootLeft",
                "ShootRight",
                "ShootUp",
                "ShootDown",
                "Bomb",
            ] {
                let key = format!("{action}-1");
                if attributes.get(key.as_str()) != Some(&"-1") {
                    mismatches.push(
                        json!({"key":key,"expected":"-1","actual":attributes.get(key.as_str())}),
                    );
                }
            }
            for (key, value) in &attributes {
                if matches!(*value, "297" | "298") {
                    mismatches.push(json!({"key":key,"unexpected_fixture_key":value}));
                }
            }
            configurations.push(json!({"file":name,"mismatches":mismatches}));
        }
        self.report["keyboard_environment"] = json!(configurations);
        self.step("S0_keyboard", self.report["keyboard_environment"].clone())?;
        if configurations
            .iter()
            .any(|v| !v["mismatches"].as_array().unwrap().is_empty())
        {
            return Err("non-default keyboard bindings; no deployment or launch".into());
        }
        let mods = self.report["environment"]["mods"]
            .as_array()
            .ok_or("missing mod inventory")?;
        let unsupported = mods
            .iter()
            .filter(|v| v["enabled"] == true && v["directory"] != "isaac-cs-ui")
            .collect::<Vec<_>>();
        if !unsupported.is_empty() {
            return Err(format!(
                "enabled third-party mods must be disabled before E2E: {unsupported:?}"
            ));
        }
        Ok(())
    }
    // 选档与 New Run 均基于已观测菜单状态，不进入 Online，也不通过 Lua 改写游戏资源。
    pub(super) fn enter_run_tx05(
        &mut self,
        game: &win32::Game,
        w: usize,
        control: &mut Control,
    ) -> Result<()> {
        let timeout = self.scenario["startup_timeout_seconds"]
            .as_u64()
            .unwrap_or(90);
        let initial = control.wait_startup(game, w, timeout)?;
        if initial.get("menu_id").map(String::as_str) == Some("1") {
            self.sequence(w, "to_file_select")?;
        }
        control.wait("menu_id", "2", 15)?;
        for _ in 0..3 {
            let s = control.command("cs status")?;
            let slot: i32 = s
                .get("selected_save")
                .ok_or("missing selected_save")?
                .parse()
                .map_err(|e| format!("invalid slot: {e}"))?;
            if slot == 1 {
                break;
            }
            self.keys(w, &[if slot < 1 { "RIGHT" } else { "LEFT" }.into()])?;
        }
        let selected = control.wait("selected_save", "1", 5)?;
        self.step("S0_selected_second_save", json!(selected))?;
        win32::screenshot(w, &self.out.join("save-slot-2.bmp"))?;
        self.keys(w, &["ENTER".into()])?;
        control.wait("menu_id", "3", 15)?;
        for _ in 0..6 {
            if control
                .command("cs status")?
                .get("menu_game_selection")
                .map(String::as_str)
                == Some("0")
            {
                break;
            }
            self.keys(w, &["UP".into()])?;
        }
        control.wait("menu_game_selection", "0", 5)?;
        self.sequence(w, "open_character_menu")?;
        control.wait("menu_id", "5", 15)?;
        self.sequence(w, "start_run")?;
        control.wait("run_active", "1", 20)?;
        // 等待首份带新 token 的 run 观测；不接受旧日志或开场动画的状态。
        let deadline = Instant::now() + Duration::from_secs(15);
        loop {
            if self
                .live_fixture()?
                .iter()
                .any(|v| v["event"] == "observation")
            {
                break;
            }
            if Instant::now() >= deadline {
                return Err("missing fresh run observation".into());
            }
            self.watch_tx05(game, control, 100)?;
        }
        // 开局约 3 秒内 game_frame 保持 0，游戏尚未开始模拟，此时按键无法验证原生透传（D21）。
        // 要求观测到两个不同的正 game_frame 后才结束 S0；等待期间不发任何键。
        let deadline = Instant::now() + Duration::from_secs(15);
        loop {
            let frames = self
                .live_fixture()?
                .iter()
                .filter(|v| v["event"] == "observation")
                .filter_map(|v| v["game_frame"].as_u64())
                .filter(|&f| f > 0)
                .collect::<std::collections::BTreeSet<_>>();
            if frames.len() >= 2 {
                self.step("S0_simulating", json!({"game_frames":frames}))?;
                break;
            }
            if Instant::now() >= deadline {
                return Err("game simulation did not start after run_active".into());
            }
            self.watch_tx05(game, control, 100)?;
        }
        let observation = self
            .live_fixture()?
            .into_iter()
            .rev()
            .find(|v| v["event"] == "observation")
            .unwrap();
        let player=if self.report["tx"]=="tx-04-07" {&observation["players"][0]}else{&observation};
        if player["player_type"] != 0
            || player["controller"] != 0
            || player["bombs"].as_i64().unwrap_or(0) < 1
        {
            return Err(format!(
                "S0 requires Isaac, keyboard controller 0 and a starting bomb: {observation}"
            ));
        }
        self.checkpoint_tx05(control, "S0.end")?;
        win32::screenshot(w, &self.out.join("S0-run.bmp"))?;
        Ok(())
    }
    fn scenes_tx05(&mut self, game: &win32::Game) -> Result<()> {
        let timeout = self.scenario["startup_timeout_seconds"]
            .as_u64()
            .unwrap_or(90);
        let w = game.wait_window(timeout)?;
        // 窗口刚出现时游戏可能仍在加载；这里只尽力激活并记录结果，首次发键前的 focus 仍为硬性要求（D20）。
        let focused = win32::focus(w);
        self.step(
            "startup_focus",
            json!({"ok":focused.is_ok(),"error":focused.err(),"windows":win32::windows(game.pid())}),
        )?;
        let mut control = Control::connect(
            self.scenario["control_port"].as_u64().unwrap_or(27091) as u16,
            game,
            timeout,
            &self.out.join("control.jsonl"),
        )?;
        self.enter_run_tx05(game, w, &mut control)?;
        let mut keys = win32::HeldKeys::new(w);
        self.hold_tx05(game, &mut control, &mut keys, "S1", "D", 1000, false)?;
        // 反向回到房间内，为后续方向观测保留空间；同属未激活的 S1。
        self.hold_tx05(
            game,
            &mut control,
            &mut keys,
            "S1.recenter",
            "A",
            1000,
            false,
        )?;
        self.checkpoint_tx05(&mut control, "S2.begin")?;
        let queued = control.command("cs solo start")?;
        self.step("S2_start", json!(queued))?;
        control.wait("session_state", "Active", 10)?;
        control.wait("a_bound", "1", 5)?;
        self.wait_ticks_tx05(game, &mut control, 8)?;
        self.checkpoint_tx05(&mut control, "S2.end")?;
        self.hold_tx05(game, &mut control, &mut keys, "S3.right", "D", 1000, true)?;
        self.hold_tx05(game, &mut control, &mut keys, "S3.up", "W", 1000, true)?;
        win32::screenshot(w, &self.out.join("S3-movement.bmp"))?;

        self.checkpoint_tx05(&mut control, "S4.begin")?;
        keys.press("RIGHT")?;
        self.watch_tx05(game, &mut control, 1000)?;
        self.checkpoint_tx05(&mut control, "S4.right")?;
        keys.press("LEFT")?;
        self.watch_tx05(game, &mut control, 500)?;
        self.checkpoint_tx05(&mut control, "S4.left")?;
        keys.release("LEFT")?;
        self.watch_tx05(game, &mut control, 500)?;
        self.checkpoint_tx05(&mut control, "S4.fallback")?;
        keys.release("RIGHT")?;
        self.checkpoint_tx05(&mut control, "S4.release")?;
        self.wait_ticks_tx05(game, &mut control, 8)?;
        self.checkpoint_tx05(&mut control, "S4.end")?;
        win32::screenshot(w, &self.out.join("S4-shooting.bmp"))?;

        self.hold_tx05(game, &mut control, &mut keys, "S5", "E", 120, true)?;
        // 离开自己放置的炸弹，避免后续阻断恢复时站在爆炸范围中。
        self.hold_tx05(game, &mut control, &mut keys, "S5.safety", "S", 650, true)?;
        self.hold_tx05(game, &mut control, &mut keys, "S5.safety-x", "A", 650, true)?;

        self.checkpoint_tx05(&mut control, "S6.begin")?;
        win32::key(w, "CONSOLE")?;
        self.watch_tx05(game, &mut control, 350)?;
        self.checkpoint_tx05(&mut control, "S6.block.begin")?;
        // 不发 ENTER；控制台内的字符不会执行原生命令。E 只用来检查屏蔽边沿不重放。
        keys.press("D")?;
        win32::key(w, "E")?;
        self.watch_tx05(game, &mut control, 1000)?;
        self.checkpoint_tx05(&mut control, "S6.block.end")?;
        keys.release("D")?;
        win32::screenshot(w, &self.out.join("S6-overlay.bmp"))?;
        win32::key(w, "ESC")?;
        self.wait_ticks_tx05(game, &mut control, 8)?;
        self.checkpoint_tx05(&mut control, "S6.end")?;

        self.checkpoint_tx05(&mut control, "S7.begin")?;
        win32::key(w, "ESC")?;
        control.wait_pause(None)?;
        self.checkpoint_tx05(&mut control, "S7.block.begin")?;
        keys.press("D")?;
        self.watch_tx05(game, &mut control, 1000)?;
        self.checkpoint_tx05(&mut control, "S7.block.end")?;
        win32::screenshot(w, &self.out.join("S7-paused.bmp"))?;
        win32::key(w, "ESC")?;
        self.wait_ticks_tx05(game, &mut control, 8)?;
        self.checkpoint_tx05(&mut control, "S7.resumed-held")?;
        keys.release("D")?;
        self.wait_ticks_tx05(game, &mut control, 8)?;
        self.checkpoint_tx05(&mut control, "S7.end")?;

        self.checkpoint_tx05(&mut control, "S8.begin")?;
        win32::key(w, "F8")?;
        self.wait_fixture_tx05(game, &mut control, "skip-update", "end")?;
        self.wait_ticks_tx05(game, &mut control, 8)?;
        self.checkpoint_tx05(&mut control, "S8.end")?;

        self.checkpoint_tx05(&mut control, "S9.begin")?;
        // 第二次 F8 由夹具解释为切房；F9 会触发原版手动截图（D21）。
        win32::key(w, "F8")?;
        self.wait_fixture_tx05(game, &mut control, "room-transition", "arrived")?;
        control.wait("a_bound", "1", 10)?;
        self.wait_ticks_tx05(game, &mut control, 8)?;
        self.checkpoint_tx05(&mut control, "S9.arrived")?;
        // 按实际入房位置向内移动，不依赖门方向被原生采纳。
        let latest = self
            .live_fixture()?
            .into_iter()
            .rev()
            .find(|v| v["event"] == "observation")
            .ok_or("missing post-room observation")?;
        let inward = if latest["x"].as_f64().ok_or("missing x")? > 320.0 {
            "A"
        } else {
            "D"
        };
        self.report["post_room_key"] = json!(inward);
        self.hold_tx05(game, &mut control, &mut keys, "S9.move", inward, 600, true)?;
        win32::screenshot(w, &self.out.join("S9-room.bmp"))?;
        self.checkpoint_tx05(&mut control, "S9.end")?;

        self.checkpoint_tx05(&mut control, "S10.begin")?;
        self.step("S10_stop", json!(control.command("cs solo stop")?))?;
        control.wait("session_state", "Idle", 5)?;
        // 房间布局随机，固定方向可能被障碍挡住；沿 S9.move 刚走过的路径反向移动，路径已知可通行（D22）。
        let back = if inward == "A" { "D" } else { "A" };
        self.report["s10_key"] = json!(back);
        self.hold_tx05(game, &mut control, &mut keys, "S10", back, 1000, false)?;
        self.checkpoint_tx05(&mut control, "S11.begin")?;
        self.step("S11_start", json!(control.command("cs solo start")?))?;
        control.wait("session_state", "Active", 10)?;
        self.wait_ticks_tx05(game, &mut control, 8)?;
        self.checkpoint_tx05(&mut control, "S11.active")?;
        let before = control.command("cs counters")?;
        self.return_to_menu(w, &mut control)?;
        control.wait("session_state", "Idle", 5)?;
        self.checkpoint_tx05(&mut control, "S11.teardown")?;
        self.watch_tx05(game, &mut control, 500)?;
        self.checkpoint_tx05(&mut control, "S11.end")?;
        let counters = control.command("cs counters")?;
        self.report["exit_hook"] = json!({"before":before.get("hook.game_exit"),"after":counters.get("hook.game_exit"),
            "increased": number(&counters,"hook.game_exit")? > number(&before,"hook.game_exit")?});
        write_json(&self.out.join("counters.json"), &json!(counters))?;
        self.report["final_status"] = json!(control.command("cs status")?);
        drop(keys);
        drop(control);
        let code = game.close()?;
        self.report["enabled_exit"] = json!(code);
        if code != 0 {
            return Err(format!("game exit code {code}"));
        }
        Ok(())
    }
    pub(super) fn execute_tx05(&mut self) -> Result<()> {
        self.step("S0", json!({"launches":1,"manual_input":"SkipIntro=1; harness takes over keyboard; stop manual input before launch"}))?;
        self.environment()?;
        self.input_environment_tx05()?;
        self.capture_sources()?;
        self.report["crash_before"] = self.crash_files()?;
        self.snapshot("save_before")?;
        self.deploy()?;
        let game = self.launch("enabled")?;
        self.report["launches"] = json!(1);
        let pid = game.pid();
        // unwind 在 Game 离开作用域前转成错误，先释放 HeldKeys，再收集仍存活的现场。
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| self.scenes_tx05(&game)))
                .unwrap_or_else(|_| Err("tx-04-05 scenario panic".into()));
        if let Err(error) = &result {
            self.capture_failure(&game, "enabled", error);
            let closed = game.close();
            self.report["failure_close"] =
                json!({"exit":closed.as_ref().ok(),"error":closed.as_ref().err()});
        }
        drop(game);
        thread::sleep(Duration::from_millis(300));
        let collection = self.collect(pid, "enabled");
        let snapshot = self.snapshot("save_mid");
        if let Err(e) = collection {
            self.report["collection_error"] = json!(e);
        }
        if let Err(e) = snapshot {
            self.report["save_mid_error"] = json!(e);
        }
        result
    }
    // 无论场景成功与否均落盘完整报告，NOT_RUN 明确区别于已观察失败。
    pub(super) fn finish_tx05(&mut self, result: Result<()>) -> Result<()> {
        self.report["failure"] = json!(result.err());
        if let Err(e) = self.cleanup() {
            self.report["cleanup_error"] = json!(e);
        }
        match self.crash_files() {
            Ok(after) => {
                let changed = after
                    .as_object()
                    .unwrap()
                    .iter()
                    .filter(|(k, v)| self.report["crash_before"][*k] != **v)
                    .map(|(k, v)| json!({"path":k,"state":v}))
                    .collect::<Vec<_>>();
                // 新增转储说明进程已崩溃；控制口 10054 等只是崩溃的后果，原始错误保留在 failure。
                let dumps = changed
                    .iter()
                    .filter_map(|c| c["path"].as_str())
                    .filter(|p| p.to_ascii_lowercase().ends_with(".dmp"))
                    .collect::<Vec<_>>();
                if !dumps.is_empty() {
                    self.report["failure_cause"] = json!({"kind":"game_crash","dumps":dumps});
                }
                self.report["crash_changes"] = json!(changed);
            }
            Err(e) => self.report["crash_scan_error"] = json!(e),
        }
        if self.out.join("save_before/snapshot.json").exists() {
            if let Err(e) = self.snapshot("save_after") {
                self.report["save_backup_error"] = json!(e);
            } else {
                match saves::changes(&self.out.join("save_before"), &self.out.join("save_after")) {
                    Ok(mut changes) => {
                        if let Err(error)=self.apply_recorded_baseline_tx05(&mut changes) {
                            self.report["save_compare_error"]=json!(error);
                        }
                        write_json(&self.out.join("save-changes.json"), &changes)?;
                        self.report["save_changes"] = changes;
                    }
                    Err(e) => self.report["save_compare_error"] = json!(e),
                }
            }
        }
        let tx06 = self.report["tx"] == "tx-04-06";
        let tx07 = self.report["tx"] == "tx-04-07";
        let evaluation = if tx07 {self.evaluate_tx07()}else if tx06 { self.evaluate_tx06() } else { self.evaluate_tx05() };
        if let Err(e) = evaluation {
            self.report["evaluation_error"] = json!(e);
        }
        let cases: Vec<&str> = if tx07 {vec!["V1","V2","V3","V4","V5","V6","V7","V8","V9","V10","V11","V12","V13","V14","V15","V16"]}else if tx06 { vec!["V1","V2","V3","V4","V5","V6","V7","V8","V9","V10","V11","V12"] } else { CASES.to_vec() };
        for &id in &cases {
            if self.report["cases"][id].is_null() {
                self.report["cases"][id] = json!({"pass":false,"status":"NOT_RUN","detail":"missing required evidence; see failure/evaluation_error"});
            }
        }
        let errors = [
            "failure",
            "cleanup_error",
            "collection_error",
            "save_mid_error",
            "save_backup_error",
            "save_compare_error",
            "evaluation_error",
            "file_monitor_error",
            "crash_scan_error",
        ];
        let pass = errors.iter().all(|k| self.report[*k].is_null())
            && cases
                .iter()
                .all(|id| self.report["cases"][id]["pass"] == true);
        self.report["pass"] = json!(pass);
        self.report["automatic_retry"] = json!(false);
        write_json(&self.out.join("report.json"), &self.report)?;
        let mut md = format!(
            "# {} E2E\n\nResult: **{}**\n\nSingle launch; no automatic retry; no save restoration.\n\n",
            self.report["tx"].as_str().unwrap_or("unknown"),
            if pass { "PASS" } else { "FAIL" }
        );
        for &id in &cases {
            md += &format!(
                "- {id}: {}\n",
                if self.report["cases"][id]["pass"] == true {
                    "PASS"
                } else if self.report["cases"][id]["status"] == "NOT_RUN" {
                    "NOT_RUN"
                } else if self.report["cases"][id]["status"] == "NOT_OBSERVED" {
                    "NOT_OBSERVED"
                } else {
                    "FAIL"
                }
            );
        }
        md += &format!("\nFailure: {}\n", self.report["failure"]);
        if !self.report["failure_cause"].is_null() {
            md += &format!("\nFailure cause: {}\n", self.report["failure_cause"]);
        }
        md += "\nArtifacts actually generated:\n\n";
        let mut actual = fs::read_dir(&self.out)
            .map_err(|e| e.to_string())?
            .map(|e| e.map(|v| v.file_name().to_string_lossy().to_string()))
            .collect::<std::result::Result<Vec<_>, _>>()
            .map_err(|e| e.to_string())?;
        actual.sort();
        for name in actual {
            md += &format!("- `{name}`\n");
        }
        fs::write(self.out.join("report.md"), md).map_err(|e| e.to_string())?;
        if pass {
            Ok(())
        } else {
            Err(format!(
                "E2E failed; see {}",
                self.out.join("report.json").display()
            ))
        }
    }
}
