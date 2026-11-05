//! I28：依据计划第7节失败模型编排同一个tx的Source/冷Client整体验收。
//! 场景请求、正式源、原生提交、绘制和人工视觉审阅分别记录；不以标签冒报通过。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},process::Command,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn n(s:&Status,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn drawn(s:&Status)->u64{s.get("client.dispatch.presentation_draw").and_then(|v|v.split(',').next()).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn backdrop_skipped(s:&Status)->u64{s.get("client.dispatch.backdrop_effects").and_then(|v|v.split(',').nth(1)).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn rev(v:&Value)->u64{v["revision"].as_str().and_then(|v|v.parse().ok()).unwrap_or(0)}
// 产品内容检查只读普通文件；清单和默认配置在同一整体E2E中检查。
pub(super) fn product_files(root:&std::path::Path,at:&std::path::Path,out:&mut Vec<std::path::PathBuf>)->Result<()> {
 for item in std::fs::read_dir(at).map_err(|e|e.to_string())? {let item=item.map_err(|e|e.to_string())?;let kind=item.file_type().map_err(|e|e.to_string())?;
  if kind.is_symlink(){return Err("product symlink".into())}if kind.is_dir(){product_files(root,&item.path(),out)?}else{out.push(item.path().strip_prefix(root).map_err(|e|e.to_string())?.to_path_buf());if out.len()>4096{return Err("product file budget".into())}}
 }Ok(())
}
// 具名恶意值只在E2E目标内修改；全部经过正式codec和Client入口，不能直接写SDK。
pub(super) fn revision(v:&mut Value,r:u64){v["revision"]=json!(r.to_string());
 for (_,c) in v["components"].as_object_mut().unwrap(){c["version"]=json!(r.to_string());}
 fn refs(v:&mut Value,r:u64){match v{Value::Object(o)=>{if o.contains_key("required_revision"){o.insert("required_revision".into(),json!(r.to_string()));}for v in o.values_mut(){refs(v,r)}},Value::Array(a)=>for v in a{refs(v,r)},_=>()}}
 refs(&mut v["components"],r);
}
impl Run {
 fn wait_tx18(&mut self,g:&win32::Game,c:&mut Control,label:&str,p:impl Fn(&Status)->bool)->Result<Status>{
  let end=Instant::now()+Duration::from_secs(90);
  loop {let s=self.watch_tx05(g,c,0)?;if p(&s){self.step(label,json!(s))?;return Ok(s)}
   if s.get("client.native_phase").map(String::as_str)==Some("retired")&&s.get("client.error").is_some_and(|v|!v.is_empty()){return Err(format!("{label} native failure: {s:?}"))}
   if Instant::now()>=end{return Err(format!("{label} timeout: {s:?}"))}thread::sleep(Duration::from_millis(65));}
 }
 fn apply_tx18(&mut self,g:&win32::Game,c:&mut Control,label:&str,mode:&str)->Result<Status>{
  let model:Value=serde_json::from_str(&crate::read(&self.out.join(format!("runtime-world/client-targets/{label}.json")))?).map_err(|e|e.to_string())?;
  let r=rev(&model);let token=self.report["token"].as_str().ok_or("token")?;
  let accepted=c.command(&format!("cs client_test {token} {mode} {label}"))?;
  if accepted.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("{label} refused {accepted:?}; {:?}",c.command("cs status")?))}
  self.wait_tx18(g,c,label,|s|n(s,"client_world_applied")==1&&n(s,"client.applied_revision")==r)
 }
 // 不接受任意文件变化作为Host合法进度；Client单独基线覆盖从Begin到窗口正常关闭。
 pub(super) fn files_tx18(&mut self,name:&str)->Result<Value>{
  self.snapshot(name)?;let changes=crate::saves::changes(&self.out.join("client_before"),&self.out.join(name))?;
  for v in changes.as_array().ok_or("file changes")?{if v["bytes_changed"]!=true{continue}let path=v["path"].as_str().ok_or("changed path")?;
   if matches!(path,"documents/log.txt"|"documents/options.ini"){continue}
   // Windows重新放置窗口会同步REPENTOGON的原版选项副本；仅豁免已观察的X/Y坐标，其他字节仍严格比较。
   if path=="documents/Repentogon/vanilla_options.ini"{
    let before=crate::read(&self.out.join("client_before").join(path))?;let after=crate::read(&self.out.join(name).join(path))?;
    let keep=|s:&str|s.lines().filter(|s|!s.starts_with("WindowPosX=")&&!s.starts_with("WindowPosY=")).map(str::to_owned).collect::<Vec<_>>();
    if keep(&before)==keep(&after){continue}
   }
   return Err(format!("Client progress changed: {changes}"))
  }Ok(changes)
 }
 pub(super) fn scenes_tx18(&mut self,g:&win32::Game)->Result<()> {
  // 真实产品清单另建临时安装树；其中不得混入fixture/Rust/E2E配置。
  let product=self.out.join("product-package");
  let packaged=Command::new("pwsh").args(["-NoProfile","-File"]).arg(self.repo.join("scripts/package.ps1")).arg("-OutputDirectory").arg(&product).output().map_err(|e|e.to_string())?;
  if !packaged.status.success(){return Err(format!("product package: {}",String::from_utf8_lossy(&packaged.stderr)))}
  let mut files=Vec::new();product_files(&product,&product,&mut files)?;
  if files.iter().any(|p|p.to_string_lossy().contains("isaac-cs-test")||p.extension().is_some_and(|v|v=="exe")){return Err("E2E tools leaked into product package".into())}
  let config:Value=serde_json::from_str(&crate::read(&product.join("Repentogon/isaac-cs/config.json"))?).map_err(|e|e.to_string())?;
  if config["enabled"]!=false||config["world_record"]["enabled"]!=false||config["diagnostics"]["enabled"]!=false||config["control_port"]["enabled"]!=false||config["world_record"]["e2e_token"].is_string(){return Err("product default diagnostics/test config".into())}
  self.case("product_package_off_no_test_tools",true,json!({"files":files,"config":config}));
  win32::set_auto_focus(true);let w=g.wait_window(90)?;
  let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;
  c.focus_window=Some(w);self.prepare_new_run_tx05(g,w,&mut c)?;
  self.report["node_role"]=self.scenario["node_role"].clone();
  let scenes=self.scenario["scenes"].as_array().ok_or("scenes")?.clone();
  if self.scenario["node_role"]=="Source" {
   self.sequence(w,"start_run")?;c.wait("run_active","1",30)?;
   c.command("cs observe start")?;c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;c.wait("session_state","Active",30)?;
   let mut samples=Vec::new();let token=self.report["token"].as_str().ok_or("token")?.to_owned();
   // 续测复用已经导出成功的完整前缀；新Source仍重放必要准备动作，失败项及后续重新采集。
   // 不改旧目标的真实Scope；Client在不同Source session之间正常退休并重新Begin。
   if let Some(path)=self.scenario["source_run"].as_str(){let previous=std::path::PathBuf::from(path);
    let report:Value=serde_json::from_str(&crate::read(&previous.join("result.json"))?).map_err(|e|e.to_string())?;
    if report["tx"]!="tx-04-18"||report["node_role"]!="Source"||!report["evidence_error"].is_null()||report["cleanup_errors"].as_array().is_none_or(|v|v.iter().any(|v|!v.is_null())){return Err("Source resume must have clean exit/environment evidence".into())}
    samples=serde_json::from_str::<Vec<Value>>(&crate::read(&previous.join("source-progress.json"))?).map_err(|e|e.to_string())?;
    // 修正Source夹具时只复用明确未受影响的前缀；原报告保持原样，后半段在新进程重新采集。
    if let Some(count)=self.scenario["source_reuse_prefix"].as_u64(){if count>samples.len() as u64{return Err("Source reuse prefix exceeds available samples".into())}samples.truncate(count as usize);}
    if samples.len()>scenes.len(){return Err("Source resume prefix size".into())}
    for (i,sample) in samples.iter().enumerate(){if sample["label"]!=scenes[i]["label"]||sample["export"]["exported"]!="1"{return Err("Source resume prefix identity".into())}
     let label=sample["label"].as_str().ok_or("resume label")?;
     for stem in [label.to_owned(),format!("source-full-{label}")]{
      let to=self.out.join(format!("runtime-world/client-targets/{stem}.json"));crate::mkdir(to.parent().ok_or("target parent")?)?;
      std::fs::copy(previous.join(format!("runtime-world/client-targets/{stem}.json")),to).map_err(|e|e.to_string())?;
     }
     std::fs::copy(previous.join(format!("source-{label}.bmp")),self.out.join(format!("source-{label}.bmp"))).map_err(|e|e.to_string())?;
    }
    self.report["source_resume"]=json!({"directory":previous,"reused_prefix":samples.len()});
   }
   let reused=samples.len();
   if reused>0{c.command(&format!("cs client_test {token} sampling prepare"))?;}
   for (i,spec) in scenes.iter().enumerate(){let label=spec["label"].as_str().ok_or("label")?;
    // 提前一个准备场景恢复采样，使最后Room承载/身份上下文先建立，再构造待续测Grid。
    if reused>0&&i==reused.saturating_sub(1){c.command(&format!("cs client_test {token} sampling capture"))?;}
    win32::key(w,"F8")?;let end=Instant::now()+Duration::from_secs(90);
    let actual=loop{self.watch_tx05(g,&mut c,70)?;let rows=self.live_fixture()?;
     if let Some(e)=rows.iter().find(|r|r["event"]=="fixture_error"){return Err(format!("Source {label}: {e}"))}
     if let Some(r)=rows.iter().find(|r|r["event"]=="ready"&&r["label"]==label&&r["action"]==i+1){break r.clone()}
     if Instant::now()>=end{let diagnosis=c.command(&format!("cs client_test {token} export {label}"))?;return Err(format!("Source ready {label}: {diagnosis:?}"))}};
    if i<reused{continue;}
    let end=Instant::now()+Duration::from_secs(45);
    let exported=loop{self.watch_tx05(g,&mut c,60)?;
     {
      let r=c.command(&format!("cs client_test {token} export {label}"))?;if r.get("exported").map(String::as_str)==Some("1"){break r}
      if Instant::now()>=end{return Err(format!("Source export {label}: {r:?}"))}
     }
     if Instant::now()>=end{return Err(format!("Source formal observation {label} timeout"))}
    };
    let world=c.command("cs world status")?;if world.get("world_state").map(String::as_str)==Some("Recording"){return Err("Source unexpectedly depends on recorder".into())}
    // Source公开原始槽集合独立对照正式库存，不能把漏掉的仍在绘制的Grid误报Complete。
    let model:Value=serde_json::from_str(&crate::read(&self.out.join(format!("runtime-world/client-targets/{label}.json")))?).map_err(|e|e.to_string())?;
    let mut indices=BTreeSet::new();
    for o in model["components"]["grid"]["state"]["objects"].as_array().ok_or("Grid objects")?{
     for member in o["memberships"].as_array().ok_or("Grid memberships")?{
      if member["source"]==11{indices.insert(member["index"].as_u64().ok_or("Grid membership index")?);}
     }
    }
    let actual_indices=actual["grid_indices"].as_array().ok_or("Source raw Grid indices")?.iter().map(|v|v.as_u64().ok_or("Source raw Grid index")).collect::<std::result::Result<BTreeSet<_>,_>>()?;
    if indices!=actual_indices{return Err(format!("{label} formal Grid inventory differs from Source raw slots: formal={indices:?} actual={actual_indices:?}"))}
    win32::screenshot(w,&self.out.join(format!("source-{label}.bmp")))?;
    samples.push(json!({"label":label,"actual":actual,"export":exported,"recorder":world}));
    crate::write_json(&self.out.join("source-progress.json"),&json!(samples))?;
   }
   c.command("cs stop")?;c.wait("session_state","Idle",30)?;c.command("cs observe stop")?;self.return_to_menu(w,&mut c)?;drop(c);
   if g.close()?!=0{return Err("Source abnormal exit".into())}
   self.report["source_samples"]=json!(samples);self.case("formal_source_recording_off",true,json!({"samples":scenes.len()}));
   self.report["acceptance_stage"]=json!("source_ready_for_independent_client");return Ok(());
  }
  let source=std::path::PathBuf::from(self.scenario["source_run"].as_str().ok_or("source_run")?);
  let source_report:Value=serde_json::from_str(&crate::read(&source.join("result.json"))?).map_err(|e|e.to_string())?;
  if source_report["tx"]!="tx-04-18"||source_report["node_role"]!="Source"||source_report["pass"]!=true{return Err("completed tx18 Source required".into())}
  let samples=source_report["source_samples"].as_array().ok_or("source samples")?;
  if samples.len()!=scenes.len(){return Err("source scenario coverage mismatch".into())}
  crate::copy_tree(&source.join("runtime-world/client-targets"),&self.out.join("runtime-world/client-targets"))?;
  self.report["source_run"]=json!(source);self.report["source_samples"]=json!(samples);
  self.snapshot("client_before")?;c.focus_window=None;
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();let mut types=BTreeSet::new();let mut shapes=BTreeSet::new();let mut applied=Vec::new();let mut last=None;
  // 统一验收内先收齐正式接纳诊断；不构造/显示原生世界，也不替代后续逐目标实机。
  let mut preflight_errors=Vec::new();
  for spec in &scenes{let label=spec["label"].as_str().ok_or("label")?;
   let result=c.command(&format!("cs client_test {token} preflight {label}"))?;
   if result.get("accepted").map(String::as_str)!=Some("1"){preflight_errors.push(json!({"label":label,"result":result}));}
  }
  if !preflight_errors.is_empty(){return Err(format!("formal target preflight: {}",json!(preflight_errors)))}
  self.case("all_formal_targets_preflight_without_native_write",true,json!({"targets":scenes.len()}));
  // 同一tx续测可复用未受修改影响且正常清理的Client前缀；新进程仍冷构建baseline及最后A。
  let mut previous=Vec::<Value>::new();let mut previous_path=None;
  if let Some(path)=self.scenario["client_previous_run"].as_str(){let dir=std::path::PathBuf::from(path);
   let report:Value=serde_json::from_str(&crate::read(&dir.join("result.json"))?).map_err(|e|e.to_string())?;
   if report["tx"]!="tx-04-18"||report["node_role"]!="Client"||report["source_run"]!=self.report["source_run"]||!report["evidence_error"].is_null()||report["cleanup_errors"].as_array().is_none_or(|v|v.iter().any(|v|!v.is_null())){return Err("Client resume requires same Source and clean prior exit".into())}
   previous=serde_json::from_str(&crate::read(&dir.join("client-progress.json"))?).map_err(|e|e.to_string())?;
   if previous.len()>scenes.len(){return Err("Client resume prefix size".into())}
   for (i,value) in previous.iter().enumerate(){if value["label"]!=scenes[i]["label"]{return Err("Client resume prefix labels".into())}}
   self.report["client_resume"]=json!({"directory":dir,"reused_prefix":previous.len(),"cold_rebuild":["baseline","last-prefix-target"]});previous_path=Some(dir);
  }
  let mut source_session=String::new();
  for (i,spec) in scenes.iter().enumerate(){let label=spec["label"].as_str().ok_or("label")?;
   let target:Value=serde_json::from_str(&crate::read(&self.out.join(format!("runtime-world/client-targets/{label}.json")))?).map_err(|e|e.to_string())?;
   let session=target["scope"]["session"].as_str().ok_or("source session")?.to_owned();
   if let Some(t)=samples[i]["actual"]["types"].as_object(){for k in t.keys(){types.insert(k.clone());}}
   if spec["kind"]=="shape"{shapes.insert(spec["value"].as_u64().ok_or("shape")?);}
   if i<previous.len()&&i!=0&&i+1!=previous.len(){
    std::fs::copy(previous_path.as_ref().ok_or("previous path")?.join(format!("client-{label}.bmp")),self.out.join(format!("client-{label}.bmp"))).map_err(|e|e.to_string())?;
    let mut value=previous[i].clone();value["reused_from"]=json!(previous_path);applied.push(value);continue;
   }
   let changed=i>0&&session!=source_session;if changed{c.command("cs stop")?;c.wait("role","Off",45)?;}
   let effect_before=if i==0||changed{0}else{backdrop_skipped(&c.command("cs status")?)};
   let result=self.apply_tx18(g,&mut c,label,if i==0||changed{"begin"}else{"target"})?;source_session=session;
   if label=="boss-backdrop"||label=="sacrifice-backdrop" {
    // 轨迹helper之外必须实际命中direct分支；防止新增场景仅换了标签而未覆盖补丁。
    let helpers=target["components"]["room"]["state"]["backdrop_wall_trace"]["steps"].as_array().map_or(0,|v|v.len() as u64);
    if backdrop_skipped(&result)<=effect_before+helpers{return Err(format!("{label} direct backdrop effect branch not exercised"))}
   }
   self.wait_tx18(g,&mut c,&format!("draw-{label}"),|s|drawn(s)>=drawn(&result)+2)?;
   // 每个显示窗口都核对原链计数，不能只用最后一次重连的基线代表整个Client过程。
   let counters=c.command("cs counters")?;
   for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{
    if n(&result,&format!("client.original_begin.{name}"))!=n(&counters,&format!("hook.{name}")){return Err(format!("{label} Client gameplay leaked {name}"))}
   }
   let grids=target["components"]["grid"]["state"]["objects"].as_array().ok_or("grids")?;
   if n(&result,"client.grid_owners")!=grids.len() as u64{return Err(format!("{label} native Grid owner count"))}
   if n(&result,"client.renderer_window")!=w as u64{return Err(format!("{label} capture window differs from native renderer"))}
   // 截取已核对的原生renderer窗口；桌面区域可能被其它应用遮挡，不能作为该窗口的像素证据。
   let screenshot=self.out.join(format!("client-{label}.bmp"));win32::screenshot(w,&screenshot)?;
   let pixels=std::fs::read(&screenshot).map_err(|e|e.to_string())?;
   let (mut neutral,mut count)=(0usize,0usize);
   for p in pixels.get(54..).ok_or("capture BMP header")?.chunks_exact(4){count+=1;if p[..3].iter().all(|v|*v>250)||p[..3].iter().all(|v|*v<5){neutral+=1}}
   if count==0||neutral*100>count*98{return Err(format!("{label} displayed window capture is blank"))}
   let repeated=if label=="dense" {20}else{2};for _ in 0..repeated {
    let duplicate=self.apply_tx18(g,&mut c,label,"target")?;
    if n(&duplicate,"client.native_attempt")!=n(&result,"client.native_attempt")||n(&duplicate,"client.grid_prefix")!=n(&result,"client.grid_prefix")||
       n(&duplicate,"client.resource_candidate_loads")!=n(&result,"client.resource_candidate_loads"){return Err(format!("{label} duplicate rebuilt native owners or reloaded resources"))}
   }
   applied.push(json!({"label":label,"status":result}));last=Some(target);
   self.report["applied_samples"]=json!(applied);crate::write_json(&self.out.join("client-progress.json"),&self.report["applied_samples"])?;
  }
  if types!=(1..=27).map(|v|v.to_string()).collect()||shapes!=(1..=12).collect(){return Err(format!("incomplete categories: types={types:?}, shapes={shapes:?}"))}
  self.case("source_cold_client_all_shapes_and_grid_types",true,json!({"types":types,"shapes":shapes,"targets":applied.len()}));
  let base=last.ok_or("last target")?;let base_rev=rev(&base);let current=c.command("cs status")?;
  for (label,mut bad) in [("bad-route",base.clone()),("bad-budget",base.clone()),("bad-pose",base.clone()),("bad-reference",base.clone()),("same-version-conflict",base.clone())]{
   revision(&mut bad,if label=="same-version-conflict"{base_rev}else{base_rev+1});
   match label{
    "bad-route"=>bad["components"]["room"]["state"]["room_index"]=json!(-100),
    "bad-budget"=>bad["components"]["grid"]["state"]["application_runtime_budget"]=json!({"embers":vec![json!({});32769]}),
    "bad-reference"=>{let mut reference=bad["components"]["grid"]["state"]["objects"][0]["subject"].clone();reference["target"]["id"]=json!("999999999");bad["components"]["grid"]["state"]["objects"][0]["required_dependency"]=reference;},
    "same-version-conflict"=>bad["components"]["entity"]["state"]["objects"][0]["common"]["fields"]["Entity.GetSprite"]["fields"]["Sprite.Rotation"]=json!(71),
    _=>bad["components"]["entity"]["state"]["objects"][0]["common"]["fields"]["Entity.GetSprite"]["fields"]["Sprite.GetFilename"]=json!("../invalid.anm2")}
   crate::write_json(&self.out.join(format!("runtime-world/client-targets/{label}.json")),&bad)?;
   let rejected=c.command(&format!("cs client_test {token} target {label}"))?;let status=c.command("cs status")?;
   if rejected.get("accepted").map(String::as_str)!=Some("0")||n(&status,"client.applied_revision")!=base_rev||n(&status,"client_world_applied")!=1{return Err(format!("prewrite rejection {label} lost A: {status:?}"))}
  }
  self.case("prewrite_invalid_route_resource_and_budget_preserve_A",true,json!(current));
  let stale=c.command(&format!("cs client_test {token} target baseline"))?;
  if stale.get("accepted").map(String::as_str)!=Some("0"){return Err("retired source scope revived".into())}
  self.case("same_revision_conflict_retired_scope_and_missing_reference",true,json!(stale));
  let ownership=c.command(&format!("cs client_test {token} exercise ownership"))?;
  if ownership.get("ownership_exercise").map(String::as_str)!=Some("1"){return Err(format!("ownership exercise {ownership:?}"))}
  self.case("thread_context_released_borrow_rejection",true,json!(ownership));
  let vm=c.command(&format!("cs client_test {token} exercise vm"))?;
  if vm.get("vm_exercise").map(String::as_str)!=Some("1"){return Err(format!("vm identity exercise {vm:?}"))}
  self.case("actual_secondary_vm_inflight_generation_rejection",true,json!(vm));
  c.command("cs stop")?;c.wait("role","Off",30)?;
  for mode in ["begin-cancel","begin-cancel-native","begin-cancel-grid","begin-cancel-display"] {
   let label=if mode=="begin-cancel-grid"{"all-grids"}else{"baseline"};
   let accepted=c.command(&format!("cs client_test {token} {mode} {label}"))?;
   if accepted.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("cancel begin {mode}: {accepted:?}"))}
   c.wait("role","Off",45)?;self.step(mode,json!(c.command("cs status")?))?;
  }
  self.case("stop_before_write_native_grid_and_display_recover",true,json!({"stages":4}));
  self.apply_tx18(g,&mut c,"all-grids","begin")?;
  let baseline_path=self.out.join("runtime-world/client-targets/all-grids.json");
  let baseline:Value=serde_json::from_str(&crate::read(&baseline_path)?).map_err(|e|e.to_string())?;
  let mut missing=baseline.clone();revision(&mut missing,rev(&baseline)+1);
  let grids=missing["components"]["grid"]["state"]["objects"].as_array_mut().ok_or("grids")?;
  let grid=grids.iter_mut().find(|g|g["type"]==1).ok_or("partial failure requires actual Decoration")?;
  grid["common"]["fields"]["GridEntity.GetSprite"]["fields"]["Sprite.GetFilename"]=json!("gfx/tx18-required-resource-missing.anm2");
  crate::write_json(&self.out.join("runtime-world/client-targets/missing-resource.json"),&missing)?;
  let accepted=c.command(&format!("cs client_test {token} target missing-resource"))?;
  if accepted.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("partial failure never reached native: {accepted:?}"))}
  let failed=self.wait_tx18(g,&mut c,"partial-grid-resource-retired",|s|n(s,"client_world_applied")==0&&s.get("client.native_phase").map(String::as_str)==Some("retired"))?;
  if failed.get("client.failure_phase").map(String::as_str)!=Some("grid_apply"){return Err(format!("unexpected partial failure {failed:?}"))}
  let mut recovery=baseline.clone();revision(&mut recovery,rev(&baseline)+2);
  crate::write_json(&self.out.join("runtime-world/client-targets/recover.json"),&recovery)?;
  let initial=self.apply_tx18(g,&mut c,"recover","target")?;
  self.case("partial_grid_write_withdraws_A_retires_and_rebuilds",true,json!(failed));
  let before=c.command("cs status")?;let isolated=c.command(&format!("cs client_test {token} exercise native"))?;
  if isolated.get("isolation_exercise").map(String::as_str)!=Some("1"){return Err(format!("native isolation {isolated:?}"))}
  let after=c.command("cs status")?;let counters=c.command("cs counters")?;
  for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{
   if n(&initial,&format!("client.original_begin.{name}"))!=n(&counters,&format!("hook.{name}")){return Err(format!("Client gameplay leaked {name}"))}}
  if n(&after,"persist_client_denied_native")<=n(&before,"persist_client_denied_native")||n(&after,"persist_client_denied_lua")<n(&before,"persist_client_denied_lua")+6{return Err("persistent front guards missing".into())}
  self.case("whole_client_window_gameplay_callback_rng_progress_guards",true,json!({"before":before,"after":after,"exercise":isolated}));
  if self.scenario["player_environment_regression"]==true{self.player_environment_tx20(g,w,&mut c,&source)?;}
  c.command("cs stop")?;c.wait("role","Off",45)?;let files=self.files_tx18("client_after")?;self.case("client_persistence",true,files);
  // 真正窗口关闭发生在Client显示期，覆盖退休至Shutdown最后writer。
  self.apply_tx18(g,&mut c,"all-grids","begin")?;drop(c);if g.close()?!=0{return Err("Client exit".into())}
  let files=self.files_tx18("client_exit")?;self.case("client_shutdown_progress",true,files);
  self.report["acceptance_stage"]=json!("automatic_checks_complete_visual_review_pending");
  Ok(())
 }
}
