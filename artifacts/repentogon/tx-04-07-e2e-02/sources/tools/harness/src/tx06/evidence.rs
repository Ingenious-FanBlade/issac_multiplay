//! V1–V12 的唯一收尾归约；证据缺失、未运行、坏字段均不能变为 PASS。
use super::*;
use crate::{
    remote::protocol,
    tx05::evidence::{Event, bounds, checkpoint, in_bounds, parse_events, unchanged, value_n},
    write_json,
};
use std::collections::{BTreeMap, BTreeSet};

fn window<'a>(
    events: &'a [Event],
    r: &Value,
    a: &str,
    b: &str,
    kind: &str,
    ordinal: Option<u64>,
) -> Vec<&'a Event> {
    events
        .iter()
        .filter(|e| {
            e.kind() == kind
                && in_bounds(e.n("loop"), bounds(r, a, b))
                && ordinal.is_none_or(|o| e.n("ordinal") == Some(o))
        })
        .collect()
}
// 把 Lua 同一窗口内前后的 ordinal 位置与原生 tick 连接，房间身份变化不算移动。
fn moved(
    lua: &[Value],
    r: &Value,
    a: &str,
    b: &str,
    ordinal: usize,
    axis: &str,
    sign: f64,
) -> bool {
    let Some((first, last)) = bounds(r, a, b) else {
        return false;
    };
    let begin = lua
        .iter()
        .rev()
        .find(|v| v["event"] == "observation" && v["loop"].as_u64().is_some_and(|l| l <= first));
    let end = lua.iter().rev().find(|v| {
        v["event"] == "observation" && v["loop"].as_u64().is_some_and(|l| l <= last && l > first)
    });
    begin.zip(end).is_some_and(|(a, b)| {
        a["room"] == b["room"]
            && a["players"][ordinal][axis]
                .as_f64()
                .zip(b["players"][ordinal][axis].as_f64())
                .is_some_and(|(x, y)| (y - x) * sign > 2.0)
    })
}
// 任一非中性 Resolve 都必须连到同 ordinal、session、绑定代次、sequence、target 的 accepted。
fn matches_command(t: &Event, submits: &[&Event]) -> bool {
    let matches = submits
        .iter()
        .filter(|s| {
            s.n("decision") == Some(1)
                && s.n("ordinal") == t.n("ordinal")
                && s.n("session_generation") == t.n("generation")
                && s.n("generation") == t.n("participant_generation")
                && s.n("sequence") == t.n("input_sequence")
                && s.n("target") == t.n("tick")
        })
        .collect::<Vec<_>>();
    matches.len() == 1
        && matches[0].seq() < t.seq()
        && matches[0].pair("move") == t.pair("move")
        && matches[0].pair("aim") == t.pair("aim")
        && matches[0].n("held") == t.n("held")
        && matches[0].n("edge") == t.n("edge")
}
fn lua_records(text: &str, token: &str) -> Result<Vec<Value>> {
    let mut out = vec![];
    for line in text.lines() {
        if let Some((_, s)) = line.split_once("[isaac-cs-test] tx06 ") {
            let v: Value = serde_json::from_str(s).map_err(|e| format!("fixture_json: {e}"))?;
            if v["token"].as_str() == Some(token) {
                out.push(v);
            }
        }
    }
    Ok(out)
}
impl Run {
    pub(crate) fn evaluate_tx06(&mut self) -> Result<()> {
        let events = parse_events(&self.out.join("enabled/trace.jsonl"))?;
        let r = self.report.clone();
        let status = &r["final_status"];
        let lua = lua_records(
            &read(&self.out.join("enabled/log.txt"))?,
            r["token"].as_str().ok_or("token")?,
        )?;
        let ticks = events
            .iter()
            .filter(|e| e.kind() == "tick")
            .collect::<Vec<_>>();
        let submits = events
            .iter()
            .filter(|e| e.kind() == "submit")
            .collect::<Vec<_>>();
        // 在可能缺少后续场景前先保存已观察到的证据，失败现场同样可复查。
        write_json(&self.out.join("lua-observations.json"), &json!(lua))?;
        write_json(
            &self.out.join("tick-evidence.json"),
            &json!({"ticks":ticks.iter().map(|e|&e.raw).collect::<Vec<_>>(),
            "submits":submits.iter().map(|e|&e.raw).collect::<Vec<_>>()}),
        )?;
        write_json(
            &self.out.join("persistent-evidence.json"),
            &json!({"events":events.iter().filter(|e|e.kind()=="persistent").map(|e|&e.raw).collect::<Vec<_>>(),
            "note":"ST8 entry/condition/value observations; no readonly or dirty rollback"}),
        )?;
        let manifest: Value = serde_json::from_str(&read(&self.out.join("hook-manifest.json"))?)
            .map_err(|e| e.to_string())?;
        let expected = manifest["hooks"].as_array().ok_or("hook_manifest")?.len() as u64;
        let patches = manifest["byte_patches"].as_array().ok_or("byte_manifest")?;
        let installed = patches.iter().all(|p| {
            events
                .iter()
                .filter(|e| {
                    e.kind() == "byte_patch"
                        && Some(e.text("id")) == p["id"].as_str()
                        && e.n("ok") == Some(1)
                })
                .count()
                == 1
        });
        let caps = ["loop", "input", "player", "join", "control", "device"]
            .iter()
            .all(|g| status[format!("capability.{g}")] == "1");
        self.case("V1",caps && installed && value_n(status,"registered")==Some(expected) &&
            value_n(status,"pointers")==Some(8) && value_n(status,"host_patches_installed")==Some(1),
            json!({"expected_hooks":expected,"patches":patches,"all_installations_observed":installed}));

        let passthrough = [("S9.raw", "Idle", -1.0), ("S10.raw", "Hosting", 1.0)]
            .iter()
            .map(|(label, state, sign)| {
                let a = format!("{label}.begin");
                let b = format!("{label}.end");
                let ok = [
                    "completed_tick",
                    "query_overrides",
                    "ticks_standard",
                    "ticks_odd",
                ]
                .iter()
                .all(|k| unchanged(&r, &a, &b, k))
                    && checkpoint(&r, &a)["session_state"] == *state
                    && checkpoint(&r, &b)["session_state"] == *state
                    && checkpoint(&r, &a)["armed"] == "0"
                    && checkpoint(&r, &b)["armed"] == "0"
                    && moved(&lua, &r, &a, &b, 0, "x", *sign);
                json!({"scene":label,"state":state,"direction":sign,"pass":ok})
            })
            .collect::<Vec<_>>();
        self.case(
            "V2",
            passthrough.iter().all(|v| v["pass"] == true)
                && unchanged(&r, "S1.begin", "S1.end", "completed_tick")
                && unchanged(&r, "S1.begin", "S1.end", "query_overrides"),
            json!(passthrough),
        );

        let sessions = events
            .iter()
            .filter(|e| e.kind() == "session")
            .collect::<Vec<_>>();
        let sequence = [
            ("Idle", "Hosting"),
            ("Hosting", "Warmup"),
            ("Warmup", "Active"),
            ("Active", "Retiring"),
            ("Retiring", "Idle"),
        ];
        let transitions = sessions.len() == 15
            && sessions.iter().enumerate().all(|(i, e)| {
                e.text("from") == sequence[i % 5].0 && e.text("to") == sequence[i % 5].1
            });
        let causes = ["Stop", "PeerTimeout", "RunTeardown"];
        let generations = ["S3.end", "S10.active", "S11.active"]
            .map(|s| value_n(checkpoint(&r, s), "generation"));
        let causes_ok = causes.iter().enumerate().all(|(i, c)| {
            sessions
                .get(i * 5 + 3)
                .is_some_and(|e| e.text("reason") == *c)
                && sessions
                    .get(i * 5 + 4)
                    .is_some_and(|e| e.text("reason") == *c)
        });
        let generations_ok = generations.iter().all(|g| g.is_some_and(|v| v > 0))
            && generations.windows(2).all(|g| g[0] < g[1]);
        self.case("V3",transitions && causes_ok && generations_ok && !events.iter().any(|e|e.kind()=="tick_reject"),
            json!({"transitions":sessions.iter().map(|e|&e.raw).collect::<Vec<_>>(),"generations":generations}));

        // 原始包全量保留；独立 codec 复读每份收发记录，收尾计数与 DLL 停止摘要对齐。
        let stops = events
            .iter()
            .filter(|e| e.kind() == "net" && e.text("event") == "stop")
            .collect::<Vec<_>>();
        let mut network = vec![];
        for round in 1..=3 {
            // 某轮未执行或日志损坏只使本轮不通过，不中止其余 V 项的归约。
            let records = match read(&self.out.join(format!("remote-{round}.jsonl"))).and_then(|t| {
                t.lines()
                    .map(|l| serde_json::from_str::<Value>(l).map_err(|e| e.to_string()))
                    .collect::<Result<Vec<_>>>()
            }) {
                Ok(records) => records,
                Err(e) => {
                    network.push(json!({"round":round,"pass":false,"status":"NOT_RUN","error":e}));
                    continue;
                }
            };
            let packets = records
                .iter()
                .filter(|v| v["event"] == "send" || v["event"] == "receive")
                .collect::<Vec<_>>();
            let packet_ok = !packets.is_empty()
                && packets.iter().all(|v| {
                    v["bytes"].as_str().is_some_and(|s| {
                        protocol::unhex(s)
                            .and_then(|b| protocol::decode(&b))
                            .is_ok()
                    })
                })
                && !records
                    .iter()
                    .any(|v| v["event"] == "send_error" || v["classification"] == "error");
            let summary = records.last().filter(|v| v["event"] == "summary");
            let stop = stops.get(round - 1).copied();
            let counts = summary.zip(stop).is_some_and(|(s, h)| {
                s["tx"].as_u64() == h.n("rx")
                    && s["rx"].as_u64() == h.n("tx")
                    && s["establishments"] == 1
                    && s["error"].is_null()
                    && s["bye"].as_u64() == Some([1, 3, 5][round - 1])
                    && h.n("foreign") == Some(0)
                    && h.n("rx_drop") == Some(0)
                    && h.n("tx_drop") == Some(0)
                    && h.n("bye_attempts") == Some(3)
            });
            network.push(json!({"round":round,"pass":packet_ok&&counts,"packet_codec":packet_ok,"counts_aligned":counts,
                "remote":summary,"host":stop.map(|e|&e.raw)}));
        }
        self.case(
            "V4",
            network.iter().all(|v| v["pass"] == true),
            json!(network),
        );

        let mut joined = vec![];
        for (i, label) in ["S3.end", "S10.active", "S11.active"].iter().enumerate() {
            let s = checkpoint(&r, label);
            let obs = &r["checkpoints"][*label]["observation"];
            let b = &obs["players"][1];
            let ok = s["players"] == "2"
                && s["a_bound"] == "1"
                && s["b_bound"] == "1"
                && s["join_creations"] == "1"
                && s["device_k_state"] == "Registered"
                && obs["player_count"] == 2
                && b["type"] == 1
                && b["variant"] == 0
                && b["player_type"] == 0
                && b["ghost"] == false
                && b["dead"] == false
                && b["controller"].as_u64() == value_n(s, "device_k_id");
            joined.push(json!({"round":i+1,"pass":ok,"observation":obs}));
        }
        let modes = events
            .iter()
            .filter(|e| e.kind() == "join" && e.text("event") == "mode")
            .collect::<Vec<_>>();
        let modes_ok = !modes.is_empty()
            && modes.iter().all(|e| {
                e.n("native_calls") == Some(1)
                    && e.n("result") == Some(0)
                    && e.n("site").is_some_and(|s| (1..=3).contains(&s))
            });
        let created = events
            .iter()
            .filter(|e| e.kind() == "join" && e.text("event") == "create")
            .count()
            == 3;
        self.case("V5",joined.iter().all(|v|v["pass"]==true) && modes_ok && created &&
            events.iter().filter(|e|e.kind()=="device_k" && e.text("event")=="registered").count()==3 &&
            events.iter().filter(|e|e.kind()=="join" && e.text("event")=="complete").count()==3,
            json!({"rounds":joined,"predicate_calls":modes.iter().map(|e|&e.raw).collect::<Vec<_>>(),"creation_count":created}));

        let lead = self.scenario["remote_lead"].as_u64().unwrap_or(2);
        let mut movement = vec![];
        for (label, axis, sign) in [("S4.right", "x", 1), ("S4.up", "y", -1)] {
            let a = format!("{label}.begin");
            let b = format!("{label}.end");
            let selected = window(&events, &r, &a, &b, "tick", Some(1));
            let base = r["scripts"][label]["base"].as_u64();
            let active = selected
                .iter()
                .filter(|e| {
                    e.pair("move")
                        .is_some_and(|(x, y)| if axis == "x" { x > 0 } else { y < 0 })
                })
                .collect::<Vec<_>>();
            let release = base.is_some_and(|base| {
                selected.iter().any(|e| {
                    e.n("tick")
                        .is_some_and(|t| t >= base + 24 && t <= base + 24 + lead + 1)
                        && e.pair("move") == Some((0, 0))
                }) && selected
                    .iter()
                    .filter(|e| e.n("tick").is_some_and(|t| t > base + 24 + lead + 1))
                    .all(|e| e.pair("move") == Some((0, 0)))
            });
            let ok = !active.is_empty()
                && active.iter().all(|t| matches_command(t, &submits))
                && active
                    .iter()
                    .any(|e| e.triple("nonzero0").is_some_and(|n| n[2] > 0))
                && release
                && moved(&lua, &r, &a, &b, 1, axis, sign as f64);
            movement.push(json!({"scene":label,"pass":ok,"release_by_lead_plus_one":release,"active_ticks":active.len()}));
        }
        self.case(
            "V6",
            movement.iter().all(|v| v["pass"] == true),
            json!(movement),
        );
        let simultaneous = window(&events, &r, "S5.begin", "S5.end", "tick", None);
        let simultaneous_ok = simultaneous
            .iter()
            .filter(|e| e.n("ordinal") == Some(0))
            .any(|a| {
                a.pair("move").is_some_and(|(x, _)| x > 0)
                    && a.pair("aim") == Some((0, 0))
                    && matches_command(a, &submits)
                    && simultaneous.iter().any(|b| {
                        b.n("ordinal") == Some(1)
                            && b.key() == a.key()
                            && b.pair("move") == Some((0, 0))
                            && b.pair("aim").is_some_and(|(x, y)| x > 0 && y == 0)
                            && b.triple("nonzero4")
                                .is_some_and(|v| v.iter().any(|n| *n > 0))
                            && matches_command(b, &submits)
                    })
            });
        self.case(
            "V7",
            simultaneous_ok,
            json!({"same_tick_isolated_inputs":simultaneous_ok,"ticks":simultaneous.len()}),
        );

        let bomb_ticks = ticks
            .iter()
            .filter(|e| e.n("ordinal") == Some(1) && e.n("edge").is_some_and(|n| n & 1 != 0))
            .collect::<Vec<_>>();
        let bomb_submits = submits
            .iter()
            .filter(|e| {
                e.n("ordinal") == Some(1)
                    && e.n("decision") == Some(1)
                    && e.n("edge").is_some_and(|n| n & 1 != 0)
            })
            .collect::<Vec<_>>();
        let bomb_delta = r["checkpoints"]["S6.begin"]["observation"]["players"][1]["bombs"]
            .as_i64()
            .zip(r["checkpoints"]["S6.end"]["observation"]["players"][1]["bombs"].as_i64())
            .is_some_and(|(a, b)| a - b == 1);
        let bomb_ok = bomb_ticks.len() == 1
            && bomb_submits.len() == 1
            && bomb_delta
            && matches_command(bomb_ticks[0], &submits)
            && bomb_ticks[0].n("bomb_queries").is_some_and(|n| n > 0)
            && bomb_ticks[0].text("bomb_query_value") == "1"
            && ticks
                .iter()
                .all(|e| e.n("bomb_query_mismatches") == Some(0));
        self.case("V8",bomb_ok,json!({"resolved":bomb_ticks.len(),"accepted":bomb_submits.len(),"bomb_delta_one":bomb_delta}));

        let overlay = window(&events, &r, "S7.block.begin", "S7.block.end", "tick", None);
        let resumed = window(&events, &r, "S7.block.end", "S7.end", "tick", Some(1));
        let transitions = lua
            .iter()
            .filter(|v| v["event"] == "room-transition")
            .collect::<Vec<_>>();
        let room_ok = transitions.len() == 2
            && transitions[0]["phase"] == "request"
            && transitions[0]["animation"] == 0
            && transitions[1]["phase"] == "arrived"
            && transitions[1]["target_matched"] == true;
        let skips = lua
            .iter()
            .filter(|v| v["event"] == "skip-update")
            .collect::<Vec<_>>();
        let skip_window = skips
            .first()
            .zip(skips.last())
            .and_then(|(a, b)| Some((a["loop"].as_u64()?, b["loop"].as_u64()?)));
        let skip_ok = skips.len() == 2
            && skips[0]["phase"] == "start"
            && skips[1]["phase"] == "end"
            && skips[1]["count"] == 30
            && skip_window.is_some_and(|(a, b)| b > a)
            && !ticks
                .iter()
                .any(|t| t.text("kind") == "StandardRoom" && in_bounds(t.n("loop"), skip_window));
        let mut rebound = true;
        for ordinal in 0..2 {
            let key = if ordinal == 0 {
                "a_generation"
            } else {
                "b_generation"
            };
            rebound &= value_n(checkpoint(&r, "S8.before-room"), key)
                .zip(value_n(checkpoint(&r, "S8.arrived"), key))
                .is_some_and(|(a, b)| b > a);
            let binding = window(
                &events,
                &r,
                "S8.before-room",
                "S8.arrived",
                "binding",
                Some(ordinal),
            );
            rebound &= binding
                .iter()
                .position(|e| e.n("bound") == Some(0))
                .is_some_and(|i| binding.iter().skip(i + 1).any(|e| e.n("bound") == Some(1)));
        }
        let mut loading_start = None;
        let mut loading = vec![];
        for e in window(
            &events,
            &r,
            "S8.before-room",
            "S8.arrived",
            "block_interval",
            None,
        ) {
            if e.n("to").is_some_and(|v| v & 2 != 0) && loading_start.is_none() {
                loading_start = e.n("loop");
            }
            if e.n("to").is_some_and(|v| v & 2 == 0) {
                if let Some(a) = loading_start.take() {
                    loading.push((a, e.n("loop").unwrap_or(a)));
                }
            }
        }
        let no_loading_ticks = loading_start.is_none()
            && !ticks.iter().any(|t| {
                t.text("kind") == "StandardRoom"
                    && loading
                        .iter()
                        .any(|(a, b)| t.n("loop").is_some_and(|l| *a <= l && l < *b))
            });
        let loading_observed = !loading.is_empty()
            || !window(
                &events,
                &r,
                "S8.before-room",
                "S8.arrived",
                "room_scope",
                None,
            )
            .is_empty();
        let sign = r["post_room_sign"].as_i64().unwrap_or(0);
        let room_movement = sign != 0
            && moved(
                &lua,
                &r,
                "S8.move.begin",
                "S8.move.end",
                1,
                "x",
                sign as f64,
            )
            && window(&events, &r, "S8.move.begin", "S8.move.end", "tick", Some(1))
                .iter()
                .any(|t| {
                    t.pair("move").is_some_and(|(x, _)| x * sign > 0)
                        && matches_command(t, &submits)
                });
        let blocking_ok = overlay.is_empty()
            && unchanged(&r, "S7.block.begin", "S7.block.end", "completed_tick")
            && value_n(checkpoint(&r, "S7.block.begin"), "blocks").is_some_and(|b| b & 1 != 0)
            && !resumed.is_empty()
            && resumed.iter().all(|e| {
                e.n("edge") == Some(0)
                    && e.pair("move") == Some((0, 0))
                    && e.pair("aim") == Some((0, 0))
            });
        self.case("V9",blocking_ok && room_ok && skip_ok && rebound && no_loading_ticks && loading_observed && room_movement,
            json!({"overlay":blocking_ok,"room_transition":room_ok,"both_rebound":rebound,"loading":loading,
                "skip_updates":skip_ok,"loading_observed":loading_observed,"loading_no_standard_ticks":no_loading_ticks,"post_room_movement":room_movement}));

        let retired=["S9.retired","S10.end","S11.end"].iter().enumerate().map(|(i,label)|{
            let s=checkpoint(&r,label);let count=if i==2{"0"}else{"1"};
            json!({"round":i+1,"pass":s["session_state"]=="Idle" && s["device_k_state"]=="Removed" &&
                s["players"]==count && s["b_bound"]=="0" && s["peer_running"]=="0" && s["peer_stop_finished"]=="1" &&
                s["cleanup_failed"]=="0" && s["retirement_cause"]==causes[i],"status":s})
        }).collect::<Vec<_>>();
        let deletes = events
            .iter()
            .filter(|e| e.kind() == "device_k" && e.text("event") == "delete")
            .collect::<Vec<_>>();
        let neutral = events
            .iter()
            .filter(|e| e.kind() == "device_k" && e.text("event") == "retiring")
            .count()
            == 3;
        self.case("V10",retired.iter().all(|v|v["pass"]==true) && deletes.len()==3 && neutral &&
            deletes.iter().all(|e|e.n("never")==Some(1)),json!({"rounds":retired,"deletes":deletes.iter().map(|e|&e.raw).collect::<Vec<_>>(),"neutral":neutral}));

        // 一个 Authority tick 对应两条 ordinal 汇总、唯一 pre/commit 和共同 owner/token。
        let mut grouped = BTreeMap::<(u64, u64), Vec<&Event>>::new();
        for t in &ticks {
            if let Some(key) = t.key() {
                grouped.entry(key).or_default().push(t);
            }
        }
        let mut faults = vec![];
        let mut last = BTreeMap::<u64, u64>::new();
        let mut loops = BTreeSet::new();
        for (&(generation, tick), pair) in &grouped {
            let phases = events
                .iter()
                .filter(|e| e.kind() == "tick_phase" && e.key() == Some((generation, tick)))
                .collect::<Vec<_>>();
            let kind = pair[0].text("kind");
            let owner = if kind == "StandardRoom" {
                "Game"
            } else if kind == "OddPlayer" {
                "OddNormal"
            } else {
                "INVALID"
            };
            let ok = pair.len() == 2
                && pair[0].n("ordinal") == Some(0)
                && pair[1].n("ordinal") == Some(1)
                && phases.len() == 2
                && phases[0].text("phase") == "pre"
                && phases[1].text("phase") == "commit"
                && phases[0].seq() < phases[1].seq()
                && phases[1].seq() < pair[0].seq()
                && owner != "INVALID"
                && phases[0].text("owner") == owner
                && tick == last.get(&generation).copied().unwrap_or(0) + 1
                && pair[0].n("loop").is_some_and(|l| loops.insert(l))
                && pair.iter().all(|t| {
                    t.n("loop") == pair[0].n("loop")
                        && t.n("owner_token") == pair[0].n("owner_token")
                        && t.n("owner_token").is_some_and(|n| n > 0)
                        && t.text("owner") == owner
                        && t.text("kind") == kind
                        && phases.iter().all(|p| {
                            p.n("loop") == t.n("loop") && p.n("owner_token") == t.n("owner_token")
                        })
                        && t.n("authority_pre") == Some(1)
                        && t.n("native_commit") == Some(1)
                        && t.n("published") == Some(1)
                        && t.n("player_update_entered")
                            .zip(t.n("player_native_reached"))
                            .zip(t.n("player_skipped_by_chain"))
                            .is_some_and(|((entered, native), skipped)| entered == native + skipped)
                        && t.pair("move").is_some()
                        && t.pair("aim").is_some()
                        && t.n("edge").is_some()
                        && t.n("held").is_some()
                });
            if !ok {
                faults.push(json!({"key":[generation,tick],"pair":pair.iter().map(|e|&e.raw).collect::<Vec<_>>()}));
            }
            last.insert(generation, tick);
        }
        let mut odd = BTreeMap::<u64, Vec<&Event>>::new();
        for e in events.iter().filter(|e| e.kind() == "odd_call") {
            odd.entry(e.n("token").unwrap_or(0)).or_default().push(e);
        }
        let odd_ok = !odd.is_empty()
            && odd.iter().all(|(token, p)| {
                *token > 0
                    && p.len() == 2
                    && p[0].text("phase") == "enter"
                    && p[1].text("phase") == "leave"
                    && p[0].n("owner_token") == p[1].n("owner_token")
                    && p[0].n("loop") == p[1].n("loop")
            });
        let nonordinary = events
            .iter()
            .filter(|e| e.kind() == "room_scope" || e.kind() == "odd_special_player")
            .all(|e| {
                if e.kind() == "odd_special_player" {
                    e.n("normal_marker") == Some(0) && e.n("tick") == Some(0)
                } else {
                    !ticks.iter().any(|t| {
                        t.text("kind") == "StandardRoom"
                            && t.n("loop") == e.n("loop")
                            && t.n("owner_token") == e.n("owner_token")
                    })
                }
            });
        let both = ["StandardRoom", "OddPlayer"]
            .iter()
            .all(|k| ticks.iter().any(|t| t.text("kind") == *k));
        self.case("V11",!grouped.is_empty() && faults.is_empty() && ticks.len()==grouped.len()*2 &&
            events.iter().filter(|e|e.kind()=="tick_phase").count()==grouped.len()*2 && odd_ok && nonordinary && both,
            json!({"ticks":grouped.len(),"faults":faults,"odd_pairs":odd.len(),"odd_pairs_valid":odd_ok,"both_kinds":both,"nonordinary":nonordinary}));

        let persistent = events
            .iter()
            .filter(|e| e.kind() == "persistent")
            .collect::<Vec<_>>();
        let writes = persistent
            .iter()
            .filter(|e| e.text("phase") == "return" && e.text("operation") != "none")
            .collect::<Vec<_>>();
        let no_added_writes = writes.iter().all(|e| {
            e.n("known") == Some(1)
                && e.n("normal") == Some(1)
                && !["pgd_local", "pgd_cloud", "button_maps"].contains(&e.text("entry"))
                && e.n("before") == e.n("after")
                && e.n("dirty_before") == e.n("dirty_after")
        });
        let observers = [
            "pgd_unlock",
            "pgd_counter",
            "pgd_local",
            "pgd_cloud",
            "button_maps",
        ]
        .iter()
        .all(|id| {
            events.iter().any(|e| {
                e.kind() == "hook_registered"
                    && e.raw["detail"]
                        .as_str()
                        .is_some_and(|d| d.starts_with(&format!("{id} ")))
            })
        });
        let returns = persistent
            .iter()
            .filter(|e| e.text("phase") == "return")
            .count();
        let persistent_ok = observers
            && no_added_writes
            && !persistent.is_empty()
            && returns
                == persistent
                    .iter()
                    .filter(|e| e.text("phase") == "enter")
                    .count()
            && persistent.iter().all(|e| e.n("known") == Some(1));
        let lua_ok = !lua.is_empty()
            && lua.iter().enumerate().all(|(i, v)| {
                v["event"] != "fixture-error"
                    && v["serial"].as_u64() == Some(i as u64 + 1)
                    && v["loop"].as_u64().is_some()
            });
        // 未到达收尾时没有 counters.json：记为缺失使 V12 不通过，而不是中止归约。
        let counters: Value = read(&self.out.join("counters.json"))
            .ok()
            .and_then(|t| serde_json::from_str(&t).ok())
            .unwrap_or(Value::Null);
        let foreign = counters.as_object().is_some_and(|c| {
            c.iter()
                .filter(|(k, _)| k.ends_with(".foreign"))
                .all(|(_, v)| v == "0")
        });
        let runtime = [
            "trace_dropped",
            "context_faults",
            "foreign",
            "queue_dropped",
        ]
        .iter()
        .all(|k| value_n(status, k) == Some(0))
            && foreign
            && status["session_error"] == ""
            && status["last_error"] == "";
        let saves = r["save_changes"]
            .as_array()
            .is_some_and(|v| v.iter().all(|c| c["allowed"] == true));
        let no_dumps = r["crash_scan_error"].is_null()
            && r["crash_changes"].as_array().is_some_and(Vec::is_empty);
        let rgon = read(&self.out.join("enabled/repentogon.log"))?;
        let errors = rgon
            .lines()
            .filter(|l| l.contains("[ERROR]") || l.contains("[FATAL]"))
            .collect::<Vec<_>>();
        self.case("V12",runtime && saves && no_dumps && lua_ok && persistent_ok && errors.is_empty() &&
            r["enabled_exit"]==0 && checkpoint(&r,"S11.end")["run_active"]=="0",
            json!({"runtime":runtime,"counters_present":!counters.is_null(),"lua":lua_ok,"save_changes_allowed":saves,"no_dumps":no_dumps,
                "persistent_observers":observers,"persistent_calls":persistent.len(),"no_cs_added_writes":no_added_writes,
                "persistent_process_evidence":persistent_ok,"repentogon_errors":errors,"recorded_baseline":r["recorded_baseline"]}));
        write_json(
            &self.out.join("tick-evidence.json"),
            &json!({"ticks":ticks.iter().map(|e|&e.raw).collect::<Vec<_>>(),
            "submits":submits.iter().map(|e|&e.raw).collect::<Vec<_>>(),"cases":self.report["cases"]}),
        )?;
        write_json(
            &self.out.join("persistent-evidence.json"),
            &json!({"events":persistent.iter().map(|e|&e.raw).collect::<Vec<_>>(),
            "note":"ST8 entry/condition/value observations plus D18/D20 byte comparisons; no readonly or dirty rollback"}),
        )?;
        Ok(())
    }
}
