//! tx-04-09统一整体验收：人数、逐身体原生消费、独立退出、多角色与正常收尾。
//! 所有材料位于本次临时目录；成功清理，必要中断才保留恢复信息，不复制源码。
use crate::remote::{Remote, Step, protocol::State};
use crate::{Control, Result, Run, win32, write_json};
use serde_json::json;
use std::{
    collections::{BTreeMap, BTreeSet},
    fs, thread,
    time::{Duration, Instant},
};
type Status = BTreeMap<String, String>;
fn number(s: &Status, key: &str) -> Result<u64> {
    s.get(key)
        .ok_or_else(|| format!("missing {key}"))?
        .parse()
        .map_err(|e| format!("{key}: {e}"))
}
fn field(i: u32, name: &str) -> String {
    format!("participant.{i}.{name}")
}
impl Run {
    // 统一等待真实主线程状态；游戏错误与夹具错误立即报告，不用固定睡眠冒充到达。
    fn wait_tx09(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        label: &str,
        predicate: impl Fn(&Status) -> bool,
    ) -> Result<Status> {
        let end = Instant::now() + Duration::from_secs(40);
        loop {
            let status = self.watch_tx05(g, c, 0)?;
            if self
                .live_fixture()?
                .iter()
                .any(|v| v["event"] == "fixture_error")
            {
                return Err("tx09_fixture_error".into());
            }
            if predicate(&status) {
                self.step(label, json!(status))?;
                return Ok(status);
            }
            if Instant::now() >= end {
                return Err(format!(
                    "{label}: timeout; participants={:?}, blocks={:?}, completed_tick={:?}; full state in control.jsonl",
                    status.get("participants"),
                    status.get("blocks"),
                    status.get("completed_tick")
                ));
            }
            thread::sleep(Duration::from_millis(40));
        }
    }
    fn action_tx09(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        w: usize,
        index: u64,
        name: &str,
    ) -> Result<()> {
        self.step("fixture_action", json!({"index":index,"name":name}))?;
        win32::key(w, "F8")?;
        let end = Instant::now() + Duration::from_secs(40);
        loop {
            self.watch_tx05(g, c, 0)?;
            let rows = self.live_fixture()?;
            if rows.iter().any(|v| v["event"] == "fixture_error") {
                return Err("tx09_fixture_error".into());
            }
            if rows
                .iter()
                .any(|v| v["event"] == "action_return" && v["action"] == index && v["name"] == name)
            {
                return Ok(());
            }
            if Instant::now() >= end {
                return Err(format!("fixture action {index} {name} timeout"));
            }
            thread::sleep(Duration::from_millis(40));
        }
    }
    fn attach_tx09(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        round: u32,
        ordinal: u32,
    ) -> Result<Remote> {
        let host = self.scenario["host_port"].as_u64().ok_or("host_port")?;
        let remote = Remote::start_participant(
            format!("127.0.0.1:{host}")
                .parse()
                .map_err(|e| format!("{e}"))?,
            &self.out.join(format!("remote-{round}-{ordinal}.jsonl")),
            3000,
            2,
            ordinal,
            |_| Ok(()),
        )?;
        self.wait_tx09(g, c, "participant_join", |s| {
            s.get(&field(ordinal, "bound")).is_some_and(|v| v == "1")
                && remote.snapshot().ack.is_some_and(|a| {
                    a.generation > 0 && a.state == 3 && a.client == u64::from(ordinal) + 1
                })
        })?;
        if remote.snapshot().establishments != 1 {
            return Err("remote_establishment_count".into());
        }
        Ok(remote)
    }
    fn start_host_tx09(&mut self, g: &win32::Game, c: &mut Control, character: u32) -> Result<()> {
        c.command(&format!(
            "cs host {} {character}",
            self.scenario["host_port"]
        ))?;
        self.wait_tx09(g, c, "host_without_remote", |s| {
            s.get("session_state").is_some_and(|v| v == "Active")
                && s.get("participants").is_some_and(|v| v == "1")
        })?;
        Ok(())
    }
    // 每个远端仅提交一条控制组脚本；原生身体各自读取同一意图，四种边沿各一次。
    fn play_tx09(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        w: usize,
        remotes: &[(u32, Remote)],
        label: &str,
        edges: bool,
    ) -> Result<()> {
        self.wait_tx09(g, c, "input_sources_same_generation", |s| {
            s.get("blocks").is_some_and(|v| v == "0")
                && remotes.iter().all(|(i, remote)| {
                    s.get(&field(*i, "bound")).is_some_and(|v| v == "1")
                        && remote.snapshot().ack.is_some_and(|a| {
                            a.state == 3
                                && a.generation == number(s, &field(*i, "generation")).unwrap_or(0)
                        })
                })
        })?;
        let before = c.command("cs status")?;
        let first = self
            .live_fixture()?
            .last()
            .and_then(|v| v["serial"].as_u64())
            .unwrap_or(0);
        for (ordinal, remote) in remotes {
            let movement = match ordinal {
                1 => [0, -32767],
                2 => [-32767, 0],
                _ => [0, 32767],
            };
            let aim = match ordinal {
                1 => [-32767, 0],
                2 => [32767, 0],
                _ => [0, 32767],
            };
            let mut script = vec![Step {
                after: 0,
                state: State {
                    movement,
                    aim,
                    held: 1,
                    edge: 0,
                },
            }];
            if edges {
                for (at, edge, held) in [(12, 1, 0), (20, 2, 2), (28, 4, 4), (36, 8, 8)] {
                    script.push(Step {
                        after: at,
                        state: State {
                            movement,
                            aim,
                            held: 1 | held,
                            edge,
                        },
                    });
                    script.push(Step {
                        after: at + 1,
                        state: State {
                            movement,
                            aim,
                            held: 1,
                            edge: 0,
                        },
                    });
                }
            }
            script.push(Step {
                after: 55,
                state: State::default(),
            });
            remote.script(label, script)?;
        }
        let mut held = win32::HeldKeys::new(w);
        held.press("D")?;
        held.press("UP")?;
        let tick = number(&before, "completed_tick")?;
        let end = Instant::now() + Duration::from_secs(30);
        let mut next = 0usize;
        loop {
            let s = self.watch_tx05(g, c, 0)?;
            for (_, remote) in remotes {
                if let Some(e) = remote.snapshot().error {
                    return Err(format!("remote:{e}"));
                }
            }
            let elapsed = number(&s, "completed_tick")?.saturating_sub(tick);
            if edges && next < 4 && elapsed >= [12, 20, 28, 36][next] {
                win32::key(w, ["E", "SPACE", "Q", "CTRL"][next])?;
                next += 1;
            }
            if elapsed >= 65 {
                break;
            }
            if Instant::now() >= end {
                return Err("tx09_input_window_timeout".into());
            }
            thread::sleep(Duration::from_millis(15));
        }
        drop(held);
        let after = c.command("cs status")?;
        let rows = self
            .live_fixture()?
            .into_iter()
            .filter(|v| v["serial"].as_u64().unwrap_or(0) > first && v["event"] == "observation")
            .collect::<Vec<_>>();
        let mut expected = BTreeSet::new();
        for i in std::iter::once(0).chain(remotes.iter().map(|(i, _)| *i)) {
            let count = number(&after, &field(i, "bodies"))?;
            for b in 0..count {
                let prefix = field(i, &format!("body.{b}."));
                if number(&after, &(prefix.clone() + "active"))? == 0 {
                    continue;
                }
                let address = number(&after, &(prefix.clone() + "address"))?;
                let id = number(&after, &(prefix + "entity_id"))?;
                if self.scenario["production"] == true {
                    if id != 0 {
                        return Err("production_recording_identity_leaked".into());
                    }
                } else if id == 0 || !expected.insert(id) {
                    return Err("missing_or_duplicate_entity_observation_identity".into());
                }
                // 每个身体分别有原生运动或射击结果；只看到ACK或查询次数不算到达。
                let character = after
                    .get(&field(i, &format!("body.{b}.character")))
                    .and_then(|v| v.parse::<i64>().ok())
                    .ok_or("body_character")?;
                let native_index = after
                    .get(&field(i, &format!("body.{b}.native_index")))
                    .and_then(|v| v.parse::<i64>().ok())
                    .ok_or("body_native_index")?;
                let views = rows
                    .iter()
                    .filter_map(|v| v["players"].as_array())
                    .flatten()
                    .filter(|p| {
                        p["ordinal"] == i
                            && p["player_type"] == character
                            && p["index"] == native_index
                    })
                    .collect::<Vec<_>>();
                let direction = match i {
                    0 => [1.0, 0.0],
                    1 => [0.0, -1.0],
                    2 => [-1.0, 0.0],
                    _ => [0.0, 1.0],
                };
                let moved = views.iter().any(|p| {
                    p["velocity"].as_array().is_some_and(|a| {
                        a.len() == 2
                            && a[0].as_f64().unwrap_or(0.0) * direction[0]
                                + a[1].as_f64().unwrap_or(0.0) * direction[1]
                                > 0.01
                    })
                });
                let hologram = views.iter().any(|p| p["hologram"] == true);
                let shot = views.iter().any(|p| {
                    p["shots"].as_u64().unwrap_or(0) > 0
                        || p["fire_direction"].as_i64().is_some_and(|d| d >= 0)
                });
                // T.Forgotten本体由Soul携带/投掷，Soul本身不发射；hologram动作由原生提前返回。
                let movement_required = character != 35;
                let attack_required = character != 40 && !hologram;
                if (movement_required && !moved) || (attack_required && !shot) {
                    return Err(format!(
                        "body did not consume input: ordinal={i} character={character} address={address} moved={moved} shot={shot}"
                    ));
                }
            }
            if edges {
                for action in 0..4 {
                    let key = field(i, &format!("edge.{action}.processed_count"));
                    if number(&after, &key)? != number(&before, &key)? + 1 {
                        return Err(format!("group edge not processed exactly once: {key}"));
                    }
                }
            }
        }
        if label == "four_double_bodies_input" {
            let mut signatures = vec![BTreeMap::<String, u64>::new(); 4];
            for event in self
                .live_fixture()?
                .iter()
                .filter(|v| v["event"] == "bomb_spawn" && v["serial"].as_u64().unwrap_or(0) > first)
            {
                let ordinal = event["ordinal"]
                    .as_u64()
                    .filter(|v| *v < 4)
                    .ok_or("canonical_bomb_spawner_unknown")?
                    as usize;
                *signatures[ordinal]
                    .entry(event["player_type"].to_string())
                    .or_default() += 1;
            }
            for (ordinal, signature) in signatures.iter().enumerate() {
                if json!(signature) != self.report["native_double_body_bombs"] {
                    return Err(format!(
                        "bomb side effects differ from native reference: ordinal={ordinal} actual={signature:?}"
                    ));
                }
            }
            self.report["canonical_double_body_bombs"] = json!(signatures);
        }
        self.case(
            label,
            true,
            json!({"before":before,"after":after,"observations":rows.len(),"entity_ids":expected}),
        );
        Ok(())
    }
    fn scenes_tx09(&mut self, g: &win32::Game) -> Result<()> {
        let w = g.wait_window(90)?;
        let mut c = Control::connect(
            self.scenario["control_port"]
                .as_u64()
                .ok_or("control_port")? as u16,
            g,
            90,
            &self.out.join("control.jsonl"),
        )?;
        c.focus_window = Some(w);
        self.prepare_new_run_tx05(g, w, &mut c)?;
        self.sequence(w, "start_run")?;
        c.wait("run_active", "1", 25)?;
        let end = Instant::now() + Duration::from_secs(20);
        while !self
            .live_fixture()?
            .iter()
            .any(|v| v["event"] == "observation" && v["game_frame"].as_u64().unwrap_or(0) > 5)
        {
            self.watch_tx05(g, &mut c, 100)?;
            if Instant::now() >= end {
                return Err("run_simulation_not_started".into());
            }
        }
        let production = self.scenario["production"] == true;
        if production {
            let config = c.command("cs status")?;
            let counters = c.command("cs counters")?;
            if number(&config, "diagnostics_enabled")? != 0
                || number(&config, "world_record_enabled")? != 0
                || number(&counters, "counters_available")? != 0
            {
                return Err("production_diagnostics_not_disabled".into());
            }
        } else {
            c.command("cs world start four-participant")?;
            // world status为独立命名空间，必须读取cs world status，不能接受上一录制的终态。
            let end = Instant::now() + Duration::from_secs(30);
            loop {
                if c.command("cs world status")?
                    .get("world_state")
                    .is_some_and(|v| v == "Recording")
                {
                    break;
                }
                if Instant::now() >= end {
                    return Err("world_recording_timeout".into());
                }
                thread::sleep(Duration::from_millis(40));
            }
        }
        self.action_tx09(g, &mut c, w, 1, "seed")?;
        self.wait_tx09(g, &mut c, "seed_run_stable", |s| {
            s.get("run_active").is_some_and(|v| v == "1")
                && s.get("pause_state").is_some_and(|v| v == "0")
        })?;
        self.start_host_tx09(g, &mut c, 0)?;
        let mut remotes = Vec::new();
        for i in 1..=3 {
            remotes.push((i, self.attach_tx09(g, &mut c, 1, i)?));
            let s = c.command("cs status")?;
            if number(&s, "participants")? != u64::from(i) + 1 {
                return Err("participant_count".into());
            }
        }
        // 满员后第二来源不能占用既有ordinal，也不能偷换原端点nonce。
        let fifth = Remote::start_participant(
            format!("127.0.0.1:{}", self.scenario["host_port"])
                .parse()
                .map_err(|e| format!("{e}"))?,
            &self.out.join("fifth.jsonl"),
            1000,
            2,
            1,
            |_| Ok(()),
        )?;
        self.watch_tx05(g, &mut c, 1200)?;
        if fifth.snapshot().establishments != 0 {
            return Err("fifth_join_accepted".into());
        }
        drop(fifth);
        self.action_tx09(g, &mut c, w, 2, "resources")?;
        self.play_tx09(g, &mut c, w, &remotes, "ordinary", true)?;
        let before = c.command("cs status")?;
        win32::key(w, "ESC")?;
        self.watch_tx05(g, &mut c, 500)?;
        let paused = c.command("cs status")?;
        let paused_tick = number(&paused, "completed_tick")?;
        self.watch_tx05(g, &mut c, 500)?;
        if number(&c.command("cs status")?, "completed_tick")? != paused_tick {
            return Err("tick_advanced_while_paused".into());
        }
        win32::key(w, "ESC")?;
        self.wait_tx09(g, &mut c, "pause_resume", |s| {
            s.get("blocks").is_some_and(|v| v == "0")
                && number(s, "completed_tick").unwrap_or(0)
                    > number(&before, "completed_tick").unwrap_or(u64::MAX)
        })?;
        self.console(w, "tx09-console", &mut c)?;
        self.action_tx09(g, &mut c, w, 3, "room")?;
        self.wait_tx09(g, &mut c, "room_rebind", |s| {
            (0..=3).all(|i| s.get(&field(i, "bound")).is_some_and(|v| v == "1"))
        })?;
        self.action_tx09(g, &mut c, w, 4, "kill3")?;
        self.wait_tx09(g, &mut c, "remote3_dead", |s| {
            s.get(&field(3, "bound")).is_some_and(|v| v == "0")
                && s.get(&field(3, "body.0.ghost")).is_some_and(|v| v == "1")
        })?;
        self.action_tx09(g, &mut c, w, 5, "revive3")?;
        self.wait_tx09(g, &mut c, "remote3_revived", |s| {
            s.get(&field(3, "bound")).is_some_and(|v| v == "1")
        })?;
        self.action_tx09(g, &mut c, w, 6, "stage")?;
        self.wait_tx09(g, &mut c, "stage_rebind", |s| {
            s.get("blocks").is_some_and(|v| v == "0")
                && (0..=3).all(|i| s.get(&field(i, "bound")).is_some_and(|v| v == "1"))
        })?;
        // 缺输入/连接超时与主动Bye分别退出，其他Peer的身份和Authority继续。
        remotes[0].1.silence()?;
        self.wait_tx09(g, &mut c, "timeout_isolated", |s| {
            s.get(&field(1, "retired")).is_some_and(|v| v == "1")
                && s.get("participants").is_some_and(|v| v == "3")
        })?;
        remotes[1].1.bye()?;
        self.wait_tx09(g, &mut c, "bye_isolated", |s| {
            s.get(&field(2, "retired")).is_some_and(|v| v == "1")
                && s.get(&field(3, "bound")).is_some_and(|v| v == "1")
                && s.get("participants").is_some_and(|v| v == "2")
        })?;
        c.command("cs stop")?;
        c.wait("session_state", "Idle", 20)?;
        drop(remotes);
        self.action_tx09(g, &mut c, w, 7, "host_jacob")?;
        self.action_tx09(g, &mut c, w, 8, "resources")?;
        self.watch_tx05(g, &mut c, 350)?;
        let first = self
            .live_fixture()?
            .last()
            .and_then(|v| v["serial"].as_u64())
            .ok_or("native reference observation")?;
        win32::key(w, "E")?;
        self.watch_tx05(g, &mut c, 800)?;
        let mut native_bombs = BTreeMap::<String, u64>::new();
        for row in self
            .live_fixture()?
            .iter()
            .filter(|v| v["event"] == "bomb_spawn" && v["serial"].as_u64().unwrap_or(0) > first)
        {
            if row["ordinal"] != 0 {
                return Err("native_reference_bomb_spawner_unknown".into());
            }
            *native_bombs
                .entry(row["player_type"].to_string())
                .or_default() += 1;
        }
        if native_bombs.is_empty() {
            return Err("native_reference_did_not_place_bomb".into());
        }
        self.report["native_double_body_bombs"] = json!(native_bombs);
        self.start_host_tx09(g, &mut c, 19)?;
        c.command("cs role 1 0")?;
        let mut twins = vec![(1, self.attach_tx09(g, &mut c, 2, 1)?)];
        self.wait_tx09(g, &mut c, "mixed_roster", |s| {
            number(s, "participants").unwrap_or(0) == 2
                && number(s, "active_bodies").unwrap_or(0) == 3
        })?;
        self.play_tx09(g, &mut c, w, &twins, "mixed_jacob_ordinary", false)?;
        self.action_tx09(g, &mut c, w, 9, "remote1_jacob")?;
        for i in 2..=3 {
            twins.push((i, self.attach_tx09(g, &mut c, 2, i)?));
        }
        self.wait_tx09(g, &mut c, "four_double_bodies", |s| {
            s.get("participants").is_some_and(|v| v == "4")
                && number(s, "active_bodies").unwrap_or(0) >= 8
        })?;
        self.action_tx09(g, &mut c, w, 10, "resources")?;
        self.play_tx09(g, &mut c, w, &twins, "four_double_bodies_input", true)?;
        self.action_tx09(g, &mut c, w, 11, "forms")?;
        self.wait_tx09(g, &mut c, "forms_rebind", |s| {
            s.get("blocks").is_some_and(|v| v == "0")
                && (0..=3).all(|i| s.get(&field(i, "bound")).is_some_and(|v| v == "1"))
        })?;
        self.play_tx09(g, &mut c, w, &twins, "forgotten_and_lazarus", false)?;
        let forgotten = c.command("cs status")?;
        win32::key(w, "CTRL")?;
        let soul = self.wait_tx09(g, &mut c, "forgotten_to_soul", |s| {
            s.get(&field(0, "body.0.character"))
                .is_some_and(|v| v == "17")
                && s.get(&field(0, "bound")).is_some_and(|v| v == "1")
        })?;
        if soul.get(&field(0, "body.0.address")) != forgotten.get(&field(0, "body.0.address")) {
            return Err("forgotten_swap_changed_control_root".into());
        }
        self.watch_tx05(g, &mut c, 400)?;
        win32::key(w, "CTRL")?;
        self.wait_tx09(g, &mut c, "soul_to_forgotten", |s| {
            s.get(&field(0, "body.0.character"))
                .is_some_and(|v| v == "16")
                && s.get(&field(0, "bound")).is_some_and(|v| v == "1")
        })?;
        let before_flip = c.command("cs status")?;
        self.action_tx09(g, &mut c, w, 12, "flip")?;
        self.wait_tx09(g, &mut c, "flip_rebind", |s| {
            s.get(&field(2, "bound")).is_some_and(|v| v == "1")
                && number(s, &field(2, "generation")).unwrap_or(0)
                    > number(&before_flip, &field(2, "generation")).unwrap_or(u64::MAX)
        })?;
        let old_esau = c.command("cs status")?;
        self.action_tx09(g, &mut c, w, 13, "esau_jr")?;
        twins[2].1.script(
            "esau_jr_first",
            vec![
                Step {
                    after: 0,
                    state: State {
                        held: 2,
                        edge: 2,
                        ..State::default()
                    },
                },
                Step {
                    after: 1,
                    state: State::default(),
                },
            ],
        )?;
        let switched_esau = self.wait_tx09(g, &mut c, "esau_jr_rebind", |s| {
            s.get(&field(3, "bound")).is_some_and(|v| v == "1")
                && s.get(&field(3, "body.0.address")) != old_esau.get(&field(3, "body.0.address"))
                && number(s, &field(3, "generation")).unwrap_or(0)
                    > number(&old_esau, &field(3, "generation")).unwrap_or(u64::MAX)
        })?;
        let backup_count = number(&switched_esau, &field(3, "bodies"))?;
        if !(0..backup_count).any(|b| {
            switched_esau
                .get(&field(3, &format!("body.{b}.role")))
                .is_some_and(|v| v == "5")
                && switched_esau
                    .get(&field(3, &format!("body.{b}.active")))
                    .is_some_and(|v| v == "0")
        }) {
            return Err("esau_jr_inactive_backup_missing".into());
        }
        let original_esau = old_esau.get(&field(3, "body.0.address")).cloned();
        self.play_tx09(g, &mut c, w, &twins, "flipped_and_backup", false)?;
        let old_esau = c.command("cs status")?;
        self.action_tx09(g, &mut c, w, 14, "esau_jr")?;
        twins[2].1.script(
            "esau_jr_return",
            vec![
                Step {
                    after: 0,
                    state: State {
                        held: 2,
                        edge: 2,
                        ..State::default()
                    },
                },
                Step {
                    after: 1,
                    state: State::default(),
                },
            ],
        )?;
        self.wait_tx09(g, &mut c, "esau_jr_return", |s| {
            s.get(&field(3, "bound")).is_some_and(|v| v == "1")
                && s.get(&field(3, "body.0.address")) == original_esau.as_ref()
                && number(s, &field(3, "generation")).unwrap_or(0)
                    > number(&old_esau, &field(3, "generation")).unwrap_or(u64::MAX)
        })?;
        // 带Esau Jr备份的Peer也必须独立退出；原生全局备份槽不冒充参与者或输入身体。
        twins[2].1.bye()?;
        self.wait_tx09(g, &mut c, "esau_jr_peer_retired", |s| {
            s.get(&field(3, "retired")).is_some_and(|v| v == "1")
                && number(s, "participants").unwrap_or(0) == 3
                && (0..=2).all(|i| s.get(&field(i, "bound")).is_some_and(|v| v == "1"))
        })?;
        // 关闭录制之后仍检验Host死亡/复活，输入生命周期不能依赖录制器存在。
        if !production {
            c.command("cs world stop")?;
            let end = Instant::now() + Duration::from_secs(30);
            loop {
                if c.command("cs world status")?
                    .get("world_state")
                    .is_some_and(|v| v == "Closed")
                {
                    break;
                }
                if Instant::now() >= end {
                    return Err("world_close_timeout".into());
                }
                thread::sleep(Duration::from_millis(40));
            }
        }
        let before_host_death = c.command("cs status")?;
        self.action_tx09(g, &mut c, w, 15, "kill0")?;
        self.wait_tx09(g, &mut c, "host_death_does_not_stop_authority", |s| {
            s.get(&field(0, "bound")).is_some_and(|v| v == "0")
                && s.get(&field(0, "body.0.ghost")).is_some_and(|v| v == "1")
                && number(s, "completed_tick").unwrap_or(0)
                    > number(&before_host_death, "completed_tick").unwrap_or(u64::MAX) + 5
                && (1..=2).all(|i| s.get(&field(i, "bound")).is_some_and(|v| v == "1"))
        })?;
        self.action_tx09(g, &mut c, w, 16, "revive0")?;
        self.wait_tx09(g, &mut c, "host_revived", |s| {
            s.get(&field(0, "bound")).is_some_and(|v| v == "1")
        })?;
        // 先验证Birthright/Flip双身体独立退休，再停止剩余T.Forgotten双身体。
        twins[1].1.bye()?;
        self.wait_tx09(g, &mut c, "lazarus_twins_retired", |s| {
            s.get(&field(2, "retired")).is_some_and(|v| v == "1")
                && number(s, "participants").unwrap_or(0) == 2
                && (0..=1).all(|i| s.get(&field(i, "bound")).is_some_and(|v| v == "1"))
        })?;
        c.command("cs stop")?;
        self.wait_tx09(g, &mut c, "multi_body_stop", |s| {
            s.get("session_state").is_some_and(|v| v == "Idle")
                && number(s, "players").unwrap_or(0) == 1
                && number(s, "cleanup_failed").unwrap_or(1) == 0
        })?;
        // 录制关闭后再建立四参与者/七活动身体，记录正常输入路径的帧节奏并保留Active菜单退出覆盖。
        self.start_host_tx09(g, &mut c, 19)?;
        let mut closing_remotes = Vec::new();
        for i in 1..=3 {
            closing_remotes.push(self.attach_tx09(g, &mut c, 3, i)?);
        }
        let perf_before = c.command("cs status")?;
        let perf_clock = Instant::now();
        let perf_after = self.watch_tx05(g, &mut c, 1500)?;
        self.report["recording_off_frame_rate"] = json!({
            "frames":number(&perf_after,"frame")?.saturating_sub(number(&perf_before,"frame")?),
            "seconds":perf_clock.elapsed().as_secs_f64(),
            "participants":number(&perf_after,"participants")?,
            "active_bodies":number(&perf_after,"active_bodies")?,
            "lifecycle_full_reads":number(&perf_after,"lifecycle_full_reads")?,
            "lifecycle_snapshot_reuses":number(&perf_after,"lifecycle_snapshot_reuses")?
        });
        if number(&perf_after, "repeat_topology_reads")? != 0
            || number(&perf_after, "lifecycle_snapshot_reuses")? == 0
        {
            return Err("production_topology_reuse_not_exercised".into());
        }
        println!(
            "recording_off_frame_rate {}",
            self.report["recording_off_frame_rate"]
        );
        self.return_to_menu(w, &mut c)?;
        c.wait("session_state", "Idle", 20)?;
        let final_status = c.command("cs status")?;
        if number(&final_status, "trace_dropped")? != 0
            || number(&final_status, "query_mismatches")? != 0
            || number(&final_status, "cleanup_failed")? != 0
        {
            return Err("tx09_final_runtime_diagnostic".into());
        }
        self.report["final_status"] = json!(final_status);
        drop(closing_remotes);
        drop(twins);
        drop(c);
        let code = g.close()?;
        if code != 0 {
            return Err(format!("game_exit_{code}"));
        }
        Ok(())
    }
    pub(super) fn execute_tx09(&mut self) -> Result<()> {
        if self.report["tx"]=="tx-04-12" { self.prepare_resume_tx12()?; }
        if self.report["tx"]=="tx-04-13" { self.prepare_resume_tx13()?; }
        if self.report["tx"]=="tx-04-14" { self.prepare_resume_tx14()?; }
        self.environment()?;
        self.report["crash_before"] = self.crash_files()?;
        self.snapshot("save_before")?;
        self.prepare_restore_tx08(1)?;
        self.disable_helper_tx08()?;
        if self.report["helper_marker_owned"] == true {
            if let Some(mods) = self.report["environment"]["mods"].as_array_mut() {
                for item in mods {
                    if item["directory"] == "repentogonreleasesbackup_3643104060" {
                        item["enabled"] = json!(false);
                    }
                }
            }
        }
        self.input_environment_tx05()?;
        self.deploy()?;
        let roots = self.saves.as_ref().ok_or("save roots")?;
        let mut watched = vec![roots.documents.clone()];
        watched.extend(roots.steam_remote.iter().map(|(_, p)| p.clone()));
        let monitor =
            crate::tx07::files::Monitor::start(watched, self.out.join("save-notifications.jsonl"))?;
        let game = self.launch("enabled")?;
        self.report["pid"] = json!(game.pid());
        let result =
            std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| if self.report["tx"]=="tx-04-15" { self.scenes_tx15(&game) } else if self.report["tx"]=="tx-04-14" { self.scenes_tx14(&game) } else if self.report["tx"]=="tx-04-13" { self.scenes_tx13(&game) } else if self.report["tx"]=="tx-04-12" { self.scenes_tx12(&game) } else if self.report["tx"]=="tx-04-11" { self.scenes_tx11(&game) } else if self.report["tx"]=="tx-04-10" { self.scenes_tx10(&game) } else { self.scenes_tx09(&game) }))
                .unwrap_or_else(|_| Err("tx09 scenario panic".into()));
        if let Err(e) = &result {
            self.capture_failure(&game, "enabled", e);
            let _ = game.close();
        }
        self.collect(game.pid(), "enabled")?;
        monitor.finish()?;
        if self.scenario["production"] == true {
            // 生产配置确实没有逐tick/query/持久入口明细，也没有创建世界录制文件。
            let trace = crate::read(&self.out.join("enabled/trace.jsonl"))?;
            for line in trace.lines() {
                let row: serde_json::Value =
                    serde_json::from_str(line).map_err(|e| e.to_string())?;
                if [
                    "tick",
                    "tick_phase",
                    "submit",
                    "shooting_dispatch",
                    "persistent",
                    "counters",
                    "query_repeat",
                ]
                .contains(&row["ev"].as_str().unwrap_or(""))
                {
                    return Err("production_verbose_trace_leaked".into());
                }
            }
            let world = self.out.join("runtime-world");
            if world.exists()
                && fs::read_dir(world)
                    .map_err(|e| e.to_string())?
                    .next()
                    .is_some()
            {
                return Err("production_world_files_leaked".into());
            }
        }
        result
    }
    pub(super) fn finish_tx09(&mut self, result: Result<()>) -> Result<()> {
        self.report["failure"] = json!(result.as_ref().err());
        let cleanup = self.cleanup();
        let restore = self.restore_environment_tx08();
        // tx16虚拟机模式的options独立恢复，即使部署恢复失败也继续还原原选项。
        let restore = if self.report["tx"]=="tx-04-16" {
            let background=self.restore_background_options_tx16();
            match (restore,background) {
                (Ok(()),Ok(()))=>Ok(()),
                (Err(a),Err(b))=>Err(format!("{a}; {b}")),
                (Err(e),_)|(_,Err(e))=>Err(e),
            }
        } else {restore};
        let helper = self.restore_helper_tx08();
        self.report["cleanup_errors"] = json!([
            cleanup.as_ref().err(),
            restore.as_ref().err(),
            helper.as_ref().err()
        ]);
        let evidence = (|| -> Result<()> {
            if self.saves.is_some() && self.out.join("save_before").exists() {
                self.snapshot("save_after")?;
                let mut changes = crate::saves::changes(
                    &self.out.join("save_before"),
                    &self.out.join("save_after"),
                )?;
                if self.report["tx"]=="tx-04-10" || self.report["tx"]=="tx-04-11" || self.report["tx"]=="tx-04-12" || self.report["tx"]=="tx-04-13" || self.report["tx"]=="tx-04-14" || self.report["tx"]=="tx-04-15" || self.report["tx"]=="tx-04-16" || self.report["tx"]=="tx-04-17" { self.save_fields_tx10(&mut changes)?; }
                else { self.apply_recorded_baseline_tx05(&mut changes)?; }
                let allowed = changes
                    .as_array()
                    .ok_or("save changes")?
                    .iter()
                    .all(|v| v["allowed"] == true);
                self.report["save_changes"] = changes;
                if !allowed {
                    return Err("save_changes_outside_D18_D20_D27".into());
                }
            }
            let crashes = self.crash_files()?;
            if crashes.as_object().is_some_and(|items| {
                items.iter().any(|(name, state)| {
                    name.to_ascii_lowercase().ends_with(".dmp")
                        && self.report["crash_before"][name] != *state
                })
            }) {
                self.report["crash_after"] = crashes;
                return Err("new_native_crash_dump".into());
            }
            Ok(())
        })();
        self.report["evidence_error"] = json!(evidence.as_ref().err());
        let pass = result.is_ok()
            && cleanup.is_ok()
            && restore.is_ok()
            && helper.is_ok()
            && evidence.is_ok();
        self.report["pass"] = json!(pass);
        write_json(&self.out.join("result.json"), &self.report)?;
        println!(
            "{} pass={pass} cases={} failure={}",
            self.report["tx"],
            self.report["cases"].as_object().map_or(0, |v| v.len()),
            self.report["failure"]
        );
        if pass {
            if self.report["tx"]=="tx-04-16" && self.scenario["node_role"]!="Local" { return Ok(()); }
            // 临时目录只由本次进程创建，先核定路径再删除；日志与备份没有永久artifact要求。
            let resolved = self.out.canonicalize().map_err(|e| e.to_string())?;
            // Windows canonicalize带\\?\前缀；父目录也须同样规范化后比较，不能混用普通路径。
            let intended = self
                .repo
                .join("out")
                .canonicalize()
                .map_err(|e| e.to_string())?;
            if resolved == intended || !resolved.starts_with(&intended) {
                return Err("temporary_cleanup_path_outside_repo_out".into());
            }
            // steps仍打开时Windows不允许递归删除；关闭文件由统一入口随后处理。
            self.report["remove_temporary_output"] = json!(true);
            if let Some(pid) = self.report["pid"].as_u64() {
                let boot = self.trace_file(pid as u32);
                if boot.exists() {
                    fs::remove_file(boot).map_err(|e| e.to_string())?;
                }
            }
            Ok(())
        } else {
            Err(format!(
                "{} E2E未完成，继续诊断材料：{}",
                self.report["tx"], self.out.display()
            ))
        }
    }
}
