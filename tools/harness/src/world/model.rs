//! 严格纯值文件模型；新旧格式附加列由解码器按域约束，绝不访问游戏。
use serde::{Deserialize, Serialize};
use serde_json::Value;
// 同录制内的引用身份，不能直接充当跨进程匹配 key。
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct LocalTarget {
    pub local_id: String,
    pub use_generation: String,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Reference {
    pub status: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub target: Option<LocalTarget>,
}
// 单字段保留原始位与可用状态；浮点归零只发生在 compare 的临时值中。
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Field {
    pub field_id: String,
    pub status: String,
    pub encoding: String,
    pub width: u32,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub raw32: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub raw64: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub bytes_le: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub reference: Option<Reference>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub diagnostics: Option<Value>,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Birth {
    pub kind: u32,
    pub entity_type: u32,
    pub variant: u32,
    pub subtype: u32,
    pub seed: u32,
}
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq, PartialOrd, Ord)]
#[serde(deny_unknown_fields)]
pub struct Membership {
    pub source: u32,
    pub index: u32,
}
// 一个本地对象/use 的纯值快照，诊断地址与跨进程 key 完全分离。
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Object {
    pub local_id: String,
    pub use_generation: String,
    pub revision: String,
    pub object_domain: u32,
    pub class: String,
    pub origin: String,
    pub lifecycle: u32,
    pub init_evidence: u32,
    pub birth_event: Option<String>,
    pub birth: Birth,
    pub ordinal: Option<u32>,
    #[serde(default)]
    pub grid_index: Option<u32>,
    pub fields: Vec<Field>,
    pub memberships: Vec<Membership>,
    pub diagnostics: Value,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Source {
    pub source: u32,
    pub status: String,
    pub count: u32,
    pub diagnostics: Value,
}
// capture 与无槽 gap 共用头部；新版专用列不能在 legacy 模式中出现。
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Capture {
    pub record: String,
    pub domain: String,
    pub version: u32,
    pub profile: String,
    pub session: String,
    pub run_epoch: String,
    pub room_epoch: String,
    pub mutation_epoch: String,
    pub attempt_seq: String,
    pub capture_seq: Option<String>,
    pub outer_token: String,
    pub phase: String,
    pub authority_tick: Option<String>,
    pub authority_kind: u32,
    pub scope_token: String,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub recording_generation: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub event_watermark: Option<String>,
    pub complete: bool,
    pub reason: String,
    pub sources: Vec<Source>,
    pub objects: Vec<Object>,
    pub required_objects: u32,
    pub required_memberships: u32,
    pub diagnostics: Value,
}
