//! v2 域读取器：逐条保留 scope／schema／完整性，未知域不能无声计为完整。
use crate::Result;
use serde_json::{Value, json};
use std::{collections::{BTreeMap, BTreeSet}, fs, io::{BufRead, BufReader}, path::Path};

const SCHEMAS: &[&str] = &[
    "isaac.cs.entity", "isaac.cs.entity.native-v1", "isaac.cs.entity.lifecycle",
    "isaac.cs.entity.lua-event", "isaac.cs.entity.query", "isaac.cs.entity.actual-call", "isaac.cs.entity.render", "isaac.cs.entity.coverage",
    "isaac.cs.grid.legacy", "isaac.cs.grid.lifecycle", "isaac.cs.grid", "isaac.cs.grid.lua-event", "isaac.cs.grid.query", "isaac.cs.grid.render",
];
// 读取不修补原记录；Partial／Gap 和未实现域仍在结果中，目录统计不能代替运行覆盖。
pub(crate) fn read_capture(root: &Path) -> Result<Value> {
    let manifest: Value = serde_json::from_str(&crate::read(&root.join("manifest.json"))?).map_err(|e|e.to_string())?;
    if manifest["format"] != "isaac.cs.observation" || manifest["version"] != 2 {
        return Err("unsupported observation format/version".into());
    }
    let file=fs::File::open(root.join("records.jsonl")).map_err(|e|e.to_string())?;
    let mut schemas=BTreeMap::<String,u64>::new();
    let mut coverage=BTreeMap::<String,u64>::new();
    let mut unsupported=BTreeSet::<String>::new();
    let mut subjects=BTreeSet::<String>::new();
    let mut classes=BTreeSet::<String>::new();
    let mut scopes=BTreeSet::<u64>::new();
    let mut entity_complete=0u64;
    let mut grid_complete=0u64; let mut grid_classes=BTreeSet::<String>::new();
    let mut records=0u64;
    for (line,text) in BufReader::new(file).lines().enumerate() {
        let text=text.map_err(|e|e.to_string())?;
        if text.len()>20*1024*1024 { return Err(format!("observation line {} exceeds budget",line+1)); }
        let row:Value=serde_json::from_str(&text).map_err(|e|format!("observation line {}: {e}",line+1))?;
        if row["format"]!="isaac.cs.observation"||row["version"]!=2 { return Err("mixed observation format".into()); }
        let schema=row["schema"].as_str().ok_or("missing domain schema")?;
        let status=row["coverage"]["status"].as_str().ok_or("missing coverage")?;
        let scope=row["context"]["scope"].as_str().ok_or("missing scope")?.parse::<u64>().map_err(|e|e.to_string())?;
        if !scopes.insert(scope) { return Err(format!("duplicate scope {scope}")); }
        if row["context"]["epoch"]!=manifest["observation_epoch"]||row["context"]["session"]!=manifest["session"] {
            return Err("cross-epoch/session record".into());
        }
        if row["context"]["phase"]=="OrdinaryPost"&&row["context"]["authority_tick"].is_null() {
            return Err("ordinary scope without committed authority tick".into());
        }
        if !["Complete","Partial","Failed","NotApplicable","NotScheduled","Disabled","Unsupported"].contains(&status) { return Err("unknown coverage status".into()); }
        if !SCHEMAS.contains(&schema)||row["schema_version"]!=2 { unsupported.insert(schema.into()); }
        *schemas.entry(schema.into()).or_default()+=1; *coverage.entry(status.into()).or_default()+=1;
        if schema=="isaac.cs.entity" {
            if status=="Complete" { entity_complete+=1; }
            if let Some(objects)=row["payload"]["objects"].as_array() {
                for object in objects {
                    if let Some(class)=object["class"].as_str() { classes.insert(class.into()); }
                    if let Some(local)=object["subject"]["local_id"].as_str() { subjects.insert(local.into()); }
                }
            }
        }
        if schema=="isaac.cs.grid" {
            if status=="Complete" { grid_complete+=1; }
            if let Some(objects)=row["payload"]["objects"].as_array() { for object in objects {
                if let Some(bodies)=object["bodies"].as_object() { for class in bodies.keys() { grid_classes.insert(class.clone()); } }
            }}
        }
        records+=1;
    }
    let result:Value=serde_json::from_str(&crate::read(&root.join("result.json"))?).map_err(|e|e.to_string())?;
    let delivery=result["closed"]==true&&result["complete_delivery"]==true&&result["records"].as_u64()==Some(records);
    Ok(json!({"format":"isaac.cs.observation.read-result","version":2,"records":records,
        "schemas":schemas,"coverage":coverage,"unsupported_schemas":unsupported,"supported_read_complete":unsupported.is_empty(),
        "delivery_complete":delivery,"entity_complete_scopes":entity_complete,"entity_classes":classes,"grid_complete_scopes":grid_complete,"grid_classes":grid_classes,"subjects":subjects.len(),
        "declared_unsupported_domains":manifest["unsupported_domains"],"world_complete":false}))
}
pub(crate) fn run_cli(args: &[String]) -> Result<()> {
    if args.len()!=3||args[1]!="--capture" { return Err("usage: harness observation-read --capture <observation-v2-directory>".into()); }
    let result=read_capture(Path::new(&args[2]))?;
    println!("{}",serde_json::to_string_pretty(&result).map_err(|e|e.to_string())?);
    if result["supported_read_complete"]!=true||result["delivery_complete"]!=true { return Err("observation read is incomplete; see reported scope/schema/delivery status".into()); }
    Ok(())
}
