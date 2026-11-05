//! I37按真实旧目录展开受影响项目；只保留场景定义，绝不复用旧离线PASS。
use crate::Result;
use serde_json::{Value,json};

// 21复用并修改18—20的来源/codec/安装/资源/退休，因而原正向场景全部受影响。
pub fn inventory()->Result<Vec<Value>>{
 let mut rows=Vec::new();
 for(tx,text)in [(18,include_str!("../../scenarios/tx-04-18.json")),(19,include_str!("../../scenarios/tx-04-19.json")),(20,include_str!("../../scenarios/tx-04-20.json"))]{
  let doc:Value=serde_json::from_str(text).map_err(|e|e.to_string())?;
  let mut prefix=Vec::new();for scene in doc["scenes"].as_array().ok_or("regression_scene_inventory")?{
   prefix.push(scene.clone());rows.push(json!({"id":format!("tx{tx}-positive-{}",scene["label"].as_str().ok_or("scene_label")?),"tx":tx,"polarity":"positive","scene":scene,"prefix":prefix,"affected":["formal-source","range-owner","compact-codec","native-adapter","resource","retirement"],"execution":"Host real fixture action;Client real network;exact-version native+visual"}));
  }
 }
 for(tx,names)in [
  (18,vec!["bad-route","bad-budget","bad-pose","bad-reference","same-version-conflict","retired-scope","missing-grid-resource","released-borrow","wrong-thread","wrong-context","secondary-vm","cancel-prepare","cancel-native","cancel-grid","cancel-display"]),
  (19,vec!["old-revision","same-revision-conflict","partial","source-version","field-read-failed","field-truncated","shader-read-failed","bad-class","bad-number","bad-bitset","bad-reference","missing-owner","dangling-graph","bad-resource","pool-budget","player-scope","fx-blend-range","fx-blend-missing","held-owner-missing","held-owner-wrong-class","caps-model","caps-catalog"]),
  (20,vec!["old","conflict","partial","source-version","mixed-window","cross-scope","missing-body","duplicate-body","unknown-role","width","unresolved","backup-slot","missing-final-state","missing-resource","history-gap","budget","wrong-group","wrong-shape","missing-twin","self-held","native-claim","caps-id","caps-source","caps-catalog"])
 ]{for name in names{rows.push(json!({"id":format!("tx{tx}-negative-{name}"),"tx":tx,"polarity":"negative","negative":name,"affected":["prewrite-reject","A-preservation","first-cause","bounded-owner"],"execution":"derive malformed model from real received closure;actual Scene prewrite reject;wire faults separately;no file import"}));}}
 // 所有Entity/Player具名前缀逐个保留；不能只验证一个代表失败owner。
 for point in ["acquire","init","fields","resources","publish","references","check","draw"]{for side in ["before","after"]{rows.push(json!({"id":format!("tx19-owner-{point}-{side}"),"tx":19,"polarity":"fault","point":format!("{point}-{side}")}));}}
 for point in ["allocate","ctor","init","fields","resources","hud","roster","publish","weapon","relations","check","draw"]{for side in ["before","after"]{rows.push(json!({"id":format!("tx20-owner-{point}-{side}"),"tx":20,"polarity":"fault","point":format!("player-{point}-{side}")}));}}
 Ok(rows)
}

pub fn cases()->Result<Vec<Value>>{Ok(inventory()?.into_iter().map(|row|{
 let action=json!({"op":"observe","check":"network-regression-entry","entry":row,"timeout_ms":300000});
 json!({"id":format!("E25.{}",row["id"].as_str().unwrap_or("invalid")),"e":"E25","initial":{"load":"L0c","network":"N0"},"p2":[action.clone()],"p4":[action],"p4_extra":["three-real-client-identities","same-regression-four-control-groups"],"timeout_ms":900000,"pass_requires_all_steps":true})
 }).collect())}
