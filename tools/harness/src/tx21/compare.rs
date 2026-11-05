//! 精确同世界/范围版本对照；缺版本、缺原生值或未审画面均不通过。
use crate::Result;
use serde_json::{Value,json};
use std::collections::BTreeMap;
use super::Driver;
// IPC按固定小页取临时纯值，任何过期/缺页都失败，不选择另一最新版本补洞。
pub fn read_snapshot(driver:&mut dyn Driver,node:&str,token:&str,id:u64)->Result<(Value,String)>{let mut text=Vec::<u8>::new();let mut render=String::new();let mut total=None;for page in 0..4096{
 let reply=driver.command(node,&format!("cs stream_test {token} snapshot {id} {page}"))?;if reply.get("ok").map(String::as_str)!=Some("1"){return Err("snapshot_expired_or_unavailable".into())}let bytes=reply.get("bytes").ok_or("snapshot_bytes")?.parse::<usize>().map_err(|e|e.to_string())?;if bytes>33554432||total.is_some_and(|old|old!=bytes){return Err("snapshot_budget_or_changed".into())}total=Some(bytes);if reply.get("encoding").map(String::as_str)!=Some("hex-utf8-bytes"){return Err("snapshot_encoding".into())}let chunk=reply.get("data").ok_or("snapshot_data")?;if chunk.len()%2!=0||chunk.len()>16384{return Err("snapshot_hex_length".into())}for b in chunk.as_bytes().chunks_exact(2){let digit=|c:u8|match c{b'0'..=b'9'=>Ok(c-b'0'),b'a'..=b'f'=>Ok(c-b'a'+10),_=>Err("snapshot_hex_digit".to_string())};text.push((digit(b[0])?<<4)|digit(b[1])?);}if let Some(r)=reply.get("render"){render=r.clone()}if text.len()==bytes{return Ok((serde_json::from_slice(&text).map_err(|e|e.to_string())?,render))}if text.len()>bytes{return Err("snapshot_page_overflow".into())}}
 Err("snapshot_page_limit".into())}
fn number(v:&Value)->Option<u64>{v.as_u64().or_else(||v.as_str()?.parse().ok())}
fn key(row:&Value)->String{format!("{}/{}/{}",number(&row["run"]).unwrap_or(0),number(&row["space"]).unwrap_or(0),number(&row["id"]).unwrap_or(0))}
fn scalar(v:&Value)->Value{if v["type"]=="int64"{if let Some(n)=v["value"].as_str().and_then(|s|s.parse::<i64>().ok()){return json!(n)}}if v["type"]=="vector"{return json!({"x":v["x"],"y":v["y"]})}if v["type"]=="bits128"{if let (Some(low),Some(high))=(number(&v["low"]),number(&v["high"])){return json!([low as u32,(low>>32)as u32,high as u32,(high>>32)as u32])}}v.clone()}
fn fields(v:&Value,out:&mut BTreeMap<String,Value>){match v{Value::Object(map)=>{for(k,v)in map{if k.contains('.')&&(v.is_number()||v.is_boolean()||matches!(v["type"].as_str(),Some("int64"|"vector"|"bits128"))){out.insert(k.clone(),scalar(v));}else{fields(v,out)}}},Value::Array(values)=>for v in values{fields(v,out)},_=>{}}}
fn actual_fields(source:&Value,objects:&Value,players:&Value)->Result<Value>{let mut expected=BTreeMap::<String,BTreeMap<String,Value>>::new();
 for row in source["entity"]["objects"].as_array().into_iter().flatten(){let mut f=BTreeMap::new();fields(row,&mut f);expected.insert(key(&row["subject"]["target"]),f);}
 for row in source["fast_bodies"].as_array().into_iter().flatten(){let mut f=BTreeMap::new();fields(row,&mut f);expected.insert(key(&row["key"]),f);}
 let mut differences=Vec::new();let mut checked=0;let mut verifier_only=Vec::new();for row in objects.as_array().into_iter().flatten().chain(players.as_array().into_iter().flatten()){
  if let Some(wanted)=expected.get(&key(row)){for(name,actual)in row["fields"].as_object().into_iter().flatten(){if let Some(value)=wanted.get(name){checked+=1;if scalar(actual)!=*value{differences.push(json!({"key":key(row),"field":name,"source":value,"actual":actual}))}}else{verifier_only.push(json!({"key":key(row),"field":name,"reason":"private-native final member verified by named installed contract;no fabricated public alias"}))}}}}
 if checked==0{return Err("no_same-version_native_fields_compared".into())}Ok(json!({"checked":checked,"differences":differences,"verifier_only":verifier_only}))}
fn membership(source:&Value,objects:&Value,players:&Value,grids:&Value)->Result<Value>{let kind=number(&source["root"]["range_kind"]).ok_or("snapshot_range_kind")?;let mut wanted=BTreeMap::<String,Value>::new();let mut actual=BTreeMap::<String,Value>::new();
 let complete=if kind==6{&source["root"]}else{source["closure"].as_array().ok_or("snapshot_closure")?.iter().find(|r|number(&r["range_kind"])==Some(3)).ok_or("snapshot_complete_structure")?};
 for m in complete["payload"]["members"].as_array().ok_or("snapshot_members")?{if number(&m["key"]["space"])==Some(7){continue}wanted.insert(key(&m["key"]),m["shape"].clone());}
 for r in objects.as_array().into_iter().flatten().chain(players.as_array().into_iter().flatten()).chain(grids.as_array().into_iter().flatten()){
  let k=key(r);if kind==6&&!wanted.contains_key(&k){continue}let shape=r.get("shape").cloned().unwrap_or(Value::Null);if let Some(old)=actual.insert(k.clone(),shape.clone()){if !old.is_null()&&!shape.is_null()&&old!=shape{return Err(format!("native_key_alias_or_shape:{k}"))}}
 }
 let missing=wanted.keys().filter(|k|!actual.contains_key(*k)).cloned().collect::<Vec<_>>();let extra=actual.keys().filter(|k|!wanted.contains_key(*k)).cloned().collect::<Vec<_>>();let shapes=wanted.iter().filter(|(k,v)|actual.get(*k).is_some_and(|a|!a.is_null()&&number(a)!=number(v))).map(|(k,_)|k.clone()).collect::<Vec<_>>();
 Ok(json!({"expected":wanted.len(),"actual":actual.len(),"missing":missing,"unexpected":extra,"shape_differences":shapes,"pass":missing.is_empty()&&extra.is_empty()&&shapes.is_empty()}))}
fn walk(path:&str,a:&Value,b:&Value,out:&mut Vec<Value>){if out.len()>=256{return}match(a,b){
 (Value::Object(x),Value::Object(y))=>{for(k,v)in x{if let Some(w)=y.get(k){walk(&format!("{path}/{k}"),v,w,out)}else{out.push(json!({"path":format!("{path}/{k}"),"reason":"missing-native-or-DTO-member"}))}}for k in y.keys(){if !x.contains_key(k){out.push(json!({"path":format!("{path}/{k}"),"reason":"unexpected-member"}))}}},
 (Value::Array(x),Value::Array(y))=>{if x.len()!=y.len(){out.push(json!({"path":path,"reason":"array-count","source":x.len(),"actual":y.len()}))}for(i,(v,w))in x.iter().zip(y).enumerate(){walk(&format!("{path}/{i}"),v,w,out)}},
 _=>if a!=b{out.push(json!({"path":path,"source":a,"actual":b,"reason":"exact-value-difference"}))}
}}
pub fn exact(source:&Value,actual:&Value)->Vec<Value>{let mut out=Vec::new();walk("",source,actual,&mut out);out}
fn lines(s:&str)->Result<BTreeMap<&str,&str>>{let mut out=BTreeMap::new();for line in s.lines(){if let Some((k,v))=line.split_once('='){if out.insert(k,v).is_some(){return Err(format!("inspect_duplicate_key:{k}"))}}}Ok(out)}
// 这里的DTO只用于证明双方对应同一版本，真实native由独立回读及具名完整后态检查产生。
pub fn snapshot(source:&Value,client:&Value,render:&Value,visual_reviewed:bool)->Result<Value>{
 if source["role"]!="source"||client["role"]!="native"||source["world"]!=client["world"]{return Err("snapshot_identity_or_role".into())}
 let differences=exact(&source["root"],&client["root"]);let dependencies=exact(&source["closure"],&client["closure"]);
 let native=lines(client["native"].as_str().ok_or("native_inspect_missing")?)?;
 let checked=native.get("ok")==Some(&"1")&&native.get("native_checked")==Some(&"1")&&native.get("player_native_checked")==Some(&"1")&&native.get("grid_native_checked")==Some(&"1");
 let mut native_missing=Vec::new();for key in ["objects","players","grids"]{if !native.contains_key(key){native_missing.push(key)}}
 let objects:Value=serde_json::from_str(native.get("objects").copied().unwrap_or("null")).map_err(|e|e.to_string())?;
 let players:Value=serde_json::from_str(native.get("players").copied().unwrap_or("null")).map_err(|e|e.to_string())?;
 let grids:Value=serde_json::from_str(native.get("grids").copied().unwrap_or("null")).map_err(|e|e.to_string())?;
 let field_comparison=actual_fields(source,&objects,&players)?;
 let members=membership(source,&objects,&players,&grids)?;
 let rendered=render["connection"]==client["connection"]&&render["route"]==client["route"]&&render["world"]==client["world"]&&render["root_revision"]==client["root"]["revision"]&&render["actual_render"]==true;
 Ok(json!({"pass":differences.is_empty()&&dependencies.is_empty()&&checked&&native_missing.is_empty()&&rendered&&visual_reviewed&&members["pass"]==true&&field_comparison["differences"].as_array().is_some_and(Vec::is_empty),"differences":differences,"dependency_differences":dependencies,"native_fields":field_comparison,"membership":members,"native_checked":checked,"native_missing":native_missing,"actual_objects":objects,"actual_players":players,"actual_grids":grids,"rendered_exact_version":rendered,"visual_reviewed":visual_reviewed,"domain_semantics":"retained is not applied;native verifier covers installed full contract;raw fields remain explicit"}))
}
