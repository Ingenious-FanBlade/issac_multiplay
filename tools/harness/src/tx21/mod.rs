//! tx21仅供统一I39入口调用的场景、真实UDP损伤、同版本比较和指标归约。
//! 失败模型见tx-04-21-e2e-plan.md；完整run由I38入口调用，不提供单项预跑入口。
pub mod cases;
pub mod compare;
pub mod proxy;
pub mod reduce;
pub mod regression;
mod node;
mod runner;
use crate::Result;
use serde_json::{Value,json};
use std::{collections::BTreeMap,path::Path};

// 启动/远端生命周期由I38的Live/Node提供；所有动作均指向真实游戏与物理窗口。
pub trait Driver {
    fn command(&mut self,node:&str,text:&str)->Result<BTreeMap<String,String>>;
    fn physical(&mut self,node:&str,keys:&[String],hold_ms:u64)->Result<()>;
    fn observe(&mut self,check:&str,parameters:&Value,timeout_ms:u64)->Result<Value>;
    fn network(&mut self,parameters:&Value)->Result<Value>;
    fn human(&mut self,instruction:&str)->Result<()>;
    fn sample(&mut self,milliseconds:u64,mark:&Value)->Result<Value>;
    fn screenshot(&mut self,node:&str,version:&Value)->Result<Value>;
    fn restore(&mut self)->Result<()>;
}

// 不把返回ok或等待超时当成场景成功；每个步骤都有具名的实际观察器。
pub fn execute(driver:&mut dyn Driver,case:&Value,phase:&str)->Result<Value>{
    let plan=case.get(phase).and_then(Value::as_array).ok_or("tx21_phase_steps_missing")?;
    let started=std::time::Instant::now();let timeout=case["timeout_ms"].as_u64().ok_or("case_timeout_required")?;
    let mut facts=Vec::new();
    let result=(||->Result<()>{for step in plan{
        if started.elapsed().as_millis()>timeout as u128{return Err("tx21_case_timeout_at_complete_step_boundary".into())}
        let fact=match step["op"].as_str().ok_or("tx21_step_op")?{
            "command"=>{let node=step["node"].as_str().ok_or("node")?;let text=step["text"].as_str().ok_or("command")?;let reply=driver.command(node,text)?;if reply.get("ok").map(String::as_str)!=Some("1"){return Err(format!("{}: {reply:?}",case["id"]))}json!(reply)},
            "physical"=>{let keys=step["keys"].as_array().ok_or("keys")?.iter().map(|v|v.as_str().map(str::to_owned).ok_or("key".into())).collect::<Result<Vec<_>>>()?;driver.physical(step["node"].as_str().ok_or("node")?,&keys,step["hold_ms"].as_u64().ok_or("hold_ms")?)?;json!({"sent_physical":true,"requires_consumer_witness":true})},
            "network"=>driver.network(step)?,
            "human"=>{driver.human(step["instruction"].as_str().ok_or("instruction")?)?;json!({"human_window_completed":true})},
            "sample"=>driver.sample(step["duration_ms"].as_u64().ok_or("duration_ms")?,step)?,
            "screenshot"=>driver.screenshot(step["node"].as_str().ok_or("node")?,&step["version"] )?,
            "observe"=>{let name=step["check"].as_str().ok_or("check")?;if name=="host-fixture-action"{driver.physical("host",&["F8".into()],50)?;driver.physical("host",&[],50)?;}let value=driver.observe(name,step,step["timeout_ms"].as_u64().ok_or("timeout_ms")?)?;if value["pass"]!=true{return Err(format!("{} {}: {value}",case["id"],step["check"]))}value},
            _=>return Err(format!("unknown tx21 action {}",step["op"])),
        };facts.push(json!({"step":step,"fact":fact}));
    }if phase=="p4"{for check in case["p4_extra"].as_array().into_iter().flatten(){let name=check.as_str().ok_or("p4 assertion name")?;let value=driver.observe(name,&json!({"phase":"p4","case":case["id"],"requires":"three real Client identities and declared four-group extension"}),30000)?;if value["pass"]!=true{return Err(format!("P4扩展未完成:{name}:{value}"))}facts.push(json!({"p4_extra":name,"fact":value}));}}if started.elapsed().as_millis()>timeout as u128{return Err("tx21_case_timeout_at_complete_step_boundary".into())}Ok(())})();
    let cleanup=driver.restore();
    Ok(json!({"id":case["id"],"phase":phase,"pass":result.is_ok()&&cleanup.is_ok(),"failure":result.err(),"cleanup_failure":cleanup.err(),"facts":facts}))
}
// 相反射击必须在同一真实物理源上保留前键，再按后键，最后释放后键观察回退。
// keys由I38读取实际键位后提供；任一缺键/目标窗口缺席都不能改用Submit。
pub fn physical_script(driver:&mut dyn Driver,node:&str,action:&str,keys:&BTreeMap<String,String>)->Result<()>{
 let lookup=|s:&str|keys.get(s).cloned().ok_or_else(||format!("physical_mapping_missing:{node}/{s}"));
 let schedule=match action{
  "move"=>vec![(vec![lookup("move-right")?],700),(vec![lookup("move-down")?],700)],
  "opposite-shoot-last-wins"|"release-fallback"=>{let l=lookup("shoot-left")?;let r=lookup("shoot-right")?;let u=lookup("shoot-up")?;let d=lookup("shoot-down")?;vec![(vec![l.clone()],250),(vec![l.clone(),r],350),(vec![l],350),(vec![],100),(vec![u.clone()],250),(vec![u.clone(),d],350),(vec![u],350)]},
  "bomb"|"active"|"card-pill"|"drop"=>vec![(vec![lookup(action)?],80)],
  _=>return Err("unknown_physical_script".into()),
 };let result=(||{for(held,ms)in schedule{driver.physical(node,&held,ms)?;}Ok(())})();let release=driver.physical(node,&[],100);result.and(release)
}

// 离线归约仍只能读取I39采集的真实事实，不生成合成通过样本。
pub fn cli(args:&[String])->Result<()>{
    if args.len()<3{return Err("tx21 run <config> <run-directory> | node <node-config> | reduce <case-directory>".into())}
    match args[1].as_str(){
      "run" if args.len()==4=>runner::run(Path::new(&args[2]),Path::new(&args[3])),
      "node" if args.len()==3=>node::serve(Path::new(&args[2])),
      "reduce" if args.len()==3=>reduce::directory(Path::new(&args[2])),
      _=>Err("tx21 unified entry arguments".into())
    }
}
