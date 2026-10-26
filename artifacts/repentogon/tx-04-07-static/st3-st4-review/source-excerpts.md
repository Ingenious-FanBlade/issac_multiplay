# tx-04-07 ST3/ST4 主审源码摘录
2026-10-25。仅为源码事实；实际 Hook 安装、顺序、条件和结果仍非运行观察。

## F:\projects_isaac\issac_multiplay\mod\tools\gen_binding.py

```text
55:             source.append(f'    cs::game::PersistenceScope persistence("{ident}", this{persistence_args});')
56:         if ident=='room_update':
57:             source.append('    cs::game::SimulationScope simulation(cs::game::SimulationKind::Room, this);')
58:         elif ident=='odd_helper':
59:             source.append('    cs::game::SimulationScope simulation(cs::game::SimulationKind::Odd);')
60:         elif ident=='player_update':
61:             source.append('    cs::game::PlayerScope player_scope(this);')
62:         elif ident=='player_native':
63:             source.append('    cs::game::PlayerNativeReached(this);')
64:         elif ident=='join_pass':
65:             source.append('    cs::game::JoinScope join_scope(this);')
66:         elif ident=='input_update':
67:             source += ['    cs::game::InputLease input_lease(cs::game::HostBusy());',
68:                        '    if (!input_lease.Ready()) cs::game::InputLeaseUnavailable();']
69:         elif ident=='game_exit':
70:             source.append('    if (scope.main) cs::game::RunTeardownBeginning();')
71:         elif ident=='device_update':
72:             source.append('    cs::game::DeviceScope device_scope(this);')
73:         elif ident=='restore':
74:             source += ['    if (scope.main) { cs::game::RestoreBeginning(); cs::RunExit(); }']
75:         consumers={'move':'Movement','bomb':'Bomb','active_item':'Item',
76:                    'card_pill':'Pill','shoot':'Shooting','drop':'Drop'}
77:         if ident in consumers:
78:             source.append(f'    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::{consumers[ident]});')
79:             if ident=='shoot': source.append('    if (!consumer.CallOriginal()) return;')
80:         source.append(f'    {"auto result = " if ret!="void" else ""}super({call});')
81:         if ident=='device_update': source.append('    device_scope.Returned(result);')
82:         if ident=='input_update': source.append('    if (scope.main) cs::game::InputFrameFinished();')
83:         if ident=='console_check': source.append('    if (scope.main) cs::game::ObserveConsole(result);')
84:         if ident=='new_room': source.append('    if (scope.main) cs::game::NewRoomReturned();')
85:         if ident=='game_exit': source.append('    if (scope.main) cs::RunExit();')
86:         if ret!='void': source.append('    return result;')
87:     source.append('}')
88:     originals = [i['id'] for i in binding['items'] if int(i.get('v12',{}).get('target') or '0',16)==rva]
89:     manifest.append(dict(id=ident, group=group, definition=name, rva=hex(rva), priority=priority,
```

## F:\projects_isaac\issac_multiplay\mod\src\game\lifecycle.cpp

```text
37: }
38: void RefreshLifecycle(SessionRuntime &r, bool authority) noexcept {
39:   if (r.finished || r.retiring || r.join.scope ||
40:       (r.mode == core::SessionMode::Host && !r.peer.established))
41:     return;
42:   std::array<Topology, 2> values{ReadTopology(), {}};
43:   const bool remote = r.mode == core::SessionMode::Host && r.join.complete;
44:   if (remote)
45:     values[1] = ReadTopology(1, r.device.Object());
46:   const auto &value = values[0];
47:   if (value.known && !value.run) {
48:     // 主菜单握手后的 Warmup 无限等待首次 run；真正经历 run 后离开才是
49:     // teardown。
50:     if (r.mode == core::SessionMode::Host && !r.run_observed) {
51:       r.topology = values;
52:       r.blocks = Lifecycle;
53:       r.observed = false;
54:       DiscardInputEdges();
55:       DiscardPeerWindow(r);
56:       return;
57:     }
58:     r.run_torn_down = true;
59:     StopSession(core::SessionCause::RunTeardown);
60:     return;
61:   }
62:   if (value.run)
63:     r.run_observed = true;
64:   if (r.mode == core::SessionMode::Host && value.known && value.run &&
65:       value.player_count && value.player_count != (remote ? 2u : 1u)) {
66:     FailRuntime("host_player_count_changed");
67:     return;
68:   }
69:   const auto old_blocks = r.blocks;
70:   unsigned blocks = 0;
71:   if (!value.known || !value.unique || !value.Alive() || !ContextHealthy())
72:     blocks |= Lifecycle;
73:   if (remote) {
74:     if (values[1].dead || values[1].removed || values[1].ghost)
75:       r.remote_life_lost = true;
76:     if (!values[1].known || !values[1].unique || !values[1].Alive() ||
77:         r.remote_life_lost)
78:       InvalidateRemote(r, "remote_lifecycle_unavailable");
79:   }
80:   if (r.console_blocked || value.pause > 0 ||
81:       (!value.focused && value.pause_on_focus_lost))
82:     blocks |= Pause;
83:   if (value.starting || value.room_transition || value.stage_transition)
84:     blocks |= Loading;
85:   if (r.observed) {
86:     for (unsigned i = 0; i < (remote ? 2u : 1u); ++i) {
87:       if (!(values[i].room == r.topology[i].room))
88:         InvalidateBinding(r, "room_identity_changed", true);
89:       else if (!SameLife(values[i], r.topology[i])) {
90:         if (i == 1)
91:           InvalidateRemote(r, "player_lifecycle_changed");
92:         else
93:           InvalidateBinding(r, "player_lifecycle_changed", true);
94:       } else if (!(values[i].pair == r.topology[i].pair) ||
95:                  values[i].unique != r.topology[i].unique) {
96:         if (i == 1)
97:           InvalidateRemote(r, "exact_pair_changed");
98:         else
99:           InvalidateBinding(r, "exact_pair_changed", true);
100:       }
101:     }
102:   }
103:   if ((blocks & ~old_blocks) != 0)
104:     InvalidateBinding(r, "block_enter", false);
105:   if ((old_blocks & ~blocks) != 0)
106:     InvalidateBinding(r, "block_resume", false);
107:   if (r.loop <= r.defer_through)
108:     blocks |= Lifecycle;
109:   r.blocks = blocks;
110:   r.topology = values;
111:   r.observed = true;
112:   if (old_blocks != blocks)
113:     Trace("block_interval",
114:           "from=%u to=%u loop=%llu console=%u pause=%d focus=%u "
115:           "room_transition=%d stage_transition=%d reason=%s",
116:           old_blocks, blocks, r.loop, r.console_blocked, value.pause,
117:           value.focused, value.room_transition, value.stage_transition,
118:           value.reason);
119:   if (authority && !blocks) {
120:     for (unsigned i = 0; i < (remote ? 2u : 1u); ++i) {
121:       if (r.players[i].BoundValid())
122:         continue;
123:       if (!values[i].unique || !values[i].Alive() ||
124:           (i == 1 && r.remote_life_lost))
125:         continue;
126:       if (!r.players[i].Bind(values[i].pair)) {
127:         FailRuntime("unique_binding_rejected");
128:         return;
129:       }
130:       r.controlled_player[i] = values[i].pair.player;
131:       if (i == 1)
132:         DiscardPeerWindow(r);
133:       Trace("binding",
134:             "ordinal=%u bound=1 generation=%llu loop=%llu "
135:             "reason=stable_authority",
136:             i, r.players[i].Generation(), r.loop);
137:     }
138:   }
139: }
140: // Warmup 的稳定 Authority 只负责 K 的一次创建/等待 Join，不分配输入或推进
141: // tick。
142: bool ReadyForAuthority(SessionRuntime &r) noexcept {
143:   RefreshLifecycle(r, true);
144:   if (r.retiring || r.blocks || !r.players[0].BoundValid())
145:     return false;
146:   if (r.mode == core::SessionMode::Host) {
147:     if (r.device.State() == DeviceKState::Empty) {
148:       if (!r.device.CreateOnce())
149:         FailRuntime("device_k_creation_failed");
150:       return false;
151:     }
152:     if (r.device.State() != DeviceKState::Registered) {
153:       FailRuntime("device_k_lost");
154:       return false;
155:     }
156:     if (!r.join.complete ||
157:         (r.session.Status().state == core::SessionState::Warmup &&
158:          !r.players[1].BoundValid()))
159:       return false;
160:   }
161:   if (r.session.Status().state == core::SessionState::Warmup) {
162:     if (!r.session.Activate(r.players[0].BoundValid(),
163:                             r.players[1].BoundValid())) {
164:       FailRuntime("activate_failed");
165:       return false;
166:     }
```

```text
192:   if (auto *r = Runtime(); r && !r->finished && !r->retiring) {
193:     InvalidateBinding(*r, "new_room_returned", true);
194:     RefreshLifecycle(*r, false);
195:   }
196: }
197: // 在原生清 Player 之前关闭会话；之后退休不再对旧 B 调 RemoveCoPlayer。
198: void RunTeardownBeginning() noexcept {
199:   if (!IsMain())
200:     return;
201:   if (auto *r = Runtime(); r && !r->finished)
202:     r->run_torn_down = true;
203:   StopSession(core::SessionCause::RunTeardown);
204: }
205: void RestoreBeginning() noexcept { RunTeardownBeginning(); }
206: } // namespace cs::game
```

## F:\projects_isaac\issac_multiplay\mod\src\game\tick.cpp

```text
29: // Authority-pre 的唯一入口：先绑定与激活，再 drain、统一 Submit、Resolve
30: // current。
31: bool BeginTick(SessionRuntime &r, core::TickKind kind,
32:                uint64_t token) noexcept {
33:   if (!RuntimeHealthy() || InBlockedContext())
34:     return false;
35:   if (!ReadyForAuthority(r))
36:     return false;
37:   core::TickBoundary boundary{kind,
38:                               r.loop,
39:                               kind == core::TickKind::StandardRoom
40:                                   ? core::TickOwnerKind::Game
41:                                   : core::TickOwnerKind::OddNormal,
42:                               token,
43:                               GetCurrentThreadId(),
44:                               MainThread};
45:   auto decision = r.pipeline.BeginDrain(boundary, false);
46:   if (decision != core::TickDecision::Started) {
47:     FailRuntime("begin_drain_rejected", decision);
48:     return false;
49:   }
50:   if (!SubmitInput(r)) {
51:     FailRuntime("input_admission_failed");
52:     return false;
53:   }
54:   decision = r.pipeline.BeginSimulation();
55:   if (decision != core::TickDecision::Started) {
56:     FailRuntime("begin_simulation_rejected", decision);
57:     return false;
58:   }
59:   r.boundary = boundary;
60:   r.last_decision = decision;
61:   for (unsigned i = 0; i < r.session.ParticipantCount(); ++i) {
62:     r.metrics[i] = {};
63:     r.metrics[i].authority_pre = 1;
64:     r.metrics[i].input_sequence = r.players[i].CurrentSequence();
65:     r.tick_input[i] = r.players[i].Current();
66:   }
67:   Trace(
68:       "tick_phase",
69:       "phase=pre tick=%llu loop=%llu owner_token=%llu generation=%llu owner=%s",
70:       r.clock.Current(), r.loop, token, r.session.Identity().generation,
71:       boundary.owner_kind == core::TickOwnerKind::Game ? "Game" : "OddNormal");
72:   return true;
73: }
74: void NativeReturn(SessionRuntime &r, bool normal) noexcept {
75:   if (!normal) {
```

```text
145:   return Busy() && solo->mode == core::SessionMode::Solo;
146: }
147: bool HostBusy() noexcept {
148:   return Busy() && solo->mode == core::SessionMode::Host;
149: }
150: bool Armed() noexcept {
151:   return Busy() &&
152:          (solo->mode == core::SessionMode::Solo || solo->peer.established);
153: }
154: bool RuntimeHealthy() noexcept {
```

```text
223: // 主菜单也可监听；只有传输握手成功后才准备 core 并接管 A。
224: const char *StartHost(const net::WorkerConfig &config, int character) noexcept {
225:   if (!IsMain() || TopOwner() != Owner::Manager)
226:     return "wrong_boundary";
227:   if (Busy())
228:     return "busy";
229:   if (!ContextHealthy() || PointerCount() != 8 || !OddPatchInstalled() ||
230:       !HostPatchesInstalled() || !Capabilities[size_t(Group::Join)])
231:     return "adaptation_unavailable";
232:   if (generation == (std::numeric_limits<uint64_t>::max)())
233:     return "generation_exhausted";
234:   try {
235:     auto next = std::make_unique<SessionRuntime>();
236:     next->mode = core::SessionMode::Host;
237:     next->join.character = character;
238:     next->peer.session_generation = ++generation;
239:     next->loop = manager_loop;
240:     const auto started = next->peer.worker.Start(config);
241:     if (!started)
242:       return net::StartErrorName(started.error);
243:     next->peer.identity = started.identity;
244:     solo = std::move(next);
245:     foreign_seen = false;
246:     Trace("session",
247:           "from=Idle to=Hosting mode=Host generation=%llu port=%u role=%d",
248:           generation, config.port, character);
249:     return "";
250:   } catch (...) {
251:     return "session_allocation_failed";
252:   }
253: }
254: void StopSolo(core::SessionCause cause) noexcept {
255:   if (SoloBusy())
256:     StopSession(cause);
257: }
258: void StopSession(core::SessionCause cause) noexcept {
259:   auto *r = Runtime();
260:   if (!r || r->finished || r->retiring)
261:     return;
262:   const auto *before = core::SessionStateName(r->session.Status().state);
263:   if (r->mode == core::SessionMode::Host && !r->peer.established) {
264:     // Hosting 尚未启动 core，也要退休 worker；不伪造 Warmup/绑定。
265:   } else if (!r->session.BeginRetirement(cause))
266:     return;
267:   r->retirement_cause = cause;
268:   r->retiring = true;
269:   DiscardInputEdges();
270:   Trace("session", "from=%s to=Retiring reason=%s generation=%llu loop=%llu",
271:         before, core::SessionCauseName(cause), r->session.Identity().generation,
272:         r->loop);
273: }
```

```text
290: }
291: void BeginManager(uint64_t loop) noexcept {
292:   manager_loop = loop;
293:   BeginContextFrame();
294:   if (auto *r = Runtime()) {
295:     r->loop = loop;
296:     if (foreign_seen.exchange(false))
297:       FailRuntime("foreign_callback");
298:     if (!r->finished) {
299:       PollPeer(*r);
300:       armed_public = Armed();
301:       if (Armed())
302:         RefreshLifecycle(*r, false);
303:       DrainPeer(*r);
304:     }
305:   }
306: }
307: // NativeReturned 与 TopExit 分离；Disabled 的 post-state 仍必须提交 completed。
308: void EndManager(bool normal) noexcept {
309:   auto *r = Runtime();
310:   if (!r || r->finished)
311:     return;
312:   if (!normal)
313:     FailRuntime("manager_exception");
314:   if (normal &&
315:       r->pipeline.Status().phase == core::TickPipelinePhase::AwaitPostState) {
316:     auto result = r->pipeline.FinishPostState(r->loop);
317:     r->last_decision = result;
318:     if (result == core::TickDecision::Committed &&
319:         r->pipeline.PublishCompleted(TraceDropped() == 0)) {
320:       if (r->boundary.kind == core::TickKind::StandardRoom)
321:         ++r->ticks_standard;
322:       else
323:         ++r->ticks_odd;
324:       for (unsigned i = 0; i < r->session.ParticipantCount(); ++i)
325:         PublishTick(*r, i);
326:     } else
327:       FailRuntime("tick_publication_failed");
328:   }
329:   const auto end = r->pipeline.EndOuter(r->loop);
330:   if (end != core::TickDecision::Idle)
331:     FailRuntime("missing_native_return", end);
332:   if (normal)
333:     PublishPeer(*r);
334:   if (r->retiring && !r->cleanup_failed) {
335:     if (HasOwner(Owner::Odd) || HasOwner(Owner::Device) ||
336:         HasOwner(Owner::Join) || HasOwner(Owner::Input) ||
337:         HasOwner(Owner::Game)) {
338:       ContextFault("retirement_callback_pending");
339:       return;
340:     }
341:     r->device.Neutralize();
342:     const bool device_ok =
343:         r->mode != core::SessionMode::Host || r->device.RemoveOnce();
344:     const bool player_ok = device_ok && (r->mode != core::SessionMode::Host ||
345:                                          RetireJoinedPlayer(*r));
346:     const bool peer_ok = StopPeer(*r);
347:     if (!device_ok || !player_ok || !peer_ok) {
348:       r->cleanup_failed = true;
349:       Trace("retirement_error", "device=%u player=%u peer=%u loop=%llu",
350:             device_ok, player_ok, peer_ok, r->loop);
351:       return; // 保留未知现场及常驻对象，不重复原生移除，不把失败报告为 Idle。
352:     }
353:     for (unsigned i = 0; i < r->session.ParticipantCount(); ++i)
354:       r->players[i].Retire();
355:     if (r->peer.established || r->mode == core::SessionMode::Solo)
356:       r->session.CompleteRetirement(true);
357:     r->finished = true;
358:     r->controlled_player.fill(nullptr);
359:     r->blocks = 0;
360:     armed_public = false;
361:     Trace("session",
362:           "from=Retiring to=Idle reason=%s generation=%llu core_state=%s "
363:           "loop=%llu",
364:           r->failure ? r->last_error
365:                      : core::SessionCauseName(r->retirement_cause),
366:           r->session.Identity().generation,
367:           core::SessionStateName(r->session.Status().state), r->loop);
368:   }
```

```text
370: SimulationScope::SimulationScope(SimulationKind kind, void *object) noexcept
371:     : kind_(kind), main_(IsMain()), exceptions_(std::uncaught_exceptions()) {
372:   if (!main_ || !Armed())
373:     return;
374:   auto &r = *Runtime();
375:   if (kind == SimulationKind::Odd) {
376:     token_ = OwnerToken(Owner::Odd);
377:     if (r.odd_scope) {
378:       FailRuntime("odd_scope_reentry");
379:       return;
380:     }
381:     r.odd_scope = token_;
382:     r.odd_started = false;
383:   } else if (TopOwner() == Owner::Game && HasOwner(Owner::Manager)) {
384:     token_ = OwnerToken(Owner::Game);
385:     if (object != ReadTopology().room.room) {
386:       FailRuntime("room_owner_mismatch");
387:       return;
388:     }
389:     started_ = BeginTick(r, core::TickKind::StandardRoom, token_);
390:   } else {
391:     Trace("room_scope", "loop=%llu owner=%u owner_token=%llu eligible=0",
392:           r.loop, unsigned(TopOwner()), OwnerToken(TopOwner()));
393:   }
394: }
395: SimulationScope::~SimulationScope() noexcept {
396:   if (!main_ || !token_)
397:     return;
398:   auto *r = Runtime();
399:   if (!r || r->finished)
400:     return;
401:   const bool normal = exceptions_ == std::uncaught_exceptions();
402:   if (kind_ == SimulationKind::Odd) {
403:     ClearOddMarkers(token_);
404:     if (r->odd_scope != token_) {
405:       FailRuntime("odd_return_scope_mismatch");
406:       return;
407:     }
408:     started_ = r->odd_started;
409:     r->odd_scope = 0;
410:   }
411:   if (started_ &&
412:       r->pipeline.Status().phase == core::TickPipelinePhase::Simulating) {
413:     if (r->boundary.scope_token != token_) {
414:       FailRuntime("simulation_return_token_mismatch",
415:                   core::TickDecision::WrongReturn);
416:       return;
417:     }
418:     NativeReturn(*r, normal);
419:   } else if (!normal)
420:     FailRuntime("simulation_exception");
421: }
422: // 在普通 shim 标记对应的首个 A Player 入口开始 Odd tick，不看返回地址或桩帧。
423: bool BeginOddPlayer(void *player) noexcept {
424:   auto *r = Runtime();
425:   if (!r || r->finished || r->retiring || !r->odd_scope ||
426:       !TakeOddNormal(player))
427:     return false;
428:   const auto topology = ReadTopology();
429:   if (topology.pair.player != player || r->odd_started)
430:     return false;
431:   if (BeginTick(*r, core::TickKind::OddPlayer, r->odd_scope)) {
432:     r->odd_started = true;
433:     return true;
434:   }
435:   return false;
436: }
437: std::string StatusFields() {
438:   auto *r = Runtime();
```

## F:\projects_isaac\issac_multiplay\mod\src\game\peer.cpp

```text
38:   if (r.mode != core::SessionMode::Host || r.finished)
39:     return;
40:   auto &p = r.peer;
41:   const auto status = p.worker.Status();
42:   if (status.state != p.observed.state ||
43:       status.terminal != p.observed.terminal)
44:     Trace("net", "event=state state=%s terminal=%s error=%u native=%u generation=%llu rx=%llu tx=%llu duplicate=%llu stale=%llu",
45:           net::protocol::PeerStateName(status.state),
46:           net::TerminalReasonName(status.terminal),
47:           unsigned(status.protocol_error), status.native_error,
48:           p.session_generation, status.received_packets, status.sent_packets,
49:           status.duplicate_packets, status.stale_packets);
50:   p.observed = status;
51:   // 粘性 terminal 优先于任何输入或激活；队列满也不会丢失断线事实。
52:   switch (status.terminal) {
53:   case net::TerminalReason::PeerBye:
54:     StopSession(core::SessionCause::PeerBye);
55:     return;
56:   case net::TerminalReason::PeerTimeout:
57:     StopSession(core::SessionCause::PeerTimeout);
58:     return;
59:   case net::TerminalReason::PeerProtocol:
60:     StopSession(core::SessionCause::PeerProtocol);
61:     return;
62:   case net::TerminalReason::RunTeardown:
63:     StopSession(core::SessionCause::RunTeardown);
64:     return;
65:   case net::TerminalReason::InternalFailure:
66:     FailRuntime("network_worker_failed");
67:     return;
68:   case net::TerminalReason::Stop:
69:     if (!r.retiring)
70:       StopSession(core::SessionCause::Stop);
71:     return;
72:   default:
73:     break;
74:   }
75:   if (r.retiring || p.established ||
76:       status.state != net::protocol::PeerState::Established)
77:     return;
78:   core::SessionIdentity identity{p.identity.session, p.identity.clients,
79:                                  p.session_generation};
80:   if (!r.session.Start(identity, core::SessionMode::Host)) {
81:     FailRuntime("host_identity_rejected");
82:     return;
83:   }
84:   for (unsigned i = 0; i < 2; ++i)
85:     r.players[i].Initialize(identity.session, identity.clients[i], i,
86:                             &r.session);
87:   p.established = true;
88:   DiscardPeerWindow(r);
89:   Trace("session", "from=Hosting to=Warmup mode=Host generation=%llu loop=%llu",
90:         identity.generation, r.loop);
91: }
92: // 每个 Manager 最多 32 包、128 条命令；冗余从旧到新入同一个 Participant
93: // admission。 当前绑定代次来自主线程 owner，接收前已积压的旧窗口绝不被赋予新
94: // generation。
95: void DrainPeer(SessionRuntime &r) noexcept {
96:   if (r.mode != core::SessionMode::Host || !r.peer.established || r.retiring)
97:     return;
98:   net::ReceivedInput packet{};
99:   for (unsigned n = 0; n < 32 && r.peer.worker.TryReceive(packet); ++n) {
100:     r.peer.observed_sequence =
101:         (std::max)(r.peer.observed_sequence, packet.input.command.sequence);
102:     if (!r.session.AcceptsInput() || r.blocks || !r.players[1].BoundValid() ||
103:         packet.received_ms <= r.peer.receive_after_ms ||
104:         packet.acknowledged_generation != r.players[1].Generation() ||
105:         packet.acknowledged_state != core::SessionState::Active) {
106:       r.peer.discarded += 1 + packet.input.redundancy_count;
107:       r.peer.discarded_sequence_floor =
108:           (std::max)(r.peer.discarded_sequence_floor,
109:                      packet.input.command.sequence);
110:       continue;
111:     }
112:     const auto submit = [&](const core::InputCmdDraftV2 &cmd) noexcept {
113:       if (cmd.sequence <= r.peer.discarded_sequence_floor) {
114:         ++r.peer.discarded;
115:         return true;
116:       }
117:       const auto decision = r.players[1].AcceptInput(
118:           cmd, r.players[1].Generation(), r.clock.Completed());
119:       ++r.peer.admitted;
120:       Trace("submit",
121:             "ordinal=1 generation=%llu sequence=%llu target=%llu loop=%llu "
122:             "packet=%llu session_generation=%llu decision=%u edge=%u move=%d,%d aim=%d,%d held=%u",
123:             r.players[1].Generation(), cmd.sequence, cmd.target_tick, r.loop,
124:             packet.packet_sequence, r.peer.session_generation, unsigned(decision), cmd.state.edge_flags,
125:             cmd.state.move_x, cmd.state.move_y, cmd.state.aim_x,
126:             cmd.state.aim_y, cmd.state.held_flags);
127:       if (decision == core::Decision::SequenceConflict) {
128:         FailRuntime("remote_sequence_conflict");
129:         return false;
130:       }
131:       return true;
132:     };
133:     for (unsigned i = 0; i < packet.input.redundancy_count; ++i)
134:       if (!submit(packet.input.redundant[i]))
```

## F:\projects_isaac\issac_multiplay\mod\src\game\join.cpp

```text
53: }
54: JoinScope::JoinScope(void *manager) noexcept
55:     : lease_(HostBusy()), exceptions_(std::uncaught_exceptions()) {
56:   auto *r = Runtime();
57:   if (!r || r->mode != core::SessionMode::Host || r->finished || r->retiring)
58:     return;
59:   if (!lease_.Ready()) {
60:     FailJoin(*r, "join_input_lock_unavailable");
61:     return;
62:   }
63:   if (r->join.scope) {
64:     FailJoin(*r, "join_scope_reentry");
65:     return;
66:   }
67:   if (!r->peer.established ||
68:       r->session.Status().state != core::SessionState::Warmup ||
69:       r->device.State() != DeviceKState::Registered || r->join.complete)
70:     return;
71:   const auto topology = ReadTopology();
72:   if (r->blocks || r->console_blocked || !topology.run || !topology.unique ||
73:       !topology.Alive() || topology.pause || topology.starting ||
74:       topology.room_transition || topology.stage_transition ||
75:       !r->players[0].BoundValid())
76:     return;
77:   // ProcessInput 的 Join 在 Manager::Update 中、Game::Update 之前，不能要求
78:   // Game owner。
79:   if (TopOwner() != Owner::Join || !HasOwner(Owner::Manager) ||
80:       HasOwner(Owner::Start) || HasOwner(Owner::Restore) ||
81:       HasOwner(Owner::Door) || HasOwner(Owner::Lifecycle))
82:     return;
83:   const auto expected = reinterpret_cast<unsigned char *>(topology.room.game) +
84:                         offset::PlayerManager;
85:   if (manager != expected || topology.player_count != 1 ||
86:       !(topology.pair == r->players[0].Bound())) {
87:     FailJoin(*r, "join_initial_topology");
88:     return;
89:   }
90:   auto &j = r->join;
91:   if (j.passes &&
92:       (j.manager != manager || j.host_character != topology.character ||
93:        !(j.host == topology.pair) || !(j.room == topology.room))) {
94:     FailJoin(*r, "join_identity_changed");
95:     return;
96:   }
97:   j.manager = manager;
98:   j.host = topology.pair;
99:   j.host_character = topology.character;
100:   j.room = topology.room;
101:   j.state = j.passes ? JoinState::Confirm : JoinState::Request;
102:   j.action = j.passes ? 14 : 19;
103:   j.scope = active_ = true;
104:   Trace("join", "event=begin pass=%u action=%d key=%d loop=%llu", j.passes + 1,
105:         j.action, r->device.Id(), r->loop);
106: }
107: // 不按查询次数消耗；原生自动确认在 P1 创建时，返回后立即收口，不再发 P2。
108: JoinScope::~JoinScope() noexcept {
109:   if (!active_)
110:     return;
111:   auto *r = Runtime();
112:   if (!r)
113:     return;
114:   auto &j = r->join;
115:   j.scope = false;
116:   j.action = -1;
117:   ++j.passes;
118:   if (exceptions_ != std::uncaught_exceptions()) {
119:     FailJoin(*r, "join_exception");
120:     return;
121:   }
122:   if (r->retiring)
123:     return;
124:   const auto a = ReadTopology(), b = ReadTopology(1, r->device.Object());
125:   if (j.creation_calls && b.unique && b.pair.key == r->device.Id())
```

```text
174: bool JoinQuery(void *device, int action, QueryKind query,
175:                float &result) noexcept {
176:   auto *r = Runtime();
177:   if (!r || !r->join.scope || device != r->device.Object())
178:     return false;
179:   ++r->join.query_count;
180:   result = !r->retiring && !r->blocks && query == QueryKind::Triggered &&
181:                    action == r->join.action
182:                ? 1.0f
183:                : 0.0f;
184:   return true;
185: }
186: // 原 predicate 已由裸 shim 恰好调用一次；此处只决定自有 key 的结果。
187: bool JoinMode(bool original, void *manager, int key, void *row, unsigned site) noexcept {
188:   auto *r = Runtime();
189:   if (!r || !r->join.scope || key != r->device.Id())
190:     return original;
191:   ++r->join.mode_calls;
192:   Trace("join", "event=mode site=%u original=%u result=0 native_calls=1 key=%d loop=%llu",
193:         site, original, key, r->loop);
194:   int row_key = -1;
195:   if (manager != r->join.manager || (row && !ValidRow(manager, row))) {
196:     FailJoin(*r, "join_mode_context");
197:     return false;
198:   }
199:   if (row && (!ReadMemory(static_cast<unsigned char *>(row) - 8, &row_key, 4) ||
200:               row_key != key))
201:     FailJoin(*r, "join_mode_row");
202:   if (original)
203:     ++r->join.baby_overrides;
204:   return false;
```

## F:\projects_isaac\issac_multiplay\mod\src\game\topology.cpp

```text
47: Topology ReadTopology(unsigned ordinal, void *owned_device) noexcept {
48:   Topology out;
49:   if (!IsMain() || !Base)
50:     return out;
51:   InputLease lease(HostBusy());
52:   if (!lease.Ready() || ordinal > 1)
53:     return out;
54:   out.room.game = GamePointer();
55:   if (!ReadAt(out.room.game, offset::RunActive, out.run))
56:     return out;
57:   if (!out.run) {
58:     out.known = true;
59:     out.reason = "not_in_run";
60:     return out;
61:   }
62:   void *manager = nullptr;
63:   if (!ReadMemory(reinterpret_cast<void *>(Base + offset::Manager), &manager,
64:                   sizeof(manager)) ||
65:       !ReadAt(manager, offset::Focus, out.focused) ||
66:       !ReadAt(manager, offset::PauseOnFocusLost, out.pause_on_focus_lost) ||
67:       !ReadAt(manager, offset::Starting, out.starting) ||
68:       !ReadAt(out.room.game, offset::CurrentRoom, out.room.room) ||
69:       !ReadAt(out.room.game, 0, out.room.stage) ||
70:       !ReadAt(out.room.game, 4, out.room.stage_type) ||
71:       !ReadAt(out.room.game, offset::RoomIndex, out.room.index) ||
72:       !ReadAt(out.room.game, offset::Dimension, out.room.dimension) ||
73:       !ReadAt(out.room.game, offset::PauseScreen, out.pause) ||
74:       !ReadAt(out.room.game, offset::RoomTransition, out.room_transition) ||
75:       !ReadAt(out.room.game, offset::StageTransition, out.stage_transition))
76:     return out;
77:   out.known = true;
78:   uintptr_t players = 0;
79:   size_t player_count = 0;
80:   if (!VectorRange(out.room.game, offset::PlayerManager, 16, players,
81:                    player_count) ||
82:       player_count <= ordinal) {
83:     out.player_count = static_cast<unsigned>(player_count);
84:     out.reason = "player_list_unknown";
85:     return out;
86:   }
87:   out.player_count = static_cast<unsigned>(player_count);
88:   if (!ReadMemory(reinterpret_cast<void *>(players + ordinal * sizeof(void *)),
89:                   &out.pair.player, sizeof(void *)))
90:     return out;
91:   int index = -1;
92:   unsigned type = 0;
93:   if (!ReadAt(out.pair.player, offset::PlayerIndex, index) ||
94:       index != int(ordinal) ||
95:       !ReadAt(out.pair.player, offset::EntityType, type) || type != 1 ||
96:       !ReadAt(out.pair.player, offset::EntityVariant, out.variant) ||
97:       out.variant != 0 ||
98:       !ReadAt(out.pair.player, offset::PlayerType, out.character) ||
99:       !ReadAt(out.pair.player, offset::ControllerIndex, out.pair.key) ||
100:       !ReadAt(out.pair.player, offset::EntityValid, out.valid) ||
101:       !ReadAt(out.pair.player, offset::EntityExists, out.exists) ||
```

## F:\projects_isaac\issac_multiplay\mod\src\game\context.cpp

```text
24: } // namespace
25: Owner TopOwner() noexcept {
26:   return depth ? stack[depth - 1].owner : Owner::None;
27: }
28: uint64_t OwnerToken(Owner owner) noexcept {
29:   for (unsigned i = depth; i; --i)
30:     if (stack[i - 1].owner == owner)
31:       return stack[i - 1].token;
32:   return 0;
33: }
34: bool HasOwner(Owner owner) noexcept { return OwnerToken(owner) != 0; }
35: bool ContextHealthy() noexcept { return healthy; }
36: bool InBlockedContext() noexcept {
37:   return HasOwner(Owner::Start) || HasOwner(Owner::Restore) ||
38:          HasOwner(Owner::Lifecycle) || HasOwner(Owner::Door) ||
39:          HasOwner(Owner::NestedDoor) || HasOwner(Owner::Join);
40: }
```

```text
75:   healthy = true;
76: }
77: void __stdcall EnterOddNormal(void *player) noexcept {
78:   if (!IsMain())
79:     return;
80:   if (odd.token || TopOwner() != Owner::Odd) {
81:     ContextFault("odd_marker_reentry");
82:     return;
83:   }
84:   odd = {player, OwnerToken(Owner::Odd), PushOwner(Owner::OddNormal), false};
85:   if (Armed())
86:     Trace("odd_call", "phase=enter token=%llu owner_token=%llu loop=%llu", odd.token, odd.owner, ManagerLoop());
87: }
88: void __stdcall LeaveOddNormal() noexcept {
89:   if (!IsMain())
90:     return;
91:   if (odd.token)
92:   {
93:     if (Armed())
94:       Trace("odd_call", "phase=leave token=%llu owner_token=%llu loop=%llu player_entries=%u", odd.token, odd.owner, ManagerLoop(), odd.player_entries);
95:     PopOwner(odd.token);
96:   }
97:   odd = {};
98: }
99: bool TakeOddNormal(void *player) noexcept {
100:   if (!ContextHealthy() || TopOwner() != Owner::OddNormal ||
101:       odd.player != player || odd.owner != OwnerToken(Owner::Odd) ||
102:       odd.consumed)
103:     return false;
104:   odd.consumed = true;
105:   return true;
106: }
107: void RecordOddPlayerEntry() noexcept {
108:   if (TopOwner() == Owner::OddNormal && odd.token)
109:     ++odd.player_entries;
110:   else if (HasOwner(Owner::Odd))
111:     Trace("odd_special_player", "loop=%llu normal_marker=0 tick=%llu", ManagerLoop(),
112:           Runtime() ? Runtime()->pipeline.SimulationTick() : 0);
113: }
114: void ClearOddMarkers(uint64_t odd_token) noexcept {
115:   // 原生异常可能越过裸 shim，Odd RAII 在弹出 owner 前收走尚未离开的标记。
```

## F:\projects_isaac\issac_multiplay\mod\src\hooks\context.cpp

```text
10: CallScope::CallScope(Hook &h) noexcept
11:     : hook(h), main(IsMain()), exceptions(std::uncaught_exceptions()) {
12:   if (h.calls.fetch_add(1) == 0)
13:     Trace("first_call", "%s owner=%u", h.id, unsigned(CurrentOwner()));
14:   if (!main) {
15:     h.foreign.fetch_add(1);
16:     game::ForeignCall();
17:     return;
18:   }
19:   if (h.owner != Owner::None) {
20:     token = game::PushOwner(h.owner);
21:     pushed = token != 0;
22:   }
23: }
24: // 异常和正常返回都弹出本层；只有正常返回才增加 completed。
25: CallScope::~CallScope() noexcept {
26:   if (exceptions == std::uncaught_exceptions()) {
27:     // 首次进入与首次正常返回分开记录，不能由 first_call 推断原函数已返回。
28:     if (hook.completed.fetch_add(1) == 0)
29:       Trace("first_return", "%s", hook.id);
30:   } else {
31:     if (main)
32:       game::ContextFault("native_exception");
33:     Trace("context_error", "exception id=%s", hook.id);
34:   }
35:   if (pushed)
36:     game::PopOwner(token);
37: }
38: FrameScope::FrameScope(bool enabled) noexcept
39:     : main(enabled), exceptions(std::uncaught_exceptions()) {
40:   if (main)
41:     FrameBegin();
42: }
43: FrameScope::~FrameScope() noexcept {
44:   if (main)
45:     FrameEnd(exceptions == std::uncaught_exceptions());
```

## F:\projects_isaac\issac_multiplay\mod\src\ops\command.cpp

```text
285:   }
286: }
287: void RunExit() noexcept {
288:   game::StopSession(core::SessionCause::RunTeardown);
289:   // 清掉旧 run 的排队变更，避免返回菜单后旧 host 命令复活会话。
290:   Mutation ignored{};
291:   while (mutations.Pop(ignored)) {
292:   }
293:   pending_host_start = pending_solo_start = false;
294:   Trace("state", "to=%s reason=run_exit frame=%llu", game::StateName(), frame);
295:   Trace("run_exit", "state=%s pending_mutations=0 resident=1",
296:         game::StateName());
297: }
298: } // namespace cs
```

## F:\projects_isaac\issac_multiplay\mod\src\game\device_k.cpp

```text
83: float Query(void *self, int action, QueryKind kind) noexcept {
84:   auto *p = Find(self);
85:   bool enabled = false;
86:   if (!p || !IsMain() || p->state.load() != DeviceKState::Registered ||
87:       !ReadAt(self, offset::DeviceEnabled, enabled) || !enabled)
88:     return 0;
89:   float result = 0;
90:   if (JoinQuery(self, action, kind, result))
91:     return result;
92:   return OverrideQuery(self, action, kind, result) ? result : 0;
93: }
94: bool __fastcall Pressed(void *self, void *, int action) noexcept {
95:   return Query(self, action, QueryKind::Pressed) != 0;
96: }
97: bool __fastcall Triggered(void *self, void *, int action) noexcept {
98:   return Query(self, action, QueryKind::Triggered) != 0;
99: }
100: float __fastcall Scalar(void *self, void *, int action) noexcept {
101:   return Query(self, action, QueryKind::Scalar);
102: }
103: bool __fastcall Poll(void *self, void *) noexcept {
104:   auto *p = Find(self);
105:   return p && p->state.load() == DeviceKState::Registered;
106: }
107: // 自有类型 RemoteInputDevice 单继承 InputDeviceBase：层次为“自身 + 基类 COL 的
```

```text
168: bool IsDeviceK(const void *device) noexcept { return Find(device) != nullptr; }
169: bool IsDeviceKId(int id) noexcept {
170:   if (id < 0)
171:     return false;
172:   for (auto *p = roots.load(std::memory_order_acquire); p; p = p->next)
173:     if (p->id.load(std::memory_order_acquire) == id)
174:       return true;
175:   return false;
176: }
```

```text
209: bool DeviceK::CreateOnce() noexcept {
210:   PersistenceOperation persistence("device_register");
211:   if (!IsMain() || attempted_)
212:     return false;
213:   attempted_ = true;
214:   if (!HostPatchesInstalled() || !PrepareTable())
215:     return false;
216:   InputLease lease(true);
217:   if (!lease.Ready())
218:     return false;
219:   HMODULE pinned = nullptr;
220:   if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
221:                               GET_MODULE_HANDLE_EX_FLAG_PIN,
222:                           reinterpret_cast<LPCWSTR>(&Query), &pinned))
223:     return false;
224:   auto *p = new (std::nothrow) OwnedRecord;
225:   if (!p)
226:     return false;
227:   record_ = p;
228:   Trace("device_k", "event=allocated object=%p never=1", Object());
229:   p->next = roots.load(std::memory_order_relaxed);
230:   roots.store(p, std::memory_order_release);
231:   // 从进入构造起即 NEVER；异常时不析构、不重试，不声称部分 Init 已恢复。
232:   p->state.store(DeviceKState::BaseConstructed);
233:   try {
234:     reinterpret_cast<void *(__thiscall *)(void *)>(Base + offset::DeviceCtor)(
235:         p->object);
236:     p->id.store(Id(), std::memory_order_release);
237:     Trace("device_k", "event=constructed id=%d object=%p never=1", Id(), Object());
238:     if (Id() < 0 || DeviceIdOccurrences(Id()) != 0)
239:       return false;
240:     const char *name = device_name;
241:     std::memcpy(p->object + offset::DeviceName, &name, sizeof(name));
242:     auto **vtable = *reinterpret_cast<void ***>(p->object);
243:     if (!reinterpret_cast<bool(__thiscall *)(void *, void *)>(vtable[1])(
244:             p->object, nullptr))
245:       return false;
246:     p->state.store(DeviceKState::Initialized);
247:     Trace("device_k", "event=initialized id=%d object=%p never=1", Id(), Object());
248:     *reinterpret_cast<uintptr_t **>(p->object) = table.slots;
249:     const auto manager = reinterpret_cast<void *>(Base + offset::InputManager);
250:     auto **slots = *reinterpret_cast<void ***>(manager);
251:     registration_attempted_ = true;
252:     reinterpret_cast<void(__thiscall *)(void *, void *)>(slots[45])(manager,
253:                                                                     p->object);
254:     if (DeviceOccurrences(p->object) != 1 || DeviceIdOccurrences(Id()) != 1)
255:       return false;
256:     p->state.store(DeviceKState::Registered);
257:     Trace("device_k", "event=registered id=%d object=%p never=1", Id(),
258:           Object());
259:     return true;
260:   } catch (...) {
261:     Trace("device_k", "event=create_exception object=%p never=1", Object());
262:     return false;
263:   }
264: }
265: void DeviceK::Neutralize() noexcept {
266:   auto *p = static_cast<OwnedRecord *>(record_);
267:   if (p && p->state.load() != DeviceKState::Removed && p->state.load() != DeviceKState::Retiring) {
268:     p->state.store(DeviceKState::Retiring);
269:     Trace("device_k", "event=retiring object=%p neutral=1 never=1", Object());
270:   }
271: }
```

## F:\projects_isaac\issac_multiplay\mod\src\game\query.cpp

```text
8: namespace cs::game {
9: // 返回 true 表示本次属于受控 A/B gameplay 查询，即使资格不足也必须输出中性。
10: bool OverrideQuery(void *device, int action, QueryKind kind,
11:                    float &result) noexcept {
12:   if (!IsMain() || !Armed() || action < 0 || action >= 12)
13:     return false;
14:   auto *context = CurrentConsumer();
15:   const auto ordinal = context ? ControlledOrdinal(context->player) : -1;
16:   if (ordinal < 0)
17:     return false;
18:   auto &r = *Runtime();
19:   RuntimeHealthy();
20:   ++r.overrides;
21:   result = 0;
22:   const auto tick = r.pipeline.SimulationTick();
23:   auto &p = r.players[ordinal];
24:   int device_id = -1, controller = -1;
25:   if (tick && !r.blocks && !r.failure && ContextHealthy() &&
26:       !InBlockedContext() && p.BoundValid() &&
27:       p.Bound().player == context->player && p.Bound().device == device &&
28:       ReadAt(device, offset::DeviceId, device_id) &&
29:       ReadAt(context->player, offset::ControllerIndex, controller) &&
30:       device_id == p.Bound().key && controller == p.Bound().key) {
31:     result =
32:         p.Read(unsigned(action), static_cast<core::Query>(kind),
33:                static_cast<core::Consumer>(context->kind), p.Bound(), tick);
34:   }
35:   RecordQuery(unsigned(ordinal), context->kind, kind, action, result);
36:   return true;
37: }
38: } // namespace cs::game
```

## F:\projects_isaac\issac_multiplay\mod\src\hooks\host_patch.cpp

```text
12: uintptr_t resumes[11]{}, alternatives[11]{};
13: extern "C" bool __stdcall CsOwnedDevice(void *device) noexcept { return IsDeviceK(device); }
14: extern "C" bool __stdcall CsJoinMode(bool original, void *manager, int key, void *row, unsigned site) noexcept {
15:   return JoinMode(original, manager, key, row, site);
16: }
17: extern "C" bool __stdcall CsJoinCreate(void *manager, void *row, int *role) noexcept {
18:   return JoinCreate(manager, row, role);
19: }
20: // 三处 mode 都恰好调用原 predicate；helper 的 GP/EFLAGS/x87/MXCSR/XMM 副作用被隔离。
```

```text
125: // 所有 K 分支只按常驻自有对象地址判断，物理设备重放完整被覆盖指令。
126: __declspec(naked) void RegisterNotify() {
127:   __asm {
128:     pushfd
129:     pushad
130:     mov ebx, esp
131:     sub esp, 528
132:     and esp, -16
133:     fxsave [esp]
134:     push dword ptr [ebx+16]
135:     call CsOwnedDevice
136:     test al, al
137:     jnz owned_saved
138:     fxrstor [esp]
139:     mov esp, ebx
140:     popad
141:     popfd
142:     mov esi, [edi+14h]
143:     mov ebx, [ebx+8]
144:     jmp dword ptr [resumes+16]
145:   owned_saved:
146:     fxrstor [esp]
147:     mov esp, ebx
148:     popad
149:     popfd
150:     jmp dword ptr [alternatives+16]
151:   }
152: }
153: __declspec(naked) void RemoveNotify() {
154:   __asm {
155:     pushfd
156:     pushad
157:     mov ebx, esp
158:     sub esp, 528
159:     and esp, -16
160:     fxsave [esp]
161:     push dword ptr [ebx+0]
162:     call CsOwnedDevice
163:     test al, al
164:     jnz owned_saved
165:     fxrstor [esp]
166:     mov esp, ebx
167:     popad
168:     popfd
169:     mov esi, [ebx+20h]
170:     cmp esi, [ebx+24h]
171:     jmp dword ptr [resumes+20]
172:   owned_saved:
173:     fxrstor [esp]
174:     mov esp, ebx
175:     popad
176:     popfd
177:     jmp dword ptr [alternatives+20]
178:   }
179: }
180: __declspec(naked) void PollNotify() {
181:   __asm {
182:     pushfd
183:     pushad
184:     mov ebx, esp
185:     sub esp, 528
186:     and esp, -16
187:     fxsave [esp]
188:     push dword ptr [ebx+24]
189:     call CsOwnedDevice
190:     test al, al
191:     jnz owned_saved
192:     fxrstor [esp]
193:     mov esp, ebx
194:     popad
195:     popfd
196:     mov esi, [ebx+20h]
197:     mov eax, [ecx+8]
198:     jmp dword ptr [resumes+24]
199:   owned_saved:
200:     fxrstor [esp]
201:     mov esp, ebx
202:     popad
203:     popfd
204:     mov eax, [ecx+8]
205:     mov [ebp-1Ch], eax
206:     jmp dword ptr [alternatives+24]
207:   }
208: }
209: __declspec(naked) void PhysicalDisconnect() {
210:   __asm {
211:     pushfd
```

## F:\projects_isaac\issac_multiplay\mod\src\core\session.cpp

```text
94: bool Session::Start(const SessionIdentity &identity,
95:                     SessionMode mode) noexcept {
96:   if (Status().state != SessionState::Dormant)
97:     return false;
98:   const bool valid_clients =
99:       mode == SessionMode::Solo
100:           ? identity.clients[1] == 0
101:           : mode == SessionMode::Host && identity.clients[1] != 0 &&
102:                 identity.clients[0] != identity.clients[1];
103:   if (!valid_clients || !identity.generation || !identity.clients[0] ||
104:       std::all_of(identity.session.begin(), identity.session.end(),
105:                   [](auto byte) { return byte == 0; })) {
106:     Fail(SessionCause::Configuration);
107:     return false;
108:   }
109:   identity_ = identity;
110:   mode_ = mode;
111:   return Move(SessionState::Dormant, SessionState::Warmup,
112:               SessionCause::Prepared);
113: }
114: bool Session::Activate(bool host_bound, bool remote_bound) noexcept {
115:   if (AcceptsInput())
116:     return true;
117:   const bool bindings_ready =
118:       host_bound && (mode_ == SessionMode::Solo ||
119:                      (mode_ == SessionMode::Host && remote_bound));
120:   return bindings_ready &&
121:          Move(SessionState::Warmup, SessionState::Active,
122:               SessionCause::BindingsReady);
123: }
124: // 退休立即关闭新的 admission；已经开始的模拟仍可正常返回并完成发布。
125: bool Session::BeginRetirement(SessionCause cause) noexcept {
126:   const bool peer_cause = cause == SessionCause::PeerBye ||
127:                          cause == SessionCause::PeerTimeout ||
128:                          cause == SessionCause::PeerProtocol;
129:   if (cause != SessionCause::Stop && cause != SessionCause::RunTeardown &&
130:       !(mode_ == SessionMode::Host && peer_cause))
131:     return false;
132:   auto status = status_.load(std::memory_order_acquire);
133:   for (;;) {
134:     const auto state = static_cast<SessionState>(status & 0xFFu);
135:     if (state == SessionState::Failed || state == SessionState::Dormant)
136:       return false;
137:     if (state == SessionState::Retiring || state == SessionState::Retired)
138:       return true;
139:     if (status_.compare_exchange_weak(status,
140:                                       Pack(SessionState::Retiring, cause),
141:                                       std::memory_order_acq_rel))
142:       return true;
143:   }
144: }
145: bool Session::CompleteRetirement(bool callbacks_drained) noexcept {
146:   if (!callbacks_drained)
147:     return false;
148:   if (Status().state == SessionState::Failed)
149:     return true; // 清理不能抹掉原始失败。
150:   return Move(SessionState::Retiring, SessionState::Retired,
151:               SessionCause::CleanupComplete);
152: }
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\LuaInterfaces\LuaGame.cpp

```text
104: LUA_FUNCTION(lua_GameStartStageTransition) {
105: 	Game* game = lua::GetLuabridgeUserdata<Game*>(L, 1, lua::Metatables::GAME, "Game");
106: 	bool sameStage = lua::luaL_checkboolean(L, 2);
107: 	int transition = (int)luaL_checkinteger(L, 3);
108: 	Entity_Player* player = nullptr;
109: 	if (lua_type(L, 4) == LUA_TUSERDATA) {
110: 		player = lua::GetLuabridgeUserdata<Entity_Player*>(L, 4, lua::Metatables::ENTITY_PLAYER, "Player");
111: 	}
112: 
113: 	game->StartStageTransition(sameStage, transition, player);
114: 	return 0;
115: }
```

```text
418: 		{ "GetDebugFlags", Lua_GameGetDebugFlags},
419: 		{ "AddDebugFlags", Lua_GameAddDebugFlags},
420: 		{ "SpawnBombCrater", Lua_GameSpawnBombCrater},
421: 		{ "DevolveEnemy", Lua_GameDevolveEnemy},
422: 		{ "IsGreedBoss", Lua_GameIsGreedBoss},
423: 		{ "IsGreedFinalBoss", Lua_GameIsGreedFinalBoss},
424: 		{ "StartStageTransition", lua_GameStartStageTransition},
425: 		{ "AddErasedEnemy", Lua_GameAddErasedEnemy},
426: 		{ "RemoveErasedEnemy", Lua_GameRemoveErasedEnemy},
```

## F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Game.zhl

```text
1: "538bdc83ec0883e4f883c404558b6b??896c24??8bec6aff68????????64a1????????505383ec105657a1????????33c5508d45??64a3????????8bf9897d??897d":
2: __thiscall void Game::constructor();
3: 
4: "558bec6aff68????????64a1????????5083ec40a1????????33c58945??535657508d45??64a3????????c745??00000000":
5: __thiscall void Game::StartDebug(int levelStage,int stageType,int difficulty,std_string* unk); 
6: 
7: "558bec6aff68????????64a1????????5081eca0000000a1????????33c58945??535657508d45??64a3????????8b1d":
8: __thiscall void Game::StartFromRerunState(GameState* state);
9: 
10: "558bec6aff68????????64a1????????5081ecac000000a1????????33c58945??535657508d45??64a3????????8bf9":
11: __thiscall void Game::RestoreState(GameState* state,bool startGame);
12: 
13: "558bec6aff68????????64a1????????5083ec78a1????????33c58945??535657508d45??64a3????????8bf1":
14: __thiscall void Game::SaveState(GameState* state);
15: 
16: "c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c681????????00":
17: __thiscall void Game::ResetState();
```

```text
68: "558bec83e4f883ec2453568b75??8bd95783fe06":
69: __thiscall void Game::StartStageTransition(bool SameStage,int param_2,Entity_Player *param_3);
```

```text
284: 	uint32_t _greedModeTreasureRoomIdx : 0x18370;
285: 	RoomConfig _roomConfig : 0x18910;
286: 	ItemPool _itemPool : 0x1a740;
287: 	BossPool _bossPool: 0x1af70;
288: 	RoomTransition _roomTransition : 0x1b83c;
289: 	StageTransition _stageTransition : 0x1ba78;
290: 	PlayerManager _playerManager : 0x1baa8;
291: 	Seeds _seedEffects : 0x1bb84;
```

## F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\StageTransition.zhl

```text
1: struct StageTransition {
2:     bool _sameStage : 0x10;
3: } : 0x20;
```

## F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\PlayerManager.zhl

```text
7: "558bec83e4f883ec0c538b5d??56578bf98b0d":
8: __thiscall Entity_Player* PlayerManager::FirstCollectibleOwner(CollectibleType collectibleID,RNG **rng, bool lazSharedGlobalTag);
```

## F:\projects_isaac\REPENTOGON-1.1.3\include\IsaacRepentance.h

```text
1631:         COLLECTIBLE_NANCY_BOMBS = 563,
1632:         COLLECTIBLE_BAR_OF_SOAP = 564,
1633:         COLLECTIBLE_BLOOD_PUPPY = 565,
1634:         COLLECTIBLE_DREAM_CATCHER = 566,
1635:         COLLECTIBLE_PASCHAL_CANDLE = 567,
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\Patches\ASMPatches\ASMPlayerManager.cpp

```text
42: /*
43: * The below patches primarily enable preservation of the ControllerIndex for twins and coplayers spawned by mods, preventing the "Controller Disconnected" popups.
44: * 
45: * For the ControllerIndexes, there are two places where this is relevant: PlayerManager::AutoReassignControllers and PauseScreen::ControllerDisconnectHandling.
46: * AutoReassignControllers runs once after continuing a run, first setting the ControllerIndex of all players to -1 and then re-assigning the available controllers.
47: * Vanilla twins/subplayers/etc are handled appropriately by the SetControllerIndex function, but ones created by mods are mistaken for separate co-op players.
48: * If a player cannot be assigned a controller here (or if a controller is disconnected later) the "Controller Disconnected" popup will kick in.
49: * 
50: * It is worth noting that GameState remembers and restores the ControllerIndex values for all players, however on continues it is overridden by AutoReassignControllers.
51: * 
52: * To generally account for mods, we make the assumption that any players that have the same ControllerIndex should be reassigned to the same ControllerIndex.
53: * This assumption likely makes no difference for vanilla, where twins/subplayers/etc are all already handled. And in general, it feels reasonable.
54: * 
55: * When the game "unassigns" a controller in either location, we'll unassign every player with the same original ControllerIndex.
56: * Then, when the game "reassigns" a new controller, we'll also reassign every other player that originally shared their ControllerIndex.
57: * 
58: * Additionally, the vanilla game's ControllerDisconnectHandling is bugged and will not work in some cases, such as if Jacob & Esau are a co-op player.
59: * In vanilla Repentance+, attempting to continue a run where Jacob & Esau was player 2+ will cause you to get stuck in the "Controller Disconnected" popup.
60: * This is because after Jacob's ControllerIndex is reassigned, it will unassign Esau's controller again afterwards, due to referring to outdated information.
61: * Jacob & Esau's ControllerIndexes are both set at the same time in either case, so this repeats.
62: * 
63: * To fix this, we force the the game to break its player loop after reassigning a controller in ControllerDisconnectHandling.
64: * This is a bit of an ungraceful fix, but it works fine in practice because if there are still players to assign, the game will do it next update.
```

```text
69: // Unassign other stray players with the same ControllerIndex and keep track of all their prior ControllerIndexes (ie, modded twins and coplayers).
70: void __stdcall UnassignController(Entity_Player* player) {
71: 	int previousControllerIndex = player->_controllerIndex;
72: 
73: 	player->SetControllerIndex(-1, true);
74: 
75: 	EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(player);
76: 	if (playerPlus && previousControllerIndex > -1) {
77: 		for (Entity_Player* otherPlayer : g_Game->GetPlayerManager()->_playerList) {
78: 			EntityPlayerPlus* otherPlayerPlus = GetEntityPlayerPlus(otherPlayer);
79: 			if (otherPlayerPlus && otherPlayer->_controllerIndex == previousControllerIndex) {
80: 				ZHL::Log("[ASMPlayerManager::UnassignController] Transitively unassigning ControllerIndex %d from player %d (via player %d)\n", previousControllerIndex, otherPlayer->_playerIndex, player->_playerIndex);
81: 				otherPlayer->SetControllerIndex(-1, false);
82: 				otherPlayerPlus->previousControllerIndex = previousControllerIndex;
83: 			}
84: 		}
85: 		playerPlus->previousControllerIndex = previousControllerIndex;
86: 	}
87: }
88: static void PatchUnassignController(char* def) {
89: 	void* addr = sASMDefinitionHolder->GetDefinition(def);
90: 
91: 	ZHL::Log("[REPENTOGON] PatchUnassignController @ %p\n", addr);
92: 
93: 	ASMPatch::SavedRegisters savedRegisters(ASMPatch::SavedRegisters::Registers::GP_REGISTERS_STACKLESS, true);
94: 	ASMPatch patch;
95: 	patch.Pop(ASMPatch::Registers::EAX)
96: 		.Pop(ASMPatch::Registers::EAX)
97: 		.PreserveRegisters(savedRegisters)
98: 		.Push(ASMPatch::Registers::ECX)
99: 		.AddInternalCall(UnassignController)
100: 		.RestoreRegisters(savedRegisters)
101: 		.AddRelativeJump((char*)addr + 0x5);
102: 	sASMPatcher.PatchAt(addr, &patch);
103: }
104: 
105: // Used to remember that a controller was reassigned in ControllerDisconnectHandling between two patches.
106: // The second patch is at the end of the for loop and cannot be missed.
107: bool reassignedDisconnectedController = false;
108: 
109: // Patches over where the game reassigns a controller (PlayerManager::AutoReassignControllers and PauseScreen::ControllerDisconnectHandling).
110: // Assign the same controller to any other players that previously shared this player's ControllerIndex (ie, modded twins and coplayers).
111: // For ControllerDisconnectHandling, additionally track that a controller was just reassigned, so that we can force ControllerDisconnectHandling
112: // to stop and properly re-check everything next update to circumvnet flaws with the vanilla handling.
113: void __stdcall ReassignController(Entity_Player* player, int newIndex, bool disconnectedController) {
114: 	const bool wasUnset = player->_controllerIndex == -1;
115: 
116: 	player->SetControllerIndex(newIndex, true);
117: 
118: 	EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(player);
119: 	if (wasUnset && playerPlus && playerPlus->previousControllerIndex > -1) {
120: 		for (Entity_Player* otherPlayer : g_Game->GetPlayerManager()->_playerList) {
121: 			EntityPlayerPlus* otherPlayerPlus = GetEntityPlayerPlus(otherPlayer);
122: 			if (otherPlayerPlus && otherPlayer->_controllerIndex == -1 && otherPlayerPlus->previousControllerIndex == playerPlus->previousControllerIndex) {
123: 				if (otherPlayer->_controllerIndex != newIndex) {
124: 					ZHL::Log("[ASMPlayerManager::ReassignController] Transitively re-assigning ControllerIndex %d to player %d (via player %d)\n", newIndex, otherPlayer->_playerIndex, player->_controllerIndex);
125: 					otherPlayer->SetControllerIndex(newIndex, false);
126: 				}
127: 				otherPlayerPlus->previousControllerIndex = -1;
128: 			}
129: 		}
130: 		playerPlus->previousControllerIndex = -1;
131: 	}
132: 
133: 	if (wasUnset && disconnectedController) {
134: 		reassignedDisconnectedController = true;
```

```text
137: static void PatchReassignController(char* def, bool disconnectedController) {
138: 	void* addr = sASMDefinitionHolder->GetDefinition(def);
139: 
140: 	ZHL::Log("[REPENTOGON] PatchReassignController @ %p\n", addr);
141: 
142: 	ASMPatch::SavedRegisters savedRegisters(ASMPatch::SavedRegisters::Registers::GP_REGISTERS_STACKLESS, true);
143: 	ASMPatch patch;
144: 	patch.Pop(ASMPatch::Registers::EAX)
145: 		.PreserveRegisters(savedRegisters)
146: 		.Push(disconnectedController)
147: 		.Push(ASMPatch::Registers::EAX)
148: 		.Push(ASMPatch::Registers::ECX)
149: 		.AddInternalCall(ReassignController)
150: 		.RestoreRegisters(savedRegisters)
151: 		.Pop(ASMPatch::Registers::EAX)
152: 		.AddRelativeJump((char*)addr + 0x5);
153: 	sASMPatcher.PatchAt(addr, &patch);
154: }
155: 
156: // Patch at the end of the player loop in ControllerDisconnectHandling.
157: // Break out of the loop if we just reassigned a controller.
158: // The game will not handle things properly if we don't, even for completely vanilla cases,
159: // such as Jacob & Esau as player 2 (this issue exists in vanilla Rep+).
160: bool __stdcall PostReassignDisconnectedController() {
161: 	if (reassignedDisconnectedController) {
162: 		reassignedDisconnectedController = false;
163: 		return true;
164: 	}
165: 	return false;
166: }
167: static void PatchPostReassignDisconnectedController() {
168: 	void* addr = sASMDefinitionHolder->GetDefinition(&AsmDefinitions::ControllerDisconnected_LoopEnd);
169: 
170: 	ZHL::Log("[REPENTOGON] PatchPostReassignDisconnectedController @ %p\n", addr);
171: 
172: 	ASMPatch::SavedRegisters savedRegisters(ASMPatch::SavedRegisters::Registers::GP_REGISTERS_STACKLESS, true);
173: 	ASMPatch patch;
174: 	patch.PreserveRegisters(savedRegisters)
175: 		.AddInternalCall(PostReassignDisconnectedController)
176: 		.RestoreRegisters(savedRegisters)
177: 		.AddConditionalRelativeJump(ASMPatcher::CondJumps::JNZ, (char*)addr + 0xC)  // Jump for true
178: 		.AddBytes(ByteBuffer().AddAny((char*)addr, 0x6))  // Restore the bytes we overwrote.
179: 		.AddRelativeJump((char*)addr + 0x6);
180: 	sASMPatcher.PatchAt(addr, &patch);
181: }
182: 
183: void ASMPatchReassignControllers() {
184: 	PatchUnassignController(&AsmDefinitions::AutoReassignControllers_Unassign);
185: 	PatchReassignController(&AsmDefinitions::AutoReassignControllers_Reassign, false);
186: 
187: 	PatchUnassignController(&AsmDefinitions::ControllerDisconnected_Unassign);
188: 	PatchReassignController(&AsmDefinitions::ControllerDisconnected_Reassign, true);
189: 
190: 	PatchPostReassignDisconnectedController();
191: }
192: 
193: // Hook after RestoreGameState to restore modded twins.
194: // We remember twin relationships ourselves (see EntitySaveStateEx.cpp) and restore that relationship here.
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\Patches\GameStateManagement.cpp

```text
219: HOOK_METHOD(Game, SaveState, (GameState* state) -> void)
220: {
221: 	super(state);
222: 
223: 	auto slot = get_game_state_slot(state);
224: 	if (slot == GameStateSlot::NULL_SLOT)
225: 	{
226: 		ZHL::Log("[INFO] [GameStateManagement] could not determine Game State slot, skipping save.\n");
227: 		return;
228: 	}
229: 
230: 	save_game_state(slot);
231: }
232: 
233: HOOK_METHOD(Game, RestoreState, (GameState* state, bool startGame) -> void)
234: {
235: 	super(state, startGame);
236: 
237: 	auto slot = get_game_state_slot(state);
238: 	if (slot == GameStateSlot::NULL_SLOT)
239: 	{
240: 		ZHL::Log("[INFO] [GameStateManagement] could not determine Game State slot, skipping restore.\n");
241: 		return;
242: 	}
243: 
244: 	restore_game_state(slot, startGame);
245: }
246: 
```

```text
316: HOOK_METHOD(GameState, read, (GameStateIO** io, bool isLocalRun) -> bool)
317: {
318: 	bool isRerun = false;
319: 	GameStateSaveInfo info = get_info(this, *io, isRerun);
320: 
321: 	bool originalSuccess = super(io, isLocalRun);
322: 	ESSM::detail::SaveData::ReadState essmReadState = ESSM::detail::SaveData::ReadGameState();
323: 	bool success = originalSuccess && !ESSM::detail::SaveData::CheckErrors(essmReadState);
324: 
325: 	if (!success)
326: 	{
327: 		if (originalSuccess)
328: 		{
329: 			ZHL::Log("[ERROR] [GameStateManagement] GameState failed internal validation on read.\n");
330: 		}
331: 		return false;
332: 	}
333: 
334: 	bool needsHandling = ESSM::detail::SaveData::NeedsHandling(essmReadState) ||
335: 		VirtualRoomSetManager::detail::GameStateNeedsHandling(*this);
336: 
337: 	if (!isLocalRun) // This occurs when loading a game state upon joining an already existing match.
338: 	{
339: 		ZHL::Log("[INFO] [GameStateManagement] reading non save file GameState, skipping read.\n");
340: 		return !needsHandling;
341: 	}
342: 
343: 	if (info.IsUnknown())
344: 	{
345: 		ZHL::Log("[INFO] [GameStateManagement] skipping GameState read.\n");
346: 		return !needsHandling;
347: 	}
348: 
349: 	if (!ESSM::detail::SaveData::Deserialize(info.fileName, essmReadState))
350: 	{
351: 		return false;
352: 	}
353: 
354: 	return read_save(*this, info, isRerun);
355: }
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\SaveStateManagement\EntitySaveStateManagement.cpp

```text
2410: HOOK_METHOD(Level, Init, (bool resetLilPortalRoom) -> void)
2411: {
2412:     ScopedDebugContext dbgCtx("Level::Init");
2413: 
2414:     auto& myosotisPickups = g_Game->_myosotisPickups;
2415: 
2416:     CollectedStates collection;
2417:     collection.reserve(myosotisPickups.size());
2418:     ESSM::Utils::CollectStates(myosotisPickups, collection);
2419:     ESSM::Core::ClearSaveStates(collection);
2420: 
2421:     super(resetLilPortalRoom);
2422: }
2423: 
```

```text
2631: HOOK_METHOD(Game, SaveState, (GameState* state) -> void)
2632: {
2633:     ScopedDebugContext context("Game::SaveState");
2634: 
2635:     // ASSUMPTION: It is assumed that the state has already been cleared.
2636:     // This is because SaveState always calls GameState::Clear before saving
2637: 	super(state);
2638:     
2639:     CollectedStates collection;
2640:     collection.reserve(DEFAULT_COLLECT_RESERVE);
2641:     ESSM::EntityIterators::InGameState::ForEachInRooms(*state, ESSM::Utils::CollectLambda, collection);
2642:     ESSM::EntityIterators::InGameState::ForEachInBackwardsStages(*state, ESSM::Utils::CollectLambda, collection);
2643:     ESSM::Core::CopySaveStates(collection);
2644: 
2645:     // players are handled by Entity_Player::Init
2646: }
2647: 
2648: HOOK_METHOD(Game, RestoreState, (GameState* state, bool startGame) -> void)
2649: {
2650:     ScopedDebugContext context("Game::RestoreState");
2651: 
2652:     CollectedStates collection;
2653:     collection.reserve(DEFAULT_COLLECT_RESERVE);
2654:     ESSM::EntityIterators::InGame::ForEachInBackwardsStages(ESSM::Utils::CollectLambda, collection);
2655:     ESSM::Core::ClearSaveStates(collection);
2656: 
2657:     // Reset backwards rooms
2658:     BackwardsStageDesc* backwardsStages = g_Game->_backwardsStages;
2659: 
2660:     for (size_t i = 0; i < NUM_BACKWARDS_STAGES; i++)
2661:     {
2662:         BackwardsStageDesc& backwardsStage = backwardsStages[i];
2663:         ESSM::Utils::ResetBackwardsStage(backwardsStage);
2664:     }
2665: 
2666:     super(state, startGame);
2667:     // copy is in a separate patch as copying it here might cause problems due to callbacks running in the mean time.
2668: }
2669: 
2670: HOOK_METHOD(Level, RestoreGameState, (GameState* state) -> void)
2671: {
2672:     ScopedDebugContext dbgCtx("Level::RestoreGameState");
2673: 
2674:     // ASSUMPTION: Confirm that the function does not reset the LilPortalRoom when calling reset_room_list (through params), since we currently have to perform that clear manually.
2675:     const size_t LIL_PORTAL_IDX = (-eGridRooms::ROOM_LIL_PORTAL_IDX) - 1;
2676:     reset_single_room(g_Game->_negativeGridRooms[LIL_PORTAL_IDX]);
2677:     super(state);
2678: }
2679: 
2680: HOOK_METHOD(GameStatePlayer, Init, () -> void)
```
