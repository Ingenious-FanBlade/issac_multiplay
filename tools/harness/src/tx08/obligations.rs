//! D/F：在最终整体入口逐条归约原始值。缺记录、未执行和真实反例分开保存。
//! 所有下标均含录制名与原始序号，地址和 Lua 标签不充当跨进程身份。
use super::evidence::{json as load, rows};
use super::trace_index::TraceIndex;
use super::*;
use crate::tx05::evidence::parse_events;
use crate::world::{
    dataset::{self, Dataset},
    profile,
};
use std::{collections::BTreeSet, fs::File, io::BufWriter};

fn dec(v: &Value) -> Option<u64> {
    v.as_str()?.parse().ok()
}
// 原始并发文件列表不变；仅排除持续活动日志与D27每日备份，其余文件须稳定读取。
fn stable_snapshot(window: &Value) -> bool {
    window["possibly_writing"].as_array().is_some_and(|rows|rows.iter().all(|v|
        v.as_str().is_some_and(|name|!saves::requires_stable_snapshot(name))))
}
// 判定状态与证据一起返回；真实反例用 FAIL，缺观察保持 NOT_OBSERVED。
fn case(pass: bool, observed: bool, detail: Value) -> Value {
    json!({"status":if pass{"PASS"}else if observed{"FAIL"}else{"NOT_OBSERVED"},"pass":pass,"detail":detail})
}
// Hook 清单与两段 Off/Closed 的实际计数对应；注册与原生到达是两项事实。
fn hooks(out: &Path, report: &Value) -> Result<Value> {
    let manifest = load(&out.join("sources/out/repentogon/hook-manifest.json"))
        .or_else(|_| load(&out.join("hook-manifest.json")))?;
    let mut specs = manifest["hooks"]
        .as_array()
        .ok_or("hook manifest rows")?
        .clone();
    for key in ["world_observations", "world_native_witnesses"] {
        specs.extend(
            manifest[key]
                .as_array()
                .ok_or("world hook manifest rows")?
                .iter()
                .cloned(),
        );
    }
    let mut proof = vec![];
    let mut ok = true;
    let mut observed = true;
    for process in 1..=2 {
        let path = out.join(format!("process-{process}"));
        let trace = parse_events(&path.join("trace.jsonl"))?;
        let before = load(&path.join("passthrough-before.json"))?;
        let after = load(&path.join("passthrough-after.json"))?;
        let final_counters = if report["test_scope"] == "V1_V10_ONLY" {
            Some(load(&path.join("hook-counters-final.json"))?)
        } else { None };
        let registered = trace
            .iter()
            .filter(|e| e.kind() == "hook_registered")
            .collect::<Vec<_>>();
        let mut rows = vec![];
        let mut inactive_calls = [0u64; 2];
        for spec in &specs {
            let id = spec["id"].as_str().ok_or("hook id")?;
            let runtime = registered
                .iter()
                .filter(|e| {
                    e.raw["detail"]
                        .as_str()
                        .is_some_and(|s| s.split_whitespace().next() == Some(id))
                })
                .collect::<Vec<_>>();
            let abi = (spec["abi"].is_object()
                || spec["flags"].as_u64().is_some()
                    && spec["args"].as_array().is_some_and(|a| {
                        a.iter()
                            .all(|p| p["abi"].as_str().is_some_and(|s| !s.is_empty()))
                    }))
                && spec["rva"].as_str().is_some_and(|v| v != "0x0");
            let registration = runtime.len() == 1
                && runtime[0].n("priority").map(|n| json!(n)) == Some(spec["priority"].clone());
            // priority 可为负，使用有符号原始文本，不把 -10000 当缺失。
            let registration = registration
                || runtime.len() == 1
                    && runtime[0].text("priority").parse::<i64>().ok() == spec["priority"].as_i64();
            let is_world = id.starts_with("world_");
            let mut windows = vec![];
            let mut active_window = Value::Null;
            if is_world {
                for (n, (a, b)) in [
                    (&before["before"], &before["after"]),
                    (&after["before"], &after["after"]),
                ]
                .into_iter()
                .enumerate()
                {
                    let key = format!("hook.{id}");
                    let calls = dec(&b[&key])
                        .zip(dec(&a[&key]))
                        .and_then(|(b, a)| b.checked_sub(a));
                    let done = dec(&b[format!("{key}.completed")])
                        .zip(dec(&a[format!("{key}.completed")]))
                        .and_then(|(b, a)| b.checked_sub(a));
                    let good = calls.is_some()
                        && calls == done
                        && dec(&b[format!("{key}.foreign")]) == Some(0);
                    ok &= good;
                    inactive_calls[n] += calls.unwrap_or(0);
                    windows.push(
                        json!({"window":n,"calls":calls,"normal_returns":done,"paired":good}),
                    );
                }
                if let Some(end) = &final_counters {
                    // 活动录制直到 Continue/菜单退出前的全部已覆盖调用，外层和原生层分别计数。
                    let key = format!("hook.{id}");
                    let calls=dec(&end[&key]).zip(dec(&before["after"][&key]))
                        .and_then(|(b,a)|b.checked_sub(a));
                    let done=dec(&end[format!("{key}.completed")]).zip(dec(&before["after"][format!("{key}.completed")]))
                        .and_then(|(b,a)|b.checked_sub(a));
                    let paired=calls.is_some() && calls==done && dec(&end[format!("{key}.foreign")])==Some(0);
                    ok &= paired;
                    active_window=json!({"calls":calls,"normal_returns":done,"paired":paired});
                }
            }
            ok &= abi && registration;
            rows.push(json!({"id":id,"abi":spec["abi"],"args":spec["args"],"flags":spec["flags"],"rva":spec["rva"],"registration":registration,"inactive_windows":windows,"active_window":active_window}));
        }
        // 外层正常一次并不足以证明 original 一次；每个观察的原生层也须同窗等量。
        let native_counter_pairs=rows.iter().filter(|r|r["id"].as_str().is_some_and(|id|
            id.starts_with("world_") && !id.ends_with("_native"))).map(|outer| {
            let id=outer["id"].as_str().unwrap();
            let inner=rows.iter().find(|r|r["id"]==format!("{id}_native"));
            let paired=inner.is_some_and(|inner|
                outer["inactive_windows"]==inner["inactive_windows"] && outer["active_window"]==inner["active_window"]);
            json!({"outer":id,"native":format!("{id}_native"),"paired":paired})
        }).collect::<Vec<_>>();
        ok &= native_counter_pairs.iter().all(|r|r["paired"]==true);
        let summary = trace.iter().find(|e| e.kind() == "hook_summary");
        let capacity = summary.is_some_and(|e| {
            e.n("rejected") == Some(0)
                && e.n("rows") == Some(specs.len() as u64)
                && e.n("used") == Some(specs.len() as u64)
                && e.n("deferred") == Some(0)
        });
        let capabilities = ["world", "loop", "player"].iter().all(|group| {
            trace.iter().any(|e| {
                e.kind() == "capability"
                    && e.raw["detail"]
                        .as_str()
                        .is_some_and(|s| s.split_whitespace().next() == Some(group))
                    && e.n("ok") == Some(1)
            })
        });
        let patches=manifest["byte_patches"].as_array().ok_or("byte patch manifest")?.iter().map(|patch| {
            let matches=trace.iter().filter(|e|e.kind()=="byte_patch" && e.text("id")==patch["id"].as_str().unwrap_or("") && e.n("ok")==Some(1)).collect::<Vec<_>>();
            json!({"spec":patch,"installed":matches.len()==1,"raw":matches.iter().map(|e|&e.raw).collect::<Vec<_>>()})
        }).collect::<Vec<_>>();
        let pointers=trace.iter().filter(|e|e.kind()=="pointer_patch" && e.n("ok")==Some(1)).map(|e|e.raw.clone()).collect::<Vec<_>>();
        // 每个输入类/槽各安装一次；仅总数相同不能排除重复槽掩盖缺槽。
        let pointer_specs=manifest["pointer_hooks"].as_array().ok_or("pointer hook manifest")?;
        let pointer_rows=pointer_specs.iter().map(|id| {
            let installed=trace.iter().filter(|e|e.kind()=="pointer_patch" && e.n("ok")==Some(1)
                && format!("{}_{}",e.text("class"),e.text("slot"))==id.as_str().unwrap_or("")).count()==1;
            json!({"id":id,"installed":installed})
        }).collect::<Vec<_>>();
        let patch_ok=patches.iter().all(|p|p["installed"]==true) && pointers.len()==pointer_specs.len()
            && pointer_rows.iter().all(|p|p["installed"]==true)
            && !trace.iter().any(|e|matches!(e.kind(),"byte_patch"|"pointer_patch") && e.n("ok")==Some(0));
        ok &= capacity
            && capabilities
            && patch_ok
            && before["world_begin"]["world_state"] == "Off"
            && before["world"]["world_state"] == "Off"
            && after["world"]["world_state"] == "Closed";
        observed &= inactive_calls.iter().all(|n| *n > 0);
        proof.push(json!({"process":process,"rows":rows,"capacity":capacity,"capabilities":capabilities,"byte_patches":patches,"pointer_hooks":pointers,"pointer_slots":pointer_rows,
            "observed_inactive_calls":inactive_calls,"native_pairs":report["lifecycle"],"native_counter_pairs":native_counter_pairs,"registration_is_not_installation_proof":true}));
    }
    let native = report["lifecycle"].as_object().is_some_and(|m| {
        m.len() == 4
            && m.values().all(|r| {
                r["error"].is_null()
                    && r["evidence"]["call_pair_mismatches"]
                        .as_array()
                        .is_some_and(Vec::is_empty)
                    && r["evidence"]["call_order_errors"]
                        .as_array()
                        .is_some_and(Vec::is_empty)
                    && r["evidence"]["open_calls"]
                        .as_array()
                        .is_some_and(Vec::is_empty)
                    && r["evidence"]["event_gaps"] == false
            })
    }) && ["primary", "continue"].iter().all(|label| {
        report["lifecycle"][label]["evidence"]["call_groups"]
            .as_u64()
            .is_some_and(|n| n > 0)
    }) && report["native_order_errors"]
        .as_array()
        .is_some_and(Vec::is_empty);
    Ok(case(
        ok && observed && native,
        observed,
        json!({"processes":proof,"native_exactly_once":native,
        "scope":"observed calls, reviewed official hook chain; unknown external hooks excluded"}),
    ))
}

// 逐录制流式扫描；大型 capture 不累积在内存，索引输出后立即释放。
pub(super) fn captures(out: &Path) -> Result<Value> {
    let mut fields = BTreeMap::<String, BTreeSet<String>>::new();
    let mut qualified = BTreeSet::new();
    let mut sources = BTreeSet::new();
    let mut aliases = 0;
    let mut grid_max = None::<u64>;
    let mut rng_counts = [0u64; 3];
    let mut phase_proofs = vec![];
    let mut performance = vec![];
    let mut rng_index = BufWriter::new(
        File::create(out.join("evidence/rng-window-index.jsonl")).map_err(|e| e.to_string())?,
    );
    for label in ["primary", "comparison", "healthy", "continue"] {
        let root = out.join("runtime-world").join(label);
        let data = Dataset::open(&root, false)?;
        let scan = dataset::scan(&data, None, None, None)?;
        let process = if label == "comparison" { 2 } else { 1 };
        let trace = parse_events(&out.join(format!("process-{process}/trace.jsonl")))?;
        let trace_index = TraceIndex::new(&trace);
        let mut input = File::open(root.join("captures.jsonl")).map_err(|e| e.to_string())?;
        let mut complete = 0u64;
        let mut phase_ok = 0u64;
        let mut elapsed_qpc = 0u64;
        let mut max_qpc = 0u64;
        for (&attempt, location) in &scan.attempts {
            let c = data.at(&mut input, location)?;
            if !c.complete {
                if c.record == "capture" {
                    writeln!(rng_index,"{}",json!({"recording":label,"attempt":attempt.to_string(),"complete":false,
                        "reason":c.reason,"window":c.diagnostics["rng_window"],"status":"UNCONFIRMED_INCOMPLETE_CAPTURE",
                        "objects":c.objects.iter().map(|o|json!({"local":o.local_id,"use":o.use_generation,"rng":o.diagnostics["rng_window"]})).collect::<Vec<_>>()})).map_err(|e|e.to_string())?;
                }
                continue;
            }
            complete += 1;
            let tick = c
                .authority_tick
                .as_deref()
                .and_then(|v| v.parse::<u64>().ok());
            let scope = dec(&json!(c.scope_token));
            let loop_id = dec(&c.diagnostics["loop"]);
            let phase = c.diagnostics["phase_witness"].as_array().is_some_and(|a| {
                a.len() == 3
                    && a.iter().enumerate().all(|(i, s)| {
                        s["stage"] == i + 1
                            && s["entered"] == 1
                            && s["returned"] == 1
                            && dec(&s["scope"]) == scope
                            && scope.is_some_and(|s| s > 0)
                    })
            });
            let phase_counts = trace_index.phase_counts(tick, loop_id, scope);
            let phase = phase && phase_counts.pre == 1 && phase_counts.commit == 1
                && phase_counts.standard_publish == 1;
            if phase {
                phase_ok += 1;
            }
            for s in &c.sources {
                if s.status == "NONE" {
                    sources.insert(s.source);
                }
                if s.source == 11 && s.status == "NONE" {
                    grid_max = Some(grid_max.unwrap_or(0).max(s.count as u64));
                }
            }
            let window = &c.diagnostics["rng_window"];
            let mut rng_good = window["scope"] == "sources_fields_references_tail"
                && window["set_stable"] == true
                && window["inventory"].as_u64() == Some(c.objects.len() as u64)
                && window["matched"] == window["inventory"]
                && window["thread_before"] == c.diagnostics["thread"]
                && window["thread_after"] == c.diagnostics["thread"]
                && c.diagnostics["thread"].as_u64().is_some_and(|n| n > 0);
            let mut rng_differences = vec![];
            let mut rng_unknown = vec![];
            for o in &c.objects {
                let required = profile::required(&o.class);
                let mut known = BTreeSet::new();
                for f in &o.fields {
                    if f.status == "VALUE" {
                        known.insert(f.field_id.clone());
                        fields
                            .entry(o.class.clone())
                            .or_default()
                            .insert(f.field_id.clone());
                    }
                }
                if o.init_evidence >= 2
                    && o.lifecycle < 5
                    && required
                        .iter()
                        .all(|id| known.contains(&format!("{id:04x}")))
                {
                    qualified.insert(o.class.clone());
                }
                if o.class == "Grid"
                    && o.memberships.iter().any(|m| m.source == 11)
                    && o.memberships.iter().any(|m| m.source == 12)
                {
                    aliases += 1;
                }
                let rng = &o.diagnostics["rng_window"];
                let shape = |v: &Value| {
                    v.as_array().is_some_and(|a| {
                        a.len() == 4
                            && a.iter().all(|v| {
                                v.as_str().is_some_and(|s| {
                                    s.len() == 8
                                        && s.bytes().all(|b| {
                                            b.is_ascii_digit() || (b'a'..=b'f').contains(&b)
                                        })
                                })
                            })
                    })
                };
                let available = rng["before_status"] == 1
                    && rng["after_status"] == 1
                    && shape(&rng["before"])
                    && shape(&rng["after"]);
                if !available {
                    rng_unknown
                        .push(json!({"local":o.local_id,"use":o.use_generation,"values":rng}));
                    rng_good = false;
                } else if rng["before"] != rng["after"] {
                    rng_good = false;
                    rng_differences
                        .push(json!({"local":o.local_id,"use":o.use_generation,"values":rng}));
                }
            }
            let duration = dec(&window["finished_qpc"])
                .zip(dec(&window["started_qpc"]))
                .and_then(|(b, a)| b.checked_sub(a));
            let frequency = dec(&window["qpc_frequency"]);
            rng_good &= duration.is_some() && frequency.is_some_and(|f| f > 0);
            rng_counts[0] += 1;
            if rng_good {
                rng_counts[1] += 1;
            }
            if !rng_differences.is_empty() {
                rng_counts[2] += 1;
            }
            elapsed_qpc = elapsed_qpc.saturating_add(duration.unwrap_or(0));
            max_qpc = max_qpc.max(duration.unwrap_or(0));
            writeln!(rng_index,"{}",json!({"recording":label,"attempt":attempt.to_string(),"tick":c.authority_tick,"outer":c.outer_token,
                "window":window,"equal_at_boundaries":rng_good,"differences":rng_differences,"unavailable":rng_unknown})).map_err(|e|e.to_string())?;
        }
        phase_proofs.push(json!({"recording":label,"complete":complete,"matched":phase_ok,"event_gaps":scan.event_gaps,
            "pass":complete>0 && complete==phase_ok && !scan.event_gaps}));
        performance.push(json!({"recording":label,"trace_index":trace_index.summary(),"raw_copy_qpc_total":elapsed_qpc.to_string(),"raw_copy_qpc_max":max_qpc.to_string(),
            "terminal":data.terminal,"counts":scan.counts()}));
    }
    rng_index.flush().map_err(|e| e.to_string())?;
    let missing = profile::CLASSES[..8]
        .iter()
        .filter(|s| !qualified.contains(**s))
        .collect::<Vec<_>>();
    Ok(
        json!({"V2":super::phases::evaluate(out)?,"phase_counts":phase_proofs,
        "V4":case(missing.is_empty() && (0..=12).all(|s|sources.contains(&s)) && aliases>0 && grid_max==Some(448),true,
            json!({"qualified_classes":qualified,"missing_classes":missing,"known_fields":fields,"sources":sources,"door_grid_aliases":aliases,"actual_grid_slots":grid_max})),
        "V6":super::readiness::evaluate(out)?,
        "V7":case(rng_counts[0]>0 && rng_counts[0]==rng_counts[1],rng_counts[0]>0,
            json!({"complete_windows":rng_counts[0],"equal":rng_counts[1],"differences":rng_counts[2],"index":"evidence/rng-window-index.jsonl",
                "scope":"Entity dropRNG and Grid RNG; no global RNG claim; equality at two boundaries cannot exclude change-and-restore"})),
        "performance":performance}),
    )
}

// 每项生命周期覆盖必须从同一实际 episode 的 native 序号和对象前后态取得。
pub(super) fn episodes(report: &Value) -> Value {
    let proof = &report["episodes"]["proof"];
    let mut obligations = BTreeMap::<String, bool>::new();
    let mut indices = vec![];
    if let Some(episodes) = proof["episodes"].as_array() {
        for ep in episodes {
            let name = ep["spec"]["name"].as_str().unwrap_or("");
            let events = ep["native_events"]
                .as_array()
                .map(Vec::as_slice)
                .unwrap_or(&[]);
            let linked = ep["related_event_sequences"]
                .as_array()
                .map(Vec::as_slice)
                .unwrap_or(&[]);
            let native = |observation: &str| {
                events.iter().any(|e| {
                    linked.contains(&e["sequence"])
                        && e["event_kind"] == 8
                        && e["observation"] == observation
                        && e["entering"] == false
                        && e["exceptional"] == false
                })
            };
            let snapshots = ep["snapshots"].as_array().map(Vec::as_slice).unwrap_or(&[]);
            let revision = snapshots.iter().any(|a| {
                a["link"] == "native_identity_or_return_address"
                    && snapshots.iter().any(|b| {
                        b["link"] == "native_identity_or_return_address"
                            && a["object"]["local_id"] == b["object"]["local_id"]
                            && a["object"]["use_generation"] == b["object"]["use_generation"]
                            && dec(&b["object"]["revision"])
                                .zip(dec(&a["object"]["revision"]))
                                .is_some_and(|(b, a)| b > a)
                    })
            });
            let body = match name {
                "spawn" => native("Spawn") && ep["coverage"]["owner_birth"] == true,
                "spawn_short_lived" => ep["coverage"]["short_lived_chain"] == true,
                "remove" => native("Remove") && native("ClearReferences") && ep["coverage"]["logical_removal_and_clear"]==true,
                "pool_cycle" => native("Factory") && ep["coverage"]["use_gt_one_observed"]==true,
                "morph_npc" => native("NpcMorph") && revision,
                "morph_pickup" => native("PickupMorph") && native("Init") && revision,
                "grid_spawn" => native("GridSpawn") && ep["coverage"]["owner_birth"] == true,
                "grid_replace" => native("GridFree") && native("GridSpawn")
                    && ep["grid_recreation"]["pass"] == true,
                "grid_remove" => native("GridFree") && ep["coverage"]["grid_retirement"] == true,
                "player_esau" => native("Replace") && ep["player_replace"]["pass"] == true,
                _ => continue,
            };
            let good = ep["status"] == "INDEXED" && body;
            obligations
                .entry(name.into())
                .and_modify(|v| *v &= good)
                .or_insert(good);
            indices.push(json!({"step":ep["step"],"action_id":ep["action_id"],"action":name,"pass":good,"raw_sequences":linked}));
            if ep["coverage"]["use_gt_one_observed"] == true && native("Factory") {
                obligations.insert("actual_reuse".into(), true);
            }
            if ep["publication_return_counts"]
                .as_object()
                .is_some_and(|p| p.values().any(|n| n.as_u64().is_some_and(|n| n >= 2)))
            {
                obligations.insert("duplicate_publication".into(), true);
            }
        }
    }
    let restored = proof["continue_native_events"].as_array().is_some_and(|a| {
        ["ManagerRestore", "PlayerRestore"]
            .iter()
            .all(|observation| {
                a.iter().any(|e| {
                    e["event_kind"] == 8
                        && e["observation"] == *observation
                        && e["entering"] == false
                        && e["exceptional"] == false
                })
            })
    });
    obligations.insert("restore".into(), restored);
    obligations.insert("physical_destruction".into(),proof["physical_destruction_events"].as_array().is_some_and(|a|!a.is_empty()));
    if proof["publication_events_by_identity"]
        .as_object()
        .is_some_and(|m| {
            m.values()
                .any(|v| v.as_array().is_some_and(|a| a.len() >= 2))
        })
    {
        obligations.insert("duplicate_publication".into(), true);
    }
    if proof["actual_factory_reuse_events"]
        .as_array()
        .is_some_and(|a| !a.is_empty())
    {
        obligations.insert("actual_reuse".into(), true);
    }
    let required = [
        "spawn",
        "spawn_short_lived",
        "remove",
        "morph_npc",
        "morph_pickup",
        "grid_spawn",
        "grid_replace",
        "grid_remove",
        "player_esau",
        "actual_reuse",
        "duplicate_publication",
        "restore",
        "physical_destruction",
    ];
    let missing = required
        .iter()
        .filter(|k| obligations.get(**k) != Some(&true))
        .collect::<Vec<_>>();
    case(
        missing.is_empty(),
        !indices.is_empty(),
        json!({"obligations":obligations,"missing":missing,"episodes":indices,
        "disabled_actions":report["disabled_actions"],"source":"evidence/episodes.json; linked event sequences, not action completion alone"}),
    )
}

impl Run {
    // 联合专项只归约 V1 与 L1–L5；其它维度保持 NOT_RUN。
    pub(super) fn evaluate_hooks_tools_tx08(&mut self) -> Result<()> {
        mkdir(&self.out.join("evidence"))?;
        self.reduce_native_calls_tx08()?;
        self.report["cases"]["V1"] = hooks(&self.out, &self.report)?;
        write_json(&self.out.join("evidence/hook-integration.json"), &self.report["cases"]["V1"])?;
        let pass=(1..=5).all(|n|self.report["offline"][format!("L{n}")]["status"]=="PASS");
        self.report["cases"]["V10"]=json!({"status":if pass{"PASS"}else{"FAIL"},"pass":pass,"detail":self.report["offline"]});
        Ok(())
    }
    // 将请求、主循环放行、worker真实终态和进程结局连成同一次关闭的证据链。
    pub(super) fn close_obligations_tx08(
        &self,
        recordings_closed: bool,
        recording_count: u64,
        host_continues: bool,
    ) -> Result<Value> {
        let mut exits = vec![];
        let mut good = true;
        for process in 1..=2 {
            let path = self.out.join(format!("process-{process}"));
            let events = parse_events(&path.join("trace.jsonl"))?;
            let request = events
                .iter()
                .filter(|e| e.kind() == "world_exit" && e.text("event") == "request")
                .collect::<Vec<_>>();
            let release = events
                .iter()
                .filter(|e| e.kind() == "world_exit" && e.text("event") == "release")
                .collect::<Vec<_>>();
            let status = events
                .iter()
                .filter(|e| e.kind() == "world_exit_status")
                .collect::<Vec<_>>();
            let native_patch = events.iter().any(|e| {
                e.kind() == "byte_patch"
                    && e.text("id") == "world_process_exit"
                    && e.n("ok") == Some(1)
                    && e.n("resident") == Some(1)
            });
            let label = if process == 1 {
                "continue"
            } else {
                "exit-menu"
            };
            let data = Dataset::open(&self.out.join("runtime-world").join(label), false)?;
            let scan = dataset::scan(&data, None, None, None)?;
            let mode = load(&path.join("direct-exit-request.json"))?;
            let outcome = &self.report["processes"][format!("process-{process}")];
            let pass = native_patch
                && request.len() == 1
                && release.len() == 1
                && status.len() == 1
                && request[0].seq() < release[0].seq()
                && request[0].n("recording") == Some(1)
                && request[0].n("generation") == data.manifest["recording_generation"].as_u64()
                && release[0].n("closed") == Some(1)
                && release[0].n("timeout") == Some(0)
                && release[0].n("pending_recording") == Some(0)
                && release[0].n("host") == Some(0)
                && ["files_flushed", "result_written", "drain_complete"]
                    .iter()
                    .all(|k| status[0].n(k) == Some(1))
                && status[0].text("world_state") == "Closed"
                && status[0].n("callbacks") == Some(0)
                && status[0].n("close_timeout") == Some(0)
                && status[0].text("outcome") == "CAPTURE_CLOSED"
                && mode["world"]["world_state"] == "Recording"
                && mode["world"]["label"] == label
                && mode["mode"]
                    == (if process == 1 {
                        "WM_CLOSE_ACTIVE_CONTINUE"
                    } else {
                        "WM_QUIT_MENU"
                    })
                && data.healthy()
                && !scan.event_gaps
                && outcome["exit_code"] == 0
                && outcome["drop_termination_requested"] == false
                && outcome["collection_error"].is_null();
            good &= pass;
            exits.push(json!({"process":process,"pass":pass,"request":request.iter().map(|e|&e.raw).collect::<Vec<_>>(),
                "release":release.iter().map(|e|&e.raw).collect::<Vec<_>>(),"status":status.iter().map(|e|&e.raw).collect::<Vec<_>>(),
                "harness_request":mode,"terminal":data.terminal,"counts":scan.counts(),"native_patch":native_patch,
                "no_crash_claim_from_exit_code_alone":true}));
        }
        Ok(case(
            good && recordings_closed
                && recording_count == 4
                && host_continues
                && self.report["cases"]["V8"]["pass"] == true,
            true,
            json!({"exits":exits,"normal_recordings_closed":recordings_closed,"normal_recording_count":recording_count,
                "host_continues_after_world_stop":host_continues,"faults":self.report["cases"]["V8"],
                "scope":"explicit stop plus active Continue WM_CLOSE and menu WM_QUIT; crashes/forced termination keep incomplete prefixes"}),
        ))
    }
    // F/V12：整个操作窗口的原生持久入口、跨帧窗口及文件证据共同参与判定。
    pub(super) fn session_obligations_tx08(&mut self) -> Result<Value> {
        // V12专项跳过其它维度归约，不能依赖那些入口顺带创建证据目录。
        mkdir(&self.out.join("evidence"))?;
        let mut process_proof = vec![];
        let mut pass = true;
        for process in 1..=2 {
            let path = self.out.join(format!("process-{process}"));
            // 缺少进程仍明确失败，但保留已完成进程的全量核对证据，不丢弃整份归约。
            if !path.join("trace.jsonl").is_file() {
                pass = false;
                process_proof.push(json!({"process":process,"runtime":false,
                    "status":"NOT_OBSERVED","reason":"missing_process_trace"}));
                continue;
            }
            let events = parse_events(&path.join("trace.jsonl"))?;
            let trace_index = TraceIndex::new(&events);
            let ticks = events
                .iter()
                .filter(|e| e.kind() == "tick")
                .collect::<Vec<_>>();
            let bad_ticks = ticks
                .iter()
                .filter(|t| {
                    !trace_index.command_matches(t)
                        || t.n("native_commit") != Some(1)
                        || t.n("authority_pre") != Some(1)
                        || t.n("query_mismatches") != Some(0)
                        || t.n("bomb_query_mismatches") != Some(0)
                        || t.n("published") != Some(1)
                })
                .map(|t| t.seq())
                .collect::<Vec<_>>();
            let acks = events
                .iter()
                .filter(|e| e.kind() == "ack" && e.text("event") == "publication")
                .collect::<Vec<_>>();
            let bad_acks = acks
                .iter()
                .filter(|a| !trace_index.ack_has_prior_tick(a))
                .map(|e| e.seq())
                .collect::<Vec<_>>();
            let active_inputs=(0..=1).map(|ordinal| {
                let consumed=ticks.iter().filter(|t|t.n("ordinal")==Some(ordinal)
                    && t.n("input_sequence").is_some_and(|n|n>0) && t.pair("move").is_some_and(|p|p!=(0,0))
                    && t.n("player_native_reached").is_some_and(|n|n>0)).map(|t|t.seq()).collect::<Vec<_>>();
                json!({"ordinal":ordinal,"nonzero_native_consumed_sequences":consumed,"pass":!consumed.is_empty()})
            }).collect::<Vec<_>>();
            // 每个指定输入窗口必须在同一会话真实消费A/B非零命令，Closed后不能只看tick上涨。
            let input_windows=if self.scenario["focus"]=="V12" {
                ["recording","closed"].iter().map(|mode| {
                    let label=format!("p{process}.{mode}.input");
                    let window=&self.report["input_windows"][&label];
                    let generation=dec(&window["before"]["session_generation"]);
                    let lower=dec(&window["before"]["completed_tick"]);
                    let upper=dec(&window["after"]["completed_tick"]);
                    let consumed=(0..=1).map(|ordinal| {
                        let sequences=ticks.iter().filter(|t|t.n("session_generation")==generation
                            && t.n("ordinal")==Some(ordinal) && t.n("tick").zip(lower.zip(upper)).is_some_and(|(t,(a,b))|a<t && t<=b)
                            && t.n("input_sequence").is_some_and(|n|n>0) && t.pair("move").is_some_and(|p|p!=(0,0))
                            && t.n("player_native_reached").is_some_and(|n|n>0)).map(|t|t.seq()).collect::<Vec<_>>();
                        json!({"ordinal":ordinal,"pass":!sequences.is_empty(),"tick_sequences":sequences})
                    }).collect::<Vec<_>>();
                    let good=generation.is_some_and(|n|n>0) && generation==dec(&window["after"]["session_generation"])
                        && lower.zip(upper).is_some_and(|(a,b)|a<b) && window["after"]["session_state"]=="Active"
                        && window["world"]["world_state"]==if *mode=="closed" {"Closed"}else{"Recording"}
                        && consumed.iter().all(|c|c["pass"]==true);
                    json!({"label":label,"pass":good,"window":window,"consumed":consumed})
                }).collect::<Vec<_>>()
            } else {vec![]};
            let join = events
                .iter()
                .filter(|e| e.kind() == "join" && e.text("event") == "complete")
                .collect::<Vec<_>>();
            let allocated = events
                .iter()
                .filter(|e| e.kind() == "device_k" && e.text("event") == "allocated")
                .collect::<Vec<_>>();
            let retired = events
                .iter()
                .filter(|e| e.kind() == "device_k" && e.text("event") == "delete")
                .collect::<Vec<_>>();
            let lifecycle = !join.is_empty()
                && allocated.len() == join.len()
                && allocated.len() == retired.len()
                && allocated
                    .iter()
                    .chain(retired.iter())
                    .all(|e| e.n("never") == Some(1))
                && events
                    .iter()
                    .filter(|e| e.kind() == "life" && e.text("event") == "retired")
                    .count()
                    == join.len();
            let persistent = events
                .iter()
                .filter(|e| e.kind() == "persistent")
                .collect::<Vec<_>>();
            let entries = persistent
                .iter()
                .filter(|e| e.text("phase") == "enter")
                .collect::<Vec<_>>();
            let mut calls = vec![];
            for e in &entries {
                let returns = trace_index.returns(e.n("call"));
                let parent = e.n("parent") == Some(0)
                    || trace_index.has_prior_entry(e.n("parent"), e.seq());
                let known = e.text("known").is_empty() || e.n("known") == Some(1);
                let normal = parent
                    && returns.len() == 1
                    && returns[0].seq() > e.seq()
                    && returns[0].n("normal") == Some(1);
                calls.push(json!({"enter":e.raw,"returns":returns.iter().map(|r|&r.raw).collect::<Vec<_>>(),"known":known,"normal":normal}));
            }
            let windows=events.iter().filter(|e|e.kind()=="persist_window" && e.text("event")=="open").map(|open|{
                let closes=trace_index.closes(open.n("id"));
                json!({"id":open.n("id"),"kind":open.text("kind"),"open":open.raw,"closes":closes.iter().map(|c|&c.raw).collect::<Vec<_>>(),
                    "pass":closes.len()==1 && closes[0].seq()>open.seq() && closes[0].n("normal")==Some(1) && closes[0].n("call_normal")==Some(1)})
            }).collect::<Vec<_>>();
            let needed = if process == 1 {
                vec![
                    "b_death",
                    "b_revive",
                    "stage_change",
                    "continue",
                    "run_exit",
                ]
            } else {
                vec!["run_exit"]
            };
            let persistence_ok = !calls.is_empty()
                && calls
                    .iter()
                    .all(|c| c["normal"] == true && c["known"] == true)
                && persistent
                    .iter()
                    .filter(|e| e.text("phase") == "return")
                    .count()
                    == calls.len()
                && windows.iter().all(|w| w["pass"] == true)
                && needed
                    .iter()
                    .all(|k| windows.iter().any(|w| w["kind"] == *k));
            let loading = events
                .iter()
                .filter(|e| {
                    e.kind() == "loading"
                        && e.text("event") == "end"
                        && e.text("reason") == "loading_clear"
                })
                .map(|end| {
                    let begin = trace_index.loading_begin(end.n("episode"), end.n("session_generation"));
                    let unscoped=trace_index.loading_unscoped(end.n("episode"),end.n("session_generation"));
                    // unknown仍表示上下文未知；只有每条实际透传返回均有同窗见证且有限为零才闭合值证据。
                    let values_known=end.n("unknown")==Some(unscoped.len() as u64)
                        && unscoped.iter().all(|q|begin.is_some_and(|b|b.seq()<q.seq() && q.seq()<end.seq())
                            && q.n("context_known")==Some(0) && q.n("overridden")==Some(0)
                            && q.n("finite")==Some(1) && q.n("zero")==Some(1) && q.text("value")=="0");
                    let valid = begin.is_some_and(|b| {
                        end.n("ticks") == Some(0)
                            && end.n("nonzero") == Some(0)
                            && values_known
                            && end.pair("submit_start") == end.pair("submit_end")
                            && end.pair("submit_start").is_some()
                            && end.pair("resolved_tick_start") == end.pair("resolved_tick_end")
                            && end.pair("resolved_tick_start").is_some()
                            && !trace_index.changes_between(b.seq(), end.seq())
                    });
                    json!({"begin":begin.map(|b|&b.raw),"end":end.raw,"pass":valid,
                        "unscoped_actual_returns":unscoped.iter().map(|q|&q.raw).collect::<Vec<_>>(),
                        "unscoped_values_known_neutral":values_known,"context_identity":"UNKNOWN for unscoped calls; no binding or extra native query inferred"})
                })
                .collect::<Vec<_>>();
            let continued = process != 1
                || events.iter().any(|e| {
                    e.kind() == "restore"
                        && e.text("event") == "preserve"
                        && e.n("start") == Some(1)
                });
            let shutdown = events
                .iter()
                .filter(|e| e.kind() == "shutdown")
                .collect::<Vec<_>>();
            let trace_ok = shutdown.len() == 1
                && [
                    "trace_dropped",
                    "trace_truncated",
                    "trace_queue_dropped",
                    "trace_write_errors",
                    "trace_wake_errors",
                ]
                .iter()
                .all(|k| shutdown[0].n(k) == Some(0))
                && events.iter().all(|e| e.raw["truncated"] == false)
                && !events.iter().any(|e| {
                    matches!(
                        e.kind(),
                        "context_error"
                            | "retirement_error"
                            | "tick_reject"
                            | "binding_error"
                            | "bootstrap_error"
                    )
                });
            let runtime = !ticks.is_empty()
                && (0..=1).all(|n| ticks.iter().any(|e| e.n("ordinal") == Some(n)))
                && bad_ticks.is_empty()
                && !acks.is_empty()
                && bad_acks.is_empty()
                && lifecycle
                && persistence_ok
                && (process != 1 || !loading.is_empty())
                && loading.iter().all(|v| v["pass"] == true)
                && continued
                && (self.scenario["focus"]!="V12" || active_inputs.iter().all(|i|i["pass"]==true))
                && input_windows.iter().all(|w|w["pass"]==true)
                && trace_ok;
            pass &= runtime;
            process_proof.push(json!({"process":process,"runtime":runtime,"trace_index":trace_index.summary(),"tick_count":ticks.len(),"bad_tick_sequences":bad_ticks,
                "ack_count":acks.len(),"bad_ack_sequences":bad_acks,"join_sequences":join.iter().map(|e|e.seq()).collect::<Vec<_>>(),
                "device_allocations":allocated.iter().map(|e|&e.raw).collect::<Vec<_>>(),"device_retirements":retired.iter().map(|e|&e.raw).collect::<Vec<_>>(),
                "session_retirement":lifecycle,"persistence_calls":calls,"persistence_windows":windows,"persistence_complete":persistence_ok,
                "loading":loading,"first_menu_continue_preserved":continued,"active_inputs":active_inputs,"input_windows":input_windows,"trace_complete":trace_ok}));
        }
        // 后续窗口材料缺失时仍保留已完成进程的全量核对结果，不靠最终报告对象暂存。
        write_json(&self.out.join("evidence/session-processes.json"),
            &json!({"processes":process_proof,"mode":"FULL_INDEXED","pass_so_far":pass}))?;
        let labels = self.report["file_windows"]
            .as_object()
            .ok_or("missing operation file windows")?
            .keys()
            .cloned()
            .collect::<Vec<_>>();
        let mut file_windows = vec![];
        for before in labels.iter().filter(|s| s.ends_with(".before")) {
            let base = before.strip_suffix(".before").unwrap();
            let after = format!("{base}.after");
            let a = self.out.join("file-windows").join(before).join("snapshot");
            let b = self.out.join("file-windows").join(&after).join("snapshot");
            let comparison = saves::changes(&a, &b).and_then(|mut changes| {
                self.apply_recorded_baseline_between(&mut changes, &a, &b)?;
                Ok(changes)
            });
            let good = comparison.as_ref().is_ok_and(|c| {
                c.as_array()
                    .is_some_and(|a| a.iter().all(|v| v["allowed"] == true))
            }) && self.report["file_windows"][before]["error"].is_null()
                && self.report["file_windows"][&after]["error"].is_null()
                && stable_snapshot(&self.report["file_windows"][before])
                && stable_snapshot(&self.report["file_windows"][&after]);
            pass &= good;
            file_windows.push(json!({"before":before,"after":after,"changes":comparison.as_ref().ok(),"error":comparison.as_ref().err(),"pass":good,
                "concurrent_before":self.report["file_windows"][before]["possibly_writing"],
                "concurrent_after":self.report["file_windows"][&after]["possibly_writing"]}));
        }
        // 保存/Continue 另有显式首尾，不能因其命名不同而漏掉原始窗口。
        for (a, b) in [
            ("p1.before-save", "p1.after-save"),
            ("p1.after-save", "p1.after-continue"),
        ] {
            let left = self.out.join("file-windows").join(a).join("snapshot");
            let right = self.out.join("file-windows").join(b).join("snapshot");
            let comparison = saves::changes(&left, &right).and_then(|mut changes| {
                self.apply_recorded_baseline_between(&mut changes, &left, &right)?;
                Ok(changes)
            });
            let good = comparison.as_ref().is_ok_and(|c| {
                c.as_array()
                    .is_some_and(|v| v.iter().all(|v| v["allowed"] == true))
            }) && self.report["file_windows"][a]["error"].is_null()
                && self.report["file_windows"][b]["error"].is_null()
                && stable_snapshot(&self.report["file_windows"][a]) && stable_snapshot(&self.report["file_windows"][b]);
            pass &= good;
            file_windows.push(json!({"before":a,"after":b,"changes":comparison.as_ref().ok(),"error":comparison.as_ref().err(),"pass":good}));
        }
        let notifications = rows(&self.out.join("file-notifications.jsonl"))?;
        let notifications_ok = !notifications.is_empty()
            && notifications.iter().all(|v| v["event"] != "error")
            && notifications
                .iter()
                .filter(|v| v["event"] == "start")
                .count()
                == notifications
                    .iter()
                    .filter(|v| v["event"] == "stop")
                    .count();
        pass &= notifications_ok && file_windows.len() >= 6;
        let value = case(
            pass,
            true,
            json!({"processes":process_proof,"file_windows":file_windows,"notifications_complete":notifications_ok,
            "rules":"D18/D20/D27 unchanged; intermediate native calls and notification sequence retained; unknown cloud/ESSM child results remain UNKNOWN"}),
        );
        write_json(
            &self.out.join("evidence/session-and-persistence.json"),
            &value,
        )?;
        Ok(value)
    }
    pub(super) fn reduce_obligations_tx08(&mut self) -> Result<()> {
        let hook_result = hooks(&self.out, &self.report);
        self.report["cases"]["V1"] =
            hook_result.unwrap_or_else(|e| case(false, false, json!({"error":e})));
        self.report["cases"]["V5"] = episodes(&self.report);
        match captures(&self.out) {
            Ok(value) => {
                for key in ["V2", "V4", "V6", "V7"] {
                    self.report["cases"][key] = value[key].clone();
                }
                write_json(
                    &self.out.join("evidence/performance.json"),
                    &value["performance"],
                )?;
            }
            Err(error) => {
                for key in ["V2", "V4", "V6", "V7"] {
                    self.report["cases"][key] = case(false, false, json!({"error":error}));
                }
            }
        }
        Ok(())
    }
}
