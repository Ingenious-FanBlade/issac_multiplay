//! 运行结束后唯一一次归约。输出逐记录索引，缺证据保持未知，不由摘要/历史 PASS 推断。
use super::*;
use super::trace_index::TraceIndex;
use crate::world::dataset::{self, Dataset};
use std::{
    collections::BTreeSet,
    fs::File,
    io::{BufRead, BufReader, BufWriter},
};

pub(super) fn json(path: &Path) -> Result<Value> {
    serde_json::from_str(&read(path)?).map_err(|e| format!("{}:{e}", path.display()))
}
pub(super) fn rows(path: &Path) -> Result<Vec<Value>> {
    let mut out = vec![];
    for line in BufReader::new(File::open(path).map_err(|e| e.to_string())?).lines() {
        out.push(
            serde_json::from_str(&line.map_err(|e| e.to_string())?).map_err(|e| e.to_string())?,
        );
    }
    Ok(out)
}
fn dec(v: &Value) -> Option<u64> {
    v.as_str()?.parse().ok()
}
// 按独立录制归约真实注入，不从 admission 或场景完成字符串推断触发。
fn fault_evidence(out: &Path, ledger: &Value) -> Result<Value> {
    let mut result = json!({});
    let modes = ["reader_pause", "objects", "memberships", "relations", "read", "source",
        "root", "reference", "events", "thread", "file"];
    for (i, mode) in modes.iter().enumerate() {
        let entry = &ledger[mode];
        let evaluated = (|| -> Result<Value> {
            if entry["status"] != "EXECUTED" { return Err("NOT_RUN".into()); }
            let root = out.join("runtime-world").join(format!("fault-{mode}"));
            let manifest = json(&root.join("manifest.json"))?;
            let terminal = json(&root.join("result.json"))?;
            let diagnostic = rows(&root.join("diagnostics.jsonl"))?;
            let injection = diagnostic.iter().filter(|r| r["kind"] == "test_injection").collect::<Vec<_>>();
            let scoped = injection.len() == 1 && injection[0]["injected"] == "TEST_INJECTED"
                && injection[0]["mode"] == i + 1 && injection[0]["applied"] == 1
                && injection[0]["value"] == if *mode=="reader_pause" {250}else{1}
                && injection[0]["recording_generation"] == manifest["recording_generation"]
                && injection[0]["session"] == manifest["session"]
                && terminal["recording_generation"] == entry["generation"]
                && manifest["session"] == entry["admission"]["recording"]
                && terminal["session"] == manifest["session"];
            let progress = dec(&entry["before"]["completed_tick"]).zip(dec(&entry["after"]["completed_tick"]))
                .is_some_and(|(a,b)| b > a) && entry["after"]["session_state"] == "Active"
                && entry["before"]["generation"] == entry["after"]["generation"]
                && entry["remote_after"]["tx"].as_u64().zip(entry["remote_before"]["tx"].as_u64()).is_some_and(|(b,a)| b > a)
                && entry["remote_after"]["ack"]["tick"].as_u64().zip(entry["remote_before"]["ack"]["tick"].as_u64()).is_some_and(|(b,a)| b > a);
            let closed = entry["terminal"]["world_state"] == "Closed"
                && entry["terminal"]["callbacks"] == "0" && entry["terminal"]["close_timeout"] == "0"
                && entry["terminal"]["result_written"] == "1";
            let reason = match *mode { "objects"|"memberships" => "BUDGET_EXCEEDED", "relations" => "RELATION_BUDGET_EXCEEDED",
                "read"=>"READ_FAILED", "source"=>"SOURCE_CHANGED", "root"=>"ROOT_CHANGED", "reference"=>"NOT_READY",
                "events"=>"EVENT_OVERFLOW", "thread"=>"WRONG_THREAD", "file"=>"FILE_ERROR", _=>"BUFFER_BACKPRESSURE" };
            let captures = if *mode == "file" { vec![] } else { rows(&root.join("captures.jsonl"))? };
            let matching = captures.iter().filter(|v| v["reason"] == reason).collect::<Vec<_>>();
            let branch = if matches!(*mode, "events"|"thread"|"file") {
                terminal["status"] == "CAPTURE_ERROR" && terminal["failure"]["reason"] == reason
                    && (*mode != "file" || terminal["diagnostics"]["files_flushed"] == false)
            } else if *mode == "reader_pause" {
                let last_gap = matching.iter().filter_map(|v| dec(&v["attempt_seq"])).max();
                !matching.is_empty() && dec(&terminal["counts"]["backpressure"]).is_some_and(|n|n>0)
                    && last_gap.is_some_and(|last|captures.iter().any(|v|v["record"]=="capture" && dec(&v["attempt_seq"]).is_some_and(|a|a>last)))
                    && terminal["status"] == "CAPTURE_CLOSED"
            } else {
                // 首因保留可能早于本次注入（如 FIRST_OBSERVED 的 NOT_READY），另核对实际拒绝分支。
                scoped && injection[0]["rejection"] == reason && captures.iter().any(|v|
                    v["record"]=="capture" && v["complete"]==false && dec(&v["attempt_seq"])
                        .zip(dec(&injection[0]["attempt"])).is_some_and(|(a,b)|a>b))
            };
            let pass = scoped && progress && closed && branch;
            Ok(json!({"status":if pass{"PASS"}else{"FAIL"},"pass":pass,"scope":scoped,"input_progress":progress,
                "closed":closed,"branch":branch,"reason":reason,"matching_attempts":matching.iter().map(|v|v["attempt_seq"].clone()).collect::<Vec<_>>(),
                "injection":injection,"terminal":terminal,"directory":root}))
        })();
        result[mode] = evaluated.unwrap_or_else(|e|json!({"status":if e=="NOT_RUN"{"NOT_RUN"}else{"FAIL"},"pass":false,"error":e}));
    }
    let pass = modes.iter().all(|m|result[m]["pass"]==true);
    Ok(json!({"status":if pass{"PASS"}else{"FAIL"},"pass":pass,"recordings":result}))
}
// 每份完整 capture 必须关联实际 StandardRoom pre/commit、同 loop/scope 的三阶段正常返回。
// 缺少新诊断的历史文件不获资格；只读原始值，不用最近帧补配。
fn capture_proof(root: &Path, trace: &Path, out: &Path) -> Result<Value> {
    let data = Dataset::open(root, false)?;
    let scan = dataset::scan(&data, None, None, None)?;
    let events = crate::tx05::evidence::parse_events(trace)?;
    let trace_index = TraceIndex::new(&events);
    let mut file = File::open(root.join("captures.jsonl")).map_err(|e| e.to_string())?;
    let mut output = BufWriter::new(File::create(out).map_err(|e| e.to_string())?);
    let (mut complete, mut matched) = (0u64, 0u64);
    let mut values = BTreeMap::<String, BTreeSet<String>>::new();
    let mut source_counts = BTreeMap::<u32, u64>::new();
    let mut aliases = 0u64;
    let mut player_first = false;
    for (&attempt, loc) in &scan.attempts {
        let c = data.at(&mut file, loc)?;
        for s in &c.sources {
            if s.status == "NONE" {
                *source_counts.entry(s.source).or_default() += 1;
            }
        }
        for o in &c.objects {
            if o.class == "Player" && o.origin == "FIRST_OBSERVED" && o.birth_event.is_none() {
                player_first = true;
            }
            if o.class == "Grid"
                && o.memberships.iter().any(|m| m.source == 11)
                && o.memberships.iter().any(|m| m.source == 12)
            {
                aliases += 1;
            }
            for f in &o.fields {
                if f.status == "VALUE" {
                    values
                        .entry(o.class.clone())
                        .or_default()
                        .insert(f.field_id.clone());
                }
            }
        }
        if !c.complete {
            continue;
        }
        complete += 1;
        let tick = c
            .authority_tick
            .as_deref()
            .and_then(|s| s.parse::<u64>().ok());
        let loop_id = dec(&c.diagnostics["loop"]);
        let scope = c.scope_token.parse::<u64>().ok();
        let stages = c.diagnostics["phase_witness"].as_array().is_some_and(|a| {
            a.len() == 3
                && a.iter().enumerate().all(|(i, s)| {
                    s["stage"] == i + 1
                        && s["entered"] == 1
                        && s["returned"] == 1
                        && dec(&s["scope"]) == scope
                        && scope.is_some_and(|s| s > 0)
                })
        });
        let counts = trace_index.phase_counts(tick, loop_id, scope);
        let (pre, commit, standard) = (counts.pre, counts.commit, counts.standard_publish);
        let ok = stages && pre == 1 && commit == 1 && standard == 1;
        if ok {
            matched += 1;
        }
        writeln!(output,"{}",json!({"attempt":attempt.to_string(),"capture":c.capture_seq,"outer":c.outer_token,"tick":tick.map(|t|t.to_string()),
            "loop":loop_id.map(|t|t.to_string()),"scope":scope.map(|t|t.to_string()),"stages":c.diagnostics["phase_witness"],
            "pre":pre,"commit":commit,"standard_publish":standard,"matched":ok})).map_err(|e|e.to_string())?;
    }
    output.flush().map_err(|e| e.to_string())?;
    let mut missing = json!({});
    for class in [
        "Player",
        "NPC",
        "Tear",
        "Projectile",
        "Pickup",
        "Familiar",
        "Effect",
        "Grid",
    ] {
        let ids = data.manifest["required"][class]
            .as_array()
            .ok_or("manifest required fields")?;
        let absent = ids
            .iter()
            .filter(|id| {
                !values
                    .get(class)
                    .is_some_and(|s| id.as_str().is_some_and(|id| s.contains(id)))
            })
            .cloned()
            .collect::<Vec<_>>();
        if !absent.is_empty() {
            missing[class] = json!(absent);
        }
    }
    Ok(
        json!({"complete":complete,"phase_matched":matched,"trace_index":trace_index.summary(),"phase_pass":complete>0 && matched==complete && !scan.event_gaps,
        "known_fields":values,"missing_fields":missing,"source_observation_counts":source_counts,"door_grid_alias_samples":aliases,
        "first_observed_player_without_birth":player_first,"healthy":data.healthy(),"index":out}),
    )
}
// odd-only 只有理由、Authority kind、线程与相同 scope 的真实递增序列全部具备才能跳过。
// 缺线程的旧 gap 保留为中断，不能根据前后样本偷偷补线程。
fn stable(root: &Path, out: &Path, target: u64, prefix: &str) -> Result<Value> {
    let data = Dataset::open(root, false)?;
    let scan = dataset::scan(&data, None, None, None)?;
    let marks = rows(&root.join("diagnostics.jsonl"))?;
    let bound = |suffix: &str| -> Result<u64> {
        let label = format!("{prefix}.stable.{suffix}");
        let m = marks
            .iter()
            .filter(|v| v["kind"] == "checkpoint" && v["label"] == label)
            .collect::<Vec<_>>();
        if m.len() != 1 {
            return Err(format!("missing_or_duplicate_watermark:{label}"));
        }
        dec(&m[0]["attempt_seq"])
            .or_else(|| dec(&m[0]["attempt"]))
            .ok_or("invalid checkpoint attempt".into())
    };
    let low = bound("begin")?;
    let high = bound("end")?;
    let mut input = File::open(root.join("captures.jsonl")).map_err(|e| e.to_string())?;
    let mut index = BufWriter::new(File::create(out).map_err(|e| e.to_string())?);
    let mut previous: Option<(u64, u64, u64, String, String, String, u64)> = None;
    let (mut count, mut odd, mut start, mut best) =
        (0u64, 0u64, 0u64, json!({"complete":0,"odd_only":0}));
    let mut classes = BTreeSet::new();
    let mut sources = BTreeSet::new();
    let mut fields = BTreeMap::<String, BTreeSet<String>>::new();
    let mut player_basis = BTreeSet::new();
    let mut unknown = BTreeMap::<String, u64>::new();
    for (&attempt, loc) in &scan.attempts {
        if attempt <= low || attempt > high {
            continue;
        }
        let c = data.at(&mut input, loc)?;
        let thread = c.diagnostics["thread"].as_u64().unwrap_or(0);
        let tick = c
            .authority_tick
            .as_deref()
            .and_then(|v| v.parse::<u64>().ok())
            .or_else(|| dec(&c.diagnostics["candidate_tick"]));
        let odd_only = !c.complete && c.reason == "ODD_PLAYER_ONLY" && c.authority_kind == 2;
        let complete = c.complete && c.authority_kind == 1 && c.reason == "NONE";
        let current = tick.map(|tick| {
            (
                attempt,
                tick,
                thread,
                c.run_epoch.clone(),
                c.room_epoch.clone(),
                c.mutation_epoch.clone(),
                loc.outer,
            )
        });
        let chain = previous
            .as_ref()
            .zip(current.as_ref())
            .is_some_and(|(p, n)| {
                p.0.checked_add(1) == Some(n.0)
                    && p.1.checked_add(1) == Some(n.1)
                    && p.2 != 0
                    && p.2 == n.2
                    && p.3 == n.3
                    && p.4 == n.4
                    && p.5 == n.5
                    && p.6 < n.6
            });
        let eligible =
            (complete || odd_only) && current.as_ref().is_some_and(|n| n.1 > 0 && n.2 != 0);
        let broken = (!chain && previous.is_some()) || !eligible;
        if broken {
            count = 0;
            odd = 0;
            start = 0;
        }
        if eligible && complete {
            if count == 0 {
                start = attempt;
            }
            count += 1;
        } else if eligible && odd_only && count > 0 {
            odd += 1;
        }
        if count > best["complete"].as_u64().unwrap_or(0) {
            best = json!({"start_attempt":start.to_string(),"end_attempt":attempt.to_string(),"complete":count,
                "odd_only":odd,"run_epoch":c.run_epoch,"room_epoch":c.room_epoch,"mutation_epoch":c.mutation_epoch,
                "end_tick":tick.map(|t|t.to_string()),"thread":thread});
        }
        writeln!(index,"{}",json!({"attempt":attempt.to_string(),"offset":loc.offset.to_string(),"length":loc.length,
            "complete":complete,"odd_only":odd_only,"eligible":eligible,"chain":chain,"interrupted":broken,
            "reason":c.reason,"tick":tick.map(|t|t.to_string()),"outer":c.outer_token,"thread":thread,
            "run":c.run_epoch,"room":c.room_epoch,"mutation":c.mutation_epoch,"current_complete":count,"current_odd":odd}))
            .map_err(|e|e.to_string())?;
        previous = if eligible { current } else { None };
        for s in &c.sources {
            if s.status == "NONE" {
                sources.insert(s.source);
            }
        }
        for o in &c.objects {
            classes.insert(o.class.clone());
            if o.class == "Player" && o.init_evidence == 3 {
                if let Some(n) = o.ordinal {
                    player_basis.insert(n);
                }
            }
            for f in &o.fields {
                if f.status == "VALUE" {
                    fields
                        .entry(o.class.clone())
                        .or_default()
                        .insert(f.field_id.clone());
                } else {
                    *unknown.entry(f.status.clone()).or_default() += 1;
                }
            }
        }
    }
    index.flush().map_err(|e| e.to_string())?;
    Ok(
        json!({"status":if best["complete"].as_u64().unwrap_or(0)>=target && !scan.event_gaps {"PASS"}else{"FAIL"},
        "target":target,"best_segment":best,"checkpoint_attempts":{"begin":low.to_string(),"end":high.to_string()},
        "healthy":data.healthy(),"event_gaps":scan.event_gaps,"counts":scan.counts(),"classes":classes,"sources":sources,
        "known_fields":fields,"unavailable_fields":unknown,"player_basis_3_ordinals":player_basis,
        "proof":out,"capture_scope":"stable checkpoint window only; complete file kept unchanged"}),
    )
}

impl Run {
    // 四份原始录制共用同一原生四边界/嵌套顺序归约，专项不借用旧报告。
    pub(super) fn reduce_native_calls_tx08(&mut self) -> Result<(bool, u64)> {
        let mut lifecycle = json!({});
        let mut errors = vec![];
        let mut closed = true;
        let mut recording_count = 0;
        for label in ["primary", "comparison", "healthy", "continue"] {
            let root = self.out.join("runtime-world").join(label);
            if !root.join("manifest.json").exists() {
                closed = false;
                continue;
            }
            recording_count += 1;
            let result = (|| -> Result<Value> {
                let data = Dataset::open(&root, false)?;
                let scan = dataset::scan(&data, None, None, None)?;
                let mut calls = BTreeMap::<String, [u64; 4]>::new();
                let mut stack = Vec::<(u64,bool)>::new();
                let mut order_errors = vec![];
                let mut kinds = BTreeMap::<String, u64>::new();
                for line in BufReader::new(
                    File::open(root.join("lifecycle-events.jsonl")).map_err(|e| e.to_string())?,
                )
                .lines()
                {
                    let row: Value = serde_json::from_str(&line.map_err(|e| e.to_string())?)
                        .map_err(|e| e.to_string())?;
                    *kinds.entry(row["event_kind"].to_string()).or_default() += 1;
                    if row["exceptional"] == true {
                        errors.push(format!("{label}:exceptional:{}", row["sequence"]));
                    }
                    if row["event_kind"] == 0 || row["event_kind"] == 8 {
                        let call=dec(&row["call"]).ok_or("native call missing")?;
                        let body=row["event_kind"]==8;
                        let entering=row["entering"]==true;
                        let order=if entering {
                            let expected=if body {Some((call,false))}else{dec(&row["parent"]).map(|p|(p,true))};
                            let valid=if body {stack.last().copied()==expected}
                                else {stack.last().map(|p|p.0)==expected.map(|p|p.0)};
                            stack.push((call,body)); valid
                        } else {stack.pop()==Some((call,body))};
                        if !order {order_errors.push(row["sequence"].clone());}
                        let key = format!("{}:{}", row["call"], row["observation"]);
                        let slot = if row["event_kind"] == 8 { 2 } else { 0 }
                            + if row["entering"] == true { 0 } else { 1 };
                        calls.entry(key).or_insert([0; 4])[slot] += 1;
                    }
                }
                let mismatches = calls
                    .iter()
                    .filter(|(_, v)| **v != [1, 1, 1, 1])
                    .map(|(k, v)| json!({"key":k,"counts":v}))
                    .collect::<Vec<_>>();
                if !mismatches.is_empty() {
                    errors.push(format!(
                        "{label}:native_body_pair_mismatches={}",
                        mismatches.len()
                    ));
                }
                if !stack.is_empty() || !order_errors.is_empty() {errors.push(format!("{label}:native_order"));}
                closed &= data.healthy();
                Ok(
                    json!({"counts":scan.counts(),"terminal":data.terminal,"healthy":data.healthy(),"event_gaps":scan.event_gaps,
                    "event_kinds":kinds,"call_groups":calls.len(),"call_pair_mismatches":mismatches,"call_order_errors":order_errors,"open_calls":stack,
                    "limitation":"lifecycle call pairing does not prove all phase/original or RNG invariants"}),
                )
            })();
            lifecycle[label] =
                json!({"evidence":result.as_ref().ok(),"error":result.as_ref().err()});
            if result.is_err() {
                closed = false;
            }
        }
        write_json(
            &self.out.join("evidence/lifecycle-and-close.json"),
            &lifecycle,
        )?;
        self.report["lifecycle"] = lifecycle;
        self.report["native_order_errors"]=json!(errors);
        Ok((closed, recording_count))
    }
    pub(super) fn evaluate_tx08(&mut self) -> Result<()> {
        mkdir(&self.out.join("evidence"))?;
        for n in 1..=12 {
            self.report["cases"][format!("V{n}")] = json!({"status":"NOT_OBSERVED","pass":false,
            "reason":"required native/runtime evidence not established; no historical results imported"});
        }
        // 最终稳定段与实时停止使用同一份冻结目标，缩短采集窗口后不能残留旧的 2,000。
        for (label, target, prefix) in [
            ("primary", self.scenario["stable_samples"].as_u64().ok_or("stable_samples")?, "p1"),
            (
                "comparison",
                self.scenario["comparison_samples"].as_u64().ok_or("comparison_samples")?,
                "p2",
            ),
        ] {
            let root = self.out.join("runtime-world").join(label);
            if root.join("result.json").exists() {
                let result = stable(
                    &root,
                    &self
                        .out
                        .join("evidence")
                        .join(format!("{label}-attempt-index.jsonl")),
                    target,
                    prefix,
                );
                self.report["stable"][label] =
                    json!({"proof":result.as_ref().ok(),"error":result.as_ref().err()});
                if let Ok(proof) = result {
                    write_json(
                        &self
                            .out
                            .join("evidence")
                            .join(format!("{label}-stable.json")),
                        &proof,
                    )?;
                }
            }
        }
        let stable = self.report["stable"]["primary"]["proof"].clone();
        let proof = capture_proof(
            &self.out.join("runtime-world/primary"),
            &self.out.join("process-1/trace.jsonl"),
            &self.out.join("evidence/primary-phase-index.jsonl"),
        );
        let primary = proof.as_ref().ok().cloned().unwrap_or(Value::Null);
        self.report["cases"]["V2"] = json!({"status":if primary["phase_pass"]==true{"PASS"}else{"NOT_OBSERVED"},
            "pass":primary["phase_pass"]==true,"evidence":primary,"error":proof.err()});
        self.report["cases"]["V3"] = json!({"status":if stable["status"]=="PASS" {"PASS"}else{"FAIL"},
            "pass":stable["status"]=="PASS","detail":self.report["stable"]["primary"]});
        let (closed, recording_count) = self.reduce_native_calls_tx08()?;
        let episode = (|| -> Result<Value> {
            let token = format!("{}-p1", self.report["token"].as_str().ok_or("token")?);
            let fixture = crate::tx05::fixture_records(&read(&self.out.join("process-1/log.txt"))?, &token)?;
            if fixture.is_empty() { return Err("missing_primary_fixture_records".into()); }
            super::episodes::reduce(&self.out, &self.scenario["actions"], &fixture)
        })();
        self.report["episodes"] = json!({"proof":episode.as_ref().ok(),"error":episode.as_ref().err()});
        let host_continues = (1..=2).all(|n| {
            json(
                &self
                    .out
                    .join(format!("process-{n}/host-after-world-stop.json")),
            )
            .ok()
            .is_some_and(|v| {
                let before = v["before"]["completed_tick"]
                    .as_str()
                    .and_then(|s| s.parse::<u64>().ok());
                let after = v["after"]["completed_tick"]
                    .as_str()
                    .and_then(|s| s.parse::<u64>().ok());
                before.zip(after).is_some_and(|(a, b)| b > a)
                    && v["before"]["generation"] == v["after"]["generation"]
                    && v["after"]["session_state"] == "Active"
            })
        });
        let faults = fault_evidence(&self.out, &self.report["fault_recordings"])?;
        write_json(&self.out.join("evidence/faults.json"), &faults)?;
        self.report["cases"]["V8"] = faults;
        self.report["cases"]["V9"]=self.close_obligations_tx08(closed,recording_count,host_continues)
            .unwrap_or_else(|e|json!({"status":"NOT_OBSERVED","pass":false,"error":e}));
        let l = self.report["offline"].clone();
        let tools = (1..=5).all(|n| l[format!("L{n}")]["status"] == "PASS");
        self.report["cases"]["V10"] =
            json!({"status":if tools {"PASS"}else{"FAIL"},"pass":tools,"detail":l});
        let dual = l["L6"]["status"] == "PASS" && l["L7"]["status"] == "PASS";
        self.report["cases"]["V11"] = json!({"status":if dual {"PASS"}else{"NOT_OBSERVED"},"pass":dual,"detail":l["L6"],"legacy":l["L7"]});
        self.reduce_obligations_tx08()?;
        // 第二进程仍是必需的完整普通稳定段，不因 L6 偶然取得一对输入而放宽。
        let both=self.report["stable"]["primary"]["proof"]["status"]=="PASS"
            && self.report["stable"]["comparison"]["proof"]["status"]=="PASS";
        self.report["cases"]["V3"]=json!({"status":if both{"PASS"}else{"FAIL"},"pass":both,"recordings":self.report["stable"]});
        Ok(())
    }
    pub(super) fn finalize_tx08(&mut self) -> Result<()> {
        let saves = self.report["save_changes"]
            .as_array()
            .is_some_and(|a| a.iter().all(|v| v["allowed"] == true));
        let windows = (1..=2).all(|n| {
            self.report["processes"][format!("process-{n}")]["save_changes"]
                .as_array()
                .is_some_and(|a| a.iter().all(|v| v["allowed"] == true))
        });
        let exits =
            (1..=2).all(|n| self.report["processes"][format!("process-{n}")]["exit_code"] == 0);
        let crashes = self.report["crash_before"]
            .as_object()
            .zip(self.report["crash_after"].as_object())
            .is_some_and(|(a, b)| {
                b.iter()
                    .all(|(k, v)| !k.to_ascii_lowercase().ends_with(".dmp") || a.get(k) == Some(v))
            });
        if self.scenario["focus"]=="V12" {
            // V12依赖最终文件/退出与整个过程证据，在统一收尾中最后判定。
            let chain=self.session_obligations_tx08();
            let passed=saves && windows && exits && crashes && chain.as_ref().is_ok_and(|v|v["pass"]==true);
            self.report["cases"]["V12"]=json!({"status":if passed{"PASS"}else{"FAIL"},"pass":passed,
                "native_chain":chain.as_ref().ok(),"reduction_error":chain.as_ref().err(),
                "save_changes_allowed":saves,"per_process_changes_allowed":windows,"two_normal_exits":exits,"no_new_crashes":crashes});
        }
        if matches!(self.scenario["focus"].as_str(),Some("V6"|"V2"|"V5"|"V1_V10"|"V12")) {
            // 专项只判本项和本次操作的清理/退出条件，其他维度保留NOT_RUN。
            let focus=self.scenario["focus"].as_str().unwrap();
            let errors_clear = ["failure","offline_error","evaluation_error","cleanup_error",
                "environment_restore_error","helper_restore_error","save_compare_error","crash_scan_error"]
                .iter().all(|key|self.report[*key].is_null());
            let selected = if focus=="V1_V10" {
                ["V1","V10"].iter().all(|key|self.report["cases"][*key]["pass"]==true)
            } else { self.report["cases"][focus]["pass"]==true };
            let pass = selected
                && saves && windows && exits && crashes && errors_clear;
            self.report["pass"] = json!(pass);
            self.report["automatic_retry"] = json!(false);
            self.report["environment_checks"] = json!({"save_changes_allowed":saves,
                "per_process_changes_allowed":windows,"two_normal_exits":exits,"no_new_crashes":crashes});
            write_json(&self.out.join("report.json"), &self.report)?;
            let mut md=format!("# tx-04-08 {focus} E2E\n\nResult: **{}**\n\n",if pass {"PASS"}else{"FAIL"});
            for n in 1..=12 { md+=&format!("- V{n}: {}\n",self.report["cases"][format!("V{n}")]["status"].as_str().unwrap_or("NOT_RUN")); }
            fs::write(self.out.join("report.md"),md).map_err(|e|e.to_string())?;
            return if pass { Ok(()) } else { Err(format!("{focus} incomplete or failed; {}",self.out.join("report.json").display())) };
        }
        let chain=self.session_obligations_tx08();
        let chains_ok=chain.as_ref().is_ok_and(|v|v["pass"]==true);
        let pass12=saves && windows && exits && crashes && chains_ok;
        self.report["cases"]["V12"] = json!({"status":if pass12 {"PASS"}else{"FAIL"},"pass":pass12,
            "save_changes_allowed":saves,"per_process_changes_allowed":windows,"two_normal_exits":exits,"no_new_crashes":crashes,
            "native_chain":chain.as_ref().ok(),"reduction_error":chain.as_ref().err()});
        let pass = (1..=12).all(|n| self.report["cases"][format!("V{n}")]["pass"] == true)
            && [
                "failure",
                "offline_error",
                "evaluation_error",
                "cleanup_error",
                "environment_restore_error",
                "helper_restore_error",
                "save_compare_error",
            ]
            .iter()
            .all(|key| self.report[*key].is_null());
        self.report["pass"] = json!(pass);
        self.report["automatic_retry"] = json!(false);
        write_json(&self.out.join("report.json"), &self.report)?;
        let mut md = format!(
            "# tx-04-08 E2E\n\nResult: **{}**\n\nTwo sequential processes; no automatic retry; saves are never restored.\n\n",
            if pass { "PASS" } else { "FAIL" }
        );
        for n in 1..=12 {
            md += &format!(
                "- V{n}: {}\n",
                self.report["cases"][format!("V{n}")]["status"]
                    .as_str()
                    .unwrap_or("NOT_RUN")
            );
        }
        md += &format!(
            "\nFailure: {}\n\nRaw fault, lifecycle, RNG, readiness and session indexes are in evidence/. Every missing or unobserved obligation stays explicit; no historical runtime result is imported.\n",
            self.report["failure"]
        );
        fs::write(self.out.join("report.md"), md).map_err(|e| e.to_string())?;
        if pass {
            Ok(())
        } else {
            Err(format!(
                "E2E incomplete or failed; {}",
                self.out.join("report.json").display()
            ))
        }
    }
}
