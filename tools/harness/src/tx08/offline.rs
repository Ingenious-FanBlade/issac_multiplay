//! L1–L7 只由最终整体入口调用；所有修改落在显式 TEST_INJECTED 自有副本中。
//! L1 保留全量录制；边界/非法输入使用一条真实完整 capture 派生的小型材料，绝不冒称 native 覆盖。
use super::evidence::{json as load, rows};
use super::*;
use crate::world::dataset::{self, Dataset};
use std::{
    fs::File,
    io::{BufRead, BufReader, BufWriter, Read},
    process::{Command, Stdio},
};

// 两份全量 canonical 输出逐块比较，无摘要/哈希，缺文件立即失败。
fn same_bytes(a: &Path, b: &Path) -> Result<bool> {
    let size = fs::metadata(a).map_err(|e| e.to_string())?.len();
    if size != fs::metadata(b).map_err(|e| e.to_string())?.len() {
        return Ok(false);
    }
    let mut a = BufReader::new(File::open(a).map_err(|e| e.to_string())?);
    let mut b = BufReader::new(File::open(b).map_err(|e| e.to_string())?);
    let mut x = [0u8; 65536];
    let mut y = [0u8; 65536];
    let mut remaining = size;
    while remaining > 0 {
        let n = remaining.min(x.len() as u64) as usize;
        a.read_exact(&mut x[..n]).map_err(|e| e.to_string())?;
        b.read_exact(&mut y[..n]).map_err(|e| e.to_string())?;
        if x[..n] != y[..n] {
            return Ok(false);
        }
        remaining -= n as u64;
    }
    Ok(true)
}
// 子进程仍是生产 harness CLI；完整参数、输出、退出码和耗时单独保存，非零不自动重试。
fn cli(root: &Path, name: &str, args: Vec<String>, expected_success: bool) -> Result<Value> {
    let log = root.join(format!("{name}-cli"));
    fs::create_dir(&log).map_err(|e| e.to_string())?;
    let start = Instant::now();
    let output = (|| -> Result<std::process::ExitStatus> {
        let mut child = Command::new(std::env::current_exe().map_err(|e| e.to_string())?)
            .args(&args)
            .stdin(Stdio::null())
            .stdout(Stdio::from(
                File::create(log.join("stdout.txt")).map_err(|e| e.to_string())?,
            ))
            .stderr(Stdio::from(
                File::create(log.join("stderr.txt")).map_err(|e| e.to_string())?,
            ))
            .spawn()
            .map_err(|e| e.to_string())?;
        loop {
            if let Some(code) = child.try_wait().map_err(|e| e.to_string())? {
                return Ok(code);
            }
            if start.elapsed() >= Duration::from_secs(300) {
                let killed = child.kill();
                let waited = child.wait();
                return Err(format!(
                    "offline_cli_timeout_300s kill={killed:?} wait={waited:?}"
                ));
            }
            thread::sleep(Duration::from_millis(100));
        }
    })();
    let evidence = json!({"arguments":args,"exit_code":output.as_ref().ok().and_then(|s|s.code()),
        "spawn_or_timeout_error":output.as_ref().err().map(ToString::to_string),"elapsed_ms":start.elapsed().as_millis(),"timeout_seconds":300,
        "expected_success":expected_success,"expectation_met":output.as_ref().is_ok_and(|s|s.success()==expected_success),
        "automatic_retry":false});
    write_json(&log.join("invocation.json"), &evidence)?;
    Ok(evidence)
}
fn replay(root: &Path, name: &str, source: &Path, success: bool, legacy: bool) -> Result<Value> {
    let mut args = vec![
        "world-replay".into(),
        "--capture".into(),
        source.to_string_lossy().into(),
        "--artifact-dir".into(),
        root.join(name).to_string_lossy().into(),
    ];
    if legacy {
        args.push("--legacy-t0".into());
    }
    cli(root, name, args, success)
}
fn compare(root: &Path, name: &str, left: &Path, right: &Path, alignment: &Path) -> Result<Value> {
    cli(
        root,
        name,
        vec![
            "world-compare".into(),
            "--left".into(),
            left.to_string_lossy().into(),
            "--right".into(),
            right.to_string_lossy().into(),
            "--alignment".into(),
            alignment.to_string_lossy().into(),
            "--artifact-dir".into(),
            root.join(name).to_string_lossy().into(),
        ],
        true,
    )
}
// 一条真实 capture 的自有派生集保留完整生命周期和诊断；调整文件计数只是派生集元数据，附原终态原件。
fn subset(source: &Path, dest: &Path, capture: &Value) -> Result<()> {
    fs::create_dir(dest).map_err(|e| e.to_string())?;
    for name in [
        "manifest.json",
        "lifecycle-events.jsonl",
        "diagnostics.jsonl",
    ] {
        fs::copy(source.join(name), dest.join(name)).map_err(|e| e.to_string())?;
    }
    let mut terminal = load(&source.join("result.json"))?;
    fs::copy(
        source.join("result.json"),
        dest.join("original-recorder-result.json"),
    )
    .map_err(|e| e.to_string())?;
    for key in [
        "attempts",
        "captures",
        "complete_captures",
        "batches_copied",
        "batches_written",
    ] {
        terminal["counts"][key] = json!("1");
    }
    for key in ["gaps", "gaps_produced", "gaps_copied", "gaps_written"] {
        terminal["counts"][key] = json!("0");
    }
    write_json(&dest.join("result.json"), &terminal)?;
    let mut f =
        BufWriter::new(File::create(dest.join("captures.jsonl")).map_err(|e| e.to_string())?);
    writeln!(f, "{capture}").map_err(|e| e.to_string())?;
    f.flush().map_err(|e| e.to_string())?;
    write_json(
        &dest.join("mutation.json"),
        &json!({"kind":"TEST_INJECTED_DERIVED_SINGLE_CAPTURE","source":source,
        "source_attempt":capture["attempt_seq"],"native_runtime_evidence":false,"original_terminal":"original-recorder-result.json"}),
    )
}
fn mutate_fields(c: &mut Value, mut change: impl FnMut(&mut Value)) {
    for o in c["objects"].as_array_mut().unwrap() {
        for f in o["fields"].as_array_mut().unwrap() {
            change(f);
        }
    }
}
fn set_scalar(f: &mut Value, key: &str, value: &str) {
    let object = f.as_object_mut().unwrap();
    for k in ["raw32", "raw64", "bytes_le", "reference"] {
        object.remove(k);
    }
    object.insert("status".into(), json!("VALUE"));
    object.insert(key.into(), json!(value));
}
impl Run {
    // 一次入口收尾时尽力完成与失败无依赖的离线项；每项错误单列后继续下一项，不重跑。
    pub(super) fn offline_tx08(&mut self) -> Result<()> {
        let root = self.out.join("offline");
        fs::create_dir(&root).map_err(|e| e.to_string())?;
        for n in 1..=7 {
            self.report["offline"][format!("L{n}")] = json!({"status":"NOT_RUN"});
        }
        let primary = self.out.join("runtime-world/primary");
        // 离线 CLI 的只读性按五份完整输入文件直接逐字节核对，不计算摘要。
        let originals=root.join("source-before");
        fs::create_dir(&originals).map_err(|e|e.to_string())?;
        let source_names=["manifest.json","captures.jsonl","lifecycle-events.jsonl","diagnostics.jsonl","result.json"];
        for name in source_names {
            if primary.join(name).is_file() { fs::copy(primary.join(name),originals.join(name)).map_err(|e|e.to_string())?; }
        }
        let example = (|| -> Result<Value> {
            let data = Dataset::open(&primary, false)?;
            let scan = dataset::scan(&data, None, None, None)?;
            let mut file=File::open(primary.join("captures.jsonl")).map_err(|e|e.to_string())?;
            let mut fallback=None;
            // Tear 只在后续原生 Spawn 场景出现；不能用首份无 raw128 的开局样本宣布未覆盖。
            for loc in scan.attempts.values().filter(|l|l.complete) {
                let capture=serde_json::to_value(data.at(&mut file,loc)?).map_err(|e|e.to_string())?;
                let raw128=capture["objects"].as_array().unwrap().iter().any(|o|
                    o["fields"].as_array().unwrap().iter().any(|f|f["width"]==16 && f["status"]=="VALUE"));
                if raw128 {
                    write_json(&root.join("example-selection.json"),&json!({"source":primary,
                        "attempt":capture["attempt_seq"],"raw128":true,"all_source_records_retained":true}))?;
                    return Ok(capture);
                }
                if fallback.is_none() {fallback=Some(capture);}
            }
            fallback.ok_or("no_complete_capture_for_offline".into())
        })();
        for (n, name) in [
            (1, "full_replay"),
            (2, "encoding_boundaries"),
            (3, "known_difference"),
            (4, "decode_rejections"),
            (5, "unknown_and_conflicts"),
            (6, "two_process_compare"),
            (7, "legacy_import"),
        ] {
            if self.scenario["focus"]=="V1_V10" && n>5 {continue;}
            let result = match n {
                1 => {
                    if primary.join("manifest.json").exists() {
                        self.l1_tx08(&root, &primary)
                    } else {
                        Ok(json!({"status":"NOT_RUN","reason":"no primary recording"}))
                    }
                }
                2..=5 => match &example {
                    Ok(value) => match n {
                        2 => self.l2_tx08(&root, &primary, value),
                        3 => self.l3_tx08(&root, &primary, value),
                        4 => self.l4_tx08(&root, &primary, value),
                        _ => self.l5_tx08(&root, &primary, value),
                    },
                    Err(e) => Ok(json!({"status":"NOT_OBSERVED","reason":e})),
                },
                6 => self.l6_tx08(&root),
                _ => self.l7_tx08(&root),
            };
            self.report["offline"][format!("L{n}")] = match result {
                Ok(v) => v,
                Err(e) => json!({"status":"FAIL","error":e,"name":name}),
            };
            write_json(&root.join("ledger.json"), &self.report["offline"])?;
        }
        let mut unchanged=json!({});
        for name in source_names {
            unchanged[name]=json!(same_bytes(&primary.join(name),&originals.join(name))?);
        }
        let intact=unchanged.as_object().unwrap().values().all(|v|v==true);
        self.report["offline"]["source_unchanged"]=json!({"pass":intact,"files":unchanged,"comparison":"full bytes; no hashes"});
        if !intact { self.report["offline"]["L1"]["status"]=json!("FAIL"); }
        write_json(&root.join("ledger.json"), &self.report["offline"])?;
        Ok(())
    }
    fn l1_tx08(&self, root: &Path, source: &Path) -> Result<Value> {
        let a = replay(root, "L1-a", source, true, false)?;
        let b = replay(root, "L1-b", source, true, false)?;
        let mut files = json!({});
        let mut all = a["expectation_met"] == true && b["expectation_met"] == true;
        for name in [
            "manifest.json",
            "captures.jsonl",
            "lifecycle-events.jsonl",
            "diagnostics.jsonl",
            "source-result.json",
        ] {
            let same = same_bytes(&root.join("L1-a").join(name), &root.join("L1-b").join(name))?;
            let fields = if name.ends_with("jsonl") {
                let a = File::open(root.join("L1-a").join(name)).map_err(|e| e.to_string())?;
                let b = File::open(root.join("L1-b").join(name)).map_err(|e| e.to_string())?;
                let mut a = BufReader::new(a).lines();
                let mut b = BufReader::new(b).lines();
                let mut count = 0u64;
                loop {
                    match (a.next(), b.next()) {
                        (None, None) => break,
                        (Some(x), Some(y)) => {
                            let x: Value = serde_json::from_str(&x.map_err(|e| e.to_string())?)
                                .map_err(|e| e.to_string())?;
                            let y: Value = serde_json::from_str(&y.map_err(|e| e.to_string())?)
                                .map_err(|e| e.to_string())?;
                            if x != y {
                                return Err(format!("field mismatch:{name}:{count}"));
                            }
                            count += 1;
                        }
                        _ => return Err("line count mismatch".into()),
                    }
                }
                json!({"records":count.to_string(),"fields_equal":true})
            } else {
                json!({"fields_equal":load(&root.join("L1-a").join(name))?==load(&root.join("L1-b").join(name))?})
            };
            files[name] = json!({"bytes_equal":same,"field_comparison":fields});
            all &= same;
        }
        Ok(
            json!({"status":if all{"PASS"}else{"FAIL"},"cli":[a,b],"files":files,"scope":"all primary records, including gaps/lifecycle"}),
        )
    }
    fn l2_tx08(&self, root: &Path, source: &Path, original: &Value) -> Result<Value> {
        let mut cases = vec![];
        let mut ordered = original.clone();
        ordered["sources"].as_array_mut().unwrap().reverse();
        ordered["objects"].as_array_mut().unwrap().reverse();
        for o in ordered["objects"].as_array_mut().unwrap() {
            o["fields"].as_array_mut().unwrap().reverse();
            o["memberships"].as_array_mut().unwrap().reverse();
        }
        cases.push(("reversed", ordered));
        let mut max = original.clone();
        max["mutation_epoch"] = json!(u64::MAX.to_string());
        cases.push(("max-u64", max));
        let has128 = original["objects"].as_array().unwrap().iter().any(|o| {
            o["fields"]
                .as_array()
                .unwrap()
                .iter()
                .any(|f| f["width"] == 16)
        });
        if has128 {
            let mut c = original.clone();
            mutate_fields(&mut c, |f| {
                if f["width"] == 16 {
                    set_scalar(f, "bytes_le", "ffffffffffffffffffffffffffffffff");
                }
            });
            cases.push(("raw128", c));
        }
        let has_float = original["objects"].as_array().unwrap().iter().any(|o| {
            o["fields"]
                .as_array()
                .unwrap()
                .iter()
                .any(|f| f["encoding"] == "F32_BITS")
        });
        if has_float {
            for (name, bits) in [
                ("plus-zero", "00000000"),
                ("minus-zero", "80000000"),
                ("finite", "3f800000"),
                ("nan", "7fc00001"),
                ("inf", "7f800000"),
            ] {
                let mut c = original.clone();
                mutate_fields(&mut c, |f| {
                    if f["encoding"] == "F32_BITS" {
                        set_scalar(f, "raw32", bits);
                    }
                });
                cases.push((name, c));
            }
        }
        let mut ledger = vec![];
        let mut all = has128 && has_float;
        for (name, c) in cases {
            let input = root.join(format!("L2-{name}-input"));
            subset(source, &input, &c)?;
            let invocation = replay(root, &format!("L2-{name}"), &input, true, false)?;
            let ok = invocation["expectation_met"] == true;
            all &= ok;
            let recoded = rows(&root.join(format!("L2-{name}/captures.jsonl")))?;
            // 对每条派生记录重排序后逐字段比较，NaN/Inf 原始位也必须保持。
            let mut expected = c.clone();
            canonical(&mut expected);
            let retained = recoded.len() == 1 && recoded[0] == expected;
            all &= retained;
            ledger.push(
                json!({"case":name,"cli":invocation,"raw_and_unknown_values_preserved":retained}),
            );
        }
        if has_float {
            let alignment = self.self_alignment_tx08(root, source, original, "L2")?;
            for name in ["signed-zero", "nan", "inf"] {
                let left = root.join(if name == "signed-zero" {
                    "L2-plus-zero-input"
                } else if name == "nan" {
                    "L2-nan-input"
                } else {
                    "L2-inf-input"
                });
                let right = if name == "signed-zero" {
                    root.join("L2-minus-zero-input")
                } else {
                    left.clone()
                };
                let invocation = compare(
                    root,
                    &format!("L2-compare-{name}"),
                    &left,
                    &right,
                    &alignment,
                )?;
                let result = load(&root.join(format!("L2-compare-{name}/result.json")))?;
                let rows = rows(&root.join(format!("L2-compare-{name}/comparisons.jsonl")))?;
                let semantic = rows.iter().any(|r| {
                    if name == "signed-zero" {
                        r["differences"].as_array().is_some_and(Vec::is_empty)
                            && r["known_field_comparisons"].as_u64().unwrap_or(0) > 0
                    } else {
                        r["status"] != "EQUAL_WITHIN_PROFILE"
                            && r["unknown"].as_array().is_some_and(|u| {
                                u.iter().any(|u| {
                                    u["reason"] == "FIELD_UNAVAILABLE"
                                        && u["right"]["encoding"] == "F32_BITS"
                                })
                            })
                    }
                });
                let pass = invocation["expectation_met"] == true && semantic;
                all &= pass;
                ledger.push(json!({"case":name,"cli":invocation,"comparison_semantics":semantic,"pass":pass,"result":result}));
            }
        }
        Ok(
            json!({"status":if all{"PASS"}else{"NOT_OBSERVED"},"raw128_available":has128,"float_available":has_float,"cases":ledger}),
        )
    }
    // 自比较映射来自同一份真实记录；出生编号逐一映射自身，仅用于离线副本反例。
    fn self_alignment_tx08(
        &self,
        root: &Path,
        source: &Path,
        c: &Value,
        name: &str,
    ) -> Result<PathBuf> {
        let data = Dataset::open(source, false)?;
        let scan = dataset::scan(&data, None, None, None)?;
        let alignment = json!({"version":1,"left":{"run_epoch":c["run_epoch"],"room_epoch":c["room_epoch"]},
            "right":{"run_epoch":c["run_epoch"],"room_epoch":c["room_epoch"]},"ticks":[{"left":c["authority_tick"],"right":c["authority_tick"]}],
            "births":scan.births.iter().filter(|(_,b)|b.run==c["run_epoch"].as_str().unwrap()).map(|(n,_)|json!({"left":n.to_string(),"right":n.to_string()})).collect::<Vec<_>>(),
            "players":[{"left":0,"right":0},{"left":1,"right":1}],"evidence":self.alignment_evidence_tx08(false)});
        let path = root.join(format!("{name}-alignment.json"));
        write_json(&path, &alignment)?;
        Ok(path)
    }
    fn alignment_evidence_tx08(&self, two: bool) -> Value {
        let p1 = self.out.join("process-1");
        let p2 = self.out.join(if two { "process-2" } else { "process-1" });
        json!({"input":{"left":p1.join("input.jsonl"),"right":p2.join("input.jsonl")},
            "scenario":{"left":self.out.join("scenario.json"),"right":self.out.join("scenario.json")},
            "births":{"left":self.out.join("runtime-world/primary/lifecycle-events.jsonl"),
                "right":self.out.join(if two{"runtime-world/comparison/lifecycle-events.jsonl"}else{"runtime-world/primary/lifecycle-events.jsonl"})},
            "players":{"left":p1.join("bindings.json"),"right":p2.join("bindings.json")}})
    }
    fn l3_tx08(&self, root: &Path, source: &Path, c: &Value) -> Result<Value> {
        let left = root.join("L3-left");
        subset(source, &left, c)?;
        let mut right = c.clone();
        let mut changed = None;
        for o in right["objects"].as_array_mut().unwrap() {
            if o["class"] != "Player" || o["ordinal"].is_null() {
                continue;
            }
            let key = o["ordinal"].clone();
            for f in o["fields"].as_array_mut().unwrap() {
                if f["status"] == "VALUE" && f["encoding"] == "F32_BITS" {
                    let raw = f["raw32"]
                        .as_str()
                        .and_then(|s| u32::from_str_radix(s, 16).ok())
                        .ok_or("float raw")?;
                    if f32::from_bits(raw).is_finite() {
                        let bits = if raw == 0x3f800000 {
                            0x40000000
                        } else {
                            0x3f800000
                        };
                        changed = Some(
                            json!({"ordinal":key,"field":f["field_id"],"before":f["raw32"],"after":format!("{bits:08x}")}),
                        );
                        f["raw32"] = json!(format!("{bits:08x}"));
                        break;
                    }
                }
            }
            if changed.is_some() {
                break;
            }
        }
        let changed = changed.ok_or("no known finite Player field")?;
        let right_path = root.join("L3-right");
        subset(source, &right_path, &right)?;
        let alignment = self.self_alignment_tx08(root, source, c, "L3")?;
        let invocation = compare(root, "L3-compare", &left, &right_path, &alignment)?;
        let result = load(&root.join("L3-compare/result.json"))?;
        let comparisons = rows(&root.join("L3-compare/comparisons.jsonl"))?;
        let id = u16::from_str_radix(changed["field"].as_str().ok_or("field id")?,16).map_err(|e|e.to_string())?;
        let expected_path = json!([c["run_epoch"],c["room_epoch"],c["authority_tick"],"Player",
            format!("player:{:02}",changed["ordinal"].as_u64().ok_or("ordinal")?),crate::world::profile::field_name(id)]);
        let precise = comparisons.len()==1 && comparisons[0]["left_tick"]==c["authority_tick"]
            && comparisons[0]["right_tick"]==c["authority_tick"]
            && comparisons[0]["differences"].as_array().is_some_and(|a|a.len()==1
                && a[0]["field_id"]==changed["field"] && a[0]["path"]==expected_path
                && comparisons[0]["first_observed_difference"]==a[0])
            && comparisons[0]["earlier_coverage_unknown"].as_bool()==comparisons[0]["unknown"].as_array().map(|a|!a.is_empty())
            && result["earlier_coverage_unknown"]==comparisons[0]["earlier_coverage_unknown"];
        Ok(
            json!({"status":if invocation["expectation_met"]==true && result["status"]=="DIFFERENT" && precise{"PASS"}else{"FAIL"},
            "cli":invocation,"changed":changed,"expected_path":expected_path,"comparisons":comparisons,"precise_single_field":precise,"result":result}),
        )
    }
    fn l4_tx08(&self, root: &Path, source: &Path, c: &Value) -> Result<Value> {
        let mut ledger = vec![];
        for name in [
            "domain",
            "version",
            "profile",
            "duplicate-source",
            "width",
            "raw-length",
            "u64-overflow",
            "extra-field",
            "truncated-json",
            "over-budget",
            "duplicate-key",
        ] {
            let mut v = c.clone();
            let input = root.join(format!("L4-{name}-input"));
            subset(source, &input, &v)?;
            match name {
                "domain" => v["domain"] = json!("invalid.domain"),
                "version" => v["version"] = json!(999),
                "profile" => v["profile"] = json!("unsupported"),
                "duplicate-source" => {
                    let s = v["sources"][0].clone();
                    v["sources"].as_array_mut().unwrap().push(s);
                }
                "width" => {
                    let f = first_field(&mut v)?;
                    f["width"] = json!(99);
                }
                "raw-length" => {
                    let f = first_field(&mut v)?;
                    set_scalar(f, "raw32", "0");
                }
                "u64-overflow" => v["attempt_seq"] = json!("18446744073709551616"),
                "extra-field" => v["unexpected"] = json!(true),
                "over-budget" => {
                    let o = v["objects"][0].clone();
                    v["objects"] = json!(vec![o; 16385]);
                }
                _ => (),
            }
            let text = if name == "truncated-json" {
                "{\"record\":".into()
            } else if name == "duplicate-key" {
                let text = serde_json::to_string(&v).map_err(|e| e.to_string())?;
                format!("{{\"record\":\"capture\",{}", &text[1..])
            } else {
                format!("{v}\n")
            };
            fs::write(input.join("captures.jsonl"), text).map_err(|e| e.to_string())?;
            write_json(
                &input.join("mutation-description.json"),
                &json!({"kind":"TEST_INJECTED","change":name}),
            )?;
            let invocation = replay(root, &format!("L4-{name}"), &input, false, false)?;
            let failure = root.join(format!("L4-{name}/failure.json"));
            let ok = invocation["expectation_met"] == true
                && failure.exists()
                && !root.join(format!("L4-{name}/result.json")).exists();
            ledger.push(
                json!({"case":name,"pass":ok,"cli":invocation,"failure":load(&failure).ok()}),
            );
        }
        Ok(
            json!({"status":if ledger.iter().all(|v|v["pass"]==true){"PASS"}else{"FAIL"},"cases":ledger}),
        )
    }
    fn l5_tx08(&self, root: &Path, source: &Path, c: &Value) -> Result<Value> {
        let base = root.join("L5-base");
        subset(source, &base, c)?;
        let alignment = self.self_alignment_tx08(root, source, c, "L5")?;
        let mut ledger = vec![];
        for name in [
            "collision",
            "NOT_READY",
            "READ_FAILED",
            "NULL",
            "KNOWN_CURRENT",
            "UNRESOLVED",
            "RETIRED",
            "AMBIGUOUS_REUSE",
            "missing-source",
            "missing-tick",
            "duplicate-tick",
        ] {
            let mut v = c.clone();
            let mut applicable = true;
            let mut changed_field = Value::Null;
            match name {
                "collision" => {
                    let found = v["objects"]
                        .as_array()
                        .unwrap()
                        .iter()
                        .find(|o| o["class"] == "Player" && !o["ordinal"].is_null())
                        .cloned();
                    if let Some(mut o) = found {
                        o["local_id"] = json!(u64::MAX.to_string());
                        o["memberships"] = json!([]);
                        v["objects"].as_array_mut().unwrap().push(o);
                        v["required_objects"] = json!(v["objects"].as_array().unwrap().len());
                    } else {
                        applicable = false;
                    }
                }
                "NOT_READY" | "READ_FAILED" => {
                    let f = first_field(&mut v)?;
                    let id = f["field_id"].clone();
                    let enc = f["encoding"].clone();
                    let w = f["width"].clone();
                    *f = json!({"field_id":id,"encoding":enc,"width":w,"status":name});
                }
                "NULL" | "KNOWN_CURRENT" | "UNRESOLVED" | "RETIRED" | "AMBIGUOUS_REUSE" => {
                    let target=v["objects"].as_array().unwrap().iter().find(|o|o["class"]=="Player" && !o["ordinal"].is_null())
                        .map(|o|json!({"local_id":o["local_id"],"use_generation":o["use_generation"]}));
                    let mut changed = false;
                    for o in v["objects"]
                        .as_array_mut()
                        .unwrap()
                        .iter_mut()
                        .filter(|o| o["class"] == "Player" && !o["ordinal"].is_null())
                    {
                        for f in o["fields"].as_array_mut().unwrap() {
                            if !changed && f["encoding"] == "REFERENCE" {
                                let id = f["field_id"].clone();
                                let width = f["width"].clone();
                                changed_field = id.clone();
                                *f = json!({"field_id":id,"encoding":"REFERENCE","width":width,"status":"VALUE","reference":{"status":name}});
                                if name == "KNOWN_CURRENT" {
                                    f["reference"]["target"] =
                                        target.clone().ok_or("mapped Player reference target")?;
                                }
                                changed = true;
                            }
                        }
                    }
                    applicable = changed;
                }
                "missing-source" => {
                    v["sources"]
                        .as_array_mut()
                        .unwrap()
                        .retain(|s| s["source"] != 0);
                }
                "missing-tick" => {
                    v["authority_tick"] = Value::Null;
                    v["complete"] = json!(false);
                    v["reason"] = json!("PHASE_INCOMPLETE");
                }
                _ => (),
            }
            if !applicable {
                ledger.push(json!({"case":name,"status":"NOT_OBSERVED","pass":false}));
                continue;
            }
            let input = root.join(format!("L5-{name}-input"));
            subset(source, &input, &v)?;
            if name == "duplicate-tick" {
                let mut next = v.clone();
                next["attempt_seq"] = json!(
                    (v["attempt_seq"]
                        .as_str()
                        .unwrap()
                        .parse::<u64>()
                        .map_err(|e| e.to_string())?
                        + 1)
                    .to_string()
                );
                next["capture_seq"] = json!(
                    (v["capture_seq"]
                        .as_str()
                        .unwrap()
                        .parse::<u64>()
                        .map_err(|e| e.to_string())?
                        + 1)
                    .to_string()
                );
                fs::write(input.join("captures.jsonl"), format!("{v}\n{next}\n"))
                    .map_err(|e| e.to_string())?;
                let mut result = load(&input.join("result.json"))?;
                for k in [
                    "attempts",
                    "captures",
                    "complete_captures",
                    "batches_copied",
                    "batches_written",
                ] {
                    result["counts"][k] = json!("2");
                }
                write_json(&input.join("result.json"), &result)?;
            }
            // 两侧使用同一个故障副本，专门证明“相同错误”不能成为 equal。
            let invocation = compare(root, &format!("L5-{name}"), &input, &input, &alignment)?;
            let result = load(&root.join(format!("L5-{name}/result.json")))?;
            let collisions = if name == "collision" {
                rows(&root.join(format!("L5-{name}/key-collisions.jsonl")))?.len()
            } else {
                0
            };
            let compared = rows(&root.join(format!("L5-{name}/comparisons.jsonl")))?;
            let specific = compared.iter().any(|row| match name {
                "collision" => collisions > 0,
                "missing-tick" => row["reason"] == "SAMPLE_GAP",
                "duplicate-tick" => row["reason"] == "ALIGNMENT_AMBIGUOUS",
                "missing-source" => row["unknown"].as_array().is_some_and(|u| {
                    u.iter()
                        .any(|u| u["reason"] == "SOURCE_UNAVAILABLE" && u["source"] == 0)
                }),
                "NULL" | "KNOWN_CURRENT" => {
                    row["known_field_comparisons"].as_u64().unwrap_or(0) > 0
                        && row["unknown"].as_array().is_some_and(|u| {
                            !u.iter().any(|u| {
                                u["reason"] == "FIELD_UNAVAILABLE"
                                    && u["field_id"] == changed_field
                                    && u["right"]["reference"]["status"] == name
                            })
                        })
                        && row["differences"].as_array().is_some_and(Vec::is_empty)
                }
                _ => row["unknown"].as_array().is_some_and(|u| {
                    u.iter().any(|u| {
                        u["reason"] == "FIELD_UNAVAILABLE"
                            && (u["right"]["status"] == name
                                || u["right"]["reference"]["status"] == name)
                    })
                }),
            });
            let pass = invocation["expectation_met"] == true
                && (["NULL", "KNOWN_CURRENT"].contains(&name)
                    || result["status"] != "EQUAL_WITHIN_PROFILE")
                && specific;
            ledger.push(json!({"case":name,"pass":pass,"cli":invocation,"result":result,"specific_unknown_observed":specific,"collision_groups":collisions}));
        }
        Ok(
            json!({"status":if ledger.iter().all(|v|v["pass"]==true){"PASS"}else{"NOT_OBSERVED"},"cases":ledger,
            "known_null_reference":"NULL and mapped KNOWN_CURRENT remain known values; identical unavailable references must not equal"}),
        )
    }
    fn l6_tx08(&self, root: &Path) -> Result<Value> {
        let a = self.out.join("runtime-world/primary");
        let b = self.out.join("runtime-world/comparison");
        if !b.join("result.json").exists() {
            return Ok(json!({"status":"NOT_RUN","reason":"second process has no closed capture"}));
        }
        let alignment = self.build_alignment_tx08(root, &a, &b)?;
        let invocation = compare(root, "L6-compare", &a, &b, &alignment)?;
        let result = load(&root.join("L6-compare/result.json"))?;
        let known = result["known_field_comparisons"]
            .as_str()
            .and_then(|s| s.parse::<u64>().ok())
            .unwrap_or(0);
        Ok(
            json!({"status":if invocation["expectation_met"]==true && known>0 && result["tick_pairs"].as_u64().unwrap_or(0)>0{"PASS"}else{"NOT_OBSERVED"},
            "cli":invocation,"result":result,"world_equality_required":false}),
        )
    }
    fn build_alignment_tx08(&self, root: &Path, left: &Path, right: &Path) -> Result<PathBuf> {
        let ld = Dataset::open(left, false)?;
        let rd = Dataset::open(right, false)?;
        let ls = dataset::scan(&ld, None, None, None)?;
        let rs = dataset::scan(&rd, None, None, None)?;
        let events =
            |p: &str| crate::tx05::evidence::parse_events(&self.out.join(p).join("trace.jsonl"));
        let le = events("process-1")?;
        let re = events("process-2")?;
        let left_trace = super::trace_index::TraceIndex::new(&le);
        let right_trace = super::trace_index::TraceIndex::new(&re);
        let generation = |p: &str| -> Result<u64> {
            load(&self.out.join(p).join("bindings.json"))?["status"]["generation"]
                .as_str()
                .ok_or("binding generation")?
                .parse()
                .map_err(|e| format!("{e}"))
        };
        let lg = generation("process-1")?;
        let rg = generation("process-2")?;
        let base = |p: &str, label: &str| -> Result<(u64,u64)> {
            let r = rows(&self.out.join(p).join("input.jsonl"))?;
            let scripts = r
                .iter()
                .filter(|r| r["event"] == "script" && r["label"] == label)
                .collect::<Vec<_>>();
            if scripts.len() != 1 {
                return Err("script anchor missing/duplicated".into());
            }
            let generation=scripts[0]["generation"].as_u64().ok_or("script generation")?;
            let bindings=load(&self.out.join(p).join("bindings.json"))?;
            if bindings["status"]["b_generation"].as_str().and_then(|s|s.parse::<u64>().ok())!=Some(generation) {
                return Err("script_generation_not_frozen_binding".into());
            }
            Ok((scripts[0]["base"].as_u64().ok_or("script base")?,generation))
        };
        let (lb,lbg) = base("process-1", "p1.stable")?;
        let (rb,rbg) = base("process-2", "p2.stable")?;
        let mut pairs = vec![];
        let mut proof = vec![];
        let mut scopes = None;
        // 完整 capture 的同 tick 组只建立一次；重复项保留，后续仍要求 len==1。
        let mut left_capture_ticks = BTreeMap::<Option<u64>, Vec<_>>::new();
        let mut right_capture_ticks = BTreeMap::<Option<u64>, Vec<_>>::new();
        for location in ls.attempts.values().filter(|l| l.complete) {
            left_capture_ticks.entry(location.tick).or_default().push(location);
        }
        for location in rs.attempts.values().filter(|l| l.complete) {
            right_capture_ticks.entry(location.tick).or_default().push(location);
        }
        for offset in 1..=self.scenario["comparison_samples"].as_u64().unwrap() + 128 {
            let l = left_capture_ticks.get(&lb.checked_add(offset)).map(Vec::as_slice).unwrap_or(&[]);
            let r = right_capture_ticks.get(&rb.checked_add(offset)).map(Vec::as_slice).unwrap_or(&[]);
            if l.len() != 1 || r.len() != 1 {
                continue;
            }
            let mut consumed = vec![];
            let mut valid = true;
            for ordinal in 0..=1 {
                let a = left_trace.ticks(Some(lg), lb.checked_add(offset), Some(ordinal));
                let b = right_trace.ticks(Some(rg), rb.checked_add(offset), Some(ordinal));
                if a.len() != 1 || b.len() != 1 {
                    valid = false;
                    break;
                }
                if ["move", "aim"]
                    .iter()
                    .any(|k| a[0].pair(k).is_none() || a[0].pair(k) != b[0].pair(k))
                    || ["held", "edge"]
                        .iter()
                        .any(|k| a[0].n(k).is_none() || a[0].n(k) != b[0].n(k))
                    || a[0].n("input_sequence").unwrap_or(0) == 0
                    || b[0].n("input_sequence").unwrap_or(0) == 0
                {
                    valid = false;
                    break;
                }
                // 双方 Submit/Resolve/消费逐条相接；相同值或相同 seed 不能代替真实命令。
                if !left_trace.command_matches(a[0]) || !right_trace.command_matches(b[0])
                    || a[0].n("session_generation")!=Some(lg) || b[0].n("session_generation")!=Some(rg)
                    || a[0].n("input_generation").is_none_or(|g|g==0)
                    || b[0].n("input_generation").is_none_or(|g|g==0) {
                    valid=false; break;
                }
                if ordinal==1 && (a[0].n("input_generation")!=Some(lbg) || b[0].n("input_generation")!=Some(rbg)) {
                    valid=false; break;
                }
                consumed.push(json!({"ordinal":ordinal,"left":a[0].raw,"right":b[0].raw}));
            }
            if !valid {
                continue;
            }
            let scope = (
                l[0].run.clone(),
                l[0].room.clone(),
                r[0].run.clone(),
                r[0].room.clone(),
            );
            if scopes.as_ref().is_some_and(|s| s != &scope) {
                continue;
            }
            scopes = Some(scope);
            pairs.push(json!({"left":l[0].tick.unwrap().to_string(),"right":r[0].tick.unwrap().to_string()}));
            proof.push(json!({"relative_script_tick":offset,"consumption":consumed,"left_attempt":l[0].offset.to_string(),"right_attempt":r[0].offset.to_string()}));
        }
        let scope = scopes.ok_or("no_actual_script_and_consumer_aligned_tick_pair")?;
        write_json(
            &root.join("L6-alignment-basis.json"),
            &json!({"rule":"same relative frozen script tick; unique complete capture and actual A/B resolved input in both processes",
            "left_base":lb.to_string(),"right_base":rb.to_string(),"left_script_generation":lbg.to_string(),"right_script_generation":rbg.to_string(),
            "seed_used_as_key":false,"address_used_as_key":false,"pairs":proof,
            "unmapped_births":"INCOMPARABLE; never auto-match by seed/type/position"}),
        )?;
        let path = root.join("L6-alignment.json");
        write_json(
            &path,
            &json!({"version":1,"left":{"run_epoch":scope.0,"room_epoch":scope.1},
            "right":{"run_epoch":scope.2,"room_epoch":scope.3},"ticks":pairs,"births":[],"players":[{"left":0,"right":0},{"left":1,"right":1}],
            "evidence":self.alignment_evidence_tx08(true)}),
        )?;
        Ok(path)
    }
    fn l7_tx08(&self, root: &Path) -> Result<Value> {
        let Some(relative) = self.scenario["legacy_capture"].as_str() else {
            return Ok(
                json!({"status":"NOT_RUN","reason":"scenario must identify existing complete legacy material"}),
            );
        };
        let source = self.out.join("legacy-input");
        for name in ["manifest.json","captures.jsonl","lifecycle-events.jsonl","result.json","consumer.json","restoration.json"] {
            if !source.join(name).is_file() { return Err(format!("legacy_missing_original:{name}")); }
        }
        let manifest = load(&source.join("manifest.json"))?;
        if manifest["domain"] != crate::world::LEGACY {
            return Err("legacy domain mismatch".into());
        }
        let invocation = replay(root, "L7-legacy", &source, true, true)?;
        Ok(
            json!({"status":if invocation["expectation_met"]==true{"PASS"}else{"FAIL"},"cli":invocation,
                "original_source":relative,"frozen_input":source,"provenance":self.scenario["legacy_source"],"runtime_compatibility":"NOT_CLAIMED"}),
        )
    }
}
fn first_field(c: &mut Value) -> Result<&mut Value> {
    c["objects"]
        .as_array_mut()
        .ok_or("objects")?
        .iter_mut()
        .filter(|o| o["class"] == "Player")
        .find_map(|o| o["fields"].as_array_mut()?.first_mut())
        .ok_or("Player field missing".into())
}
fn canonical(c: &mut Value) {
    c["sources"]
        .as_array_mut()
        .unwrap()
        .sort_by_key(|s| s["source"].as_u64().unwrap());
    c["objects"].as_array_mut().unwrap().sort_by_key(|o| {
        (
            [
                "Player",
                "NPC",
                "Tear",
                "Projectile",
                "Pickup",
                "Familiar",
                "Effect",
                "Grid",
                "UnknownEntity",
            ]
            .iter()
            .position(|v| o["class"] == *v)
            .unwrap_or(8),
            o["object_domain"].as_u64().unwrap(),
            o["local_id"].as_str().unwrap().parse::<u64>().unwrap(),
            o["use_generation"]
                .as_str()
                .unwrap()
                .parse::<u64>()
                .unwrap(),
        )
    });
    for o in c["objects"].as_array_mut().unwrap() {
        o["fields"]
            .as_array_mut()
            .unwrap()
            .sort_by_key(|f| f["field_id"].as_str().unwrap().to_string());
        o["memberships"]
            .as_array_mut()
            .unwrap()
            .sort_by_key(|m| (m["source"].as_u64().unwrap(), m["index"].as_u64().unwrap()));
    }
}
