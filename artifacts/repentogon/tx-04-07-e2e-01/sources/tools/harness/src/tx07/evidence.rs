//! V1–V16 仅在唯一整体 E2E 收尾归约；缺字段、未自然观察和未执行分别保留。
use super::*;
use crate::{
    read,
    remote::protocol,
    tx05::evidence::{Event, bounds, checkpoint, in_bounds, parse_events, unchanged, value_n},
    write_json,
};
use std::collections::{BTreeMap, BTreeSet};

fn records(path: &std::path::Path) -> Result<Vec<Value>> {
    read(path)?
        .lines()
        .map(|l| serde_json::from_str(l).map_err(|e| e.to_string()))
        .collect()
}
fn window<'a>(
    events: &'a [Event],
    report: &Value,
    a: &str,
    b: &str,
    kind: &str,
    ordinal: Option<u64>,
) -> Vec<&'a Event> {
    events
        .iter()
        .filter(|e| {
            e.kind() == kind
                && in_bounds(e.n("loop"), bounds(report, a, b))
                && ordinal.is_none_or(|o| e.n("ordinal") == Some(o))
        })
        .collect()
}

impl Run {
    // 推进率只用 DLL 自身微秒时钟与 completed；暂停/加载不得混入预设 Active 窗口。
    fn fault_evidence_tx07(
        &mut self,
        events: &[Event],
        r: &Value,
        remotes: &[(u64, Vec<Value>)],
        relays: &[(u64, Vec<Value>)],
    ) -> Result<()> {
        let rate = |a: &str, b: &str| -> Option<Value> {
            let selected = window(events, r, a, b, "tick", Some(0));
            let (first, last) = (*selected.first()?, *selected.last()?);
            let elapsed = last.raw["t"]
                .as_u64()?
                .checked_sub(first.raw["t"].as_u64()?)?;
            let advance = last.n("tick")?.checked_sub(first.n("tick")?)?;
            let blocked = window(events, r, a, b, "block_interval", None)
                .iter()
                .any(|e| e.n("to").is_some_and(|n| n != 0));
            Some(
                json!({"microseconds":elapsed,"completed":advance,"ticks_per_second":advance as f64*1000000.0/elapsed.max(1) as f64,
                "active":!blocked && checkpoint(r,a)["session_state"]=="Active" && checkpoint(r,b)["session_state"]=="Active",
                "adequate":elapsed>=9000000,"first":first.raw,"last":last.raw}),
            )
        };
        let clean = rate("S7.clean.begin", "S7.clean.end");
        let fault = rate("S7.begin", "S7.end");
        let ratio = clean.as_ref().zip(fault.as_ref()).and_then(|(a, b)| {
            Some(b["ticks_per_second"].as_f64()? / a["ticks_per_second"].as_f64()?)
        });
        let rate_ok = clean.as_ref().zip(fault.as_ref()).is_some_and(|(a, b)| {
            a["active"] == true
                && b["active"] == true
                && a["adequate"] == true
                && b["adequate"] == true
        }) && ratio.is_some_and(|r| r.is_finite() && (0.8..=1.2).contains(&r));
        let moderate = relays
            .iter()
            .find(|(round, _)| *round == 1)
            .map(|(_, v)| {
                v.iter()
                    .filter(|v| v["event"] == "receive" && v["profile"] == "moderate")
                    .collect::<Vec<_>>()
            })
            .unwrap_or_default();
        let hits = [
            (
                "drop",
                moderate.iter().filter(|v| v["decision"] == "drop").count(),
            ),
            (
                "duplicate",
                moderate.iter().filter(|v| v["duplicate"] == true).count(),
            ),
            (
                "reorder",
                moderate.iter().filter(|v| v["reorder"] == true).count(),
            ),
        ];
        let hits_observed = hits.iter().all(|(_, count)| *count > 0)
            && delta(r, "S7.begin", "S7.end", "net_duplicate").is_some_and(|v| v > 0)
            && delta(r, "S7.begin", "S7.end", "net_stale").is_some_and(|v| v > 0);
        let protocol_ok = ["S7.end", "S8.end", "S17.end"].iter().all(|label| {
            checkpoint(r, label).as_object().is_some_and(|s| {
                let errors = s
                    .iter()
                    .filter(|(k, _)| k.starts_with("net_error."))
                    .collect::<Vec<_>>();
                !errors.is_empty() && errors.iter().all(|(_, v)| **v == "0")
            }) && checkpoint(r, label)["session_state"] == "Active"
        }) && !events
            .iter()
            .any(|e| e.text("reason") == "PeerProtocol" || e.text("terminal") == "PeerProtocol");
        let fault_ticks = window(events, r, "S7.begin", "S7.end", "tick", Some(1));
        let outage_ticks = window(events, r, "S8.begin", "S8.end", "tick", Some(1));
        let missing = fault_ticks
            .iter()
            .chain(outage_ticks.iter())
            .filter(|e| e.n("input_sequence") == Some(0))
            .collect::<Vec<_>>();
        let invariant = !fault_ticks.is_empty()
            && !missing.is_empty()
            && missing.iter().all(|e| neutral(e))
            && fault_ticks
                .iter()
                .chain(outage_ticks.iter())
                .filter(|e| !neutral(e))
                .all(|e| matches_command(e, events));
        let short = unchanged(r, "S8.begin", "S8.end", "b_generation")
            && unchanged(r, "S8.begin", "S8.end", "b_rebinds")
            && unchanged(r, "S8.begin", "S8.end", "session_generation")
            && delta(r, "S8.begin", "S8.end", "peer_accepted_commands").is_some_and(|n| n > 0)
            && outage_ticks
                .iter()
                .filter(|e| e.n("input_sequence") == Some(0) && neutral(e))
                .count()
                >= 3;
        let handshake = remotes
            .iter()
            .find(|(round, _)| *round == 3)
            .is_some_and(|(_, v)| {
                v.last().is_some_and(|s| s["establishments"] == 1)
                    && v.iter()
                        .any(|s| s["event"] == "send" && s["classification"] == "hello_retry")
                    && v.iter().any(|s| {
                        s["event"] == "receive"
                            && s["classification"] == "duplicate"
                            && s["packet"]["content"]["purpose"] == 1
                    })
            });
        let host_timeout = events.iter().find(|e| {
            e.kind() == "net" && e.text("event") == "stop" && e.n("generation") == Some(3)
        });
        let timeout_age =
            host_timeout.and_then(|e| e.n("terminal_at_ms")?.checked_sub(e.n("last_receive_ms")?));
        let remote_timeout = remotes
            .iter()
            .find(|(round, _)| *round == 3)
            .and_then(|(_, v)| v.iter().find(|e| e["event"] == "timeout"));
        let remote_age = remote_timeout.and_then(|e| e["last_receive_elapsed_ms"].as_u64());
        let long_drops = relays
            .iter()
            .find(|(round, _)| *round == 3)
            .map(|(_, v)| {
                v.iter()
                    .filter(|v| {
                        v["event"] == "receive"
                            && v["profile"] == "outage_long"
                            && v["direction"] == 1
                            && v["packet"]["kind"] == 4
                    })
                    .collect::<Vec<_>>()
            })
            .unwrap_or_default();
        let long = timeout_age.is_some_and(|v| v >= 3000)
            && remote_age.is_some_and(|v| v >= 3000)
            && remote_timeout.is_some_and(|v| v["established"] == true && v["expected"] == true)
            && long_drops.len() == 3
            && long_drops
                .iter()
                .all(|v| v["decision"] == "outage" && v["copies"] == 0)
            && r["remote_rounds"]["3"]["terminal_cause"] == "peer_timeout";
        let detail = json!({"S7":{"clean":clean,"fault":fault,"ratio":ratio,"tolerance":[0.8,1.2],"rate_pass":rate_ok,
            "actual_hits":hits,"actual_hits_status":if hits_observed{"PASS"}else{"NOT_OBSERVED"},"non_waiting_input_invariants":invariant},
            "protocol_clean":protocol_ok,"missing_neutral_ticks":missing.len(),"S8":{"pass":short},
            "S17":{"pass":handshake,"status":if handshake{"PASS"}else{"NOT_OBSERVED"}},
            "S18":{"pass":long,"host_age_ms":timeout_age,"remote_age_ms":remote_age,"host_bye_outage_occurrences":long_drops.len()}});
        write_json(&self.out.join("fault-evidence.json"), &detail)?;
        let observed = hits_observed
            && handshake
            && ratio.is_some()
            && timeout_age.is_some()
            && remote_age.is_some();
        let violation = events
            .iter()
            .any(|e| e.text("reason") == "PeerProtocol" || e.text("terminal") == "PeerProtocol")
            || missing.iter().any(|e| !neutral(e))
            || timeout_age.is_some_and(|n| n < 3000)
            || remote_age.is_some_and(|n| n < 3000);
        self.verdict_tx07(
            "V10",
            &[7, 8, 17, 18],
            observed || violation,
            observed && rate_ok && invariant && protocol_ok && short && long,
            detail,
        );
        Ok(())
    }
    fn life_evidence_tx07(
        &mut self,
        events: &[Event],
        lua: &[Value],
        r: &Value,
        remotes: &[(u64, Vec<Value>)],
    ) -> Result<()> {
        let lost_ticks = window(events, r, "S10.lost.begin", "S10.lost.end", "tick", Some(1));
        let lost_a = window(events, r, "S10.lost.begin", "S10.lost.end", "tick", Some(0));
        let accepted = window(
            events,
            r,
            "S10.lost.begin",
            "S10.lost.end",
            "submit",
            Some(1),
        )
        .iter()
        .filter(|e| e.n("decision") == Some(1))
        .count();
        let lost = lost_ticks.len() >= 3
            && lost_ticks
                .iter()
                .all(|e| neutral(e) && e.text("b_life") == "Lost" && e.n("b_bound") == Some(0))
            && accepted == 0
            && lost_a.iter().any(|e| {
                e.pair("move").is_some_and(|(x, y)| x < 0 && y == 0) && matches_command(e, events)
            })
            && delta(
                r,
                "S10.lost.begin",
                "S10.lost.end",
                "peer_discard.life_lost",
            )
            .is_some_and(|n| n > 0);
        let kill = lua
            .iter()
            .filter(|v| v["event"] == "b-kill")
            .collect::<Vec<_>>();
        let ghost = kill.iter().any(|v| v["phase"] == "ghost")
            && events
                .iter()
                .any(|e| e.kind() == "life" && e.text("event") == "lost");
        let invalidations = window(events, r, "S10.begin", "S11.end", "binding", Some(1))
            .into_iter()
            .filter(|e| e.n("bound") == Some(0) && e.text("reason") == "remote_life_lost")
            .collect::<Vec<_>>();
        let rebind = window(events, r, "S11.begin", "S11.end", "binding", Some(1))
            .into_iter()
            .find(|e| e.n("bound") == Some(1) && e.n("revived") == Some(1));
        let old = remotes
            .iter()
            .find(|(round, _)| *round == 1)
            .and_then(|(_, v)| v.iter().find(|v| v["event"] == "old_generation_sample"));
        let rejected = old.is_some_and(|o| {
            events.iter().any(|e| {
                e.kind() == "peer_discard"
                    && e.text("reason") == "old_generation"
                    && e.n("sample_sequence") == o["command_sequence"].as_u64()
                    && e.n("sample_last_ack") == o["last_ack"].as_u64()
                    && e.n("sample_ack_known") == Some(1)
                    && e.n("sample_ack_generation") == o["old_generation"].as_u64()
                    && e.n("known_old_generation_commands").is_some_and(|n| n > 0)
            }) && !events.iter().any(|e| {
                e.kind() == "submit"
                    && e.n("ordinal") == Some(1)
                    && e.n("session_generation") == Some(1)
                    && e.n("sequence") == o["command_sequence"].as_u64()
                    && e.n("decision") == Some(1)
            })
        });
        let chain = rebind.is_some_and(|b| {
            let invalidate = invalidations.first().copied();
            let ack = events.iter().find(|e| {
                e.kind() == "ack"
                    && e.n("session_generation") == Some(1)
                    && e.n("b_generation") == b.n("generation")
                    && e.n("queued") == Some(1)
                    && e.seq() > b.seq()
            });
            invalidate.is_some_and(|i| i.seq() < b.seq())
                && ack.is_some_and(|a| {
                    events.iter().any(|e| {
                        e.kind() == "submit"
                            && e.n("ordinal") == Some(1)
                            && e.n("session_generation") == Some(1)
                            && e.n("decision") == Some(1)
                            && e.n("generation") == b.n("generation")
                            && e.seq() > a.seq()
                    })
                })
        });
        let before = checkpoint(r, "S10.begin");
        let after = checkpoint(r, "S11.end");
        let generation = invalidations.len() == 1
            && value_n(before, "b_generation")
                .zip(invalidations.first().and_then(|e| e.n("generation")))
                .is_some_and(|(a, b)| b == a + 1)
            && rebind.is_some_and(|b| value_n(after, "b_generation") == b.n("generation"));
        let life_before = events
            .iter()
            .find(|e| e.kind() == "life_ownership" && e.text("event") == "lost");
        let life_after = window(events, r, "S11.begin", "S11.end", "life_ownership", None)
            .into_iter()
            .find(|e| e.text("event") == "rebind");
        let same_object = life_before.zip(life_after).is_some_and(|(a, b)| {
            !a.text("device").is_empty()
                && a.text("device") == b.text("device")
                && a.n("key") == b.n("key")
        });
        let same_k = before["device_k_id"] == after["device_k_id"]
            && same_object
            && before["device_k_state"] == "Registered"
            && after["device_k_state"] == "Registered";
        let no_recreation = ["join_creations", "join_passes"]
            .iter()
            .all(|k| unchanged(r, "S10.begin", "S11.end", k))
            && window(events, r, "S10.begin", "S11.end", "join", None)
                .iter()
                .all(|e| !["create", "retired", "complete"].contains(&e.text("event")))
            && window(events, r, "S10.begin", "S11.end", "device_k", None).is_empty();
        let revive = lua
            .iter()
            .any(|v| v["event"] == "b-revive" && v["phase"] == "healthy")
            && lua
                .iter()
                .any(|v| v["event"] == "b-revive" && v["phase"] == "animation_finished")
            && after["b_life"] == "Alive"
            && after["b_bound"] == "1"
            && after["b_deaths"] == "1"
            && after["b_revives"] == "1";
        let direction = r["movement_directions"]["S11.fresh"]
            .as_array()
            .and_then(|v| Some([v.first()?.as_i64()?, v.get(1)?.as_i64()?]));
        let moving = direction.is_some_and(|d| moved(lua, r, "S11.begin", "S11.end", 1, d));
        let detail = json!({"lost_window":{"pass":lost,"complete_ticks":lost_ticks.len(),"accepted":accepted},"ghost_observed":ghost,
            "dead_observation":if kill.iter().any(|v|v["phase"]=="dead"){"OBSERVED"}else{"NOT_OBSERVED"},
            "invalidations":invalidations.iter().map(|e|&e.raw).collect::<Vec<_>>(),"generation":generation,"same_K":same_k,
            "no_recreation":no_recreation,"invalidate_bind_ack_fresh":chain,"old_generation_rejected":rejected,
            "old_probe":old,"revive_and_animation":revive,"movement":moving});
        write_json(&self.out.join("life-evidence.json"), &detail)?;
        self.verdict_tx07(
            "V11",
            &[10, 11],
            ghost && rebind.is_some() && old.is_some(),
            lost && ghost
                && generation
                && same_k
                && no_recreation
                && chain
                && rejected
                && revive
                && moving,
            detail,
        );
        let held = bounds(r, "S15.begin", "S15.release");
        let released = bounds(r, "S15.release", "S15.end");
        let lasers = lua
            .iter()
            .filter(|v| v["event"] == "laser-spawn")
            .collect::<Vec<_>>();
        let attributed = |v: &&Value| v["attribution"] == "B" || v["laser"]["attribution"] == "B";
        let early = lasers
            .iter()
            .copied()
            .filter(attributed)
            .filter(|v| in_bounds(v["loop"].as_u64(), held))
            .collect::<Vec<_>>();
        let late = lasers
            .iter()
            .copied()
            .filter(attributed)
            .filter(|v| in_bounds(v["loop"].as_u64(), released))
            .collect::<Vec<_>>();
        let unknown = lasers
            .iter()
            .filter(|v| {
                in_bounds(v["loop"].as_u64(), held) || in_bounds(v["loop"].as_u64(), released)
            })
            .filter(|v| v["attribution"] == "UNKNOWN" || v["laser"]["attribution"] == "UNKNOWN")
            .count();
        let dispatch = window(
            events,
            r,
            "S15.begin",
            "S15.end",
            "shooting_dispatch",
            Some(1),
        );
        let suppressed = dispatch
            .iter()
            .filter(|e| e.n("tick") == Some(0) || e.n("blocks").is_some_and(|b| b != 0))
            .collect::<Vec<_>>();
        let continuity = window(events, r, "S15.begin", "S15.release", "tick", Some(1));
        let continuous = !continuity.is_empty()
            && continuity
                .iter()
                .filter(|e| {
                    e.n("tick")
                        .zip(value_n(checkpoint(r, "S15.begin"), "completed_tick"))
                        .is_some_and(|(t, start)| t > start + 3)
                })
                .all(|e| {
                    e.pair("aim") == Some((32767, 0))
                        && e.n("held").is_some_and(|n| n & 1 != 0)
                        && matches_command(e, events)
                });
        let armed = remotes
            .iter()
            .find(|(round, _)| *round == 2)
            .is_some_and(|(_, v)| {
                v.iter()
                    .any(|v| v["event"] == "arm_consumed" && v["label"] == "S15.reheld")
            });
        self.verdict_tx07("V12",&[15],!early.is_empty() || !late.is_empty()&&!suppressed.is_empty() || suppressed.iter().any(|e|e.n("original")!=Some(0)),early.is_empty() && !late.is_empty() && unknown==0
            && suppressed.iter().all(|e|e.n("original")==Some(0)) && continuous && armed,
            json!({"before_release":early,"after_release":late,"unknown_attribution":unknown,"blocked_dispatch":suppressed.iter().map(|e|&e.raw).collect::<Vec<_>>(),
                "fresh_held_continuity":continuous,"armed_new_source":armed,"release_window_ticks":self.scenario["laser_release_ticks"]}));
        Ok(())
    }
}
fn neutral(e: &Event) -> bool {
    e.pair("move") == Some((0, 0))
        && e.pair("aim") == Some((0, 0))
        && e.n("edge") == Some(0)
        && e.n("held") == Some(0)
}
// 采用 Authority-pre 的 input_generation，允许 tick 尾部发生合法失效，但不能串到新代次的 Accepted。
fn matches_command(t: &Event, events: &[Event]) -> bool {
    let submits = events
        .iter()
        .filter(|s| {
            s.kind() == "submit"
                && s.n("decision") == Some(1)
                && s.n("ordinal") == t.n("ordinal")
                && s.n("session_generation") == t.n("session_generation")
                && s.n("generation") == t.n("input_generation")
                && s.n("sequence") == t.n("input_sequence")
                && s.n("target") == t.n("tick")
        })
        .collect::<Vec<_>>();
    submits.len() == 1
        && submits[0].seq() < t.seq()
        && ["move", "aim"]
            .iter()
            .all(|k| submits[0].pair(k) == t.pair(k))
        && ["held", "edge"].iter().all(|k| submits[0].n(k) == t.n(k))
}
fn moved(lua: &[Value], r: &Value, a: &str, b: &str, ordinal: usize, direction: [i64; 2]) -> bool {
    let Some((first, last)) = bounds(r, a, b) else {
        return false;
    };
    let before = lua
        .iter()
        .rev()
        .find(|v| v["event"] == "observation" && v["loop"].as_u64().is_some_and(|l| l <= first));
    let after = lua.iter().rev().find(|v| {
        v["event"] == "observation" && v["loop"].as_u64().is_some_and(|l| l > first && l <= last)
    });
    before.zip(after).is_some_and(|(a, b)| {
        let (p, q) = (&a["players"][ordinal], &b["players"][ordinal]);
        a["room"] == b["room"]
            && p["present"] == true
            && q["present"] == true
            && p["controller"] == q["controller"]
            && ["x", "y"].iter().enumerate().any(|(i, k)| {
                direction[i] != 0
                    && p[*k]
                        .as_f64()
                        .zip(q[*k].as_f64())
                        .is_some_and(|(x, y)| (y - x) * direction[i].signum() as f64 > 2.0)
            })
    })
}
fn delta(r: &Value, a: &str, b: &str, key: &str) -> Option<u64> {
    value_n(checkpoint(r, b), key)?.checked_sub(value_n(checkpoint(r, a), key)?)
}
fn pair_text(e: &Event, key: &str) -> Option<(u64, u64)> {
    let (a, b) = e.text(key).split_once("..")?;
    Some((a.parse().ok()?, b.parse().ok()?))
}

impl Run {
    // 加载原生 owner、窗口计数及逐键覆盖分别给结论；零查询不能证明“查询中性”。
    fn loading_evidence_tx07(&mut self, events: &[Event], r: &Value) -> Result<()> {
        let mut loading = vec![];
        let mut coverage = vec![];
        for scene in [12, 20] {
            let (a, b) = (format!("S{scene}.begin"), format!("S{scene}.end"));
            let selected = window(events, r, &a, &b, "loading", None);
            let ends = selected
                .iter()
                .copied()
                .filter(|e| e.text("event") == "end" && e.text("reason") == "loading_clear")
                .collect::<Vec<_>>();
            let mut episodes = vec![];
            for end in ends {
                let begin = selected.iter().copied().find(|e| {
                    e.text("event") == "begin"
                        && e.n("episode") == end.n("episode")
                        && e.n("session_generation") == end.n("session_generation")
                });
                let interval = begin.and_then(|begin| Some((begin.n("loop")?, end.n("loop")?)));
                let no_ticks = interval.is_some()
                    && end.n("ticks") == Some(0)
                    && !events.iter().any(|e| {
                        e.kind() == "tick"
                            && e.n("loop")
                                .zip(interval)
                                .is_some_and(|(l, (a, b))| a <= l && l < b)
                    });
                let no_input = begin.is_some_and(|begin| {
                    end.pair("submit_start") == end.pair("submit_end")
                        && end.pair("submit_start").is_some()
                        && end.pair("resolved_tick_start") == end.pair("resolved_tick_end")
                        && end.pair("resolved_tick_start").is_some()
                        && pair_text(end, "admitted").is_some_and(|(a, b)| a == b)
                        && !events.iter().any(|e| {
                            e.kind() == "submit" && e.seq() > begin.seq() && e.seq() < end.seq()
                        })
                });
                let lifecycle_state = begin.is_some_and(|begin| {
                    let parse = |text: &str| -> Option<(u64, u64)> {
                        let (a, b) = text.split_once(',')?;
                        Some((a.parse().ok()?, b.parse().ok()?))
                    };
                    let generations = end
                        .text("generation")
                        .split_once("->")
                        .and_then(|(a, b)| Some((parse(a)?, parse(b)?)));
                    let queues = end
                        .text("queue")
                        .split_once("->")
                        .and_then(|(_, b)| parse(b));
                    generations.is_some_and(|((a, b), (c, d))| {
                        let counts = [0, 1].map(|ordinal| {
                            events
                                .iter()
                                .filter(|e| {
                                    e.kind() == "binding"
                                        && e.n("ordinal") == Some(ordinal)
                                        && e.n("bound") == Some(0)
                                        && begin.seq() < e.seq()
                                        && e.seq() < end.seq()
                                })
                                .count() as u64
                        });
                        c.checked_sub(a) == Some(counts[0]) && d.checked_sub(b) == Some(counts[1])
                    }) && queues == Some((0, 0))
                });
                let owner_window = window(events, r, &a, &b, "loading_owner", None);
                // owner 进入时 episode 可以尚未创建，用同一 token 的后续返回关联。
                let owners = owner_window
                    .iter()
                    .copied()
                    .filter(|e| {
                        e.n("episode") == end.n("episode")
                            || owner_window.iter().any(|other| {
                                other.n("episode") == end.n("episode")
                                    && other.n("owner_token") == e.n("owner_token")
                            })
                    })
                    .collect::<Vec<_>>();
                let owner_ok = !owners.is_empty()
                    && owners
                        .iter()
                        .filter(|e| e.text("event") == "enter")
                        .all(|enter| {
                            owners.iter().any(|leave| {
                                leave.text("event") == "return"
                                    && leave.n("owner_token") == enter.n("owner_token")
                                    && leave.n("normal") == Some(1)
                                    && leave.seq() > enter.seq()
                            })
                        });
                let bindings = window(events, r, &a, &b, "binding", None)
                    .into_iter()
                    .filter(|e| e.n("bound") == Some(1))
                    .collect::<Vec<_>>();
                let rebound = [0, 1].iter().all(|o| {
                    bindings.iter().any(|e| {
                        e.n("ordinal") == Some(*o)
                            && e.n("loop").zip(end.n("loop")).is_some_and(|(a, b)| a >= b)
                            && owners
                                .iter()
                                .filter(|e| e.text("event") == "return")
                                .all(|owner| owner.seq() < e.seq() && owner.n("loop") < e.n("loop"))
                    })
                });
                let known_queries = end
                    .n("queries")
                    .zip(end.n("unknown"))
                    .and_then(|(a, b)| a.checked_sub(b));
                episodes.push(json!({"episode":end.n("episode"),"begin":begin.map(|e|&e.raw),"end":end.raw,
                    "no_tick":no_ticks,"no_submit_resolve":no_input,"owners_normal":owner_ok,"stable_rebind_after_owners":rebound,
                    "known_context_queries":known_queries,"neutral_query_status":if known_queries.is_some_and(|n|n>0){if end.n("nonzero")==Some(0){"PASS"}else{"FAIL"}}else{"NOT_OBSERVED"},
                    "lifecycle_state_only":lifecycle_state,"invariants_pass":no_ticks && no_input && lifecycle_state && owner_ok && rebound && end.n("nonzero")==Some(0),
                    "state_change_attribution":"only explicit lifecycle invalidations are allowed; no Submit/Resolve during episode"}));
            }
            loading
                .push(json!({"scene":scene,"observed":!episodes.is_empty(),"episodes":episodes}));
            let mut maps = [
                BTreeMap::<(u64, u64, String), [u64; 3]>::new(),
                BTreeMap::new(),
            ];
            let mut rows_ok = true;
            for (index, label) in [&a, &b].iter().enumerate() {
                let rows = events
                    .iter()
                    .filter(|e| {
                        e.kind() == "query_coverage"
                            && e.text("scope") == "checkpoint"
                            && e.text("checkpoint") == label.as_str()
                    })
                    .collect::<Vec<_>>();
                rows_ok &= rows.len() == 12
                    && rows
                        .iter()
                        .all(|e| e.n("complete") == Some(1) && e.n("nonfinite") == Some(0));
                for row in rows {
                    for text in row.text("keys").split(',') {
                        let parsed = (|| -> Option<((u64, u64, String), [u64; 3])> {
                            let (key, counts) = text.split_once('=')?;
                            let values = counts
                                .split('/')
                                .map(str::parse)
                                .collect::<std::result::Result<Vec<u64>, _>>()
                                .ok()?;
                            Some((
                                (row.n("ordinal")?, row.n("consumer")?, key.into()),
                                values.try_into().ok()?,
                            ))
                        })();
                        if let Some((key, counts)) = parsed {
                            if maps[index].insert(key, counts).is_some() {
                                rows_ok = false;
                            }
                        } else {
                            rows_ok = false;
                        }
                    }
                }
            }
            let keys=maps[1].iter().map(|(key,after)| {
                let change=maps[0].get(key).and_then(|before|Some([after[0].checked_sub(before[0])?,after[1].checked_sub(before[1])?,after[2].checked_sub(before[2])?]));
                let status=match change {Some([_,_,n]) if n>0=>"FAIL",Some([reads,repeats,0]) if reads>0 && repeats>0=>"PASS",Some(_)=>"NOT_OBSERVED",None=>"FAIL"};
                json!({"ordinal":key.0,"consumer":key.1,"action_query":key.2,"counts":change,"status":status})
            }).collect::<Vec<_>>();
            let summaries = events
                .iter()
                .filter(|e| {
                    e.kind() == "query_summary"
                        && (e.text("checkpoint") == a || e.text("checkpoint") == b)
                })
                .collect::<Vec<_>>();
            let consistent = rows_ok
                && maps[0].len() == 432
                && maps[1].len() == 432
                && summaries.len() == 2
                && summaries.iter().all(|e| {
                    e.n("mismatches") == Some(0)
                        && e.n("overflow") == Some(0)
                        && e.n("nonfinite") == Some(0)
                })
                && keys.iter().all(|v| v["status"] != "FAIL");
            // 按动作和查询类型汇总仍分别保留 A/B；不能拿某个动作的重复替代其它动作。
            let actions=(0..2).flat_map(|ordinal|(0..12).flat_map(move|action|(1..=3).map(move|kind|(ordinal,action,kind)))).map(|(ordinal,action,kind)| {
                let name=format!("a{action}.k{kind}");let observed=keys.iter().any(|v|v["ordinal"]==ordinal && v["action_query"]==name && v["status"]=="PASS");
                json!({"ordinal":ordinal,"action":action,"query":kind,"status":if observed{"PASS"}else{"NOT_OBSERVED"}})
            }).collect::<Vec<_>>();
            coverage.push(json!({"scene":scene,"rows_complete":rows_ok,"invariants_pass":consistent,"keys":keys,"actions":actions}));
        }
        let continued = checkpoint(r, "S20.begin")["session_state"] == "Warmup"
            && checkpoint(r, "S20.active")["session_state"] == "Active"
            && unchanged(r, "S20.begin", "S20.active", "session_generation")
            && checkpoint(r, "S20.active")["join_creations"] == "1";
        let lua = read(&self.out.join("lua-observations.json"))
            .ok()
            .and_then(|s| serde_json::from_str::<Vec<Value>>(&s).ok())
            .unwrap_or_default();
        let save_request = lua.iter().find(|v| {
            v["event"] == "run-exit"
                && in_bounds(v["loop"].as_u64(), bounds(r, "S19.begin", "S19.end"))
        });
        let loaded = lua.iter().find(|v| {
            v["event"] == "run-started"
                && v["continued"] == true
                && in_bounds(v["loop"].as_u64(), bounds(r, "S20.begin", "S20.active"))
        });
        let restored = save_request
            .is_some_and(|v| v["should_save"] == true && v["snapshot"]["player_count"] == 1)
            && loaded.is_some_and(|v| {
                v["snapshot"]["player_count"] == 1 && v["snapshot"]["players"][0]["controller"] == 0
            });
        let invariant = continued
            && restored
            && loading.iter().all(|v| {
                v["observed"] == true
                    && v["episodes"]
                        .as_array()
                        .is_some_and(|a| a.iter().all(|e| e["invariants_pass"] == true))
            })
            && coverage.iter().all(|v| v["invariants_pass"] == true);
        let observed = loading.iter().all(|v| {
            v["episodes"].as_array().is_some_and(|a| {
                !a.is_empty() && a.iter().all(|v| v["neutral_query_status"] == "PASS")
            })
        }) && coverage.iter().all(|v| {
            v["actions"]
                .as_array()
                .is_some_and(|a| a.iter().all(|v| v["status"] == "PASS"))
        });
        let detail = json!({"loading":loading,"coverage":coverage,"continue_preserved":continued,"save_request":save_request,
            "loaded_before_join":loaded,"save_load_topology":restored,"encoded_saved_records":"UNKNOWN; request and actual loaded topology are separate observations",
            "invariants_pass":invariant,"coverage_complete":observed});
        write_json(&self.out.join("loading-query-evidence.json"), &detail)?;
        let violation = events.iter().any(|e| {
            e.kind() == "query_summary"
                && (e.n("mismatches").is_some_and(|n| n > 0)
                    || e.n("overflow").is_some_and(|n| n > 0))
        }) || loading.iter().any(|v| {
            v["episodes"]
                .as_array()
                .is_some_and(|a| a.iter().any(|e| e["invariants_pass"] == false))
        });
        self.verdict_tx07(
            "V13",
            &[12, 20],
            observed || violation,
            invariant && observed,
            detail,
        );
        Ok(())
    }
    fn retirement_evidence_tx07(&mut self, events: &[Event], r: &Value) -> Result<()> {
        let rounds = (1..=4)
            .map(|round| {
                let label = format!("round{round}.retired");
                let s = checkpoint(r, &label);
                let count = if round == 4 { "0" } else { "1" };
                let cause = ["Stop", "PeerBye", "PeerTimeout", "RunTeardown"][round - 1];
                let ok = s["session_state"] == "Idle"
                    && s["device_k_state"] == "Removed"
                    && s["players"] == count
                    && s["b_bound"] == "0"
                    && s["peer_running"] == "0"
                    && s["peer_stop_finished"] == "1"
                    && s["cleanup_failed"] == "0"
                    && s["retirement_cause"] == cause
                    && s["join_state"] == "None"
                    && s["join_last"] == "Joined"
                    && s["b_life"] == "None";
                json!({"round":round,"pass":ok,"status":s})
            })
            .collect::<Vec<_>>();
        let devices = events
            .iter()
            .filter(|e| e.kind() == "device_k" && e.text("event") == "registered")
            .collect::<Vec<_>>();
        let deletes=devices.iter().map(|device| {
            let selected=events.iter().filter(|e|e.kind()=="device_k" && e.text("event")=="delete" && e.text("object")==device.text("object")).collect::<Vec<_>>();
            let neutral=events.iter().any(|e|e.kind()=="device_k" && e.text("event")=="retiring" && e.text("object")==device.text("object") && e.n("neutral")==Some(1));
            json!({"object":device.text("object"),"delete_calls":selected.len(),"neutral":neutral,
                "pass":neutral && selected.len()==1 && selected[0].n("never")==Some(1)})
        }).collect::<Vec<_>>();
        let safety = !events.iter().any(|e| {
            e.kind() == "retirement_error"
                || e.kind() == "retirement"
                    && ["conflict", "unverified", "unconfirmed"].contains(&e.text("result"))
        }) && events
            .iter()
            .filter(|e| {
                e.kind() == "life" && e.text("event") == "retired" && e.text("status") == "None"
            })
            .count()
            == 4;
        let detail = json!({"rounds":rounds,"devices":deletes,"safe_current_ownership":safety});
        write_json(&self.out.join("retirement-evidence.json"), &detail)?;
        self.verdict_tx07(
            "V14",
            &[13, 16, 18, 21],
            !devices.is_empty(),
            rounds.iter().all(|v| v["pass"] == true)
                && devices.len() == 4
                && deletes.iter().all(|v| v["pass"] == true)
                && safety,
            detail,
        );
        Ok(())
    }
    // 一个 tick 必须有共同 owner/token 的两条 ordinal 及一次 pre/commit；odd 内部更新不能冒充 tick。
    fn tick_evidence_tx07(&mut self, events: &[Event]) -> Result<()> {
        let ticks = events
            .iter()
            .filter(|e| e.kind() == "tick")
            .collect::<Vec<_>>();
        let mut grouped = BTreeMap::<(u64, u64), Vec<&Event>>::new();
        for t in &ticks {
            if let Some(key) = t.key() {
                grouped.entry(key).or_default().push(t);
            }
        }
        let mut faults = vec![];
        let mut last = BTreeMap::new();
        let mut loops = BTreeSet::new();
        for (&(generation, tick), pair) in &grouped {
            let phases = events
                .iter()
                .filter(|e| e.kind() == "tick_phase" && e.key() == Some((generation, tick)))
                .collect::<Vec<_>>();
            let kind = pair[0].text("kind");
            let owner = match kind {
                "StandardRoom" => "Game",
                "OddPlayer" => "OddNormal",
                _ => "INVALID",
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
                    t.n("session_generation") == Some(generation)
                        && t.n("loop") == pair[0].n("loop")
                        && t.n("owner_token") == pair[0].n("owner_token")
                        && t.n("owner_token").is_some_and(|n| n > 0)
                        && t.text("owner") == owner
                        && t.text("kind") == kind
                        && phases.iter().all(|p| {
                            p.n("loop") == t.n("loop") && p.n("owner_token") == t.n("owner_token")
                        })
                        && ["authority_pre", "native_commit", "published"]
                            .iter()
                            .all(|k| t.n(k) == Some(1))
                        && t.n("player_update_entered")
                            .zip(t.n("player_native_reached"))
                            .zip(t.n("player_skipped_by_chain"))
                            .is_some_and(|((a, b), c)| a == b + c)
                        && t.pair("move").is_some()
                        && t.pair("aim").is_some()
                        && t.n("held").is_some()
                        && t.n("edge").is_some()
                });
            if !ok {
                faults.push(json!({"session_generation":generation,"tick":tick,"pair":pair.iter().map(|e|&e.raw).collect::<Vec<_>>()}));
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
            .all(|kind| ticks.iter().any(|e| e.text("kind") == *kind));
        let detail = json!({"ticks":grouped.len(),"faults":faults,"odd_pairs":odd.len(),"odd_pairs_valid":odd_ok,"nonordinary":nonordinary,"both_kinds":both});
        write_json(&self.out.join("tick-integrity.json"), &detail)?;
        self.verdict_tx07(
            "V15",
            &[3],
            !ticks.is_empty(),
            !grouped.is_empty()
                && faults.is_empty()
                && ticks.len() == grouped.len() * 2
                && events.iter().filter(|e| e.kind() == "tick_phase").count() == grouped.len() * 2
                && odd_ok
                && nonordinary
                && both,
            detail,
        );
        Ok(())
    }
    // 入口完整性、跨 loop 窗口关联、文件许可分别判定；允许的原生写入不套旧零变化规则。
    fn persistence_evidence_tx07(
        &mut self,
        events: &[Event],
        lua: &[Value],
        manifest: &Value,
        r: &Value,
    ) -> Result<()> {
        let persistent = events
            .iter()
            .filter(|e| e.kind() == "persistent")
            .collect::<Vec<_>>();
        let calls=persistent.iter().filter(|e|e.text("phase")=="enter").map(|enter| {
            let returns=persistent.iter().filter(|e|e.text("phase")=="return" && e.n("call")==enter.n("call")).collect::<Vec<_>>();
            let parent=enter.n("parent");let parent_known=parent==Some(0) || persistent.iter().any(|e|e.text("phase")=="enter" && e.n("call")==parent && e.seq()<enter.seq());
            json!({"entry":enter.text("entry"),"call":enter.n("call"),"parent":parent,"enter":enter.raw,
                "returns":returns.iter().map(|e|&e.raw).collect::<Vec<_>>(),"parent_known":parent_known,
                "pass":enter.n("call").is_some_and(|n|n>0) && parent_known && returns.len()==1 && returns[0].seq()>enter.seq() && returns[0].n("normal")==Some(1),
                "result_status":if returns.first().is_some_and(|e|e.n("has_value")==Some(1)){"OBSERVED"}else{"VOID_OR_UNKNOWN"},
                "attribution":if enter.text("windows")=="none" || enter.text("windows").is_empty(){"UNATTRIBUTED"}else{"WINDOW_IDS_RECORDED"}})
        }).collect::<Vec<_>>();
        let windows=events.iter().filter(|e|e.kind()=="persist_window" && e.text("event")=="open").map(|open| {
            let closes=events.iter().filter(|e|e.kind()=="persist_window" && e.text("event")=="close" && e.n("id")==open.n("id")).collect::<Vec<_>>();
            json!({"kind":open.text("kind"),"id":open.n("id"),"open":open.raw,"close":closes.iter().map(|e|&e.raw).collect::<Vec<_>>(),
                "pass":closes.len()==1 && closes[0].n("normal")==Some(1) && closes[0].n("call_normal")==Some(1) && closes[0].seq()>open.seq()})
        }).collect::<Vec<_>>();
        let expected = [
            "pgd_unlock",
            "pgd_counter",
            "pgd_local",
            "pgd_cloud",
            "button_maps",
            "entity_kill",
            "revive_ghost",
            "stage_transition",
            "load_game_state",
            "gamestate_write",
            "gamestate_read",
            "gamestate_delete",
            "gamestate_delete_rerun",
        ];
        let observers = expected.iter().all(|id| {
            manifest["hooks"]
                .as_array()
                .is_some_and(|h| h.iter().any(|h| h["id"] == *id))
                && events.iter().any(|e| {
                    e.kind() == "hook_registered"
                        && e.raw["detail"]
                            .as_str()
                            .is_some_and(|d| d.starts_with(&format!("{id} ")))
                })
        });
        let complete = observers
            && !calls.is_empty()
            && calls.iter().all(|v| v["pass"] == true)
            && persistent
                .iter()
                .filter(|e| e.text("phase") == "return")
                .count()
                == calls.len();
        let required_windows = [
            "b_death",
            "b_revive",
            "stage_change",
            "continue",
            "run_exit",
        ];
        let associated = required_windows.iter().all(|kind| {
            windows
                .iter()
                .any(|v| v["kind"] == *kind && v["pass"] == true)
        }) && windows.iter().all(|v| v["pass"] == true);
        let file_notifications = records(&self.out.join("file-notifications.jsonl"));
        let notification_ok = file_notifications.as_ref().is_ok_and(|v| {
            !v.is_empty()
                && !v.iter().any(|e| e["event"] == "error")
                && v.iter().filter(|e| e["event"] == "start").count()
                    == v.iter().filter(|e| e["event"] == "stop").count()
        });
        let snapshot_ok = [10, 11, 12, 19, 20, 21].iter().all(|n| {
            ["before", "after"].iter().all(|phase| {
                let window = &r["file_windows"][format!("S{n}.{phase}")];
                !window.is_null() && window["error"].is_null()
            })
        });
        let baseline_summary = self.report["recorded_baseline"].clone();
        let mut file_windows = vec![];
        for scene in [10, 11, 12, 19, 20, 21] {
            let before = self
                .out
                .join(format!("file-windows/S{scene}.before/snapshot"));
            let after = self
                .out
                .join(format!("file-windows/S{scene}.after/snapshot"));
            let comparison = crate::saves::changes(&before, &after).and_then(|mut changes| {
                self.apply_recorded_baseline_between(&mut changes, &before, &after)?;
                Ok(changes)
            });
            file_windows.push(json!({"scene":scene,"before":before,"after":after,"changes":comparison.as_ref().ok(),
                "error":comparison.as_ref().err(),"pass":comparison.as_ref().is_ok_and(|c|c.as_array().is_some_and(|c|c.iter().all(|v|v["allowed"]==true))),
                "baseline":self.report["recorded_baseline"],"causal_attribution":"UNKNOWN; see native window IDs and raw notifications"}));
        }
        self.report["recorded_baseline"] = baseline_summary;
        write_json(
            &self.out.join("file-window-changes.json"),
            &json!(file_windows),
        )?;
        let saves = r["save_changes"]
            .as_array()
            .is_some_and(|c| c.iter().all(|v| v["allowed"] == true));
        let status = &r["final_status"];
        let counters: Value = read(&self.out.join("counters.json"))
            .ok()
            .and_then(|s| serde_json::from_str(&s).ok())
            .unwrap_or(Value::Null);
        let foreign = counters.as_object().is_some_and(|c| {
            let values = c
                .iter()
                .filter(|(k, _)| k.ends_with(".foreign"))
                .collect::<Vec<_>>();
            !values.is_empty() && values.iter().all(|(_, v)| **v == "0")
        });
        let runtime = [
            "trace_dropped",
            "trace_truncated",
            "context_faults",
            "foreign",
            "queue_dropped",
            "persist_window_overflow",
            "persist_return_errors",
        ]
        .iter()
        .all(|k| value_n(status, k) == Some(0))
            && status["session_error"] == ""
            && status["last_error"] == ""
            && foreign
            && events.iter().all(|e| e.raw["truncated"] == false)
            && events.iter().any(|e| {
                e.kind() == "shutdown"
                    && e.n("trace_dropped") == Some(0)
                    && e.n("trace_truncated") == Some(0)
            });
        let action_ok = ["skip", "room", "kill_b", "revive_b", "stage", "room"]
            .iter()
            .enumerate()
            .all(|(i, name)| {
                let action = lua
                    .iter()
                    .filter(|v| {
                        v["event"] == "action"
                            && v["step"].as_u64() == Some(i as u64 + 1)
                            && v["action"] == *name
                    })
                    .collect::<Vec<_>>();
                action.len() == 2
                    && action[0]["phase"] == "request"
                    && action[1]["phase"] == "arrived"
                    && action[0]["action_id"] == action[1]["action_id"]
            });
        let lua_ok = action_ok
            && !lua.is_empty()
            && lua.iter().enumerate().all(|(i, v)| {
                v["serial"].as_u64() == Some(i as u64 + 1)
                    && v["event"] != "fixture-error"
                    && v["event"] != "laser-overflow"
                    && v["fixture_error"] != true
                    && v["loop"].as_u64().is_some()
            });
        let rgon = read(&self.out.join("enabled/repentogon.log"));
        let errors = rgon.as_ref().ok().map(|s| {
            s.lines()
                .filter(|l| l.contains("[ERROR]") || l.contains("[FATAL]"))
                .map(str::to_string)
                .collect::<Vec<_>>()
        });
        let no_dumps = r["crash_scan_error"].is_null()
            && r["crash_changes"].as_array().is_some_and(|v| {
                v.iter().all(|v| {
                    !v["path"]
                        .as_str()
                        .is_some_and(|p| p.to_ascii_lowercase().ends_with(".dmp"))
                })
            });
        let unknowns = calls
            .iter()
            .filter(|v| {
                v["attribution"] == "UNATTRIBUTED" || v["result_status"] == "VOID_OR_UNKNOWN"
            })
            .collect::<Vec<_>>();
        let detail = json!({"entry_integrity":complete,"window_association":associated,"calls":calls,"windows":windows,
            "unattributed_or_void_result":unknowns,"file_notifications":notification_ok,"window_snapshots":snapshot_ok,"file_windows":file_windows,
            "save_changes_allowed":saves,"save_changes":r["save_changes"],"recorded_baseline":r["recorded_baseline"],
            "runtime":runtime,"lua":lua_ok,"no_new_dump":no_dumps,"repentogon_errors":errors,
            "operation_results":{"ESSM_binary":"UNKNOWN unless original bool/log/file evidence resolves it",
                "ESSM_ex_JSON":"UNKNOWN; independent child write result not inferred from parent success",
                "VirtualRoom":"UNKNOWN unless official log and exact file change resolve it","ItemPool":"UNKNOWN unless official log and exact file change resolve it",
                "achievements_JSON":"observed file changes retain existing permission rules; no new whitelist","cloud_commit":"UNKNOWN; local cache is not remote commit"},
            "note":"Notifications do not prove caller/success; unchanged final bytes do not exclude intermediate writes. No readonly changes, rollback, hash or new file permission."});
        write_json(&self.out.join("persistent-evidence.json"), &detail)?;
        self.verdict_tx07(
            "V16",
            &[21],
            !persistent.is_empty() && !status.is_null(),
            r["enabled_exit"] == 0
                && no_dumps
                && saves
                && complete
                && associated
                && notification_ok
                && snapshot_ok
                && file_windows.iter().all(|v| v["pass"] == true)
                && runtime
                && lua_ok
                && errors.as_ref().is_some_and(Vec::is_empty),
            detail,
        );
        Ok(())
    }
}
// 每项依赖自身场景；后段未执行不会把前段已有的观察抹成全局 NOT_RUN。
impl Run {
    fn verdict_tx07(
        &mut self,
        id: &str,
        scenes: &[u64],
        observed: bool,
        pass: bool,
        detail: Value,
    ) {
        let complete = scenes
            .iter()
            .all(|n| self.report["scenes"][format!("S{n}")] == "COMPLETE");
        let failed = scenes
            .iter()
            .any(|n| self.report["scenes"][format!("S{n}")] == "FAILED");
        let status = if failed {
            "FAIL"
        } else if pass && observed {
            "PASS"
        } else if !complete && !observed {
            "NOT_RUN"
        } else if !observed {
            "NOT_OBSERVED"
        } else {
            "FAIL"
        };
        self.report["cases"][id] =
            json!({"pass":status=="PASS","status":status,"required_scenes":scenes,"detail":detail});
    }
    pub(crate) fn evaluate_tx07(&mut self) -> Result<()> {
        let events = match parse_events(&self.out.join("enabled/trace.jsonl")) {
            Ok(v) => v,
            Err(e) => {
                self.report["trace_parse_error"] = json!(e);
                vec![]
            }
        };
        let r = self.report.clone();
        let status = &r["final_status"];
        let lua_result = (|| -> Result<Vec<Value>> {
            let mut values = vec![];
            for line in read(&self.out.join("enabled/log.txt"))?.lines() {
                if let Some((_, raw)) = line.split_once("[isaac-cs-test] tx07 ") {
                    let v: Value = serde_json::from_str(raw).map_err(|e| e.to_string())?;
                    if v["token"] == r["token"] {
                        values.push(v);
                    }
                }
            }
            Ok(values)
        })();
        if let Err(e) = &lua_result {
            self.report["lua_parse_error"] = json!(e);
        }
        let lua = lua_result.unwrap_or_default();
        let ticks = events
            .iter()
            .filter(|e| e.kind() == "tick")
            .collect::<Vec<_>>();
        let manifest: Value = read(&self.out.join("hook-manifest.json"))
            .ok()
            .and_then(|s| serde_json::from_str(&s).ok())
            .unwrap_or(Value::Null);
        write_json(&self.out.join("lua-observations.json"), &json!(lua))?;
        write_json(
            &self.out.join("tick-evidence.json"),
            &json!({"ticks":ticks.iter().map(|e|&e.raw).collect::<Vec<_>>(),
            "submits":events.iter().filter(|e|e.kind()=="submit").map(|e|&e.raw).collect::<Vec<_>>()}),
        )?;
        let hooks = manifest["hooks"].as_array();
        let pointers = manifest["pointer_hooks"].as_array();
        let patches = manifest["byte_patches"].as_array();
        let hook_ok = hooks.is_some_and(|h| {
            !h.is_empty()
                && h.iter().all(|h| {
                    events
                        .iter()
                        .filter(|e| {
                            e.kind() == "hook_registered"
                                && e.raw["detail"].as_str().is_some_and(|s| {
                                    s.starts_with(&format!(
                                        "{} ",
                                        h["id"].as_str().unwrap_or("MISSING")
                                    ))
                                })
                        })
                        .count()
                        == 1
                })
        });
        let patch_ok = patches.is_some_and(|p| {
            !p.is_empty()
                && p.iter().all(|p| {
                    events
                        .iter()
                        .filter(|e| {
                            e.kind() == "byte_patch"
                                && Some(e.text("id")) == p["id"].as_str()
                                && e.n("ok") == Some(1)
                        })
                        .count()
                        == 1
                })
        });
        let pointer_ok = pointers.is_some_and(|p| {
            value_n(status, "pointers") == Some(p.len() as u64)
                && p.iter().all(|id| {
                    id.as_str().is_some_and(|id| {
                        let Some((class, slot)) = id.rsplit_once('_') else {
                            return false;
                        };
                        events
                            .iter()
                            .filter(|e| {
                                e.kind() == "pointer_patch"
                                    && e.n("ok") == Some(1)
                                    && e.text("class") == class
                                    && e.text("slot") == slot
                            })
                            .count()
                            == 1
                    })
                })
        });
        let capabilities = [
            "loop", "input", "player", "join", "control", "device", "observe",
        ]
        .iter()
        .all(|g| status[format!("capability.{g}")] == "1");
        self.verdict_tx07("V1",&[0],!events.is_empty(),hook_ok && patch_ok && pointer_ok && capabilities
            && value_n(status,"registered")==hooks.map(|h|h.len() as u64),json!({"hooks_once":hook_ok,"byte_patches_once":patch_ok,"pointers_from_manifest":pointer_ok,"capabilities":capabilities}));
        let raw = [("S13.raw", "Idle", [-1, 0]), ("S14.raw", "Hosting", [1, 0])]
            .iter()
            .map(|(label, state, direction)| {
                let (a, b) = (format!("{label}.begin"), format!("{label}.end"));
                let p = checkpoint(&r, &a);
                let q = checkpoint(&r, &b);
                let ok = [
                    "completed_tick",
                    "ticks_standard",
                    "ticks_odd",
                    "query_overrides",
                ]
                .iter()
                .all(|k| unchanged(&r, &a, &b, k))
                    && p["session_state"] == *state
                    && q["session_state"] == *state
                    && p["armed"] == "0"
                    && q["armed"] == "0"
                    && moved(&lua, &r, &a, &b, 0, *direction);
                json!({"scene":label,"pass":ok})
            })
            .collect::<Vec<_>>();
        let menu = ["S1", "S20.hosting"].iter().all(|label| {
            let (a, b) = (format!("{label}.begin"), format!("{label}.end"));
            unchanged(&r, &a, &b, "query_overrides")
                && unchanged(&r, &a, &b, "completed_tick")
                && checkpoint(&r, &a)["armed"] == "0"
                && checkpoint(&r, &b)["armed"] == "0"
        });
        self.verdict_tx07(
            "V2",
            &[1, 13, 14, 20],
            ["S13.raw.end", "S14.raw.end", "S20.hosting.end"]
                .iter()
                .all(|label| !checkpoint(&r, label).is_null()),
            menu && raw.iter().all(|v| v["pass"] == true),
            json!({"menu":menu,"raw_windows":raw}),
        );
        let sessions = events
            .iter()
            .filter(|e| e.kind() == "session" && !e.text("from").is_empty())
            .collect::<Vec<_>>();
        let sequence = [
            ("Idle", "Hosting"),
            ("Hosting", "Warmup"),
            ("Warmup", "Active"),
            ("Active", "Retiring"),
            ("Retiring", "Idle"),
        ];
        let causes = ["Stop", "PeerBye", "PeerTimeout", "RunTeardown"];
        let generations = ["S3.end", "S14.active", "S17.end", "S20.active"]
            .map(|label| value_n(checkpoint(&r, label), "session_generation"));
        let transitions = sessions.len() == 20
            && sessions.iter().enumerate().all(|(i, e)| {
                e.text("from") == sequence[i % 5].0 && e.text("to") == sequence[i % 5].1
            })
            && causes.iter().enumerate().all(|(i, c)| {
                sessions
                    .get(i * 5 + 3)
                    .is_some_and(|e| e.text("reason") == *c)
                    && sessions
                        .get(i * 5 + 4)
                        .is_some_and(|e| e.text("reason") == *c)
            });
        let generation_ok = generations == [Some(1), Some(2), Some(3), Some(4)];
        self.verdict_tx07("V3",&[3,13,14,16,17,18,20,21],!sessions.is_empty(),transitions && generation_ok
            && !events.iter().any(|e|e.kind()=="tick_reject"),json!({"transitions":sessions.iter().map(|e|&e.raw).collect::<Vec<_>>(),"session_generations":generations}));

        // 三方在各轮最终停止后按 occurrence/copy 对账；不调整本地 UDP 损失或尾包差额。
        let mut network = vec![];
        let mut all_remote = vec![];
        let mut all_relay = vec![];
        let stops = events
            .iter()
            .filter(|e| e.kind() == "net" && e.text("event") == "stop")
            .collect::<Vec<_>>();
        for round in 1..=4 {
            let remote = records(&self.out.join(format!("remote-{round}.jsonl")));
            let relay = records(&self.out.join(format!("relay-{round}.jsonl")));
            let (remote, relay) = match (remote, relay) {
                (Ok(a), Ok(b)) => (a, b),
                (a, b) => {
                    network.push(json!({"round":round,"pass":false,"status":"NOT_RUN","remote_error":a.err(),"relay_error":b.err()}));
                    continue;
                }
            };
            let rs = remote.last().filter(|v| v["event"] == "summary");
            let ls = relay
                .last()
                .filter(|v| v["event"] == "summary")
                .map(|v| &v["summary"]);
            let hs = stops
                .iter()
                .find(|e| e.n("generation") == Some(round))
                .copied();
            let mut ledger = vec![];
            let mut accounting = true;
            for direction in 0..2 {
                let receives = relay
                    .iter()
                    .filter(|v| v["event"] == "receive" && v["direction"] == direction)
                    .collect::<Vec<_>>();
                let sends = relay
                    .iter()
                    .filter(|v| v["event"] == "send" && v["direction"] == direction)
                    .collect::<Vec<_>>();
                let summary = ls.map(|s| &s["directions"][direction]);
                let source = if direction == 0 {
                    rs.and_then(|s| s["tx"].as_u64())
                } else {
                    hs.and_then(|s| s.n("tx"))
                };
                let destination = if direction == 0 {
                    hs.and_then(|s| s.n("rx"))
                } else {
                    rs.and_then(|s| s["rx"].as_u64())
                };
                let ok = summary.is_some_and(|s| {
                    let received = receives.len() as u64;
                    let sent = sends.iter().filter(|v| v["ok"] == true).count() as u64;
                    source == Some(received)
                        && destination == Some(sent)
                        && s["received"].as_u64() == Some(received)
                        && s["send_ok"].as_u64() == Some(sent)
                        && s["send_error"] == 0
                        && s["pending"] == 0
                        && s["shutdown_drop"] == 0
                        && s["policy_drop"].as_u64()
                            == Some(receives.iter().filter(|v| v["copies"] == 0).count() as u64)
                        && s["copies_scheduled"].as_u64() == Some(sends.len() as u64)
                        && receives
                            .iter()
                            .enumerate()
                            .all(|(i, v)| v["occurrence_id"].as_u64() == Some(i as u64 + 1))
                        && receives.iter().all(|v| {
                            let copies = v["copies"].as_u64().unwrap_or(u64::MAX);
                            let actual = sends
                                .iter()
                                .filter(|s| s["occurrence_id"] == v["occurrence_id"])
                                .collect::<Vec<_>>();
                            actual.len() as u64 == copies
                                && actual
                                    .iter()
                                    .map(|v| v["copy_id"].as_u64())
                                    .collect::<BTreeSet<_>>()
                                    .len()
                                    == actual.len()
                        })
                });
                accounting &= ok;
                ledger.push(json!({"direction":direction,"pass":ok,"source_tx":source,"relay_received":receives.len(),
                    "relay_send_ok":sends.iter().filter(|v|v["ok"]==true).count(),"destination_rx":destination,"summary":summary,
                    "source_difference":source.map(|s|s as i128-receives.len() as i128),
                    "destination_difference":destination.map(|s|s as i128-sends.iter().filter(|v|v["ok"]==true).count() as i128)}));
            }
            let codec = remote
                .iter()
                .filter(|v| v["event"] == "send" || v["event"] == "receive")
                .all(|v| {
                    v["bytes"].as_str().is_some_and(|s| {
                        protocol::unhex(s)
                            .and_then(|b| protocol::decode(&b))
                            .is_ok()
                    })
                })
                && !remote
                    .iter()
                    .any(|v| v["event"] == "send_error" || v["classification"] == "error");
            let ending = rs.is_some_and(|s| {
                s["establishments"] == 1
                    && if round == 3 {
                        s["error"] == "peer_timeout"
                            && s["terminal_cause"] == "peer_timeout"
                            && s["expected_timeout"] == true
                    } else {
                        s["error"].is_null()
                            && if round == 2 {
                                s["terminal_cause"] == "local_bye"
                            } else {
                                s["bye"].as_u64() == Some(if round == 1 { 1 } else { 5 })
                            }
                    }
            });
            let relay_healthy = !relay.iter().any(|v| v["event"] == "error")
                && relay
                    .last()
                    .is_some_and(|v| v["event"] == "summary" && v["error"].is_null());
            let host = hs.is_some_and(|s| {
                s.n("stopped") == Some(1)
                    && s.n("foreign") == Some(0)
                    && s.n("rx_drop") == Some(0)
                    && s.n("tx_drop") == Some(0)
                    && s.n("bye_attempts") == Some(if round == 2 { 0 } else { 3 })
            });
            let active_label =
                ["S3.end", "S14.active", "S17.end", "S20.active"][(round - 1) as usize];
            let start = relay.iter().find(|v| v["event"] == "start");
            let locked = relay
                .iter()
                .filter(|v| v["event"] == "remote_locked")
                .count()
                == 1
                && ls.is_some_and(|s| s["foreign"] == 0)
                && checkpoint(&r, active_label)["peer_locked"] == "1"
                && start.is_some_and(|start| {
                    start["host_socket"] == checkpoint(&r, active_label)["peer_address"]
                });
            network.push(json!({"round":round,"pass":accounting && codec && ending && host && locked && relay_healthy,"ledger":ledger,"codec":codec,"terminal":ending,"exact_relay_peer":locked,"relay_healthy":relay_healthy,
                "host":hs.map(|e|&e.raw),"remote":rs,"relay":ls}));
            all_remote.push((round, remote));
            all_relay.push((round, relay));
        }
        write_json(&self.out.join("network-ledger.json"), &json!(network))?;
        self.verdict_tx07(
            "V4",
            &[2, 13, 16, 17, 18, 20, 21],
            !all_remote.is_empty(),
            network.len() == 4 && network.iter().all(|v| v["pass"] == true),
            json!(network),
        );
        let mut joins = vec![];
        for (i, label) in ["S3.end", "S14.active", "S17.end", "S20.active"]
            .iter()
            .enumerate()
        {
            let s = checkpoint(&r, label);
            let b = &r["checkpoints"][*label]["observation"]["players"][1];
            let ok = s["players"] == "2"
                && s["a_bound"] == "1"
                && s["b_bound"] == "1"
                && s["join_creations"] == "1"
                && s["device_k_state"] == "Registered"
                && b["type"] == 1
                && b["variant"] == 0
                && b["player_type"] == if i == 1 { 7 } else { 0 }
                && b["ghost"] == false
                && b["dead"] == false
                && b["controller"].as_u64() == value_n(s, "device_k_id");
            joins.push(json!({"round":i+1,"pass":ok,"b":b}));
        }
        let modes = events
            .iter()
            .filter(|e| e.kind() == "join" && e.text("event") == "mode")
            .collect::<Vec<_>>();
        let first_modes = modes
            .iter()
            .filter(|e| {
                e.n("loop")
                    .zip(value_n(checkpoint(&r, "S3.end"), "frame"))
                    .is_some_and(|(l, end)| l <= end)
            })
            .collect::<Vec<_>>();
        let mode_ok = !first_modes.is_empty()
            && first_modes.iter().all(|e| e.n("original") == Some(0))
            && modes
                .iter()
                .any(|e| e.n("original") == Some(1) && e.n("result") == Some(0))
            && modes.iter().all(|e| {
                e.n("native_calls") == Some(1)
                    && e.n("result") == Some(0)
                    && e.n("site").is_some_and(|s| (1..=3).contains(&s))
            });
        let creates = events
            .iter()
            .filter(|e| e.kind() == "join" && e.text("event") == "create")
            .collect::<Vec<_>>();
        let creation_ok = creates.len() == 4
            && creates.iter().all(|e| e.n("calls") == Some(1))
            && events
                .iter()
                .filter(|e| e.kind() == "device_k" && e.text("event") == "registered")
                .count()
                == 4
            && events
                .iter()
                .filter(|e| e.kind() == "join" && e.text("event") == "complete")
                .count()
                == 4;
        let per_round=(0..4).map(|round| {
            let scope=sessions.get(round*5).zip(sessions.get(round*5+4)).map(|(a,b)|(a.seq(),b.seq()));
            let count=|kind:&str,event:&str|events.iter().filter(|e|e.kind()==kind && e.text("event")==event
                && scope.is_some_and(|(a,b)|a<e.seq() && e.seq()<b)).count();
            json!({"round":round+1,"allocated":count("device_k","allocated"),"registered":count("device_k","registered"),
                "created":count("join","create"),"joined":count("join","complete"),
                "pass":scope.is_some() && count("device_k","allocated")==1 && count("device_k","registered")==1 && count("join","create")==1 && count("join","complete")==1})
        }).collect::<Vec<_>>();
        let predicates=modes.iter().map(|e| {
            let pass=events.iter().rev().find(|b|b.kind()=="join" && b.text("event")=="begin" && b.seq()<e.seq()).and_then(|b|b.n("pass"));
            json!({"pass":pass,"site":e.n("site"),"loop":e.n("loop"),"native_calls":e.n("native_calls"),"raw":e.raw})
        }).collect::<Vec<_>>();
        self.verdict_tx07("V5",&[3,14,17,20],!modes.is_empty(),mode_ok && creation_ok && per_round.iter().all(|v|v["pass"]==true) && joins.iter().all(|v|v["pass"]==true),
            json!({"rounds":joins,"mode_calls_by_pass_and_site":predicates,"creation_by_round":per_round,"factory_once_per_round":creation_ok}));
        let mut movement = vec![];
        let mut paths = vec![("S4.right", [1, 0], 24), ("S4.up", [0, -1], 24)];
        for label in ["S9.move", "S12.move", "S20.move"] {
            if let Some(d) = r["movement_directions"][label]
                .as_array()
                .and_then(|v| Some([v.first()?.as_i64()?, v.get(1)?.as_i64()?]))
            {
                paths.push((label, d, 18));
            } else {
                movement.push(json!({"label":label,"pass":false,"status":"NOT_RUN"}));
            }
        }
        for (label, direction, release_at) in paths {
            let (a, b) = (format!("{label}.begin"), format!("{label}.end"));
            let selected = window(&events, &r, &a, &b, "tick", Some(1));
            let active = selected.iter().filter(|e| !neutral(e)).collect::<Vec<_>>();
            let base = r["scripts"][label]["base"].as_u64();
            let released = base.is_some_and(|base| {
                selected.iter().any(|e| {
                    e.n("tick")
                        .is_some_and(|t| t >= base + release_at && t <= base + release_at + 3)
                        && neutral(e)
                }) && selected
                    .iter()
                    .filter(|e| e.n("tick").is_some_and(|t| t > base + release_at + 3))
                    .all(|e| neutral(e))
            });
            let ok = !active.is_empty()
                && active.iter().all(|e| matches_command(e, &events))
                && released
                && active
                    .iter()
                    .any(|e| e.triple("nonzero0").is_some_and(|v| v[2] > 0))
                && moved(&lua, &r, &a, &b, 1, direction);
            movement.push(json!({"label":label,"pass":ok,"release_by_lead_plus_one":released}));
        }
        // 每次可用代次都需要真实 +1 Accepted/Resolve；只发出了 startup 包还不算生效。
        let mut startups = vec![];
        for (round, remote) in &all_remote {
            for sample in remote
                .iter()
                .filter(|v| v["event"] == "sample" && v["startup"] == true)
            {
                let commands = sample["commands"].as_array();
                let generation = sample["generation"].as_u64();
                let ack_tick = sample["ack_tick"].as_u64();
                let ok = commands.is_some_and(|c| {
                    !c.is_empty()
                        && c[0]["target"]
                            .as_u64()
                            .zip(ack_tick)
                            .is_some_and(|(t, a)| t == a + 1)
                        && c.len() == 2
                        && c[1]["target"]
                            .as_u64()
                            .zip(ack_tick)
                            .is_some_and(|(t, a)| t == a + 2)
                        && c[1]["state"]["edge"] == 0
                        && ticks.iter().any(|t| {
                            t.n("ordinal") == Some(1)
                                && t.n("session_generation") == Some(*round)
                                && t.n("input_generation") == generation
                                && t.n("input_sequence") == c[0]["sequence"].as_u64()
                                && t.n("tick") == c[0]["target"].as_u64()
                                && matches_command(t, &events)
                        })
                });
                startups.push(json!({"round":round,"sample":sample,"pass":ok}));
            }
        }
        let input_ok = ticks
            .iter()
            .filter(|e| !neutral(e))
            .all(|e| matches_command(e, &events));
        let expected_startups = events
            .iter()
            .filter(|e| e.kind() == "ack" && e.n("queued") == Some(1) && e.n("state") == Some(3))
            .filter_map(|e| Some((e.n("session_generation")?, e.n("b_generation")?)))
            .filter(|(_, g)| *g > 0)
            .collect::<BTreeSet<_>>();
        let startups_complete = !expected_startups.is_empty()
            && expected_startups.iter().all(|(session, generation)| {
                startups.iter().any(|v| {
                    v["round"] == *session
                        && v["sample"]["generation"] == *generation
                        && v["pass"] == true
                })
            });
        self.verdict_tx07("V6",&[3,4,9,11,12,20],!movement.is_empty()&&!ticks.is_empty(),movement.iter().all(|v|v["pass"]==true)
            && startups_complete && startups.iter().all(|v|v["pass"]==true) && input_ok,json!({"movement":movement,"startup_samples":startups,
                "expected_startup_generations":expected_startups,"startup_coverage":startups_complete,"all_non_neutral_matches_accepted":input_ok}));
        let simultaneous = window(
            &events,
            &r,
            "S4.simultaneous.begin",
            "S4.simultaneous.end",
            "tick",
            None,
        );
        let simultaneous_ok = simultaneous
            .iter()
            .filter(|e| e.n("ordinal") == Some(0))
            .any(|a| {
                a.pair("move").is_some_and(|(x, y)| x > 0 && y == 0)
                    && a.pair("aim") == Some((0, 0))
                    && matches_command(a, &events)
                    && simultaneous.iter().any(|b| {
                        b.n("ordinal") == Some(1)
                            && b.key() == a.key()
                            && b.pair("move") == Some((0, 0))
                            && b.pair("aim").is_some_and(|(x, y)| x > 0 && y == 0)
                            && b.triple("nonzero4")
                                .is_some_and(|v| v.iter().any(|n| *n > 0))
                            && matches_command(b, &events)
                    })
            });
        let priority = [
            ("begin", "right", 1),
            ("right", "left", -1),
            ("left", "fallback", 1),
            ("fallback", "end", 0),
        ]
        .iter()
        .map(|(a, b, sign)| {
            let selected = window(
                &events,
                &r,
                &format!("S4.priority.{a}"),
                &format!("S4.priority.{b}"),
                "tick",
                Some(0),
            );
            let ok = selected.iter().any(|e| {
                e.pair("aim")
                    .is_some_and(|(x, y)| y == 0 && x.signum() == *sign)
            });
            json!({"end":b,"sign":sign,"pass":ok})
        })
        .collect::<Vec<_>>();
        self.verdict_tx07(
            "V7",
            &[4],
            !simultaneous.is_empty(),
            simultaneous_ok && priority.iter().all(|v| v["pass"] == true),
            json!({"simultaneous":simultaneous_ok,"priority":priority}),
        );
        let bombs=(0..2).map(|ordinal| {
            let resolved=window(&events,&r,"S5.begin","S5.end","tick",Some(ordinal)).into_iter().filter(|e|e.n("edge").is_some_and(|n|n&1!=0)).collect::<Vec<_>>();
            let accepted=window(&events,&r,"S5.begin","S5.end","submit",Some(ordinal)).into_iter().filter(|e|e.n("decision")==Some(1) && e.n("edge").is_some_and(|n|n&1!=0)).collect::<Vec<_>>();
            let before=&r["checkpoints"]["S5.begin"]["observation"]["players"][ordinal as usize]["bombs"];
            let after=&r["checkpoints"]["S5.end"]["observation"]["players"][ordinal as usize]["bombs"];
            let resource=before.as_i64().zip(after.as_i64()).is_some_and(|(a,b)|a-b==1);
            json!({"ordinal":ordinal,"resolved":resolved.len(),"accepted":accepted.len(),"bombs_delta_one":resource,
                "pass":resource && accepted.len()==1 && resolved.len()==1 && matches_command(resolved[0],&events)
                    && resolved[0].n("bomb_queries").is_some_and(|n|n>0) && resolved[0].text("bomb_query_value")=="1"})
        }).collect::<Vec<_>>();
        let fault_bombs = window(&events, &r, "S7.begin", "S7.end", "tick", Some(1))
            .iter()
            .filter(|e| e.n("edge").is_some_and(|n| n & 1 != 0))
            .count();
        self.verdict_tx07(
            "V8",
            &[5, 7],
            !window(&events, &r, "S5.begin", "S5.end", "tick", None).is_empty(),
            bombs.iter().all(|v| v["pass"] == true) && fault_bombs <= 1,
            json!({"S5":bombs,"S7_applied":fault_bombs}),
        );

        let actions = lua
            .iter()
            .filter(|v| v["event"] == "action")
            .collect::<Vec<_>>();
        let action_ok = ["skip", "room"].iter().enumerate().all(|(i, name)| {
            let selected = actions
                .iter()
                .filter(|v| v["step"].as_u64() == Some(i as u64 + 1) && v["action"] == *name)
                .collect::<Vec<_>>();
            selected.len() == 2
                && selected[0]["phase"] == "request"
                && selected[1]["phase"] == "arrived"
                && selected[0]["action_id"] == selected[1]["action_id"]
                && (*name != "room" || selected[1]["target_matched"] == true)
        });
        let skip = actions
            .iter()
            .find(|v| v["step"] == 1 && v["phase"] == "request")
            .zip(
                actions
                    .iter()
                    .find(|v| v["step"] == 1 && v["phase"] == "arrived"),
            );
        let skip_ok = skip.is_some_and(|(a, b)| {
            b["skipped"] == 30
                && a["loop"]
                    .as_u64()
                    .zip(b["loop"].as_u64())
                    .is_some_and(|(a, b)| {
                        b > a
                            && !ticks.iter().any(|t| {
                                t.text("kind") == "StandardRoom"
                                    && t.n("loop").is_some_and(|l| a < l && l < b)
                            })
                    })
        });
        let blocks = ["S6.console", "S6.pause"]
            .iter()
            .map(|label| {
                let (a, b) = (format!("{label}.begin"), format!("{label}.end"));
                let ok = unchanged(&r, &a, &b, "completed_tick")
                    && value_n(checkpoint(&r, &a), "blocks").is_some_and(|n| n & 1 != 0)
                    && window(&events, &r, &a, &b, "tick", None).is_empty();
                json!({"label":label,"pass":ok})
            })
            .collect::<Vec<_>>();
        let command_sources = ["lua", "console", "port"]
            .iter()
            .map(|source| {
                let found = events.iter().any(|e| {
                    e.kind() == "command"
                        && e.text("source") == *source
                        && (e.n("result_ok") == Some(1) || e.text("result") == "ok=1")
                        && if *source == "console" {
                            e.raw["detail"].as_str().is_some_and(|d| {
                                r["console_token"]
                                    .as_str()
                                    .is_some_and(|token| d.contains(token))
                            })
                        } else {
                            true
                        }
                });
                json!({"source":source,"pass":found})
            })
            .collect::<Vec<_>>();
        let mut room_intervals = vec![];
        let mut room_start = None;
        for change in window(&events, &r, "S9.begin", "S9.end", "block_interval", None) {
            if change.n("to").is_some_and(|n| n & 2 != 0) {
                room_start.get_or_insert(change.seq());
            } else if let Some(start) = room_start.take() {
                room_intervals.push((start, change.seq()));
            }
        }
        let scopes = window(&events, &r, "S9.begin", "S9.end", "room_scope", None);
        let no_room_ticks = room_start.is_none()
            && (!room_intervals.is_empty() || !scopes.is_empty())
            && !ticks.iter().any(|t| {
                t.text("kind") == "StandardRoom"
                    && (room_intervals
                        .iter()
                        .any(|(a, b)| *a < t.seq() && t.seq() < *b)
                        || scopes.iter().any(|e| {
                            e.n("loop") == t.n("loop") && e.n("owner_token") == t.n("owner_token")
                        }))
            });
        let rebound = ["a_generation", "b_generation"]
            .iter()
            .all(|key| delta(&r, "S9.begin", "S9.end", key).is_some_and(|n| n > 0));
        self.verdict_tx07("V9",&[1,6,9],skip.is_some(),skip_ok && blocks.iter().all(|v|v["pass"]==true) && command_sources.iter().all(|v|v["pass"]==true)
            && action_ok && no_room_ticks && rebound && window(&events,&r,"S6.pause.end","S6.end","tick",None).iter().all(|e|e.n("edge")==Some(0)),
            json!({"skip":skip_ok,"blocks":blocks,"command_sources":command_sources,"actions":action_ok,
                "room_intervals_by_sequence":room_intervals,"no_standard_tick_in_room_transition":no_room_ticks,"both_rebound":rebound}));
        self.fault_evidence_tx07(&events, &r, &all_remote, &all_relay)?;
        self.life_evidence_tx07(&events, &lua, &r, &all_remote)?;
        self.loading_evidence_tx07(&events, &r)?;
        self.retirement_evidence_tx07(&events, &r)?;
        self.tick_evidence_tx07(&events)?;
        self.persistence_evidence_tx07(&events, &lua, &manifest, &r)?;
        Ok(())
    }
}
