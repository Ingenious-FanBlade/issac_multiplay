//! tx-04-08 单一整体验收编排：两次顺序启动、真实 world 文件、生产 CLI 和一次最终归约。
//! 失败方式先列于设计 F1–F14；这里不重启、不重发动作、不自动配对未知 tick/出生。
mod evidence;
mod episodes;
mod offline;
use crate::remote::{Remote, Step, protocol::State};
use crate::{Control, Result, Run, mkdir, read, saves, win32, write_json};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    fs,
    io::Write,
    path::{Path, PathBuf},
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
fn remote_view(r: &Remote) -> Value {
    let s = r.snapshot();
    json!({"tx":s.tx,"rx":s.rx,"error":s.error,"script_label":s.script_label,"script_base":s.script_base,
        "script_generation":s.script_generation,"ack":s.ack.map(|a|json!({"tick":a.tick,"accepted":a.accepted,
            "processed":a.processed,"source":a.source,"generation":a.generation,"state":a.state,"rejected":a.rejected}))})
}
// JSON 只作为数据转为 Lua literal，拒绝非有限数和非字符串键；不能注入 Lua 源码。
fn lua(value: &Value) -> Result<String> {
    Ok(match value {
        Value::Null => "nil".into(),
        Value::Bool(v) => v.to_string(),
        Value::Number(v) => v.to_string(),
        Value::String(s) => {
            let mut out = "\"".to_string();
            for b in s.bytes() {
                match b {
                    b'"' => out.push_str("\\\""),
                    b'\\' => out.push_str("\\\\"),
                    32..=126 => out.push(b as char),
                    _ => out.push_str(&format!("\\{b:03}")),
                }
            }
            out + "\""
        }
        Value::Array(a) => format!(
            "{{{}}}",
            a.iter().map(lua).collect::<Result<Vec<_>>>()?.join(",")
        ),
        Value::Object(o) => format!(
            "{{{}}}",
            o.iter()
                .map(|(k, v)| Ok(format!("[{}]={}", lua(&json!(k))?, lua(v)?)))
                .collect::<Result<Vec<_>>>()?
                .join(",")
        ),
    })
}
impl Run {
    pub(super) fn fixture_config_tx08(&self, process: u64) -> Result<String> {
        let actions = self.scenario[if process == 1 {
            "actions"
        } else {
            "comparison_actions"
        }]
        .as_array()
        .ok_or("tx08_actions")?
        .iter()
        .filter(|a| a["enabled"] != false)
        .map(|a| {
            let mut a = a.clone();
            a.as_object_mut().ok_or("action object")?.remove("enabled");
            Ok(a)
        })
        .collect::<Result<Vec<_>>>()?;
        let seed = if process == 1 {
            self.scenario["seed"].clone()
        } else {
            self.report["resolved_seed"].clone()
        };
        Ok(format!(
            "-- 临时 tx08 配置；本次 token 和显式动作参数随 artifact 保存。\nreturn {}\n",
            lua(&json!({
                "tx":"tx-04-08","token":format!("{}-p{process}",self.report["token"].as_str().ok_or("token")?),"menu_host":false,"host_port":self.scenario["host_port"],
                "expected_b_player_type":0,"actions":actions,"observation_heartbeat":30,"laser_event_cap":4096,
                "seed":seed,"animation_timeout_ms":15000
            }))?
        ))
    }
    // 逐次人工检查点保存到独立文件；未收到精确确认就等待，EOF/取消不算确认。
    // 此文件只允许恢复启动前的等待，不提供运行失败后的自动续跑。
    fn ready_tx08(&mut self, process: u64) -> Result<()> {
        if self.out.join(format!("ready-{process}.txt")).exists() {
            return Err("readiness_confirmation_predates_this_prompt".into());
        }
        let name = format!("process-{process}");
        let actual = serde_json::to_string_pretty(
            &self.scenario[if process == 1 {
                "actions"
            } else {
                "comparison_actions"
            }]
            .as_array()
            .ok_or("actions")?
            .iter()
            .filter(|a| a["enabled"] != false)
            .collect::<Vec<_>>(),
        )
        .map_err(|e| e.to_string())?;
        let prompt = format!(
            "第 {process}/2 次启动：临时部署 DLL/测试 mod/配置，使用第二存档；SkipIntro=1。\n\
            harness 接管键盘，本机远端发送冻结脚本；请从现在起停止手动操作，直到本次退出并提示。\n\
            实际 Lua 动作及参数：\n{actual}\n\
            另执行暂停/控制台、Host/Join 和显式 world 停止；第一进程保存退菜单并 Continue。\n\
            Download Helper 临时禁用并恢复；不回滚存档。\n\
            确认这些操作已获授权且已经准备好后，在 {} 中写入 READY {process}；取消则写入 CANCEL。",
            self.out.join(format!("ready-{process}.txt")).display()
        );
        let checkpoint = self.out.join(format!("readiness-{process}.json"));
        write_json(
            &checkpoint,
            &json!({"process":process,"status":"WAITING","prompt":prompt,"token":self.report["token"],
            "confirmation_file":format!("ready-{process}.txt"),"actions":self.scenario[if process==1 {"actions"}else{"comparison_actions"}]}),
        )?;
        println!("{prompt}");
        std::io::stdout().flush().map_err(|e| e.to_string())?;
        self.step(
            "manual_readiness_wait",
            json!({"process":name,"checkpoint":checkpoint}),
        )?;
        // 只等待本 artifact 的独立确认文件，不占用 stdin，不留下会争抢第二次确认的读取线程。
        let expected = format!("READY {process}");
        let confirmation = loop {
            if let Ok(text) = fs::read_to_string(self.out.join(format!("ready-{process}.txt"))) {
                if text.trim() == expected {
                    break "file";
                }
                if text.trim() == "CANCEL" {
                    return Err("readiness_cancelled".into());
                }
            }
            thread::sleep(Duration::from_millis(200));
        };
        write_json(
            &checkpoint,
            &json!({"process":process,"status":"READY","prompt":prompt,
            "token":self.report["token"],"confirmation":confirmation,"confirmation_value":expected}),
        )?;
        self.step(
            "manual_ready",
            json!({"process":process,"confirmation":confirmation}),
        )
    }
    fn world_wait_tx08(&mut self, g: &win32::Game, c: &mut Control, state: &str) -> Result<Status> {
        let deadline = Instant::now()
            + Duration::from_millis(
                self.scenario["operation_timeout_ms"]
                    .as_u64()
                    .ok_or("operation_timeout_ms")?,
            );
        loop {
            if let Some(code) = g.exited() {
                return Err(format!("world wait process exited:{code}"));
            }
            let status = c.command("cs world status")?;
            self.report["last_world_status"] = json!(status);
            if status.get("world_state").map(String::as_str) == Some(state) {
                return Ok(status);
            }
            if state != "Closed"
                && matches!(
                    status.get("world_state").map(String::as_str),
                    Some("Faulted" | "Closed")
                )
            {
                return Err(format!("unexpected world terminal:{status:?}"));
            }
            if Instant::now() >= deadline {
                return Err(format!("world_{state}_timeout:{status:?}"));
            }
            thread::sleep(Duration::from_millis(100));
        }
    }
    fn world_mark_tx08(&mut self, c: &mut Control, label: &str) -> Result<()> {
        let request = c.command(&format!("cs world checkpoint {label}"))?;
        let session = c.command("cs status")?;
        let world = c.command("cs world status")?;
        self.report["world_checkpoints"][label] =
            json!({"request":request,"session":session,"world":world});
        self.step(
            "world_checkpoint",
            json!({"label":label,"request":request,"session":session,"world":world}),
        )
    }
    // DLL 的 Active 快照与远端实际 Ack 必须共同确认 exact pair 代次，不能在握手包到达前装脚本。
    fn wait_binding_tx08(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: &Remote,
    ) -> Result<Status> {
        let end = Instant::now() + Duration::from_secs(45);
        loop {
            let status = self.watch_tx05(g, c, 0)?;
            let snapshot = r.snapshot();
            if let Some(error) = snapshot.error {
                return Err(format!("remote:{error}"));
            }
            if status.get("session_state").map(String::as_str) == Some("Active")
                && status.get("a_bound").map(String::as_str) == Some("1")
                && status.get("b_bound").map(String::as_str) == Some("1")
                && snapshot.ack.as_ref().is_some_and(|a| {
                    a.state == 3 && number(&status, "b_generation").ok() == Some(a.generation)
                })
            {
                return Ok(status);
            }
            if Instant::now() >= end {
                return Err("DLL_remote_generation_confirmation_timeout".into());
            }
            thread::sleep(Duration::from_millis(80));
        }
    }
    fn action_tx08(
        &mut self,
        g: &win32::Game,
        w: usize,
        c: &mut Control,
        index: u64,
        name: &str,
    ) -> Result<()> {
        self.step("fixture_action", json!({"index":index,"name":name}))?;
        win32::key(w, "F8")?;
        let deadline = Instant::now()
            + Duration::from_millis(
                self.scenario["action_timeout_ms"]
                    .as_u64()
                    .ok_or("action_timeout_ms")?,
            );
        loop {
            self.watch_tx05(g, c, 0)?;
            let rows = self.live_fixture()?;
            if rows
                .iter()
                .any(|v| v["event"] == "fixture-error" || v["fixture_error"] == true)
            {
                return Err("tx08_fixture_error".into());
            }
            let terminals = rows
                .iter()
                .filter(|v| {
                    v["event"] == "action"
                        && v["step"] == index
                        && v["action"] == name
                        && v["phase"] == "arrived"
                })
                .count();
            if terminals == 1 {
                return Ok(());
            }
            if terminals > 1 {
                return Err("duplicate_action_terminal".into());
            }
            if Instant::now() >= deadline {
                return Err(format!("fixture_timeout:{index}:{name}"));
            }
            thread::sleep(Duration::from_millis(60));
        }
    }
    // 达到数量只用于结束观测窗口；稳定性在最终文件归约中按全部 attempt 重建。
    fn capture_window_tx08(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        r: &Remote,
        process: u64,
    ) -> Result<()> {
        let target = self.scenario[if process == 1 {
            "stable_samples"
        } else {
            "comparison_samples"
        }]
        .as_u64()
        .ok_or("samples")?;
        let label = format!("p{process}.stable");
        self.world_mark_tx08(c, &format!("{label}.begin"))?;
        r.script(&label, self.input_script_tx08()?)?;
        let base = c.command("cs world status")?;
        let completed = number(&base, "complete")?;
        let deadline = Instant::now()
            + Duration::from_secs(
                self.scenario["capture_timeout_seconds"]
                    .as_u64()
                    .ok_or("capture_timeout_seconds")?,
            );
        loop {
            self.watch_tx05(g, c, 0)?;
            if let Some(error) = r.snapshot().error {
                return Err(format!("remote:{error}"));
            }
            let world = c.command("cs world status")?;
            if world.get("world_state").map(String::as_str) != Some("Recording") {
                return Err(format!("world_not_recording:{world:?}"));
            }
            if world.get("reason").map(String::as_str) != Some("NONE") {
                return Err(format!("world_failed:{world:?}"));
            }
            if number(&world, "complete")?.saturating_sub(completed) >= target {
                break;
            }
            if Instant::now() >= deadline {
                return Err("capture_window_timeout_no_extension".into());
            }
            thread::sleep(Duration::from_millis(150));
        }
        self.report["processes"][format!("process-{process}")]["remote_end"] = remote_view(r);
        self.world_mark_tx08(c, &format!("{label}.end"))
    }
    fn input_script_tx08(&self) -> Result<Vec<Step>> {
        self.scenario["input_script"]
            .as_array()
            .ok_or("input_script")?
            .iter()
            .map(|s| {
                let axis = |key: &str| -> Result<[i16; 2]> {
                    let a = s[key].as_array().filter(|a| a.len() == 2).ok_or("axis")?;
                    Ok([
                        i16::try_from(a[0].as_i64().ok_or("axis")?).map_err(|e| e.to_string())?,
                        i16::try_from(a[1].as_i64().ok_or("axis")?).map_err(|e| e.to_string())?,
                    ])
                };
                Ok(Step {
                    after: s["after"].as_u64().ok_or("after")?,
                    state: State {
                        held: 0,
                        edge: 0,
                        movement: axis("movement")?,
                        aim: axis("aim")?,
                    },
                })
            })
            .collect()
    }
    // 每次录制必须真实 Closed、后台交付完成才允许下一次 start/进程退出。
    fn close_world_tx08(&mut self, g: &win32::Game, c: &mut Control, label: &str) -> Result<()> {
        c.command("cs world stop")?;
        let status = self.world_wait_tx08(g, c, "Closed")?;
        self.report["recordings"][label] = json!({"terminal":status});
        if status.get("files_flushed").map(String::as_str) != Some("1")
            || status.get("result_written").map(String::as_str) != Some("1")
            || status.get("drain_complete").map(String::as_str) != Some("1")
        {
            return Err(format!("world_closed_incomplete:{status:?}"));
        }
        Ok(())
    }
    // 故障上下文逐一真实关闭；错误产物也保留终态，禁止把故障当健康收口。
    fn faults_tx08(&mut self, g: &win32::Game, c: &mut Control, remote: &Remote) -> Result<()> {
        let modes = ["reader_pause", "objects", "memberships", "relations", "read", "source",
            "root", "reference", "events", "thread", "file"];
        for mode in modes {
            self.report["fault_recordings"][mode] = json!({"status":"NOT_RUN"});
        }
        for (i, mode) in modes.iter().enumerate() {
            let label = format!("fault-{mode}");
            c.command(&format!("cs world start {label}"))?;
            let recording = self.world_wait_tx08(g, c, "Recording")?;
            let before = c.command("cs status")?;
            let remote_before = remote_view(remote);
            let generation = number(&recording, "generation")?;
            let token = self.report["token"].as_str().ok_or("token")?;
            let value = if *mode == "reader_pause" { 250 } else { 1 };
            let admission = c.command(&format!("cs world_test {token} {generation} {mode} {value}"))?;
            self.report["fault_recordings"][mode] = json!({"status":"RUNNING","label":label,
                "generation":generation,"recording":recording,"admission":admission,
                "before":before,"remote_before":remote_before});
            // 自有注入不会阻塞输入。等待实际触发，未触发也只停止一次并保留失败事实。
            let deadline = Instant::now() + Duration::from_secs(5);
            let observed = loop {
                if let Some(code) = g.exited() { return Err(format!("fault_process_exit:{mode}:{code}")); }
                let state = c.command("cs world status")?;
                if state.get("test_applied").map(String::as_str) == Some("1")
                    || state.get("world_state").map(String::as_str) == Some("Closed")
                    || Instant::now() >= deadline { break state; }
                self.watch_tx05(g, c, 80)?;
            };
            if *mode == "reader_pause" { self.watch_tx05(g, c, 600)?; }
            c.command("cs world stop")?;
            let terminal = self.world_wait_tx08(g, c, "Closed")?;
            self.report["recordings"][&label] = json!({"terminal":terminal});
            self.watch_tx05(g, c, 500)?;
            let after = c.command("cs status")?;
            self.report["fault_recordings"][mode]["status"] = json!("EXECUTED");
            self.report["fault_recordings"][mode]["mode_id"] = json!(i + 1);
            self.report["fault_recordings"][mode]["observed"] = json!(observed);
            self.report["fault_recordings"][mode]["terminal"] = json!(terminal);
            self.report["fault_recordings"][mode]["after"] = json!(after);
            self.report["fault_recordings"][mode]["remote_after"] = remote_view(remote);
            write_json(&self.out.join("fault-recordings.json"), &self.report["fault_recordings"])?;
            // Closed 表示线程真实结束；file 故障允许 flush=0，其余错误仍需完整排空。
            if terminal.get("close_timeout").map(String::as_str) != Some("0")
                || terminal.get("callbacks").map(String::as_str) != Some("0") {
                return Err(format!("fault_close_unconfirmed:{mode}"));
            }
        }
        self.report["scenes"]["S7"] = json!("EXECUTED");
        self.report["scenes"]["S8"] = json!("EXECUTED");
        Ok(())
    }
    fn scenes_tx08(&mut self, g: &win32::Game, process: u64) -> Result<()> {
        let prefix = format!("process-{process}");
        let path = self.out.join(&prefix);
        mkdir(&path)?;
        let w = g.wait_window(90)?;
        let mut c = Control::connect(
            self.scenario["control_port"]
                .as_u64()
                .ok_or("control_port")? as u16,
            g,
            90,
            &path.join("control.jsonl"),
        )?;
        c.focus_window = Some(w);
        self.enter_run_tx05(g, w, &mut c)?;
        let actions = self.scenario[if process == 1 {
            "actions"
        } else {
            "comparison_actions"
        }]
        .as_array()
        .unwrap()
        .clone();
        let active = actions
            .iter()
            .filter(|s| s["enabled"] != false)
            .cloned()
            .collect::<Vec<_>>();
        let mut index = 0;
        for spec in active.iter().take_while(|s| s["phase"] == "setup") {
            index += 1;
            self.action_tx08(
                g,
                w,
                &mut c,
                index,
                spec["name"].as_str().ok_or("action name")?,
            )?;
        }
        let rows = self.live_fixture()?;
        let players = rows
            .iter()
            .rev()
            .find(|v| v["event"] == "observation")
            .ok_or("missing_A_observation")?;
        if players["player_count"] != 1
            || players["players"][0]["player_type"] != 0
            || players["players"][0]["player_index"] != 0
        {
            return Err("tx08_requires_single_Isaac_A_before_world_and_host".into());
        }
        let seed = rows
            .iter()
            .rev()
            .find_map(|v| v["detail"]["seed"].as_str())
            .ok_or("missing_actual_seed")?
            .to_string();
        if process == 1 {
            self.report["resolved_seed"] = json!(seed);
        } else if self.report["resolved_seed"] != seed {
            return Err("comparison_seed_mismatch".into());
        }
        write_json(
            &path.join("seed-and-players.json"),
            &json!({"seed":seed,"fixture":rows,"status":c.command("cs status")?}),
        )?;
        let label = if process == 1 {
            "primary"
        } else {
            "comparison"
        };
        let before = c.command("cs world status")?;
        write_json(
            &path.join("before-world.json"),
            &json!({"world":before,"session":c.command("cs status")?,
            "recording_directory_exists":self.out.join("runtime-world").join(label).exists()}),
        )?;
        if before.get("world_state").map(String::as_str) != Some("Off") {
            return Err("unexpected_world_before_first_start".into());
        }
        c.command(&format!("cs world start {label}"))?;
        self.world_wait_tx08(g, &mut c, "Recording")?;
        self.world_mark_tx08(&mut c, &format!("p{process}.before_host"))?;
        // A 已存在且 world 已 Recording 才 Host；B 仅由 C/S Join 原生 owner 创建。
        c.command(&format!("cs host {} 0", self.scenario["host_port"]))?;
        let remote = Remote::start(
            format!("127.0.0.1:{}", self.scenario["host_port"])
                .parse()
                .map_err(|e| format!("{e}"))?,
            &path.join("input.jsonl"),
            self.scenario["peer_timeout_ms"].as_u64().unwrap(),
            2,
        )?;
        let outcome = (|| -> Result<()> {
            self.wait_binding_tx08(g, &mut c, &remote)?;
            self.report["scenes"][if process == 1 { "S1" } else { "S12" }] = json!("COMPLETE");
            write_json(
                &path.join("bindings.json"),
                &json!({"status":c.command("cs status")?,"remote":remote_view(&remote)}),
            )?;
            self.capture_window_tx08(g, &mut c, &remote, process)?;
            self.report["scenes"][if process == 1 { "S2" } else { "S13" }] = json!("COMPLETE");
            if process == 1 {
                self.world_mark_tx08(&mut c, "p1.pause.begin")?;
                win32::key(w, "ESC")?;
                c.wait_pause(None)?;
                self.watch_tx05(g, &mut c, 1000)?;
                win32::key(w, "ESC")?;
                c.wait("pause_state", "0", 5)?;
                self.console(w, "tx08-blocking", &mut c)?;
                self.world_mark_tx08(&mut c, "p1.pause.end")?;
                for spec in active.iter().skip(index as usize) {
                    index += 1;
                    let scene = spec["scene"].as_str().unwrap_or("S4");
                    self.world_mark_tx08(&mut c, &format!("p1.action{index}.begin"))?;
                    self.action_tx08(
                        g,
                        w,
                        &mut c,
                        index,
                        spec["name"].as_str().ok_or("action name")?,
                    )?;
                    if matches!(spec["name"].as_str(), Some("room" | "revive_b" | "stage")) {
                        self.wait_binding_tx08(g, &mut c, &remote)?;
                    }
                    if spec["name"] == "revive_b" {
                        // 原生复活健康与 Appear 动画完成分开；换层不能抢在动画和恢复观察之前。
                        let deadline = Instant::now() + Duration::from_secs(15);
                        loop {
                            let rows = self.live_fixture()?;
                            let action_id = rows
                                .iter()
                                .find(|v| {
                                    v["event"] == "action"
                                        && v["step"] == index
                                        && v["phase"] == "arrived"
                                })
                                .map(|v| v["action_id"].clone())
                                .ok_or("revive action terminal missing")?;
                            if rows.iter().any(|v| {
                                v["event"] == "b-revive"
                                    && v["phase"] == "animation_finished"
                                    && v["action_id"] == action_id
                            }) {
                                break;
                            }
                            if Instant::now() >= deadline {
                                return Err("revive_animation_timeout".into());
                            }
                            self.watch_tx05(g, &mut c, 80)?;
                        }
                    }
                    self.world_mark_tx08(&mut c, &format!("p1.action{index}.end"))?;
                    self.report["scene_actions"][scene][index.to_string()] = spec.clone();
                }
            }
            self.close_world_tx08(g, &mut c, label)?;
            // world 停止不退休 Host：保留关闭后的真实 completed/Ack 窗口。
            let before = c.command("cs status")?;
            self.watch_tx05(g, &mut c, 1000)?;
            let after = c.command("cs status")?;
            write_json(
                &path.join("host-after-world-stop.json"),
                &json!({"before":before,"after":after,"remote":remote_view(&remote)}),
            )?;
            if process == 1 {
                self.faults_tx08(g, &mut c, &remote)?;
                c.command("cs world start healthy")?;
                self.world_wait_tx08(g, &mut c, "Recording")?;
                self.world_mark_tx08(&mut c, "p1.healthy.begin")?;
                self.watch_tx05(g, &mut c, 1500)?;
                self.world_mark_tx08(&mut c, "p1.healthy.end")?;
                self.close_world_tx08(g, &mut c, "healthy")?;
                self.report["scenes"]["S9"] = json!("COMPLETE");
            }
            c.command("cs stop")?;
            c.wait("session_state", "Idle", 15)?;
            Ok(())
        })();
        let _ = remote.bye();
        drop(remote);
        // 即使主流程失败也只做一次有界关闭，不重做失败场景。
        if outcome.is_err() {
            let current = c.command("cs world status").ok();
            let current_label = current.as_ref().and_then(|s|s.get("label")).map(String::as_str).unwrap_or(label);
            let close = self.close_world_tx08(g, &mut c, current_label);
            self.report["failure_world_close"] = json!(close.err());
        }
        outcome?;
        if process == 1 && self.scenario["save_continue"] == true {
            self.files_tx07("p1.before-save")?;
            self.return_to_menu(w, &mut c)?;
            self.files_tx07("p1.after-save")?;
            c.command("cs world start continue")?;
            self.world_wait_tx08(g, &mut c, "Recording")?;
            self.world_mark_tx08(&mut c, "p1.continue.menu")?;
            c.command(&format!("cs host {} 0", self.scenario["host_port"]))?;
            self.continue_tx07(g, w, &mut c)?;
            let remote = Remote::start(
                format!("127.0.0.1:{}", self.scenario["host_port"])
                    .parse()
                    .map_err(|e| format!("{e}"))?,
                &path.join("continue-input.jsonl"),
                3000,
                2,
            )?;
            let continued = (|| -> Result<()> {
                self.wait_binding_tx08(g, &mut c, &remote)?;
                self.watch_tx05(g, &mut c, 1500)?;
                self.world_mark_tx08(&mut c, "p1.continue.active")?;
                self.close_world_tx08(g, &mut c, "continue")?;
                c.command("cs stop")?;
                c.wait("session_state", "Idle", 15)?;
                Ok(())
            })();
            let _ = remote.bye();
            drop(remote);
            continued?;
            self.files_tx07("p1.after-continue")?;
            self.report["scenes"]["S10"] = json!("COMPLETE");
        }
        win32::screenshot(w, &path.join("before-close.bmp"))?;
        self.report["processes"][&prefix]["close_requested"] = json!(true);
        let code = g.close()?;
        self.report["processes"][&prefix]["exit_code"] = json!(code);
        if code != 0 {
            return Err(format!("process exit:{code}"));
        }
        Ok(())
    }
    pub(super) fn execute_tx08(&mut self) -> Result<()> {
        self.report["invocation"] = json!(std::env::args().collect::<Vec<_>>());
        write_json(
            &self.out.join("invocation.json"),
            &self.report["invocation"],
        )?;
        self.validate_tx08()?;
        for n in 0..=13 {
            self.report["scenes"][format!("S{n}")] = json!("NOT_RUN");
        }
        self.environment()?;
        self.disable_helper_tx08()?;
        self.environment()?;
        self.input_environment_tx05()?;
        self.report["scenes"]["S0"] = json!("COMPLETE");
        self.capture_sources()?;
        mkdir(&self.out.join("binaries"))?;
        for name in ["harness.exe", "zhlIsaacCS.dll"] {
            let source = if name == "harness.exe" {
                std::env::current_exe().map_err(|e| e.to_string())?
            } else {
                self.repo.join("out/repentogon").join(name)
            };
            fs::copy(source, self.out.join("binaries").join(name)).map_err(|e| e.to_string())?;
        }
        write_json(
            &self.out.join("environment.json"),
            &self.report["environment"],
        )?;
        self.report["crash_before"] = self.crash_files()?;
        self.snapshot("save_before")?;
        let saves = self.saves.as_ref().unwrap();
        let mut roots = vec![saves.documents.clone()];
        roots.extend(saves.steam_remote.iter().map(|(_, p)| p.clone()));
        let monitor =
            crate::tx07::files::Monitor::start(roots, self.out.join("file-notifications.jsonl"))?;
        for process in 1..=2 {
            self.ready_tx08(process)?;
            self.disable_helper_tx08()?;
            self.environment()?;
            self.input_environment_tx05()?;
            self.prepare_restore_tx08(process)?;
            self.report["deploy_process"] = json!(process);
            self.deploy()?;
            self.write_fixture_tx08(process)?;
            self.snapshot(&format!("save-p{process}-before"))?;
            let label = format!("process-{process}");
            mkdir(&self.out.join(&label))?;
            write_json(
                &self.out.join(&label).join("environment.json"),
                &self.report["environment"],
            )?;
            self.report["processes"][&label]["crash_before"] = self.crash_files()?;
            let game = self.launch(&label)?;
            let pid = game.pid();
            self.report["processes"][&label]["pid"] = json!(pid);
            let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                self.scenes_tx08(&game, process)
            }))
            .unwrap_or_else(|_| Err("tx08_scene_panic".into()));
            if let Err(e) = &result {
                self.capture_failure(&game, &label, e);
                if self.report["processes"][&label]["close_requested"] != true {
                    let closed = game.close();
                    self.report["processes"][&label]["failure_close"] =
                        json!({"code":closed.as_ref().ok(),"error":closed.as_ref().err()});
                }
            }
            self.report["processes"][&label]["drop_termination_requested"] =
                json!(game.exited().is_none());
            drop(game);
            let collection = self.collect(pid, &label);
            self.report["processes"][&label]["collection_error"] = json!(collection.err());
            self.snapshot(&format!("save-p{process}-after"))?;
            let before = self.out.join(format!("save-p{process}-before"));
            let after = self.out.join(format!("save-p{process}-after"));
            let mut changes = saves::changes(&before, &after)?;
            self.apply_recorded_baseline_between(&mut changes, &before, &after)?;
            write_json(
                &self.out.join(format!("{label}/save-changes.json")),
                &changes,
            )?;
            self.report["processes"][&label]["save_changes"] = changes;
            self.report["processes"][&label]["crash_after"] = self.crash_files()?;
            self.step("process_finished",json!({"process":process,"error":result.as_ref().err(),"manual_input":"本次进程已退出，可恢复操作；下次启动会单独提示"}))?;
            println!("第 {process} 次进程已退出，可以恢复手动操作。");
            result?;
            let restored = (|| -> Result<()> {
                self.cleanup()?;
                self.fixture_owned = false;
                self.deployed = false;
                self.restore_environment_tx08()?;
                self.restore_helper_tx08()
            })(); // 每次进程独立恢复。
            self.report["processes"][&label]["cleanup_error"] = json!(restored.as_ref().err());
            restored?;
            self.report["scenes"][if process == 1 { "S11" } else { "S12" }] = json!("COMPLETE");
        }
        monitor.finish()?;
        Ok(())
    }
    fn write_fixture_tx08(&mut self, process: u64) -> Result<()> {
        self.report["active_fixture_token"] = json!(format!(
            "{}-p{process}",
            self.report["token"].as_str().ok_or("token")?
        ));
        let content = self.fixture_config_tx08(process)?;
        fs::write(
            self.game.join("mods/isaac-cs-test/harness_config.lua"),
            &content,
        )
        .map_err(|e| e.to_string())?;
        fs::write(self.out.join(format!("fixture-p{process}.lua")), &content)
            .map_err(|e| e.to_string())?;
        Ok(())
    }
    // Helper 只创建本次拥有的禁用标记；先记录所有权，避免部分部署失败后丢失恢复依据。
    fn disable_helper_tx08(&mut self) -> Result<()> {
        let path = self.game.join("mods/repentogonreleasesbackup_3643104060");
        if path.is_dir() && !path.join("disable.it").exists() {
            let mut marker = fs::OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(path.join("disable.it"))
                .map_err(|e| e.to_string())?;
            self.report["helper_marker_owned"] = json!(true);
            marker
                .write_all(self.report["token"].as_str().unwrap().as_bytes())
                .map_err(|e| e.to_string())?;
            marker.sync_all().map_err(|e| e.to_string())?;
            self.step(
                "helper_disabled",
                json!({"path":path.join("disable.it"),"owned":true}),
            )?;
        }
        Ok(())
    }
    fn restore_helper_tx08(&mut self) -> Result<()> {
        if self.report["helper_marker_owned"] != true {
            return Ok(());
        }
        let marker = self
            .game
            .join("mods/repentogonreleasesbackup_3643104060/disable.it");
        if read(&marker)? != self.report["token"].as_str().ok_or("token")? {
            return Err("helper_marker_changed_preserved".into());
        }
        fs::remove_file(marker).map_err(|e| e.to_string())?;
        self.report["helper_marker_owned"] = json!(false);
        self.step("helper_restored", json!({"restored":true}))?;
        Ok(())
    }
    // 先备齐本次会覆盖的环境，再声明部署所有权；部分复制失败也能回到真实 before。
    fn prepare_restore_tx08(&mut self, process: u64) -> Result<()> {
        let root = self.out.join(format!("environment-before-p{process}"));
        mkdir(&root)?;
        let mut journal = vec![];
        for (name, relative) in [
            ("dll", "Repentogon/zhlIsaacCS.dll"),
            ("ui", "mods/isaac-cs-ui"),
            ("config", "Repentogon/isaac-cs/config.json"),
        ] {
            let target = self.game.join(relative);
            let backup = root.join(name);
            let exists = target.exists();
            if exists {
                if fs::symlink_metadata(&target)
                    .map_err(|e| e.to_string())?
                    .file_type()
                    .is_symlink()
                {
                    return Err("deployment_target_symlink".into());
                }
                if target.is_dir() {
                    crate::copy_tree(&target, &backup)?;
                } else {
                    fs::copy(&target, &backup).map_err(|e| e.to_string())?;
                }
            }
            journal.push(json!({"target":target,"backup":backup,"existed":exists,"directory":target.is_dir()}));
        }
        write_json(&root.join("journal.json"), &json!(journal))?;
        self.report["deployment_restore"] = json!(journal);
        self.report["deployment_started"] = json!(true);
        Ok(())
    }
    fn restore_environment_tx08(&mut self) -> Result<()> {
        if self.report["deployment_started"] != true {
            return Ok(());
        }
        let mut errors = vec![];
        for item in self.report["deployment_restore"]
            .as_array()
            .ok_or("restore journal")?
        {
            let result = (|| -> Result<()> {
                let target = PathBuf::from(item["target"].as_str().ok_or("restore target")?);
                let backup = PathBuf::from(item["backup"].as_str().ok_or("restore backup")?);
                if !target.exists() && item["existed"] == false {
                    return Ok(());
                }
                let parent = target
                    .parent()
                    .ok_or("restore parent")?
                    .canonicalize()
                    .map_err(|e| e.to_string())?;
                let game = self.game.canonicalize().map_err(|e| e.to_string())?;
                if !parent.starts_with(&game) {
                    return Err("restore target escaped game directory".into());
                }
                if target.exists() {
                    let resolved = target.canonicalize().map_err(|e| e.to_string())?;
                    if resolved.parent() != Some(parent.as_path())
                        || resolved.file_name() != target.file_name()
                    {
                        return Err("restore target changed/escaped".into());
                    }
                    if fs::symlink_metadata(&target)
                        .map_err(|e| e.to_string())?
                        .file_type()
                        .is_symlink()
                    {
                        return Err("restore target became symlink".into());
                    }
                    if target.is_dir() {
                        fs::remove_dir_all(&resolved).map_err(|e| e.to_string())?;
                    } else {
                        fs::remove_file(&resolved).map_err(|e| e.to_string())?;
                    }
                }
                if item["existed"] == true {
                    if item["directory"] == true {
                        crate::copy_tree(&backup, &target)?;
                    } else {
                        fs::copy(&backup, &target).map_err(|e| e.to_string())?;
                    }
                }
                Ok(())
            })();
            if let Err(e) = result {
                errors.push(e);
            }
        }
        if errors.is_empty() {
            self.report["deployment_started"] = json!(false);
            Ok(())
        } else {
            Err(errors.join("; "))
        }
    }
    fn validate_tx08(&self) -> Result<()> {
        for folder in ["mod", "tools/harness", "ui", "tests/e2e-mods", "scripts"] {
            if self.out.starts_with(
                self.repo
                    .join(folder)
                    .canonicalize()
                    .map_err(|e| e.to_string())?,
            ) {
                return Err("artifact_inside_source_tree_would_recurse_during_capture".into());
            }
        }
        if self
            .out
            .starts_with(self.game.canonicalize().map_err(|e| e.to_string())?)
        {
            return Err("artifact_must_be_outside_game_directory".into());
        }
        if self.scenario["stable_samples"] != 2000
            || self.scenario["comparison_samples"]
                .as_u64()
                .is_none_or(|n| n < 256 || n > 10000)
            || self.scenario["remote_lead"] != 2
            || self.scenario["save_continue"] != true
            || self.scenario["offline_cli_timeout_seconds"] != 300
        {
            return Err("tx08_fixed_scope".into());
        }
        for (key, min, max) in [
            ("control_port", 1, 65535),
            ("host_port", 1, 65535),
            ("operation_timeout_ms", 1000, 120000),
            ("close_timeout_ms", 1000, 60000),
            ("action_timeout_ms", 1000, 120000),
            ("capture_timeout_seconds", 90, 600),
            ("peer_timeout_ms", 1000, 30000),
        ] {
            if !self.scenario[key]
                .as_u64()
                .is_some_and(|v| (min..=max).contains(&v))
            {
                return Err(format!("invalid_tx08_{key}"));
            }
        }
        if self.scenario["control_port"] == self.scenario["host_port"] {
            return Err("ports_must_differ".into());
        }
        let steps = self.input_script_tx08()?;
        if steps.is_empty()
            || steps[0].after != 0
            || steps.windows(2).any(|p| p[0].after >= p[1].after)
            || steps.iter().any(|s| s.state.aim != [0, 0])
            || steps.last().unwrap().state.movement != [0, 0]
        {
            return Err("stable_input_script_contract".into());
        }
        for key in ["actions", "comparison_actions"] {
            let actions = self.scenario[key].as_array().ok_or("actions")?;
            if actions.len() > 128 {
                return Err("action_budget".into());
            }
            let mut setup_done = false;
            for action in actions.iter().filter(|a| a["enabled"] != false) {
                let name = action["name"].as_str().ok_or("action name")?;
                if ![
                    "seed",
                    "spawn",
                    "spawn_short_lived",
                    "remove",
                    "morph_npc",
                    "morph_pickup",
                    "grid_spawn",
                    "grid_remove",
                    "grid_replace",
                    "world_observe",
                    "skip",
                    "room",
                    "kill_b",
                    "revive_b",
                    "stage",
                ]
                .contains(&name)
                {
                    return Err(format!("unsupported_action:{name}"));
                }
                if action["phase"] != "setup" {
                    setup_done = true;
                } else if setup_done {
                    return Err("setup_actions_must_be_prefix".into());
                }
            }
            if actions
                .iter()
                .filter(|a| a["enabled"] != false)
                .next()
                .is_none_or(|a| a["name"] != "seed" || a["phase"] != "setup")
            {
                return Err("first_action_must_resolve_seed".into());
            }
        }
        self.fixture_config_tx08(1)?;
        Ok(())
    }
    // 归约/离线失败不阻止环境恢复，任何清理失败写入报告并使整体失败。
    pub(super) fn finish_tx08(&mut self, result: Result<()>) -> Result<()> {
        self.report["failure"] = json!(result.err());
        self.report["blocked_scenes"] = self.scenario["unimplemented_runtime_requirements"].clone();
        self.report["runtime_observation_requirements"] = self.scenario["observation_requirements"].clone();
        self.report["disabled_actions"] = json!(self.scenario["actions"].as_array().map(|a| {
            a.iter()
                .filter(|s| s["enabled"] == false)
                .collect::<Vec<_>>()
        }));
        let reduction = self.offline_tx08();
        self.report["offline_error"] = json!(reduction.err());
        let evaluation = self.evaluate_tx08();
        self.report["evaluation_error"] = json!(evaluation.err());
        let cleanup = self.cleanup();
        self.report["cleanup_error"] = json!(cleanup.err());
        let environment = self.restore_environment_tx08();
        self.report["environment_restore_error"] = json!(environment.err());
        let restored = self.restore_helper_tx08();
        self.report["helper_restore_error"] = json!(restored.err());
        if self.saves.is_some() && self.out.join("save_before").exists() {
            let result = (|| -> Result<()> {
                self.snapshot("save_after")?;
                let mut changes =
                    saves::changes(&self.out.join("save_before"), &self.out.join("save_after"))?;
                self.apply_recorded_baseline_tx05(&mut changes)?;
                write_json(&self.out.join("save-changes.json"), &changes)?;
                self.report["save_changes"] = changes;
                Ok(())
            })();
            self.report["save_compare_error"] = json!(result.err());
        }
        match self.crash_files() {
            Ok(value) => self.report["crash_after"] = value,
            Err(error) => self.report["crash_scan_error"] = json!(error),
        }
        write_json(
            &self.out.join("cleanup.json"),
            &json!({"fixture_and_config_error":self.report["cleanup_error"],
            "environment_restore_error":self.report["environment_restore_error"],
            "helper_error":self.report["helper_restore_error"],"saves_restored":false,"hooks_unloaded":false}),
        )?;
        let final_result = self.finalize_tx08();
        final_result
    }
}
