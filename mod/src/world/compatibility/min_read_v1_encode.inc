// 冻结的 v1 域编码器。借用 writer 私有 JSON sink 与计数接口，只读取自有值。
void WriteManifestFields(JsonOut &j, int depth) noexcept {
  Indent(j, depth);
  j.Lit("\"fields\":[\n");
  const auto fields = ProfileFields();
  for (std::size_t i = 0; i < fields.size(); ++i) {
    const auto &field = fields[i];
    Indent(j, depth + 1);
    j.Lit("{\"field_id\":");
    j.Hex(field.id, 4);
    j.Lit(",\"name\":");
    j.Str(field.name);
    j.Lit(",\"width\":");
    j.Num(field.width);
    j.Lit(",\"encoding\":");
    j.Str(EncodingName(field.encoding));
    j.Lit("}");
    j.Lit(i + 1 == fields.size() ? "\n" : ",\n");
  }
  Indent(j, depth);
  j.Lit("],\n");
}
void WriteManifestRequired(JsonOut &j, int depth) noexcept {
  Indent(j, depth);
  j.Lit("\"required\":{\n");
  for (std::uint32_t value = 0; value < kClassCount; ++value) {
    const auto kind = static_cast<Class>(value);
    const auto ids = ProfileFieldsFor(kind);
    Indent(j, depth + 1);
    j.Str(ClassName(kind));
    j.Lit(":[");
    for (std::size_t i = 0; i < ids.size(); ++i) {
      j.Hex(ids[i], 4);
      if (i + 1 != ids.size())
        j.Lit(",");
    }
    j.Lit("]");
    j.Lit(value + 1 == kClassCount ? "\n" : ",\n");
  }
  Indent(j, depth);
  j.Lit("},\n");
}
void WriteManifestLayout(JsonOut &j, const ManifestInput &input,
                         int depth) noexcept {
  const bool declared = input.layout_field_count > 0;
  const bool complete = input.layout_field_count >= ProfileFields().size();
  Indent(j, depth);
  j.Lit("\"layout\":{\n");
  Indent(j, depth + 1);
  j.Lit("\"state\":");
  j.Str(declared ? (complete ? "DECLARED" : "INCOMPLETE") : "UNDECLARED");
  j.Lit(",\n");
  Indent(j, depth + 1);
  j.Lit("\"declared_fields\":");
  j.Num(input.layout_field_count);
  j.Lit(",\n");
  Indent(j, depth + 1);
  j.Lit("\"fields\":[");
  for (std::size_t i = 0; i < input.layout_field_count; ++i) {
    const auto &field = input.layout_fields[i];
    j.Lit(i ? ",\n" : "\n");
    Indent(j, depth + 2);
    j.Lit("{\"field_id\":");
    j.Hex(field.id, 4);
    j.Lit(",\"offset\":");
    j.Num(field.offset);
    j.Lit(",\"evidence\":");
    j.Str(field.evidence ? field.evidence : "");
    j.Lit("}");
  }
  j.Lit("],\n");
  Indent(j, depth + 1);
  j.Lit("\"sources\":[");
  for (std::size_t i = 0; i < input.layout_source_count; ++i) {
    const auto &source = input.layout_sources[i];
    j.Lit(i ? ",\n" : "\n");
    Indent(j, depth + 2);
    j.Lit("{\"source\":");
    j.Num(source.source);
    j.Lit(",\"location\":");
    j.Str(source.location ? source.location : "");
    j.Lit(",\"capacity\":");
    j.Num(source.capacity);
    j.Lit(",\"evidence\":");
    j.Str(source.evidence ? source.evidence : "");
    j.Lit("}");
  }
  j.Lit("],\n");
  Indent(j, depth + 1);
  j.Lit("\"classes\":[");
  for (std::size_t i = 0; i < input.layout_class_count; ++i) {
    const auto &declaration = input.layout_classes[i];
    j.Lit(i ? ",\n" : "\n");
    Indent(j, depth + 2);
    j.Lit("{\"class\":");
    j.Str(ClassName(declaration.kind));
    j.Lit(",\"descriptor_rva\":");
    j.Str(declaration.descriptor_rva ? declaration.descriptor_rva : "");
    j.Lit(",\"evidence\":");
    j.Str(declaration.evidence ? declaration.evidence : "");
    j.Lit("}");
  }
  j.Lit("]\n");
  Indent(j, depth);
  j.Lit("},\n");
}

// 顶层字符串数组字段（假设/未知/未确认）。
void WriteStringArray(JsonOut &j, const char *key, const char *const *values,
                      std::size_t count, int depth) noexcept {
  Indent(j, depth);
  j.Key(key);
  j.Lit("[");
  for (std::size_t i = 0; i < count; ++i) {
    if (i)
      j.Lit(",");
    j.Str(values[i] ? values[i] : "");
  }
  j.Lit("],\n");
}

bool WriteManifest(WriterState &state) {
  const auto path = state.directory + L"\\manifest.json";
  if (!state.small.Open(path, kDiagnosticBufferBytes))
    return false;
  const auto &input = state.manifest;
  const auto status = state.storage->Status();
  JsonOut j(state.small);
  j.Lit("{\n");
  Indent(j, 1);
  j.Key("domain");
  j.Str(kDomain);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("version");
  j.Num(kSchemaVersion);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("profile");
  j.Str(kProfile);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("recording_generation");
  j.Num(input.recording_generation);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("session");
  j.Str(state.config.session);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("phase");
  j.Str(input.phase ? input.phase : "POST_STANDARD_UPDATE_PRE_TOP_EXIT");
  j.Lit(",\n");
  Indent(j, 1);
  j.Lit("\"producer\":{\"component\":");
  j.Str("zhlIsaacCS.dll");
  j.Lit(",\"build\":");
  j.Str(input.build ? input.build : "");
  j.Lit(",\"game\":");
  j.Str(input.game_version ? input.game_version : "");
  j.Lit(",\"repentogon\":");
  j.Str(input.repentogon_version ? input.repentogon_version : "");
  j.Lit("},\n");
  Indent(j, 1);
  j.Key("hook_manifest");
  j.Str(input.hook_manifest ? input.hook_manifest : "");
  j.Lit(",\n");
  // 实际启用的观察入口：19 项语义中哪些本次登记为启用。
  Indent(j, 1);
  j.Lit("\"observations\":{\"declared\":");
  j.Num(kObservationCount);
  j.Lit(",\"enabled\":[");
  {
    bool first = true;
    for (std::uint32_t i = 0; i < kObservationCount; ++i) {
      if (!input.observations || !input.observations[i])
        continue;
      if (!first)
        j.Lit(",");
      j.Str(ObservationName(static_cast<Observation>(i)));
      first = false;
    }
  }
  j.Lit("]},\n");
  Indent(j, 1);
  j.Key("classes");
  j.Lit("[");
  for (std::uint32_t i = 0; i < kClassCount; ++i) {
    if (i)
      j.Lit(",");
    j.Str(ClassName(static_cast<Class>(i)));
  }
  j.Lit("],\n");
  Indent(j, 1);
  j.Key("sources");
  j.Lit("[");
  for (std::uint32_t i = 0; i < kSourceCount; ++i) {
    if (i)
      j.Lit(",");
    j.Str(SourceName(i));
  }
  j.Lit("],\n");
  WriteManifestFields(j, 1);
  WriteManifestRequired(j, 1);
  WriteManifestLayout(j, input, 1);
  // 预算：同时登记设计上限与本次实际生效值。
  Indent(j, 1);
  j.Lit("\"budgets\":{");
  j.Lit("\"objects_per_batch\":");
  j.Num(kObjectBudget);
  j.Lit(",\"memberships_per_batch\":");
  j.Num(kMembershipBudget);
  j.Lit(",\"fields_per_object\":");
  j.Num(kFieldBudget);
  j.Lit(",\"player_relation_nodes\":");
  j.Num(kRelationNodes);
  j.Lit(",\"batches\":");
  j.Num(status.limits.slots);
  j.Lit(",\"identities\":");
  j.Num(kIdentityBudget);
  j.Lit(",\"events\":");
  j.Num(status.limits.event_capacity);
  j.Lit(",\"gap_records\":");
  j.Num(status.limits.gap_capacity);
  j.Lit(",\"checkpoints\":");
  j.Num(kCheckpointCapacity);
  j.Lit(",\"close_timeout_ms\":");
  j.Num(state.config.close_timeout_ms);
  j.Lit(",\"queue_bytes\":");
  j.Num(status.limits.queue_bytes);
  j.Lit(",\"queue_items\":");
  j.Num(status.limits.queue_items);
  j.Lit(",\"io_buffer_bytes\":");
  j.Num(kIoBufferBytes);
  j.Lit(",\"world_budget_bytes\":");
  j.Num(kWorldBudgetBytes);
  j.Lit(",\"planned_bytes\":");
  j.Num(PlannedWorldBytes());
  j.Lit(",\"allocated_bytes\":");
  j.Num(status.budget_used);
  j.Lit("},\n");
  // 枚举图例：离线工具据此解释数值列，不依赖隐式约定。
  Indent(j, 1);
  j.Lit("\"event_kinds\":{");
  {
    bool first = true;
    for (std::uint32_t i = 0; i <= static_cast<std::uint32_t>(EventKind::NativeBody); ++i) {
      if (i == 5)
        continue; // 原型保留空号，不登记为有效类别
      if (!first)
        j.Lit(",");
      j.Lit("\"");
      j.Num(i);
      j.Lit("\":");
      j.Str(EventKindName(static_cast<EventKind>(i)));
      first = false;
    }
  }
  j.Lit("},\n");
  Indent(j, 1);
  j.Lit("\"readiness_evidence\":{");
  for (std::uint32_t i = 0; i < 4; ++i) {
    if (i)
      j.Lit(",");
    j.Lit("\"");
    j.Num(i);
    j.Lit("\":");
    j.Str(ReadinessName(static_cast<Readiness>(i)));
  }
  j.Lit("},\n");
  Indent(j, 1);
  j.Lit("\"lifecycle\":{");
  for (std::uint32_t i = 0; i < 7; ++i) {
    if (i)
      j.Lit(",");
    j.Lit("\"");
    j.Num(i);
    j.Lit("\":");
    j.Str(LifecycleName(static_cast<Lifecycle>(i)));
  }
  j.Lit("},\n");
  Indent(j, 1);
  j.Lit("\"origin\":{");
  for (std::uint32_t i = 0; i < 3; ++i) {
    if (i)
      j.Lit(",");
    j.Lit("\"");
    j.Num(i);
    j.Lit("\":");
    j.Str(OriginName(static_cast<Origin>(i)));
  }
  j.Lit("},\n");
  Indent(j, 1);
  j.Lit("\"relation\":[");
  for (std::uint32_t i = 0; i < 5; ++i) {
    if (i)
      j.Lit(",");
    j.Str(RelationName(static_cast<Relation>(i)));
  }
  j.Lit("],\n");
  Indent(j, 1);
  j.Lit("\"field_status\":[");
  for (std::uint32_t i = 0; i < 5; ++i) {
    if (i)
      j.Lit(",");
    j.Str(FieldStatusName(static_cast<FieldStatus>(i)));
  }
  j.Lit("],\n");
  Indent(j, 1);
  j.Lit("\"encoding\":[");
  for (std::uint32_t i = 0; i < 4; ++i) {
    if (i)
      j.Lit(",");
    j.Str(EncodingName(static_cast<Encoding>(i)));
  }
  j.Lit("],\n");
  Indent(j, 1);
  j.Key("float_comparison");
  j.Str(kFloatComparison);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("hp_evidence");
  j.Str(kHpEvidence);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("current_player_read_rule");
  j.Str(kCurrentPlayerRule);
  j.Lit(",\n");
  WriteStringArray(j, "unconfirmed", kUnconfirmedFields,
                   std::size(kUnconfirmedFields), 1);
  WriteStringArray(j, "assumptions", input.assumptions, input.assumption_count,
                   1);
  WriteStringArray(j, "unknowns", input.unknowns, input.unknown_count, 1);
  WriteStringArray(j, "native_diagnostics_excluded", kNativeDiagnosticsExcluded,
                   std::size(kNativeDiagnosticsExcluded), 1);
  Indent(j, 1);
  j.Key("runtime_acceptance");
  j.Str("NOT_RUN");
  j.Lit("\n}\n");
  if (!j.Ok())
    return false;
  // manifest 先于任何 capture 落盘：失败即准备失败，不开始录制。
  return state.small.Close();
}

// ===========================================================================
// captures.jsonl / lifecycle-events.jsonl / gap 记录
// ===========================================================================

// 成员关系按对象分组：计数 → 起始偏移 → 稳定填充，避免 O(对象×成员) 扫描。
bool GroupMemberships(WriterState &state, const OwnedBatch &batch) noexcept {
  const auto objects = batch.object_count;
  const auto memberships = batch.membership_count;
  auto *start = state.membership_start.get();
  auto *cursor = state.membership_cursor.get();
  std::fill_n(start, static_cast<std::size_t>(objects) + 1, 0u);
  for (std::uint32_t i = 0; i < memberships; ++i) {
    const auto object = batch.memberships[i].object;
    if (object >= objects)
      return false;
    ++start[object + 1];
  }
  for (std::uint32_t i = 1; i <= objects; ++i)
    start[i] += start[i - 1];
  std::copy_n(start, objects, cursor);
  for (std::uint32_t i = 0; i < memberships; ++i) {
    const auto &membership = batch.memberships[i];
    state.sorted_memberships[cursor[membership.object]++] = membership;
  }
  return true;
}

bool WriteField(JsonOut &j, const RawField &field) noexcept {
  const auto *declaration = ProfileField(field.id);
  // 与 profile 不符的字段不能写进新 domain：这是采集器错误，不是未知值。
  if (!declaration || declaration->width != field.width ||
      declaration->encoding != field.encoding)
    return false;
  if (field.status != FieldStatus::Value &&
      field.status != FieldStatus::Unconfirmed &&
      field.status != FieldStatus::NotApplicable &&
      field.status != FieldStatus::NotReady &&
      field.status != FieldStatus::ReadFailed)
    return false;
  j.Lit("{\"field_id\":");
  j.Hex(field.id, 4);
  j.Lit(",\"status\":");
  j.Str(FieldStatusName(field.status));
  j.Lit(",\"encoding\":");
  j.Str(EncodingName(field.encoding));
  j.Lit(",\"width\":");
  j.Num(field.width);
  if (field.status == FieldStatus::Value) {
    if (field.encoding == Encoding::Reference) {
      if (static_cast<std::uint32_t>(field.relation) > 4)
        return false;
      j.Lit(",\"reference\":{\"status\":");
      j.Str(RelationName(field.relation));
      if (field.relation == Relation::KnownCurrent) {
        j.Lit(",\"target\":{\"local_id\":");
        j.Dec(field.target_id);
        j.Lit(",\"use_generation\":");
        j.Dec(field.target_use);
        j.Lit("}");
      }
      j.Lit("}");
      // 引用字段的原始地址只作诊断，不能当跨进程身份。
      std::uint32_t address = 0;
      std::memcpy(&address, field.raw.data(), sizeof(address));
      j.Lit(",\"diagnostics\":{\"native_address\":");
      j.Hex(address, 8);
      j.Lit("}");
    } else if (field.width == 4) {
      std::uint32_t value = 0;
      std::memcpy(&value, field.raw.data(), sizeof(value));
      j.Lit(",\"raw32\":");
      j.Hex(value, 8);
    } else if (field.width == 8) {
      std::uint64_t value = 0;
      std::memcpy(&value, field.raw.data(), sizeof(value));
      j.Lit(",\"raw64\":");
      j.Hex(value, 16);
    } else if (field.width > 0 && field.width <= 16) {
      j.Lit(",\"bytes_le\":");
      j.HexBytes(field.raw.data(), field.width);
    } else {
      return false;
    }
  }
  j.Lit("}");
  return true;
}

bool WriteCapture(WriterState &state, const OwnedBatch &batch) noexcept {
  const auto &header = batch.header;
  if (batch.object_count > kObjectBudget ||
      batch.membership_count > kMembershipBudget) {
    state.Fail(Reason::InvalidRecord, 0, "capture_budget");
    return false;
  }
  if (static_cast<std::uint32_t>(header.reason) >= kReasonCount) {
    state.Fail(Reason::InvalidRecord, 0, "capture_reason");
    return false;
  }
  if (!GroupMemberships(state, batch)) {
    state.Fail(Reason::InvalidRecord, 0, "membership_index");
    return false;
  }
  JsonOut j(state.captures);
  const auto phase =
      state.manifest.phase ? state.manifest.phase : "POST_STANDARD_UPDATE_PRE_TOP_EXIT";
  j.Lit("{\"record\":\"capture\",\"domain\":");
  j.Str(kDomain);
  j.Lit(",\"version\":");
  j.Num(kSchemaVersion);
  j.Lit(",\"profile\":");
  j.Str(kProfile);
  j.Lit(",\"session\":");
  j.Str(state.config.session);
  j.Lit(",\"recording_generation\":");
  j.Num(header.recording_generation);
  j.Lit(",\"run_epoch\":");
  j.Dec(header.run);
  j.Lit(",\"room_epoch\":");
  j.Dec(header.room);
  j.Lit(",\"mutation_epoch\":");
  j.Dec(header.mutation);
  j.Lit(",\"attempt_seq\":");
  j.Dec(header.attempt);
  j.Lit(",\"capture_seq\":");
  if (header.capture)
    j.Dec(header.capture);
  else
    j.Null();
  j.Lit(",\"outer_token\":");
  j.Dec(header.outer);
  j.Lit(",\"phase\":");
  j.Str(phase);
  j.Lit(",\"authority_tick\":");
  if (header.authority)
    j.Dec(header.tick);
  else
    j.Null();
  j.Lit(",\"authority_kind\":");
  j.Num(header.commit_kind);
  j.Lit(",\"scope_token\":");
  j.Dec(header.scope);
  j.Lit(",\"event_watermark\":");
  j.Dec(header.event_watermark);
  j.Lit(",\"complete\":");
  j.Bool(header.complete != 0);
  j.Lit(",\"reason\":");
  j.Str(ReasonName(static_cast<Reason>(header.reason)));
  // 来源：15 项固定顺序，NOT_APPLICABLE 与读取失败分开。
  j.Lit(",\"sources\":[");
  for (std::uint32_t i = 0; i < kSourceCount; ++i) {
    const auto &source = header.sources[i];
    if (i)
      j.Lit(",");
    j.Lit("{\"source\":");
    j.Num(i);
    j.Lit(",\"status\":");
    if (source.status == kSourceNotApplicable) {
      j.Str("NOT_APPLICABLE");
    } else if (source.status == 0) {
      j.Str("NONE");
    } else {
      j.Str(ReasonName(static_cast<Reason>(source.status)));
    }
    j.Lit(",\"count\":");
    j.Num(source.count);
    j.Lit(",\"diagnostics\":{\"descriptor\":[");
    for (std::uint32_t d = 0; d < 4; ++d) {
      if (d)
        j.Lit(",");
      j.Num(source.descriptor[d]);
    }
    j.Lit("]}}");
  }
  j.Lit("],\"objects\":[");
  for (std::uint32_t i = 0; i < batch.object_count; ++i) {
    const auto &object = batch.objects[i];
    if (i)
      j.Lit(",");
    if (object.kind >= kClassCount ||
        object.origin >= static_cast<std::uint32_t>(Origin::ObservedCreation) + 1 ||
        object.lifecycle > static_cast<std::uint32_t>(Lifecycle::Destroyed) ||
        object.readiness >
            static_cast<std::uint32_t>(Readiness::CurrentPlayerUpdateWitness) ||
        object.field_count > kFieldBudget) {
      state.Fail(Reason::InvalidRecord, 0, "object_schema");
      return false;
    }
    j.Lit("{\"local_id\":");
    j.Dec(object.local_id);
    j.Lit(",\"use_generation\":");
    j.Dec(object.use);
    j.Lit(",\"revision\":");
    j.Dec(object.revision);
    j.Lit(",\"object_domain\":");
    j.Num(object.domain);
    j.Lit(",\"class\":");
    j.Str(ClassName(static_cast<Class>(object.kind)));
    j.Lit(",\"origin\":");
    j.Str(OriginName(static_cast<Origin>(object.origin)));
    j.Lit(",\"lifecycle\":");
    j.Num(object.lifecycle);
    j.Lit(",\"init_evidence\":");
    j.Num(object.readiness);
    j.Lit(",\"birth_event\":");
    if (object.birth_known)
      j.Dec(object.birth);
    else
      j.Null();
    j.Lit(",\"birth\":{\"kind\":");
    j.Num(object.birth_kind);
    j.Lit(",\"entity_type\":");
    j.Num(object.birth_type);
    j.Lit(",\"variant\":");
    j.Num(object.birth_variant);
    j.Lit(",\"subtype\":");
    j.Num(object.birth_subtype);
    j.Lit(",\"seed\":");
    j.Num(object.birth_seed);
    j.Lit("},\"ordinal\":");
    if (object.ordinal < 4)
      j.Num(object.ordinal);
    else
      j.Null();
    j.Lit(",\"grid_index\":");
    if (object.grid_index != kNoGridIndex)
      j.Num(object.grid_index);
    else
      j.Null();
    j.Lit(",\"fields\":[");
    for (std::uint32_t f = 0; f < object.field_count; ++f) {
      if (f)
        j.Lit(",");
      if (!WriteField(j, object.fields[f])) {
        state.Fail(Reason::InvalidRecord, 0, "field_schema");
        return false;
      }
    }
    j.Lit("],\"memberships\":[");
    const auto begin = state.membership_start[i];
    const auto end = state.membership_start[i + 1];
    for (std::uint32_t m = begin; m < end; ++m) {
      if (m != begin)
        j.Lit(",");
      const auto &membership = state.sorted_memberships[m];
      j.Lit("{\"source\":");
      j.Num(membership.source);
      j.Lit(",\"index\":");
      j.Num(membership.index);
      j.Lit("}");
    }
    j.Lit("],\"diagnostics\":{\"address\":");
    j.Hex(object.address, 8);
    j.Lit(",\"vptr\":");
    j.Hex(object.vptr, 8);
    j.Lit(",\"rng_window\":{\"before_status\":"); j.Num(object.rng.before_status);
    j.Lit(",\"after_status\":"); j.Num(object.rng.after_status);
    for (unsigned side = 0; side < 2; ++side) {
      j.Lit(side ? ",\"after\":" : ",\"before\":");
      const auto status = side ? object.rng.after_status : object.rng.before_status;
      if (status != 1) { j.Null(); continue; }
      j.Lit("[");
      const auto &words = side ? object.rng.after : object.rng.before;
      for (unsigned word = 0; word < 4; ++word) { if (word) j.Lit(","); j.Hex(words[word], 8); }
      j.Lit("]");
    }
    j.Lit("}");
    j.Lit("}}");
  }
  j.Lit("],\"required_objects\":");
  j.Num(header.required_objects);
  j.Lit(",\"required_memberships\":");
  j.Num(header.required_memberships);
  j.Lit(",\"diagnostics\":{\"thread\":");
  j.Num(header.thread);
  j.Lit(",\"loop\":"); j.Dec(header.loop);
  // 原始 QPC 与频率一并输出，不把墙钟当 Authority tick；后台不解引用对象。
  j.Lit(",\"rng_window\":{\"scope\":\"sources_fields_references_tail\",\"inventory\":"); j.Num(header.rng_inventory);
  j.Lit(",\"matched\":"); j.Num(header.rng_matched);
  j.Lit(",\"set_stable\":"); j.Bool(header.rng_set_stable != 0);
  j.Lit(",\"thread_before\":"); j.Num(header.rng_thread_before);
  j.Lit(",\"thread_after\":"); j.Num(header.rng_thread_after);
  j.Lit(",\"started_qpc\":"); j.Dec(header.copy_started_qpc);
  j.Lit(",\"finished_qpc\":"); j.Dec(header.copy_finished_qpc);
  j.Lit(",\"qpc_frequency\":"); j.Dec(header.qpc_frequency);
  j.Lit("}");
  j.Lit(",\"phase_witness\":[");
  for (std::size_t i = 0; i < header.phase_scopes.size(); ++i) {
    if (i) j.Lit(",");
    j.Lit("{\"stage\":"); j.Num(static_cast<std::uint32_t>(i + 1));
    j.Lit(",\"scope\":"); j.Dec(header.phase_scopes[i]);
    j.Lit(",\"entered\":"); j.Num(header.phase_entered[i]);
    j.Lit(",\"returned\":"); j.Num(header.phase_returned[i]);
    j.Lit("}");
  }
  j.Lit("]");
  j.Lit("}}\n");
  if (!j.Ok()) {
    state.Fail(Reason::FileError, state.captures.LastError(), "captures_write");
    return false;
  }
  state.records_written.fetch_add(1, std::memory_order_relaxed);
  ++state.reason_counts[static_cast<std::uint32_t>(header.reason)];
  return true;
}

bool WriteGap(WriterState &state, const GapRecord &gap) noexcept {
  if (static_cast<std::uint32_t>(gap.reason) >= kReasonCount) {
    state.Fail(Reason::InvalidRecord, 0, "gap_reason");
    return false;
  }
  JsonOut j(state.captures);
  j.Lit("{\"record\":\"gap\",\"domain\":");
  j.Str(kDomain);
  j.Lit(",\"version\":");
  j.Num(kSchemaVersion);
  j.Lit(",\"profile\":");
  j.Str(kProfile);
  j.Lit(",\"session\":");
  j.Str(state.config.session);
  j.Lit(",\"recording_generation\":");
  j.Num(gap.recording_generation);
  j.Lit(",\"run_epoch\":");
  j.Dec(gap.run);
  j.Lit(",\"room_epoch\":");
  j.Dec(gap.room);
  j.Lit(",\"mutation_epoch\":");
  j.Dec(gap.mutation);
  j.Lit(",\"attempt_seq\":");
  j.Dec(gap.attempt);
  j.Lit(",\"capture_seq\":null,\"outer_token\":");
  j.Dec(gap.outer);
  j.Lit(",\"phase\":");
  j.Str(state.manifest.phase ? state.manifest.phase
                             : "POST_STANDARD_UPDATE_PRE_TOP_EXIT");
  j.Lit(",\"authority_tick\":null,\"authority_kind\":");
  j.Num(gap.authority_kind);
  j.Lit(",\"scope_token\":");
  j.Dec(gap.scope);
  j.Lit(",\"event_watermark\":");
  j.Dec(gap.event_watermark);
  j.Lit(",\"complete\":false,\"reason\":");
  j.Str(ReasonName(gap.reason));
  j.Lit(",\"sources\":[],\"objects\":[],\"required_objects\":0,"
        "\"required_memberships\":0,\"diagnostics\":{\"candidate_tick\":");
  j.Dec(gap.tick);
  j.Lit(",\"thread\":");
  j.Num(gap.thread);
  j.Lit("}}\n");
  if (!j.Ok()) {
    state.Fail(Reason::FileError, state.captures.LastError(), "gap_write");
    return false;
  }
  state.gaps_written.fetch_add(1, std::memory_order_relaxed);
  ++state.reason_counts[static_cast<std::uint32_t>(gap.reason)];
  return true;
}

bool WriteEvent(WriterState &state, const LifecycleEvent &event) noexcept {
  if (event.observation != kNoObservation &&
      event.observation >= kObservationCount) {
    state.Fail(Reason::InvalidRecord, 0, "event_observation");
    return false;
  }
  if (static_cast<std::uint32_t>(event.reason) >= kReasonCount) {
    state.Fail(Reason::InvalidRecord, 0, "event_reason");
    return false;
  }
  const auto kind = static_cast<std::uint32_t>(event.kind);
  if (kind > static_cast<std::uint32_t>(EventKind::NativeBody)) {
    state.Fail(Reason::InvalidRecord, 0, "event_kind");
    return false;
  }
  JsonOut j(state.events);
  j.Lit("{\"record\":\"lifecycle_event\",\"sequence\":");
  j.Dec(event.sequence);
  j.Lit(",\"run_epoch\":");
  j.Dec(event.run);
  j.Lit(",\"room_epoch\":");
  j.Dec(event.room);
  j.Lit(",\"outer_token\":");
  j.Dec(event.outer);
  j.Lit(",\"local_id\":");
  if (event.local_id)
    j.Dec(event.local_id);
  else
    j.Null();
  j.Lit(",\"use_generation\":");
  if (event.local_id)
    j.Dec(event.use);
  else
    j.Null();
  j.Lit(",\"birth_event\":");
  if (event.birth)
    j.Dec(event.birth);
  else
    j.Null();
  j.Lit(",\"revision\":");
  if (event.local_id)
    j.Dec(event.revision);
  else
    j.Null();
  j.Lit(",\"call\":");
  if (event.call != kNoCall)
    j.Dec(event.call);
  else
    j.Null();
  j.Lit(",\"parent\":");
  if (event.parent != kNoCall)
    j.Dec(event.parent);
  else
    j.Null();
  j.Lit(",\"observation\":");
  if (event.observation == kNoObservation)
    j.Null();
  else
    j.Str(ObservationName(static_cast<Observation>(event.observation)));
  j.Lit(",\"entering\":");
  j.Bool(event.entering != 0);
  j.Lit(",\"exceptional\":");
  j.Bool(event.exceptional != 0);
  j.Lit(",\"event_kind\":");
  j.Num(kind);
  j.Lit(",\"reason\":");
  j.Str(ReasonName(event.reason));
  j.Lit(",\"memberships\":");
  j.Num(event.memberships);
  j.Lit(",\"diagnostics\":{\"address\":");
  j.Hex(event.address, 8);
  j.Lit(",\"result\":");
  j.Hex(event.result, 8);
  j.Lit(",\"mutation\":"); j.Dec(event.mutation);
  j.Lit(",\"candidate_tick\":"); j.Dec(event.tick);
  j.Lit(",\"thread\":"); j.Num(event.thread);
  j.Lit(",\"domain\":"); j.Num(event.domain);
  j.Lit(",\"lifecycle\":"); j.Num(event.lifecycle);
  j.Lit(",\"readiness\":"); j.Num(event.readiness);
  j.Lit(",\"ordinal\":"); j.Num(event.ordinal);
  j.Lit(",\"session\":"); j.Dec(event.session);
  j.Lit(",\"participant_generation\":"); j.Dec(event.participant_generation);
  j.Lit(",\"assignment_key\":");
  if (event.assignment_key < 0) j.Lit("-1"); else j.Num(static_cast<std::uint32_t>(event.assignment_key));
  j.Lit(",\"device\":"); j.Hex(event.device, 8);
  j.Lit(",\"slot_before\":"); j.Hex(event.slot_before, 8);
  j.Lit(",\"slot_after\":"); j.Hex(event.slot_after, 8);
  j.Lit(",\"args\":[");
  for (std::uint32_t i = 0; i < event.arg_count && i < event.args.size(); ++i) {
    if (i) j.Lit(","); j.Hex(event.args[i], 8);
  }
  j.Lit("]");
  j.Lit("}}\n");
  if (!j.Ok()) {
    state.Fail(Reason::FileError, state.events.LastError(), "events_write");
    return false;
  }
  state.events_written.fetch_add(1, std::memory_order_relaxed);
  return true;
}

// 诊断行：与常规 trace 分离，只描述 world 自有状态。
