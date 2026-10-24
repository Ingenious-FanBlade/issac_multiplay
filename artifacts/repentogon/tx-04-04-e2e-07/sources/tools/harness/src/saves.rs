//! 存档只读快照与逐字节对比；覆盖 Documents 和全部 Steam 云存储账户。
use crate::{Result, copy_tree, files, mkdir, win32};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    fs,
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

// 仅精确允许方案中的第二槽文件；不能用宽泛后缀误放行其他文件。
fn slot_two(base: &str) -> bool {
    matches!(
        base,
        "persistentgamedata2.dat"
            | "gamestate2.dat"
            | "rgon_persistentgamedata2.dat"
            | "rgon_steam_persistentgamedata2.dat"
            | "rep+persistentgamedata2.dat"
            | "rep+gamestate2.dat"
    )
}
fn allowed_change(name: &str, added: bool) -> bool {
    let base = Path::new(name)
        .file_name()
        .unwrap()
        .to_string_lossy()
        .to_lowercase();
    if name.starts_with("documents/save_backups/") {
        // 每日备份可新增任意槽；已有备份只允许第二槽改变。
        return added || base.split_once('.').is_some_and(|(_, tail)| slot_two(tail));
    }
    if slot_two(&base) || base == "rgon_savesyncstatus.json" || base == "savesyncstatus.json" {
        return true;
    }
    name.starts_with("documents/")
        && (base == "log.txt"
            || base == "cmd_history.txt"
            || base.starts_with("inputconfigs")
            || base.ends_with(".ini"))
}

// 直接比较内容，不计算摘要；附带时间变化，按每次启动独立归因。
pub fn changes(before: &Path, after: &Path) -> Result<Value> {
    let mut a = files(before)?;
    let mut b = files(after)?;
    let am: Value = serde_json::from_slice(
        &a.remove("snapshot.json")
            .ok_or("missing before snapshot metadata")?,
    )
    .map_err(|e| e.to_string())?;
    let bm: Value = serde_json::from_slice(
        &b.remove("snapshot.json")
            .ok_or("missing after snapshot metadata")?,
    )
    .map_err(|e| e.to_string())?;
    let names: std::collections::BTreeSet<_> = a.keys().chain(b.keys()).collect();
    let mut changed = vec![];
    for name in names {
        let bytes_changed = a.get(name) != b.get(name);
        let before_state = &am["files"][name];
        let after_state = &bm["files"][name];
        if bytes_changed || before_state != after_state {
            changed.push(json!({"path":name,"bytes_changed":bytes_changed,
                "allowed":!bytes_changed || allowed_change(name,!a.contains_key(name)),
                "before":before_state,"after":after_state,
                "possible_source":if bytes_changed {"game or REPENTOGON save synchronization; attribution UNKNOWN"}
                    else {"timestamp only; identical bytes"}}));
        }
    }
    Ok(json!(changed))
}
