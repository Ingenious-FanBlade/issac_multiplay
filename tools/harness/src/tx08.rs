//! tx-04-08 单一整体验收编排：两次顺序启动、真实 world 文件、生产 CLI 和一次最终归约。
//! 失败方式先列于设计 F1–F14；这里不重启、不重发动作、不自动配对未知 tick/出生。
mod evidence;
mod episodes;
mod offline;
mod obligations;
mod trace_index;
mod readiness;
mod phases;
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
        // 本次用户明确授权直接启动；把授权来源留在产物中，不伪造人工 READY 文件。
        if matches!(self.scenario["launch_authorization"].as_str(),Some("USER_DIRECT_START_V1_V10"|"USER_DIRECT_START_V12")) {
            write_json(&self.out.join(format!("readiness-{process}.json")),
                &json!({"process":process,"status":"AUTHORIZED_DIRECT_START",
                    "source":"2026-10-30 user message: game testing may start directly without reminders",
                    "token":self.report["token"],"manual_confirmation_required":false}))?;
            return Ok(());
        }
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
            另执行暂停/控制台、Host/Join 和显式 world 停止；第一进程保存退菜单并 Continue，活动录制时 WM_CLOSE；第二进程返回菜单另录 exit-menu 后 WM_QUIT。\n\
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
    // start/stop 只确认命令入队；旧快照可能延续到下一次主线程发布。
    // 仅用本次回复的录制身份判定状态，旧代次终态不能结束新请求的等待。
    fn world_wait_tx08(&mut self, g: &win32::Game, c: &mut Control, state: &str,
                      admitted: &Status) -> Result<Status> {
        let generation = number(admitted, "generation")?;
        let recording = admitted.get("recording").ok_or("missing admitted recording")?;
        let request = number(admitted, "request")?;
        let mut stale = 0u64;
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
            let observed = status.get("generation").map(|_| number(&status, "generation")).transpose()?;
            if observed.is_some_and(|value| value > generation) {
                return Err(format!("world_recording_superseded:{admitted:?}:{status:?}"));
            }
            let current = observed == Some(generation);
            if current && (status.get("recording") != Some(recording)
                || (state == "Recording" && number(&status, "request")? != request)) {
                return Err(format!("world_recording_identity_conflict:{admitted:?}:{status:?}"));
            }
            if !current { stale += 1; }
            if current && status.get("world_state").map(String::as_str) == Some(state) {
                self.step("world_wait_complete", json!({"target":state,"admitted":admitted,
                    "status":status,"stale_snapshots":stale}))?;
                return Ok(status);
            }
            if current && state != "Closed"
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
    // 所有录制入口共用一次 start 和身份限定等待，不重发启动请求。
    fn start_world_tx08(&mut self, g: &win32::Game, c: &mut Control,
                        label: &str) -> Result<(Status, Status)> {
        let queued = c.command(&format!("cs world start {label}"))?;
        let ready = self.world_wait_tx08(g, c, "Recording", &queued)?;
        if ready.get("label").map(String::as_str) != Some(label) {
            return Err(format!("world_recording_label_conflict:{label}:{ready:?}"));
        }
        Ok((queued, ready))
    }
    fn world_mark_tx08(&mut self, c: &mut Control, label: &str) -> Result<()> {
        let before = if self.scenario["focus"]!="V6" {
            Some(number(&c.command("cs world status")?,"checkpoint_produced")?)
        } else {None};
        let request = c.command(&format!("cs world checkpoint {label}"))?;
        // V2 的暂停边界必须已由主线程处理并交付，才能发送解除暂停的物理输入。
        if let Some(before)=before {
            let deadline=Instant::now()+Duration::from_secs(10);
            loop {
                let status=c.command("cs world status")?;
                if number(&status,"checkpoint_written")?>before {break;}
                if Instant::now()>=deadline {return Err(format!("checkpoint_delivery_timeout:{label}"));}
                thread::sleep(Duration::from_millis(25));
            }
        }
        let session = c.command("cs status")?;
        let world = c.command("cs world status")?;
        self.report["world_checkpoints"][label] =
            json!({"request":request,"session":session,"world":world});
        self.step(
            "world_checkpoint",
            json!({"label":label,"request":request,"session":session,"world":world}),
        )
    }
    // Host 回复只表示请求入队；必须观察新会话的 UDP bind/worker 成功后才允许远端首包。
    // 只发送一次 Host 请求，按既有操作期限读取状态；不重发 Hello、不忽略网络错误。
    fn host_listening_tx08(
        &mut self,
        g: &win32::Game,
        c: &mut Control,
        label: &str,
    ) -> Result<()> {
        let before = self.watch_tx05(g, c, 0)?;
        if before.get("session_state").map(String::as_str) != Some("Idle") {
            return Err(format!("host_requires_idle:{label}:{before:?}"));
        }
        let previous_generation = number(&before, "session_generation")?;
        let request = c.command(&format!("cs host {} 0", self.scenario["host_port"]))?;
        let started = Instant::now();
        let deadline = started + Duration::from_millis(
            self.scenario["operation_timeout_ms"].as_u64().ok_or("operation_timeout_ms")?,
        );
        loop {
            let status = self.watch_tx05(g, c, 0)?;
            self.report["host_readiness"][label] = json!({
                "before":before,"request":request,"status":status,
                "elapsed_ms":started.elapsed().as_millis(),"ready":false,
            });
            if number(&status, "session_generation")? > previous_generation {
                if status.get("session_state").map(String::as_str) != Some("Hosting")
                    || status.get("session_mode").map(String::as_str) != Some("Host")
                    || status.get("peer_terminal").map(String::as_str) != Some("None")
                    || number(&status, "net_native_error")? != 0
                {
                    return Err(format!("host_failed_before_remote:{label}:{status:?}"));
                }
                // Worker::Start 在 bind 成功后发布 Negotiating/running，主线程随后发布快照。
                if status.get("peer_running").map(String::as_str) == Some("1")
                    && status.get("peer_state").map(String::as_str) == Some("Negotiating")
                {
                    self.report["host_readiness"][label]["ready"] = json!(true);
                    return self.step("host_listening", json!({
                        "label":label,"evidence":self.report["host_readiness"][label],
                    }));
                }
            }
            if Instant::now() >= deadline {
                return Err(format!("host_listening_timeout:{label}:{status:?}"));
            }
            thread::sleep(Duration::from_millis(25));
        }
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
        let mut item_input_sent=false;
        loop {
            self.watch_tx05(g, c, 0)?;
            let rows = self.live_fixture()?;
            if rows
                .iter()
                .any(|v| v["event"] == "fixture-error" || v["fixture_error"] == true)
            {
                return Err("tx08_fixture_error".into());
            }
            if name=="player_esau" {
                let prepared=rows.iter().filter(|v|v["event"]=="world-fixture" && v["phase"]=="input_prepared"
                    && v["step"]==index && v["action"]==name).collect::<Vec<_>>();
                if prepared.len()>1 {return Err("duplicate_item_preparation".into());}
                if prepared.len()==1 && !item_input_sent {
                    if prepared[0]["input_key"]!="SPACE" || prepared[0]["input_count"]!=1 || prepared[0]["item"]!=703
                        || prepared[0]["slot"]!=0 || prepared[0]["charge"]!=1 || prepared[0]["item_state"]!=0
                        || prepared[0]["native_readiness"]["controls_enabled"]!=true
                        || prepared[0]["native_readiness"]["animation_finished"]!=true
                        || prepared[0]["native_readiness"]["item_cooldown"].as_i64().is_none_or(|n|n>0)
                        || prepared[0]["native_readiness"]["active_item"]!=703
                        || prepared[0]["native_readiness"]["min_charge"]!=1
                        || prepared[0]["native_readiness"]["max_charge"]!=1
                        || prepared[0]["native_readiness"]["active_charge"].as_u64().is_none_or(|n|n<1) {
                        return Err("unexpected_native_item_input".into());
                    }
                    self.step("native_item_input",json!({"step":index,"action_id":prepared[0]["action_id"],"key":"SPACE","count":1,
                        "route":"physical A to canonical queue/Resolve then original Item consumer"}))?;
                    item_input_sent=true;
                    win32::key(w,"SPACE")?;
                }
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
    // V12 通过真实A物理输入与B脚本共用Submit/Resolve；在Recording和Closed各观察一次。
    // 只用短时移动，释放后等待中性；失败依靠HeldKeys析构释放，不直接改玩家坐标。
    fn input_continuity_tx08(&mut self, g: &win32::Game, w: usize, c: &mut Control,
                             remote: &Remote, label: &str) -> Result<()> {
        let before=c.command("cs status")?;
        let mut keys=win32::HeldKeys::new(w);
        remote.script(label,self.input_script_tx08()?)?;
        keys.press("D")?;
        self.watch_tx05(g,c,180)?;
        keys.release("D")?;
        self.watch_tx05(g,c,800)?;
        let after=c.command("cs status")?;
        self.report["input_windows"][label]=json!({"before":before,"after":after,
            "world":c.command("cs world status")?,"remote":remote_view(remote),
            "a_key":"D","hold_ms":180,"b_script":self.scenario["input_script"]});
        Ok(())
    }
    // 原生退出与Steam云端文件可读就绪不是同一时刻；顺序再启动前保留冷却与磁盘静稳证据。
    // 这里只读磁盘元数据，既不重试Steam读取，也不声称磁盘静稳能证明云接口成功。
    fn wait_save_settle_tx08(&mut self) -> Result<()> {
        let start=Instant::now();
        let roots=self.saves.as_ref().ok_or("save roots")?;
        fn inventory(path: &Path, prefix: &str, result: &mut BTreeMap<String,Value>) -> Result<()> {
            for entry in fs::read_dir(path).map_err(|e|e.to_string())? {
                let entry=entry.map_err(|e|e.to_string())?;
                let name=format!("{prefix}/{}",entry.file_name().to_string_lossy());
                if name.starts_with("documents/save_backups/") || name=="documents/save_backups" {continue;}
                let kind=entry.file_type().map_err(|e|e.to_string())?;
                if kind.is_symlink() {return Err("save_settle_symlink".into());}
                if kind.is_dir() {inventory(&entry.path(),&name,result)?;}
                else if saves::requires_stable_snapshot(&name) {result.insert(name,win32::file_state(&entry.path()));}
            }
            Ok(())
        }
        let mut previous=BTreeMap::new();let mut stable_since=Instant::now();let mut observations=vec![];
        loop {
            let mut current=BTreeMap::new();
            inventory(&roots.documents,"documents",&mut current)?;
            for (account,path) in &roots.steam_remote {inventory(path,&format!("steam_remote/{account}"),&mut current)?;}
            let changed=current!=previous;
            if changed {stable_since=Instant::now();previous=current;}
            observations.push(json!({"elapsed_ms":start.elapsed().as_millis(),"changed":changed,"files":previous.len()}));
            if start.elapsed()>=Duration::from_secs(10) && stable_since.elapsed()>=Duration::from_secs(5) {break;}
            if start.elapsed()>=Duration::from_secs(30) {return Err("relaunch_save_files_not_stable".into());}
            thread::sleep(Duration::from_millis(500));
        }
        self.report["relaunch_save_settle"]=json!({"elapsed_ms":start.elapsed().as_millis(),"observations":observations,
            "metadata":previous,"cloud_readiness":"UNKNOWN","files_written":false});
        write_json(&self.out.join("relaunch-save-settle.json"),&self.report["relaunch_save_settle"])
    }
    // 专项的重启录制/Continue必须取得新的完整副本；固定睡眠不能证明采样覆盖。
    fn wait_full_phase_tx08(&mut self, g: &win32::Game, c: &mut Control, label: &str) -> Result<()> {
        let deadline=Instant::now()+Duration::from_secs(self.scenario["capture_timeout_seconds"].as_u64().ok_or("capture timeout")?);
        loop {
            self.watch_tx05(g,c,0)?;
            let status=c.command("cs world status")?;
            if status.get("label").map(String::as_str)!=Some(label)
                || status.get("world_state").map(String::as_str)!=Some("Recording") {
                return Err(format!("phase_recording_changed:{label}:{status:?}"));
            }
            if number(&status,"complete")?>=32 {return Ok(());}
            if Instant::now()>=deadline {return Err(format!("phase_complete_timeout:{label}:{status:?}"));}
            thread::sleep(Duration::from_millis(100));
        }
    }
    // 每次录制必须真实 Closed、后台交付完成才允许下一次 start/进程退出。
    fn close_world_tx08(&mut self, g: &win32::Game, c: &mut Control, label: &str) -> Result<()> {
        let queued = c.command("cs world stop")?;
        let status = self.world_wait_tx08(g, c, "Closed", &queued)?;
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
            let (_, recording) = self.start_world_tx08(g, c, &label)?;
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
            let queued = c.command("cs world stop")?;
            let terminal = self.world_wait_tx08(g, c, "Closed", &queued)?;
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
        // e2e-02：监听线程早于首个 Manager 就绪，不能立即发送须主线程处理的 counters。
        // 沿用既有菜单等待及启动期限；只轮询只读 status，不重发世界动作或放宽控制口超时。
        let startup=c.wait_startup(g,w,self.scenario["startup_timeout_seconds"].as_u64().ok_or("startup_timeout_seconds")?)?;
        self.step("S0_startup_menu_ready",json!(startup))?;
        // Off 证据仅覆盖真实选档/菜单调用；不得为保留旧窗口而错过开局 Init。
        let inactive_begin=c.command("cs counters")?;
        let inactive_world=c.command("cs world status")?;
        self.prepare_new_run_tx05(g, w, &mut c)?;
        // V1 的 Off 透传必须覆盖真实创建/Init/Publish，菜单零调用不算观察。
        // 随后的 seed 重开仍在 Recording 内，完整采集的初始化依据不放宽。
        let hook_tools = self.scenario["focus"] == "V1_V10";
        if hook_tools { self.start_new_run_tx05(g, w, &mut c)?; }
        let label = if process == 1 { "primary" } else { "comparison" };
        let before = c.command("cs world status")?;
        write_json(&path.join("passthrough-before.json"), &json!({
            "world_begin":inactive_world,"world":before,"before":inactive_begin,"after":c.command("cs counters")?,
            "scope":if hook_tools {"second-save native New Run while world Off; no Host or recording"}
                else {"second-save selection and character menu while world Off; no New Run or seed"}}))?;
        write_json(
            &path.join("before-world.json"),
            &json!({"world":before,"session":c.command("cs status")?,
            "recording_directory_exists":self.out.join("runtime-world").join(label).exists()}),
        )?;
        if before.get("world_state").map(String::as_str) != Some("Off") {
            return Err("unexpected_world_before_first_start".into());
        }
        // start 回复只代表请求入队；必须等后台准备、主线程附着及 Activate 全部完成。
        let (queued, ready) = self.start_world_tx08(g, &mut c, label)?;
        self.world_mark_tx08(&mut c, &format!("p{process}.{}",
            if hook_tools {"before_seed_restart"} else {"before_new_run"}))?;
        write_json(&path.join("recording-start.json"), &json!({
            "recording":label,"queued":queued,"ready":ready,"session":c.command("cs status")?,
            "boundary":if hook_tools {"active Off-created run before seed restart; full capture still requires Authority"}
                else {"character menu before native New Run and seed; full capture still requires Authority"}}))?;
        if !hook_tools { self.start_new_run_tx05(g, w, &mut c)?; }
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
            // seed 会重开 run；同一 Recording 必须覆盖它的进入与返回，不能在回调之后才开启。
            if spec["name"] == "seed" {
                let before_seed = c.command("cs world status")?;
                if before_seed.get("world_state").map(String::as_str) != Some("Recording") {
                    return Err("world_not_recording_before_seed".into());
                }
                self.world_mark_tx08(&mut c, &format!("p{process}.before_seed"))?;
            }
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
            return Err("tx08_requires_single_Isaac_A_before_host".into());
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
        let after_setup = c.command("cs world status")?;
        write_json(&path.join("after-setup-world.json"), &json!(after_setup))?;
        if after_setup.get("world_state").map(String::as_str) != Some("Recording") {
            return Err("world_not_recording_after_new_run_and_seed".into());
        }
        self.world_mark_tx08(&mut c, &format!("p{process}.before_host"))?;
        // A 已存在且 world 已 Recording 才 Host；B 仅由 C/S Join 原生 owner 创建。
        self.host_listening_tx08(g, &mut c, &format!("p{process}.initial"))?;
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
            if self.scenario["focus"]=="V12" {
                self.input_continuity_tx08(g,w,&mut c,&remote,&format!("p{process}.recording.input"))?;
            }
            self.report["scenes"][if process == 1 { "S2" } else { "S13" }] = json!("COMPLETE");
            if process == 1 {
                self.world_mark_tx08(&mut c, "p1.pause.begin")?;
                win32::key(w, "ESC")?;
                c.wait_pause(None)?;
                if self.scenario["focus"]!="V6" {self.world_mark_tx08(&mut c,"p1.pause.blocked.begin")?;}
                self.watch_tx05(g, &mut c, 1000)?;
                if self.scenario["focus"]!="V6" {self.world_mark_tx08(&mut c,"p1.pause.blocked.end")?;}
                win32::key(w, "ESC")?;
                c.wait("pause_state", "0", 5)?;
                self.console(w, "tx08-blocking", &mut c)?;
                self.world_mark_tx08(&mut c, "p1.pause.end")?;
                for spec in active.iter().skip(index as usize) {
                    if matches!(spec["phase"].as_str(),Some("post_continue"|"healthy")) {continue;}
                    index += 1;
                    let persistence = matches!(spec["name"].as_str(),Some("room"|"kill_b"|"revive_b"|"stage"|"player_esau"));
                    if persistence {self.files_tx07(&format!("p1.action{index}.before"))?;}
                    let scene = spec["scene"].as_str().unwrap_or("S4");
                    self.world_mark_tx08(&mut c, &format!("p1.action{index}.begin"))?;
                    self.action_tx08(
                        g,
                        w,
                        &mut c,
                        index,
                        spec["name"].as_str().ok_or("action name")?,
                    )?;
                    if matches!(spec["name"].as_str(), Some("room" | "revive_b" | "stage" | "player_esau")) {
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
                    if persistence {self.files_tx07(&format!("p1.action{index}.after"))?;}
                    self.report["scene_actions"][scene][index.to_string()] = spec.clone();
                }
            }
            self.close_world_tx08(g, &mut c, label)?;
            // world 停止不退休 Host：保留关闭后的真实 completed/Ack 窗口。
            let before = c.command("cs status")?;
            let counters = c.command("cs counters")?;
            if self.scenario["focus"]=="V12" {
                self.input_continuity_tx08(g,w,&mut c,&remote,&format!("p{process}.closed.input"))?;
            }
            self.watch_tx05(g, &mut c, 1000)?;
            let after = c.command("cs status")?;
            write_json(
                &path.join("host-after-world-stop.json"),
                &json!({"before":before,"after":after,"remote":remote_view(&remote),
                    "world":c.command("cs world status")?,"counters_before":counters,"counters_after":c.command("cs counters")?}),
            )?;
            if process == 1 {
                if !matches!(self.scenario["focus"].as_str(),Some("V6"|"V2"|"V5"|"V1_V10"|"V12")) { self.faults_tx08(g, &mut c, &remote)?; }
                self.start_world_tx08(g, &mut c, "healthy")?;
                self.world_mark_tx08(&mut c, "p1.healthy.begin")?;
                for spec in active.iter().filter(|s|s["phase"]=="healthy") {
                    index+=1;
                    self.world_mark_tx08(&mut c,&format!("p1.action{index}.begin"))?;
                    self.action_tx08(g,w,&mut c,index,spec["name"].as_str().ok_or("healthy action")?)?;
                    self.wait_binding_tx08(g,&mut c,&remote)?;
                    self.world_mark_tx08(&mut c,&format!("p1.action{index}.end"))?;
                    self.report["scene_actions"]["V2"][index.to_string()]=spec.clone();
                }
                self.watch_tx05(g, &mut c, 1500)?;
                if self.scenario["focus"]=="V2" {self.wait_full_phase_tx08(g,&mut c,"healthy")?;}
                self.world_mark_tx08(&mut c, "p1.healthy.end")?;
                self.close_world_tx08(g, &mut c, "healthy")?;
                self.report["scenes"]["S9"] = json!("COMPLETE");
            }
            let inactive_begin=c.command("cs counters")?;
            c.command("cs stop")?;
            c.wait("session_state", "Idle", 15)?;
            write_json(&path.join("passthrough-after.json"),&json!({"world":c.command("cs world status")?,
                "before":inactive_begin,"after":c.command("cs counters")?,"scope":"planned native Host/B retirement after world Closed"}))?;
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
        let mut exit_remote=None;
        if process == 1 && self.scenario["save_continue"] == true {
            self.files_tx07("p1.before-save")?;
            self.return_to_menu(w, &mut c)?;
            self.files_tx07("p1.after-save")?;
            self.start_world_tx08(g, &mut c, "continue")?;
            self.world_mark_tx08(&mut c, "p1.continue.menu")?;
            self.host_listening_tx08(g, &mut c, "p1.continue")?;
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
                // 特殊Replace在Continue已恢复后的独立动作窗口观察；不污染前序健康录制的资格。
                for spec in active.iter().filter(|s|s["phase"]=="post_continue") {
                    index+=1;
                    // 首次Replace之前必须取得实际世界前态；Continue的odd-only不能充当快照。
                    if self.scenario["focus"]=="V5" && spec["name"]=="player_esau" && spec["expect_new_player"]==true {
                        self.wait_full_phase_tx08(g,&mut c,"continue")?;
                    }
                    self.files_tx07(&format!("p1.action{index}.before"))?;
                    self.world_mark_tx08(&mut c,&format!("p1.action{index}.begin"))?;
                    self.action_tx08(g,w,&mut c,index,spec["name"].as_str().ok_or("post Continue action")?)?;
                    self.wait_binding_tx08(g,&mut c,&remote)?;
                    self.world_mark_tx08(&mut c,&format!("p1.action{index}.end"))?;
                    self.files_tx07(&format!("p1.action{index}.after"))?;
                    self.report["scene_actions"]["S5"][index.to_string()]=spec.clone();
                }
                if self.scenario["focus"]=="V2" {self.wait_full_phase_tx08(g,&mut c,"continue")?;}
                Ok(())
            })();
            continued?;
            // Continue 的录制和 Host 保持活动，直接窗口关闭必须自行停止 publication 并排空。
            exit_remote=Some(remote);
            self.files_tx07("p1.after-continue")?;
            self.report["scenes"]["S10"] = json!("COMPLETE");
        }
        if process==2 {
            self.return_to_menu(w,&mut c)?;
            self.start_world_tx08(g, &mut c, "exit-menu")?;
            self.watch_tx05(g,&mut c,500)?;
        }
        win32::screenshot(w, &path.join("before-close.bmp"))?;
        // 包含 Continue/菜单录制之后的实际观察计数；只在最终 V1 归约中判定。
        if hook_tools { write_json(&path.join("hook-counters-final.json"), &json!(c.command("cs counters")?))?; }
        let exit_mode=if process==1{"WM_CLOSE_ACTIVE_CONTINUE"}else{"WM_QUIT_MENU"};
        self.report["processes"][&prefix]["direct_exit"]=json!({"mode":exit_mode,"world":c.command("cs world status")?,"session":c.command("cs status")?});
        write_json(&path.join("direct-exit-request.json"),&self.report["processes"][&prefix]["direct_exit"])?;
        self.report["processes"][&prefix]["close_requested"] = json!(true);
        let code = if process==1 {g.close()?}else{g.close_via_quit()?};
        if let Some(remote)=exit_remote {let _=remote.bye();drop(remote);}
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
        // 保存本次生产构建日志和实现约定，运行产物可回查实际编译与静态边界。
        let build_folder=match self.scenario["focus"].as_str() {
            Some("V1_V10")=>"artifacts/repentogon/tx-04-08-v1-v10-build",
            Some("V12")=>"artifacts/repentogon/tx-04-08-v12-build",
            Some("V2")=>"artifacts/repentogon/tx-04-08-v2-build",Some("V5")=>"artifacts/repentogon/tx-04-08-v5-build",
            _=>"artifacts/repentogon/tx-04-08-v6-build"};
        for folder in [build_folder,"docs/design/tx-04-08","docs/research"] {
            crate::copy_tree(&self.repo.join(folder),&self.out.join("sources").join(folder))?;
        }
        // 旧材料只复制原始文件，在最终 L7 才通过生产 decoder 导入；不在准备阶段试跑工具。
        if let Some(source) = self.scenario["legacy_capture"].as_str() {
            let source = self.repo.join(source);
            let frozen = self.out.join("legacy-input"); mkdir(&frozen)?;
            let mut inventory = vec![];
            for name in ["manifest.json","captures.jsonl","lifecycle-events.jsonl","result.json","consumer.json","restoration.json"] {
                let path = source.join(name);
                let meta = fs::symlink_metadata(&path).map_err(|e|e.to_string())?;
                if !meta.is_file() || meta.file_type().is_symlink() { return Err(format!("legacy_original_not_regular:{name}")); }
                fs::copy(&path,frozen.join(name)).map_err(|e|e.to_string())?;
                inventory.push(json!({"name":name,"bytes":meta.len().to_string(),"source":path}));
            }
            write_json(&self.out.join("legacy-provenance.json"),&json!({"original":source,"frozen":frozen,
                "source":self.scenario["legacy_source"],"files":inventory,"hashes_used":false,"historical_runtime_pass_imported":false}))?;
        }
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
            if process==2 && self.scenario["focus"]=="V12" {self.wait_save_settle_tx08()?;}
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
            if !matches!(self.scenario["launch_authorization"].as_str(),Some("USER_DIRECT_START_V1_V10"|"USER_DIRECT_START_V12")) {
                println!("第 {process} 次进程已退出，可以恢复手动操作。");
            }
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
    pub(super) fn disable_helper_tx08(&mut self) -> Result<()> {
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
    pub(super) fn restore_helper_tx08(&mut self) -> Result<()> {
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
    pub(super) fn prepare_restore_tx08(&mut self, process: u64) -> Result<()> {
        let root = self.out.join(format!("environment-before-p{process}"));
        mkdir(&root)?;
        let mut journal = vec![];
        let mut targets=vec![
            ("dll", "Repentogon/zhlIsaacCS.dll"),
            ("ui", "mods/isaac-cs-ui"),
            ("observation", "mods/isaac-cs-observation"),
            ("config", "Repentogon/isaac-cs/config.json"),
        ];
        if self.report["tx"]=="tx-04-17" {targets.extend([("presentation_fixture","mods/isaac-cs-presentation"),("presentation_data","data/isaac-cs-presentation")]);}
        // REPENTOGON用模组目录名保存数据；只备份本tx真正部署的框架及夹具命名空间。
        if self.report["tx"]=="tx-04-14" {targets.extend([
            ("stageapi", "mods/stageapi15"),
            ("stageapi_data", "data/stageapi15"),
            ("tx14_data", "data/isaac-cs-test"),
        ]);}
        if self.report["tx"]=="tx-04-15" {targets.push(("tx15_data", "data/isaac-cs-test"));}
        for (name,relative) in targets {
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
    pub(super) fn restore_environment_tx08(&mut self) -> Result<()> {
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
        if !self.scenario["focus"].is_null() && !matches!(self.scenario["focus"].as_str(),Some("V6"|"V2"|"V5"|"V1_V10"|"V12")) {
            return Err("unknown_tx08_focus".into());
        }
        if !self.scenario["launch_authorization"].is_null()
            && !((self.scenario["focus"]=="V1_V10" && self.scenario["launch_authorization"]=="USER_DIRECT_START_V1_V10")
                || (self.scenario["focus"]=="V12" && self.scenario["launch_authorization"]=="USER_DIRECT_START_V12")) {
            return Err("unknown_tx08_launch_authorization".into());
        }
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
        // 用户按 240→30 秒同步缩减样本规模；历史录制按各自冻结的 scenario 保留。
        if self.scenario["stable_samples"] != 250
            || self.scenario["comparison_samples"]
                .as_u64()
                .is_none_or(|n| n < 32 || n > 10000)
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
            // 用户将采集等待窗口改为 30 秒；样本目标仍独立校验，不因缩时降为通过。
            ("capture_timeout_seconds", 30, 600),
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
            let mut post_continue = false;
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
                    "player_esau",
                    "pool_cycle",
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
                // 夹具的替换路线须显式声明；不在直接覆盖失败后静默改用删除或重试。
                if name=="grid_replace" && (action["replacement_mode"]!="remove_then_spawn"
                    || action["path_trail"]!=0 || action["keep_decoration"]!=false) {
                    return Err("grid_replace_requires_explicit_remove_then_spawn".into());
                }
                if action["phase"] != "setup" {
                    setup_done = true;
                } else if setup_done {
                    return Err("setup_actions_must_be_prefix".into());
                }
                if action["phase"]=="post_continue" {
                    if key!="actions" || !(action["name"]=="player_esau"
                        || self.scenario["focus"]=="V2" && action["name"]=="room" && action["return_to_previous"]==true
                        || self.scenario["focus"]=="V5" && action["name"]=="room") {
                        return Err("unsupported_post_continue_action".into());
                    }
                    post_continue=true;
                } else if post_continue {return Err("post_continue_actions_must_be_suffix".into());}
                if action["phase"]=="healthy" && (key!="actions" || self.scenario["focus"]!="V2" || action["name"]!="room") {
                    return Err("unsupported_healthy_action".into());
                }
            }
            if post_continue && self.scenario["save_continue"]!=true {return Err("post_continue_requires_real_continue".into());}
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
        // 用户指定的专项不执行L1–L7或其他维度归约；共用真实两进程生命周期编排。
        let focus=self.scenario["focus"].as_str().unwrap_or("").to_owned();
        let focused = matches!(focus.as_str(),"V6"|"V2"|"V5"|"V12");
        let reduction = if focused { Ok(()) } else { self.offline_tx08() };
        self.report["offline_error"] = json!(reduction.err());
        let evaluation = if focus == "V1_V10" {
            self.report["test_scope"] = json!("V1_V10_ONLY");
            for n in 1..=12 { self.report["cases"][format!("V{n}")] =
                json!({"status":"NOT_RUN","pass":false,"reason":"outside user-requested V1/V10 scope"}); }
            self.evaluate_hooks_tools_tx08()
        } else if focused {
            self.report["test_scope"] = json!(format!("{focus}_ONLY"));
            for n in 1..=12 { self.report["cases"][format!("V{n}")] =
                json!({"status":"NOT_RUN","pass":false,"reason":format!("outside user-requested {focus} scope")}); }
            let evaluated=match focus.as_str() {
                "V12"=>Ok(json!({"status":"NOT_OBSERVED","pass":false,"reason":"final environment and session reduction pending"})),
                "V2"=>phases::evaluate(&self.out),
                "V5"=>episodes::evaluate(&self.out,&self.scenario["actions"],self.report["token"].as_str().ok_or("token")?)
                    .map(|(result,proof)|{self.report["episodes"]=json!({"proof":proof,"error":null});result}),
                _=>readiness::evaluate(&self.out),
            };
            match evaluated {
                Ok(v) => { self.report["cases"][&focus] = v; Ok(()) },
                Err(e) => Err(e),
            }
        } else { self.evaluate_tx08() };
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
