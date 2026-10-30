//! 全量 trace 关联索引：一次遍历建表，再按完整键查找；不抽样、不覆盖重复记录。
//! Option 数值键保留原归约对缺字段的比较语义，原始 Event 只借用、不复制大 JSON。
use crate::tx05::evidence::Event;
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};
use std::ops::Bound::Excluded;

type Number = Option<u64>;
type Triple = (Number, Number, Number);
type CommandKey = (Number, Number, Number, Number, Number);
type PlayerKey = (Number, Number, Number, Number);

// 阶段计数保留所有重复：只有恰好一个 pre/commit/标准发布才满足原判据。
#[derive(Clone, Copy, Default)]
pub(super) struct PhaseCounts {
    pub pre: usize,
    pub commit: usize,
    pub standard_publish: usize,
    pub pre_sequence: u64,
    pub commit_sequence: u64,
    pub publish_sequence: u64,
    pub pre_generation: Number,
    pub commit_generation: Number,
    pub publish_generation: Number,
    pub normal_publish: bool,
}

impl PhaseCounts {
    // 同键数量、实际顺序和会话代次共同成立，不能仅由三个计数拼出完整提交。
    pub fn ordered(self) -> bool {
        self.pre == 1 && self.commit == 1 && self.standard_publish == 1
            && self.pre_sequence > 0 && self.pre_sequence < self.commit_sequence
            && self.commit_sequence < self.publish_sequence && self.normal_publish
            && self.pre_generation.is_some_and(|n|n>0)
            && self.pre_generation == self.commit_generation
            && self.pre_generation == self.publish_generation
    }
}

// 索引生命周期短于原始 events；每个录制/进程分别建立，身份不跨进程混用。
#[derive(Default)]
pub(super) struct TraceIndex<'a> {
    source_events: usize,
    accepted: BTreeMap<CommandKey, Vec<&'a Event>>,
    phases: BTreeMap<Triple, PhaseCounts>,
    native_players: BTreeMap<PlayerKey, usize>,
    native_odd: BTreeMap<(Number, Number), usize>,
    ticks: BTreeMap<Triple, Vec<&'a Event>>,
    earliest_a_tick: BTreeMap<(Number, Number), u64>,
    persistent_entries: BTreeMap<Number, Vec<&'a Event>>,
    persistent_returns: BTreeMap<Number, Vec<&'a Event>>,
    window_closes: BTreeMap<Number, Vec<&'a Event>>,
    loading_begins: BTreeMap<(Number, Number), Vec<&'a Event>>,
    // 无consumer的查询逐条保留实际透传值；未知上下文不变成受控绑定依据。
    loading_unscoped: BTreeMap<(Number, Number), Vec<&'a Event>>,
    changing_sequences: BTreeSet<u64>,
}

impl<'a> TraceIndex<'a> {
    // 只提取归约实际使用的键；Vec 保留同键全部事件，存在性查询才使用集合/最早序号。
    pub fn new(events: &'a [Event]) -> Self {
        let mut index = Self { source_events: events.len(), ..Self::default() };
        for e in events {
            match e.kind() {
                "submit" => {
                    index.changing_sequences.insert(e.seq());
                    if e.n("decision") == Some(1) {
                        index.accepted.entry((e.n("ordinal"), e.n("session_generation"),
                            e.n("generation"), e.n("sequence"), e.n("target")))
                            .or_default().push(e);
                    }
                }
                "tick" => {
                    index.changing_sequences.insert(e.seq());
                    index.ticks.entry((e.n("generation"), e.n("tick"), e.n("ordinal")))
                        .or_default().push(e);
                    if e.n("ordinal") == Some(0) {
                        index.earliest_a_tick.entry((e.n("session_generation"), e.n("tick")))
                            .and_modify(|seq| *seq = (*seq).min(e.seq())).or_insert(e.seq());
                    }
                    if e.n("player_native_reached").is_some_and(|n| n > 0)
                        && e.n("native_commit") == Some(1) {
                        *index.native_players.entry((e.n("ordinal"), e.n("tick"),
                            e.n("session_generation"), e.n("participant_generation"))).or_default() += 1;
                    }
                    if e.text("kind") == "StandardRoom" && e.n("ordinal") == Some(0)
                        && e.n("published") == Some(1) {
                        let phase = index.phases.entry((e.n("tick"), e.n("loop"), e.n("owner_token"))).or_default();
                        phase.standard_publish += 1;
                        phase.publish_sequence = e.seq();
                        phase.publish_generation = e.n("generation");
                        phase.normal_publish = e.n("authority_pre")==Some(1)
                            && e.n("native_commit")==Some(1) && e.text("owner")=="Game";
                    }
                    if e.text("kind")=="OddPlayer" && e.n("ordinal")==Some(0)
                        && e.n("published")==Some(1) && e.n("native_commit")==Some(1)
                        && e.n("authority_pre")==Some(1) {
                        *index.native_odd.entry((e.n("tick"),e.n("owner_token"))).or_default() += 1;
                    }
                }
                "tick_phase" => {
                    let counts = index.phases.entry((e.n("tick"), e.n("loop"), e.n("owner_token")))
                        .or_default();
                    match e.text("phase") {
                        "pre" => { counts.pre += 1; counts.pre_sequence=e.seq(); counts.pre_generation=e.n("generation"); },
                        "commit" => { counts.commit += 1; counts.commit_sequence=e.seq(); counts.commit_generation=e.n("generation"); },
                        _ => {}
                    }
                }
                "persistent" => match e.text("phase") {
                    "enter" => index.persistent_entries.entry(e.n("call")).or_default().push(e),
                    "return" => index.persistent_returns.entry(e.n("call")).or_default().push(e),
                    _ => {}
                },
                "persist_window" if e.text("event") == "close" => {
                    index.window_closes.entry(e.n("id")).or_default().push(e);
                }
                "loading" if e.text("event") == "begin" => {
                    index.loading_begins.entry((e.n("episode"), e.n("session_generation")))
                        .or_default().push(e);
                }
                "loading_unscoped_query" => {
                    index.loading_unscoped.entry((e.n("episode"),e.n("session_generation")))
                        .or_default().push(e);
                }
                _ => {}
            }
        }
        index
    }

    // 每个 tick 仍逐条校验：仅同代次/ordinal/sequence/target 的唯一 Accepted 可匹配。
    pub fn command_matches(&self, tick: &Event) -> bool {
        let neutral = tick.pair("move") == Some((0, 0))
            && tick.pair("aim") == Some((0, 0))
            && tick.n("held") == Some(0) && tick.n("edge") == Some(0);
        if tick.n("input_sequence") == Some(0) { return neutral; }
        let key = (tick.n("ordinal"), tick.n("session_generation"), tick.n("input_generation"),
            tick.n("input_sequence"), tick.n("tick"));
        let commands = self.accepted.get(&key).map(Vec::as_slice).unwrap_or(&[]);
        commands.len() == 1 && commands[0].seq() < tick.seq()
            && ["move", "aim"].iter().all(|k| commands[0].pair(k).is_some()
                && commands[0].pair(k) == tick.pair(k))
            && ["held", "edge"].iter().all(|k| commands[0].n(k).is_some()
                && commands[0].n(k) == tick.n(k))
    }

    // 按同 tick/loop/scope 返回全量计数，不因重复覆盖而制造唯一见证。
    pub fn phase_counts(&self, tick: Number, loop_id: Number, scope: Number) -> PhaseCounts {
        self.phases.get(&(tick, loop_id, scope)).copied().unwrap_or_default()
    }

    // gap 不携带已发布 tick；只按当前候选 tick/scope 找实际奇数帧提交，禁止借用最近一帧。
    pub fn odd_native(&self, tick: Number, scope: Number) -> bool {
        self.native_odd.get(&(tick,scope)) == Some(&1)
    }

    // Player 读取依据仍要求同真实代次、正常原生消费返回；这里只替换查找方式。
    pub fn native_player(&self, ordinal: Number, tick: Number, session: Number, generation: Number) -> bool {
        self.native_players.get(&(ordinal, tick, session, generation)) == Some(&1)
    }

    // L6 需要唯一消费，返回整个同键组，保留重复 tick 的拒绝能力。
    pub fn ticks(&self, generation: Number, tick: Number, ordinal: Number) -> &[&'a Event] {
        self.ticks.get(&(generation, tick, ordinal)).map(Vec::as_slice).unwrap_or(&[])
    }

    // 原规则只要求 Ack 之前至少存在一条同会话 A tick；最早序号与 any 语义等价。
    pub fn ack_has_prior_tick(&self, ack: &Event) -> bool {
        match ack.n("completed") {
            None => false,
            Some(0) => true,
            tick => self.earliest_a_tick.get(&(ack.n("generation"), tick))
                .is_some_and(|seq| *seq < ack.seq()),
        }
    }

    // 调用返回保留整组，正常返回仍须唯一且晚于进入。
    pub fn returns(&self, call: Number) -> &[&'a Event] {
        self.persistent_returns.get(&call).map(Vec::as_slice).unwrap_or(&[])
    }

    // 输入 events 已按原始序号排序；父调用存在性保持“至少一条更早进入”的原语义。
    pub fn has_prior_entry(&self, call: Number, sequence: u64) -> bool {
        self.persistent_entries.get(&call).and_then(|v| v.first())
            .is_some_and(|e| e.seq() < sequence)
    }

    // 持久窗口关闭保留重复组，不改变原有恰好一次/正常返回判定。
    pub fn closes(&self, id: Number) -> &[&'a Event] {
        self.window_closes.get(&id).map(Vec::as_slice).unwrap_or(&[])
    }

    // 与旧 find 一样取同 episode/代次的首条 begin，保持缺字段和事件顺序语义。
    pub fn loading_begin(&self, episode: Number, generation: Number) -> Option<&'a Event> {
        self.loading_begins.get(&(episode, generation)).and_then(|v| v.first()).copied()
    }
    // 保留整个同代次窗口的返回值见证；数量和边界在最终V12判定中一起核对。
    pub fn loading_unscoped(&self, episode: Number, generation: Number) -> &[&'a Event] {
        self.loading_unscoped.get(&(episode,generation)).map(Vec::as_slice).unwrap_or(&[])
    }

    // 对原始序号开区间查询；反向/空区间本来不可能含 tick 或 submit。
    pub fn changes_between(&self, begin: u64, end: u64) -> bool {
        begin < end && self.changing_sequences.range((Excluded(begin), Excluded(end))).next().is_some()
    }

    // 可复查的规模信息明确本次使用全量索引，无随机采样。
    pub fn summary(&self) -> Value {
        json!({"mode":"FULL_INDEXED","source_events":self.source_events,
            "accepted_groups":self.accepted.len(),"tick_groups":self.ticks.len(),
            "phase_groups":self.phases.len(),"sampled":false})
    }
}
