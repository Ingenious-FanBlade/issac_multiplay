//! 存档只读快照与逐字节对比；覆盖 Documents 和全部 Steam 云存储账户。
use crate::{Result, copy_tree, mkdir, win32};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    fs,
    io::Read,
    path::{Path, PathBuf},
};

// 运行前冻结来源目录；两次启动和失败收尾使用同一份目录集合。
pub struct SaveRoots {
    pub documents: PathBuf,
    pub steam_remote: Vec<(String, PathBuf)>,
}
impl SaveRoots {
    pub fn discover(documents: PathBuf) -> Result<Self> {
        let userdata = win32::steam_path()?.join("userdata");
        let mut steam_remote = Vec::new();
        for entry in fs::read_dir(&userdata).map_err(|e| format!("{}: {e}", userdata.display()))? {
            let entry = entry.map_err(|e| e.to_string())?;
            let account = entry.file_name().to_string_lossy().to_string();
            if !account.is_empty() && account.bytes().all(|b| b.is_ascii_digit()) {
                let remote = entry.path().join("250900/remote");
                if remote.is_dir() {
                    steam_remote.push((account, win32::ordinary_path(&remote)?));
                }
            }
        }
        steam_remote.sort_by(|a, b| a.0.cmp(&b.0));
        Ok(Self {
            documents,
            steam_remote,
        })
    }
    pub fn describe(&self) -> Value {
        json!({"documents":self.documents,"steam_remote":self.steam_remote.iter()
            .map(|(account,path)|json!({"account":account,"path":path})).collect::<Vec<_>>()})
    }
    // 同一快照保存源路径、大小与修改时间；时间变化不代表字节变化。
    pub fn snapshot(&self, dest: &Path) -> Result<()> {
        copy_tree(&self.documents, &dest.join("documents"))?;
        mkdir(&dest.join("steam_remote"))?;
        for (account, remote) in &self.steam_remote {
            copy_tree(remote, &dest.join("steam_remote").join(account))?;
        }
        let mut metadata = BTreeMap::new();
        fn walk(
            root: &Path,
            path: &Path,
            prefix: &str,
            out: &mut BTreeMap<String, Value>,
        ) -> Result<()> {
            for entry in fs::read_dir(path).map_err(|e| e.to_string())? {
                let entry = entry.map_err(|e| e.to_string())?;
                if entry.file_type().map_err(|e| e.to_string())?.is_dir() {
                    walk(root, &entry.path(), prefix, out)?;
                } else {
                    let name = entry
                        .path()
                        .strip_prefix(root)
                        .unwrap()
                        .to_string_lossy()
                        .replace('\\', "/");
                    out.insert(format!("{prefix}/{name}"), win32::file_state(&entry.path()));
                }
            }
            Ok(())
        }
        walk(&self.documents, &self.documents, "documents", &mut metadata)?;
        for (account, remote) in &self.steam_remote {
            walk(
                remote,
                remote,
                &format!("steam_remote/{account}"),
                &mut metadata,
            )?;
        }
        crate::write_json(
            &dest.join("snapshot.json"),
            &json!({"roots":self.describe(),"files":metadata}),
        )
    }
}

// 槽位主存档文件名（本地与云端两种模式），依据 REPENTOGON SaveSyncing.cpp:52-71。
// 仅精确匹配这些名字，不能用宽泛后缀误放行其他文件。
fn slot_file(base: &str, slot: u8) -> bool {
    [
        format!("persistentgamedata{slot}.dat"),
        format!("gamestate{slot}.dat"),
        format!("rgon_persistentgamedata{slot}.dat"),
        format!("rgon_steam_persistentgamedata{slot}.dat"),
        format!("rep+persistentgamedata{slot}.dat"),
        format!("rep+gamestate{slot}.dat"),
    ]
    .iter()
    .any(|name| name == base)
}
fn slot_two(base: &str) -> bool {
    slot_file(base, 2)
}
// REPENTOGON 按槽保存的游戏状态附属文件，位于存档目录的 Repentogon 子目录，
// 例如 EntitySaveStateManagement/remote_gamestate2.dat、ex_remote_gamestate2.json。
fn repentogon_sidecar(name: &str, slot: u8) -> bool {
    if !name.starts_with("documents/Repentogon/") {
        return false;
    }
    let base = Path::new(name)
        .file_name()
        .unwrap()
        .to_string_lossy()
        .to_lowercase();
    let stem = base.strip_prefix("ex_").unwrap_or(&base);
    let stem = stem.strip_prefix("remote_").unwrap_or(stem);
    stem == format!("gamestate{slot}.dat") || stem == format!("gamestate{slot}.json")
}
// 第一、三槽的文件：主存档与附属文件（D18/D20 基线对比只作用于这些文件）。
pub fn other_slot(name: &str) -> bool {
    let base = Path::new(name)
        .file_name()
        .unwrap()
        .to_string_lossy()
        .to_lowercase();
    [1u8, 3].iter().any(|&slot| slot_file(&base, slot) || repentogon_sidecar(name, slot))
}
// 每日备份目录；D27 起由 changes 整体排除，不进入白名单或基线判定。
fn is_daily_backup(name: &str) -> bool {
    name.starts_with("documents/save_backups/")
}
// 活动日志和D27排除的每日备份仍完整复制；其并发写入不等于存档快照不稳定。
// 其余路径一律要求稳定，不扩展存档变化白名单，也不把父调用返回当文件写入成功。
pub fn requires_stable_snapshot(name: &str) -> bool {
    name != "documents/log.txt" && !is_daily_backup(name)
}
fn allowed_change(name: &str) -> bool {
    let base = Path::new(name)
        .file_name()
        .unwrap()
        .to_string_lossy()
        .to_lowercase();
    if slot_two(&base) || base == "rgon_savesyncstatus.json" || base == "savesyncstatus.json" {
        return true;
    }
    // D18：REPENTOGON 第二槽的游戏状态附属文件属于第二个存档。
    if repentogon_sidecar(name, 2) {
        return true;
    }
    name.starts_with("documents/")
        && (base == "log.txt"
            || base == "cmd_history.txt"
            || base.starts_with("inputconfigs")
            || base.ends_with(".ini"))
}

// D18 基线对比：第一、三槽文件在关闭配置那次启动中同样出现字节变化时，
// 归为 REPENTOGON 自身行为（那次启动本 DLL 不注册任何 Hook）；只在启用启动中变化的仍判失败。
// disabled_ran 为假（关闭配置启动没有执行）时不做任何放行，保持严格判定。
pub fn apply_slot_baseline(intervals: &mut [Value], disabled_ran: bool) {
    if !disabled_ran {
        return;
    }
    let disabled_changed: std::collections::BTreeSet<String> = intervals
        .iter()
        .filter(|i| i["launch"] == "disabled_or_cleanup")
        .flat_map(|i| i["changes"].as_array().cloned().unwrap_or_default())
        .filter(|c| c["bytes_changed"] == true)
        .filter_map(|c| c["path"].as_str().map(str::to_string))
        .filter(|p| other_slot(p))
        .collect();
    for interval in intervals.iter_mut() {
        let label = interval["launch"].as_str().unwrap_or("").to_string();
        let Some(changes) = interval["changes"].as_array_mut() else {
            continue;
        };
        for change in changes.iter_mut() {
            let path = change["path"].as_str().unwrap_or("").to_string();
            if change["allowed"] == true || change["bytes_changed"] != true || !other_slot(&path) {
                continue;
            }
            if label == "disabled_or_cleanup" {
                change["allowed"] = json!(true);
                change["baseline"] = json!("disabled launch; no hooks registered");
            } else if label == "enabled" && disabled_changed.contains(&path) {
                change["allowed"] = json!(true);
                change["baseline"] = json!("also changed in disabled launch");
            } else {
                change["baseline"] = json!("changed only in enabled launch");
            }
        }
    }
}

// 直接比较内容，不计算摘要；附带时间变化，按每次启动独立归因。
// D27（用户决定）：判定不看每日备份 save_backups 中的任何存档，快照仍原样复制作原始证据。
pub fn changes(before: &Path, after: &Path) -> Result<Value> {
    // 失败情形：单边缺失、长度不同、任意字节不同、读取失败或符号链接；空文件相等。
    // 快照可能含数百MiB历史转储，i686不能把两棵树的全部内容同时载入内存。
    fn names(root:&Path,path:&Path,out:&mut std::collections::BTreeSet<String>)->Result<()> {
        for entry in fs::read_dir(path).map_err(|e|e.to_string())? {
            let entry=entry.map_err(|e|e.to_string())?;let kind=entry.file_type().map_err(|e|e.to_string())?;
            if kind.is_symlink(){return Err("symlink in snapshot".into());}
            if kind.is_dir(){names(root,&entry.path(),out)?;}else{
                let name=entry.path().strip_prefix(root).unwrap().to_string_lossy().replace('\\',"/");
                if name!="snapshot.json"{out.insert(name);}
            }
        } Ok(())
    }
    fn equal(a:&Path,b:&Path)->Result<bool> {
        if !a.exists()||!b.exists(){return Ok(false);}
        let mut a=fs::File::open(a).map_err(|e|e.to_string())?;let mut b=fs::File::open(b).map_err(|e|e.to_string())?;
        let mut remaining=a.metadata().map_err(|e|e.to_string())?.len();if remaining!=b.metadata().map_err(|e|e.to_string())?.len(){return Ok(false);}
        let mut left=[0u8;65536];let mut right=[0u8;65536];
        while remaining>0 {let bytes=remaining.min(left.len() as u64) as usize;
            a.read_exact(&mut left[..bytes]).map_err(|e|e.to_string())?;b.read_exact(&mut right[..bytes]).map_err(|e|e.to_string())?;
            if left[..bytes]!=right[..bytes]{return Ok(false);}remaining-=bytes as u64;
        } Ok(true)
    }
    let am:Value=serde_json::from_slice(&fs::read(before.join("snapshot.json")).map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;
    let bm:Value=serde_json::from_slice(&fs::read(after.join("snapshot.json")).map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;
    let mut a_names=std::collections::BTreeSet::new();let mut b_names=std::collections::BTreeSet::new();names(before,before,&mut a_names)?;names(after,after,&mut b_names)?;
    let names_set:std::collections::BTreeSet<_>=a_names.union(&b_names).cloned().collect();
    let mut changed = vec![];
    for name in &names_set {
        if is_daily_backup(name) {
            continue;
        }
        let bytes_changed = !a_names.contains(name)||!b_names.contains(name)||!equal(&before.join(name),&after.join(name))?;
        let before_state = &am["files"][name];
        let after_state = &bm["files"][name];
        if bytes_changed || before_state != after_state {
            changed.push(json!({"path":name,"bytes_changed":bytes_changed,
                "allowed":!bytes_changed || allowed_change(name),
                "before":before_state,"after":after_state,
                "possible_source":if bytes_changed {"game or REPENTOGON save synchronization; attribution UNKNOWN"}
                    else {"timestamp only; identical bytes"}}));
        }
    }
    Ok(json!(changed))
}
