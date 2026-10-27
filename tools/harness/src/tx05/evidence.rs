//! V1–V10 的证据归约；缺字段、坏记录和未完成场景均不视为通过。
use super::*;
use std::collections::{BTreeMap, BTreeSet};

// 解析 DLL 的 key=value 诊断，只在已知结构的事件中读取字段。
pub(crate) struct Event {
    pub(crate) raw: Value,
    fields: BTreeMap<String, String>,
}
// 第一、三档存档末尾依次为 Counter、Checksum 两个 u32（REPENTOGON SaveFile 日志的值与之相同）。
// 返回同长前后文件中变化字节所在的末尾字段；长度不同，或任一变化落在末尾 8 字节之外，返回 None。
fn trailer_fields(before: &[u8], after: &[u8]) -> Option<BTreeSet<&'static str>> {
    if before.len() != after.len() || before.len() < 8 {
        return None;
    }
    let len = before.len();
    let mut fields = BTreeSet::new();
    for (i, (x, y)) in before.iter().zip(after).enumerate() {
        if x == y {
            continue;
        }
        match len - i {
            5..=8 => fields.insert("counter"),
            1..=4 => fields.insert("checksum"),
            _ => return None,
        };
    }
    Some(fields)
}
impl Event {
    pub(crate) fn kind(&self) -> &str {
        self.raw["ev"].as_str().unwrap_or("")
    }
    pub(crate) fn text(&self, k: &str) -> &str {
        self.fields.get(k).map(String::as_str).unwrap_or("")
    }
    pub(crate) fn n(&self, k: &str) -> Option<u64> {
        self.text(k).parse().ok()
    }
    pub(crate) fn pair(&self, k: &str) -> Option<(i64, i64)> {
        let (x, y) = self.text(k).split_once(',')?;
        Some((x.parse().ok()?, y.parse().ok()?))
    }
    pub(crate) fn triple(&self, k: &str) -> Option<[u64; 3]> {
        let v = self
            .text(k)
            .split(',')
            .map(str::parse)
            .collect::<std::result::Result<Vec<u64>, _>>()
            .ok()?;
        v.try_into().ok()
    }
    pub(crate) fn seq(&self) -> u64 {
        self.raw["seq"].as_u64().unwrap_or(0)
    }
    pub(crate) fn key(&self) -> Option<(u64, u64)> {
        Some((self.n("generation")?, self.n("tick")?))
    }
}
pub(crate) fn parse_events(path: &std::path::Path) -> Result<Vec<Event>> {
    let mut events = vec![];
    let mut sequence = BTreeSet::new();
    for line in read(path)?.lines() {
        let raw: Value = serde_json::from_str(line).map_err(|e| format!("bad trace JSON: {e}"))?;
        let fields = raw["detail"]
            .as_str()
            .ok_or("missing trace detail")?
            .split_whitespace()
            .filter_map(|s| {
                s.split_once('=')
                    .map(|(k, v)| (k.to_string(), v.to_string()))
            })
            .collect();
        let seq = raw["seq"].as_u64().ok_or("missing trace sequence")?;
        if seq == 0 || !sequence.insert(seq) {
            return Err("trace sequence zero or duplicate".into());
        }
        events.push(Event { raw, fields });
    }
    if events.is_empty() {
        return Err("empty trace".into());
    }
    if sequence.last().copied() != Some(events.len() as u64) {
        return Err("trace sequence gap".into());
    }
    // 多生产者取得序号和入队之间可能被抢占；按逻辑序号还原，而不要求磁盘行序相同。
    events.sort_by_key(Event::seq);
    Ok(events)
}
pub(crate) fn checkpoint<'a>(r: &'a Value, label: &str) -> &'a Value {
    &r["checkpoints"][label]["status"]
}
pub(crate) fn value_n(v: &Value, k: &str) -> Option<u64> {
    v[k].as_str()?.parse().ok()
}
pub(crate) fn bounds(r: &Value, a: &str, b: &str) -> Option<(u64, u64)> {
    let (a, b) = (
        value_n(checkpoint(r, a), "frame")?,
        value_n(checkpoint(r, b), "frame")?,
    );
    (b > a).then_some((a, b))
}
pub(crate) fn in_bounds(loop_id: Option<u64>, window: Option<(u64, u64)>) -> bool {
    matches!((loop_id,window),(Some(l),Some((a,b))) if a<l && l<=b)
}
fn ticks_in<'a>(ticks: &[&'a Event], r: &Value, a: &str, b: &str) -> Vec<&'a Event> {
    ticks
        .iter()
        .copied()
        .filter(|e| in_bounds(e.n("loop"), bounds(r, a, b)))
        .collect()
}
pub(crate) fn unchanged(r: &Value, a: &str, b: &str, key: &str) -> bool {
    let (a, b) = (checkpoint(r, a), checkpoint(r, b));
    value_n(a, key)
        .zip(value_n(b, key))
        .is_some_and(|(x, y)| x == y)
        && value_n(a, "frame")
            .zip(value_n(b, "frame"))
            .is_some_and(|(x, y)| y > x)
}
// 取窗口前最后一份位置作为起点，窗口内实际记录作为终点；要求身份不变。
fn observations<'a>(lua: &'a [Value], window: Option<(u64, u64)>) -> Vec<&'a Value> {
    let Some((a, b)) = window else {
        return vec![];
    };
    let mut out = vec![];
    if let Some(prior) = lua
        .iter()
        .rev()
        .find(|v| v["event"] == "observation" && v["loop"].as_u64().is_some_and(|l| l <= a))
    {
        out.push(prior);
    }
    out.extend(lua.iter().filter(|v| {
        v["event"] == "observation" && v["loop"].as_u64().is_some_and(|l| a < l && l <= b)
    }));
    out
}
fn moved(obs: &[&Value], axis: &str, sign: f64) -> bool {
    if obs.len() < 2 {
        return false;
    }
    let (a, b) = (obs[0], obs[obs.len() - 1]);
    a["room"] == b["room"]
        && a[axis]
            .as_f64()
            .zip(b[axis].as_f64())
            .is_some_and(|(x, y)| (y - x) * sign > 2.0)
}
fn neutral_by_lead(ticks: &[&Event], r: &Value, release: &str, end: &str, axis: &str) -> bool {
    let selected = ticks_in(ticks, r, release, end);
    let Some(completed) = value_n(checkpoint(r, release), "completed_tick") else {
        return false;
    };
    selected
        .iter()
        .any(|e| e.pair(axis) == Some((0, 0)) && e.n("tick").is_some_and(|t| t <= completed + 3))
        && selected
            .iter()
            .filter(|e| e.n("tick").is_some_and(|t| t > completed + 3))
            .all(|e| e.pair(axis) == Some((0, 0)))
}

impl Run {
    // 单启动不制造新的关闭配置样本；历史 D18 仅对同源、完全相同的字节变化放行。
    pub(crate) fn apply_recorded_baseline_tx05(&mut self, changes: &mut Value) -> Result<()> {
        let before=self.out.join("save_before");let after=self.out.join("save_after");
        self.apply_recorded_baseline_between(changes,&before,&after)
    }
    // 窗口快照沿用同一 D20 字段规则，防止首尾净变化掩盖窗口内的非许可修改。
    pub(crate) fn apply_recorded_baseline_between(&mut self,changes:&mut Value,current_before:&std::path::Path,current_after:&std::path::Path)->Result<()> {
        let Some(relative) = self.scenario["save_baseline"].as_str() else {
            return Ok(());
        };
        let root = self.repo.join(relative);
        let result = (|| -> Result<()> {
            let baseline: Value = serde_json::from_str(&read(&root.join("report.json"))?)
                .map_err(|e| e.to_string())?;
            if baseline["tx"] != "tx-04-04"
                || baseline["pass"] != true
                || baseline["cases"]["E6"]["pass"] != true
                || baseline["environment"]["pe_timestamp"]
                    != self.report["environment"]["pe_timestamp"]
                || baseline["environment"]["save_roots"] != self.report["environment"]["save_roots"]
            {
                return Err("recorded baseline environment/disabled observation mismatch".into());
            }
            let ledger: Value = serde_json::from_str(&read(&root.join("save-changes.json"))?)
                .map_err(|e| e.to_string())?;
            let disabled = ledger
                .as_array()
                .ok_or("invalid baseline intervals")?
                .iter()
                .find(|v| v["launch"] == "disabled_or_cleanup")
                .ok_or("no disabled baseline interval")?;
            let before = disabled["before"]
                .as_str()
                .ok_or("baseline missing before")?;
            let after = disabled["after"].as_str().ok_or("baseline missing after")?;
            // 路径来自可编辑 artifact，必须限制在对应快照内。
            if before != "save_mid" || after != "save_after" {
                return Err("unexpected baseline snapshot paths".into());
            }
            let mut matched = vec![];
            for change in changes.as_array_mut().ok_or("invalid changes")? {
                let name = change["path"]
                    .as_str()
                    .ok_or("missing change path")?
                    .to_string();
                if change["allowed"] == true || !saves::other_slot(&name) {
                    continue;
                }
                if std::path::Path::new(&name)
                    .components()
                    .any(|c| !matches!(c, std::path::Component::Normal(_)))
                {
                    return Err("unsafe baseline relative path".into());
                }
                let observed = disabled["changes"].as_array().is_some_and(|c| {
                    c.iter()
                        .any(|v| v["path"] == name && v["bytes_changed"] == true)
                });
                let bytes = |p: std::path::PathBuf| -> Result<Option<Vec<u8>>> {
                    if p.is_file() {
                        fs::read(p).map(Some).map_err(|e| e.to_string())
                    } else if !p.exists() {
                        Ok(None)
                    } else {
                        Err("snapshot entry is not a file".into())
                    }
                };
                // D20：每次启动都会改写末尾计数与校验，历史前后字节不可能与本次相等。
                // 改为按字段比较：历史关闭配置启动与本次启动都必须存在前后两份文件，
                // 本次只改动历史也改动过的末尾字段；缺文件不会被当作“两边都不存在”的匹配。
                let historical = match (
                    bytes(root.join(before).join(&name))?,
                    bytes(root.join(after).join(&name))?,
                ) {
                    (Some(a), Some(b)) if observed => trailer_fields(&a, &b),
                    _ => None,
                };
                let current = match (
                    bytes(current_before.join(&name))?,
                    bytes(current_after.join(&name))?,
                ) {
                    (Some(a), Some(b)) => trailer_fields(&a, &b),
                    _ => None,
                };
                if let (Some(allowed), Some(actual)) = (historical, current) {
                    if !actual.is_empty() && actual.is_subset(&allowed) {
                        change["allowed"] = json!(true);
                        change["baseline"] = json!({"artifact":relative,
                            "rule":"D20: only trailing Counter/Checksum u32 fields changed, within the fields changed by the historical disabled launch",
                            "fields":actual,"historical_fields":allowed});
                        matched.push(name);
                    }
                }
            }
            self.report["recorded_baseline"] =
                json!({"artifact":relative,"matched":matched,"fresh_disabled_launch":false});
            Ok(())
        })();
        if let Err(e) = result {
            // 基线不可用时保持原严格结果，不终止清理与报告生成。
            self.report["recorded_baseline"] =
                json!({"artifact":relative,"unavailable":e,"fresh_disabled_launch":false});
        }
        Ok(())
    }
    pub(super) fn evaluate_tx05(&mut self) -> Result<()> {
        let events = parse_events(&self.out.join("enabled/trace.jsonl"))?;
        let lua = fixture_records(
            &read(&self.out.join("enabled/log.txt"))?,
            self.report["token"].as_str().ok_or("missing token")?,
        )?;
        write_json(&self.out.join("lua-observations.json"), &json!(lua))?;
        write_json(
            &self.out.join("checkpoints.json"),
            &self.report["checkpoints"],
        )?;
        let ticks = events
            .iter()
            .filter(|e| e.kind() == "tick")
            .collect::<Vec<_>>();
        let submits = events
            .iter()
            .filter(|e| e.kind() == "submit")
            .collect::<Vec<_>>();
        let r = self.report.clone();
        let status = &r["final_status"];
        let manifest: Value = serde_json::from_str(&read(&self.out.join("hook-manifest.json"))?)
            .map_err(|e| e.to_string())?;
        let expected = manifest["hooks"]
            .as_array()
            .ok_or("missing hook list")?
            .len() as u64;
        let caps = ["loop", "input", "player", "join", "control", "device"]
            .iter()
            .all(|g| status[format!("capability.{g}")] == "1");
        let installed = events.iter().any(|e| {
            e.kind() == "byte_patch" && e.text("id") == "odd_normal" && e.n("ok") == Some(1)
        });
        self.case("V1", caps && value_n(status,"registered")==Some(expected) && value_n(status,"pointers")==Some(8)
            && value_n(status,"odd_patch_installed")==Some(1) && installed,
            json!({"expected_registered":expected,"status":status,"odd_install_observed":installed}));

        let mut passthrough = vec![];
        for label in ["S1", "S10"] {
            let (a, b) = (format!("{label}.begin"), format!("{label}.end"));
            let obs = observations(&lua, bounds(&r, &a, &b));
            // S10 沿已知可通行路径回走，方向由场景记录，不能固定为正 X。
            let sign = if label == "S10" {
                match r["s10_key"].as_str() { Some("A") => -1.0, Some("D") => 1.0, _ => 0.0 }
            } else { 1.0 };
            let ok = sign != 0.0 && [
                "query_overrides",
                "ticks_standard",
                "ticks_odd",
                "completed_tick",
            ]
            .iter()
            .all(|k| unchanged(&r, &a, &b, k))
                && checkpoint(&r, &a)["session_state"] == "Idle"
                && checkpoint(&r, &b)["session_state"] == "Idle"
                && moved(&obs, "x", sign);
            passthrough.push(json!({"scene":label,"pass":ok,"observations":obs}));
        }
        self.case(
            "V2",
            passthrough.iter().all(|v| v["pass"] == true),
            json!(passthrough),
        );

        let session = events
            .iter()
            .filter(|e| e.kind() == "session")
            .collect::<Vec<_>>();
        let expected_states = [
            ("Idle", "Warmup"),
            ("Warmup", "Active"),
            ("Active", "Retiring"),
            ("Retiring", "Idle"),
        ];
        let transitions = session.len() == 8
            && session.iter().enumerate().all(|(i, e)| {
                let (a, b) = expected_states[i % 4];
                e.text("from") == a && e.text("to") == b
            });
        let g1 = value_n(checkpoint(&r, "S2.end"), "generation");
        let g2 = value_n(checkpoint(&r, "S11.active"), "generation");
        let rejected = events
            .iter()
            .filter(|e| e.kind() == "tick_reject")
            .map(|e| e.raw.clone())
            .collect::<Vec<_>>();
        let session_ok = transitions
            && g1.zip(g2).is_some_and(|(a, b)| b > a && a > 0)
            && session.get(2).is_some_and(|e| e.text("reason") == "Stop")
            && session
                .get(6)
                .is_some_and(|e| e.text("reason") == "RunTeardown")
            && rejected.is_empty();
        self.case("V3",session_ok,json!({"transitions":session.iter().map(|e|&e.raw).collect::<Vec<_>>(),"tick_reject":rejected,"generations":[g1,g2]}));

        // 每个已发布 tick 必须有唯一的 pre、commit；顺序、owner、loop 和 token 必须相同。
        let mut faults = vec![];
        let mut last = BTreeMap::<u64, u64>::new();
        let mut loops = BTreeSet::new();
        for tick in &ticks {
            let Some((generation, id)) = tick.key() else {
                faults.push(json!({"missing_tick_identity":tick.raw}));
                continue;
            };
            let phases = events
                .iter()
                .filter(|e| e.kind() == "tick_phase" && e.key() == tick.key())
                .collect::<Vec<_>>();
            let owner = match tick.text("kind") {
                "StandardRoom" => "Game",
                "OddPlayer" => "OddNormal",
                _ => "INVALID",
            };
            let ordered = phases.len() == 2
                && phases[0].text("phase") == "pre"
                && phases[1].text("phase") == "commit"
                && phases[0].seq() < phases[1].seq()
                && phases[1].seq() < tick.seq()
                && phases.iter().all(|p| {
                    p.n("loop") == tick.n("loop") && p.n("owner_token") == tick.n("owner_token")
                })
                && phases[0].text("owner") == owner;
            let expected_tick = last.get(&generation).copied().unwrap_or(0) + 1;
            let metrics_present = [
                "player_update_entered",
                "player_native_reached",
                "player_skipped_by_chain",
                "held",
                "edge",
                "input_sequence",
            ]
            .iter()
            .all(|k| tick.n(k).is_some())
                && tick.pair("move").is_some()
                && tick.pair("aim").is_some();
            if !ordered
                || !metrics_present
                || id != expected_tick
                || tick.text("owner") != owner
                || owner == "INVALID"
                || tick.n("owner_token").is_none_or(|n| n == 0)
                || !tick.n("loop").is_some_and(|l| loops.insert(l))
                || tick.n("authority_pre") != Some(1)
                || tick.n("native_commit") != Some(1)
                || tick.n("published") != Some(1)
            {
                faults.push(tick.raw.clone());
            }
            last.insert(generation, id);
        }
        let mut odd = BTreeMap::<u64, Vec<&Event>>::new();
        for e in events.iter().filter(|e| e.kind() == "odd_call") {
            odd.entry(e.n("token").unwrap_or(0)).or_default().push(e);
        }
        let odd_ok = !odd.is_empty()
            && odd.iter().all(|(token, calls)| {
                *token > 0
                    && calls.len() == 2
                    && calls[0].text("phase") == "enter"
                    && calls[1].text("phase") == "leave"
                    && calls[1].n("player_entries") == Some(1)
                    && calls[0].n("owner_token") == calls[1].n("owner_token")
                    && calls[0].n("loop") == calls[1].n("loop")
            });
        let nonordinary = events
            .iter()
            .filter(|e| e.kind() == "room_scope" || e.kind() == "odd_special_player")
            .collect::<Vec<_>>();
        let nonordinary_ok = nonordinary.iter().all(|e| {
            if e.kind() == "room_scope" {
                e.n("eligible") == Some(0)
                    && !ticks.iter().any(|t| {
                        t.n("loop") == e.n("loop")
                            && t.text("kind") == "StandardRoom"
                            && t.n("owner_token") == e.n("owner_token")
                    })
            } else {
                e.n("normal_marker") == Some(0) && e.n("tick") == Some(0)
            }
        });
        let stable = ticks_in(&ticks, &r, "S2.end", "S5.end");
        let both = ["StandardRoom", "OddPlayer"]
            .iter()
            .all(|k| stable.iter().any(|e| e.text("kind") == *k));
        let no_orphan_phases =
            events.iter().filter(|e| e.kind() == "tick_phase").count() == ticks.len() * 2;
        self.case("V4",!ticks.is_empty() && faults.is_empty() && odd_ok && nonordinary_ok && both && no_orphan_phases,
            json!({"ticks":ticks.len(),"faults":faults,"odd_pairs":odd.len(),"odd_exact_once":odd_ok,"both_stable_kinds":both,
                "nonordinary_observations":nonordinary.len(),"nonordinary_no_ticks":nonordinary_ok,"no_orphan_phases":no_orphan_phases,
                "special_odd_observations":nonordinary.iter().filter(|e|e.kind()=="odd_special_player").count()}));

        // 把 Resolve 的 sequence/target 与唯一 accepted 命令连接；本地 A 没有旁路证据。
        let command_matches = |tick: &&Event| {
            let matching = submits
                .iter()
                .filter(|s| {
                    s.n("decision") == Some(1)
                        && s.n("session_generation") == tick.n("generation")
                        && s.n("generation") == tick.n("a_generation")
                        && s.n("target") == tick.n("tick")
                        && s.n("sequence") == tick.n("input_sequence")
                })
                .collect::<Vec<_>>();
            matching.len() == 1
                && matching[0].pair("move") == tick.pair("move")
                && matching[0].pair("aim") == tick.pair("aim")
                && matching[0].n("edge") == tick.n("edge")
                && matching[0].seq() < tick.seq()
                && events.iter().any(|sample| {
                    sample.kind() == "sample"
                        && sample
                            .n("sample")
                            .zip(matching[0].n("sample"))
                            .is_some_and(|(a, b)| a <= b)
                        && sample.seq() < matching[0].seq()
                        && sample.pair("move") == tick.pair("move")
                })
        };
        let mut movement = vec![];
        for (label, axis, sign) in [("S3.right", "x", 1.0), ("S3.up", "y", -1.0)] {
            let (a, b) = (format!("{label}.begin"), format!("{label}.end"));
            let selected = ticks_in(&ticks, &r, &a, &b);
            let nonzero = selected
                .iter()
                .filter(|e| {
                    e.pair("move")
                        .is_some_and(|(x, y)| if axis == "x" { x > 0 } else { y < 0 })
                })
                .copied()
                .collect::<Vec<_>>();
            let reads = nonzero
                .iter()
                .any(|e| e.triple("nonzero0").is_some_and(|v| v[2] > 0));
            let obs = observations(&lua, bounds(&r, &a, &b));
            let release = neutral_by_lead(&ticks, &r, &format!("{label}.release"), &b, "move");
            let ok = !nonzero.is_empty()
                && nonzero.iter().all(command_matches)
                && reads
                && moved(&obs, axis, sign)
                && release;
            movement.push(json!({"scene":label,"pass":ok,"resolved_nonzero":nonzero.len(),"consumer_nonzero":reads,"neutral_by_lead_plus_one":release,"observations":obs}));
        }
        self.case(
            "V5",
            movement.iter().all(|v| v["pass"] == true),
            json!(movement),
        );

        let mut aiming = vec![];
        for (a, b, sign) in [
            ("S4.begin", "S4.right", 1),
            ("S4.right", "S4.left", -1),
            ("S4.left", "S4.fallback", 1),
        ] {
            let selected = ticks_in(&ticks, &r, a, b);
            let tail = selected.iter().rev().take(5).collect::<Vec<_>>();
            let ok = tail.len() == 5
                && tail
                    .iter()
                    .all(|e| e.pair("aim").is_some_and(|(x, y)| x * sign > 0 && y == 0))
                && selected.iter().any(|e| {
                    e.triple("nonzero4")
                        .is_some_and(|v| v.iter().sum::<u64>() > 0)
                });
            aiming.push(json!({"from":a,"to":b,"sign":sign,"pass":ok}));
        }
        let shot_obs = observations(&lua, bounds(&r, "S4.begin", "S4.end"));
        let tears = shot_obs.iter().any(|o| {
            o["tears"]
                .as_u64()
                .zip(shot_obs.first().and_then(|v| v["tears"].as_u64()))
                .is_some_and(|(a, b)| a > b)
        });
        let dispatch = events
            .iter()
            .filter(|e| e.kind() == "shooting_dispatch")
            .collect::<Vec<_>>();
        let no_tick_seen = dispatch
            .iter()
            .any(|e| e.n("tick") == Some(0) && e.n("original") == Some(0));
        let dispatch_ok = !dispatch.is_empty()
            && dispatch.iter().all(|e| match e.n("original") {
                Some(0) => true,
                Some(1) => {
                    e.n("tick").is_some_and(|t| t > 0)
                        && e.n("blocks") == Some(0)
                        && ticks
                            .iter()
                            .any(|t| t.n("tick") == e.n("tick") && t.n("loop") == e.n("loop"))
                }
                _ => false,
            });
        let normal_shooting = stable
            .iter()
            .filter(|e| e.n("player_native_reached").is_some_and(|n| n > 0))
            .all(|e| e.n("shooting_suppressed") == Some(0) && e.n("c4").is_some_and(|n| n > 0));
        self.case("V6",aiming.iter().all(|v|v["pass"]==true) && tears && dispatch_ok && normal_shooting
            && neutral_by_lead(&ticks,&r,"S4.release","S4.end","aim"),json!({"aim":aiming,"tears_increased":tears,"dispatch_valid":dispatch_ok,"normal_shooting":normal_shooting,"no_tick_suppression_observed":no_tick_seen}));

        let bomb_ticks = ticks
            .iter()
            .filter(|e| e.n("edge").is_some_and(|v| v & 1 != 0))
            .copied()
            .collect::<Vec<_>>();
        let bomb_submits = submits
            .iter()
            .filter(|e| e.n("decision") == Some(1) && e.n("edge").is_some_and(|v| v & 1 != 0))
            .collect::<Vec<_>>();
        let bomb_obs = observations(&lua, bounds(&r, "S5.begin", "S5.end"));
        let bomb_count = bomb_obs.first().zip(bomb_obs.last()).is_some_and(|(a, b)| {
            a["bombs"]
                .as_i64()
                .zip(b["bombs"].as_i64())
                .is_some_and(|(x, y)| x - y == 1)
        });
        let bomb_ok = bomb_ticks.len() == 1
            && bomb_submits.len() == 1
            && in_bounds(bomb_ticks[0].n("loop"), bounds(&r, "S5.begin", "S5.end"))
            && command_matches(&bomb_ticks[0])
            && bomb_ticks[0].n("bomb_queries").is_some_and(|n| n > 0)
            && bomb_ticks[0].text("bomb_query_value") == "1"
            && ticks
                .iter()
                .all(|e| e.n("bomb_query_mismatches") == Some(0))
            && bomb_count;
        self.case("V7",bomb_ok,json!({"resolved_edges":bomb_ticks.iter().map(|e|&e.raw).collect::<Vec<_>>(),"accepted_edges":bomb_submits.len(),"bombs_decreased_by_one":bomb_count,"observations":bomb_obs,
            "query_rule":"same-tick repeated reads allowed; mismatches must be zero"}));

        let mut blocking = vec![];
        for label in ["S6", "S7"] {
            let (a, b) = (format!("{label}.block.begin"), format!("{label}.block.end"));
            let selected = ticks_in(&ticks, &r, &a, &b);
            let (resume, end) = (format!("{label}.block.end"), format!("{label}.end"));
            let recovered = ticks_in(&ticks, &r, &resume, &end);
            let ok = unchanged(&r, &a, &b, "completed_tick")
                && unchanged(&r, &a, &b, "ticks_standard")
                && selected.is_empty()
                && !recovered.is_empty()
                && recovered.iter().all(|e| e.n("edge") == Some(0))
                && value_n(checkpoint(&r, &a), "blocks").is_some_and(|n| n & 1 != 0);
            blocking.push(json!({"scene":label,"pass":ok,"blocked_ticks":selected.len(),"resumed_ticks":recovered.len()}));
        }
        let skips = lua
            .iter()
            .filter(|v| v["event"] == "skip-update")
            .collect::<Vec<_>>();
        let skip_window = skips
            .first()
            .zip(skips.last())
            .and_then(|(a, b)| Some((a["loop"].as_u64()?, b["loop"].as_u64()?)));
        // end 在将要允许的 PRE_UPDATE 内，尚未发生该轮 Room；上一帧快照不包含 end 的 Room。
        let skipped = ticks
            .iter()
            .filter(|e| in_bounds(e.n("loop"), skip_window))
            .copied()
            .collect::<Vec<_>>();
        let skip_ok = skips.len() == 2
            && skips[0]["phase"] == "start"
            && skips[1]["phase"] == "end"
            && skips[1]["count"] == 30
            && skip_window.is_some_and(|(a, b)| b > a)
            && skipped.iter().all(|e| e.text("kind") != "StandardRoom");
        let held = ticks_in(&ticks, &r, "S7.block.end", "S7.resumed-held");
        let rebound = value_n(checkpoint(&r, "S7.begin"), "a_generation")
            .zip(value_n(checkpoint(&r, "S7.end"), "a_generation"))
            .is_some_and(|(a, b)| b > a);
        self.case("V8",blocking.iter().all(|v|v["pass"]==true) && skip_ok && rebound
            && held.iter().any(|e|e.pair("move").is_some_and(|(x,_)|x>0)),json!({"blocking":blocking,"skip":skips,"skip_valid":skip_ok,"odd_ticks_during_skip":skipped.iter().filter(|e|e.text("kind")=="OddPlayer").count(),"pause_rebound":rebound}));

        let transition = lua
            .iter()
            .filter(|v| v["event"] == "room-transition")
            .collect::<Vec<_>>();
        let transition_ok = transition.len() == 2
            && transition[0]["phase"] == "request"
            && transition[0]["animation"] == 0
            && transition[0]["dimension"] == -1
            && transition[1]["phase"] == "arrived"
            && transition[1]["target_matched"] == true
            && transition[0]["source"] != transition[1]["actual"];
        let room_bounds = bounds(&r, "S9.begin", "S9.arrived");
        let bindings = events
            .iter()
            .filter(|e| e.kind() == "binding" && in_bounds(e.n("loop"), room_bounds))
            .collect::<Vec<_>>();
        let invalidated = bindings.iter().position(|e| e.n("bound") == Some(0));
        let rebound = invalidated
            .is_some_and(|i| bindings.iter().skip(i + 1).any(|e| e.n("bound") == Some(1)));
        let loaded = events
            .iter()
            .filter(|e| e.kind() == "block_interval" && in_bounds(e.n("loop"), room_bounds))
            .collect::<Vec<_>>();
        // 直接从 block_interval 的 Loading 位构造区间，保留动画=0 无持续加载区间的事实。
        let mut loading_start = None;
        let mut loading_intervals = vec![];
        for e in &loaded {
            if e.n("to").is_some_and(|v| v & 2 != 0) && loading_start.is_none() {
                loading_start = e.n("loop");
            }
            if e.n("to").is_some_and(|v| v & 2 == 0) {
                if let Some(a) = loading_start.take() {
                    loading_intervals.push((a, e.n("loop").unwrap_or(a)));
                }
            }
        }
        let nonordinary_room = nonordinary
            .iter()
            .filter(|e| e.kind() == "room_scope" && in_bounds(e.n("loop"), room_bounds))
            .count();
        let loading_ok = loading_start.is_none()
            && (!loading_intervals.is_empty() || nonordinary_room > 0)
            && !ticks.iter().any(|t| {
                t.text("kind") == "StandardRoom"
                    && loading_intervals
                        .iter()
                        .any(|(a, b)| t.n("loop").is_some_and(|l| *a <= l && l < *b))
            });
        let room_obs = observations(&lua, bounds(&r, "S9.move.begin", "S9.move.end"));
        let room_ticks = ticks_in(&ticks, &r, "S9.move.begin", "S9.move.end");
        let sign = if r["post_room_key"] == "A" { -1 } else { 1 };
        let movement = room_ticks.iter().any(|e| {
            e.pair("move").is_some_and(|(x, _)| x * sign > 0)
                && command_matches(e)
                && e.triple("nonzero0").is_some_and(|v| v[2] > 0)
        }) && moved(&room_obs, "x", sign as f64);
        self.case("V9",transition_ok && rebound && loading_ok && movement,json!({"transition":transition,"binding_events":bindings.iter().map(|e|&e.raw).collect::<Vec<_>>(),
            "loading_intervals":loading_intervals,"nonordinary_room_calls":nonordinary_room,"loading_no_standard_ticks":loading_ok,"post_room_movement":movement,
            "final_position_order":"UNKNOWN; arrival log does not assert final native placement"}));

        let log = read(&self.out.join("enabled/repentogon.log"))?;
        let errors = log
            .lines()
            .filter(|l| l.contains("[ERROR]") || l.contains("[FATAL]"))
            .collect::<Vec<_>>();
        let lua_ok = !lua.is_empty()
            && lua
                .iter()
                .all(|v| v["event"] != "fixture-error" && v["loop"].as_u64().is_some())
            && lua
                .iter()
                .enumerate()
                .all(|(i, v)| v["serial"].as_u64() == Some(i as u64 + 1));
        let counters: Value = serde_json::from_str(&read(&self.out.join("counters.json"))?)
            .map_err(|e| e.to_string())?;
        let foreign_ok = counters.as_object().is_some_and(|c| {
            c.iter()
                .filter(|(k, _)| k.ends_with(".foreign"))
                .all(|(_, v)| v == "0")
        });
        let runtime_ok = [
            "trace_dropped",
            "context_faults",
            "foreign",
            "queue_dropped",
        ]
        .iter()
        .all(|k| value_n(status, k) == Some(0))
            && foreign_ok
            && status["session_error"] == ""
            && status["last_error"] == "";
        let save_ok = r["save_changes"]
            .as_array()
            .is_some_and(|v| v.iter().all(|c| c["allowed"] == true));
        let exit_ok = r["enabled_exit"] == 0
            && r["exit_hook"]["increased"] == true
            && checkpoint(&r, "S11.end")["session_state"] == "Idle"
            && checkpoint(&r, "S11.end")["run_active"] == "0"
            && ["completed_tick", "query_overrides"]
                .iter()
                .all(|k| unchanged(&r, "S11.teardown", "S11.end", k));
        let dumps_ok = r["crash_scan_error"].is_null()
            && r["crash_changes"].as_array().is_some_and(Vec::is_empty);
        self.case("V10",runtime_ok && lua_ok && save_ok && exit_ok && dumps_ok && errors.is_empty(),json!({"runtime_ok":runtime_ok,"lua_records_valid":lua_ok,
            "save_changes_allowed":save_ok,"normal_exit":exit_ok,"no_new_dumps":dumps_ok,"repentogon_errors":errors,"recorded_baseline":r["recorded_baseline"]}));
        write_json(
            &self.out.join("tick-evidence.json"),
            &json!({"ticks":ticks.iter().map(|e|&e.raw).collect::<Vec<_>>(),
            "submit":submits.iter().map(|e|&e.raw).collect::<Vec<_>>(),"phases":events.iter().filter(|e|e.kind()=="tick_phase").map(|e|&e.raw).collect::<Vec<_>>(),
            "rejected":rejected,"cases":self.report["cases"]}),
        )?;
        Ok(())
    }
}
