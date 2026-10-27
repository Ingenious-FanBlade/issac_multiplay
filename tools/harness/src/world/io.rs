//! 有界文件 I/O；JSON 对象拒绝重复键，成功报告必须晚于显式 flush。
use super::Result;
use serde::{
    Deserialize, Serialize,
    de::{self, MapAccess, SeqAccess, Visitor},
};
use serde_json::{Map, Value};
use std::{
    fmt,
    fs::{File, OpenOptions},
    io::{BufRead, BufReader, BufWriter, Read, Write},
    path::Path,
};

pub const BUFFER: usize = 1024 * 1024;
pub const MAX_RECORD: usize = 128 * 1024 * 1024;

// Value 的默认反序列化会覆盖重复键；这里在所有层级拒绝重复键及越界数字。
struct Strict(Value);
impl<'de> Deserialize<'de> for Strict {
    fn deserialize<D: de::Deserializer<'de>>(d: D) -> std::result::Result<Self, D::Error> {
        struct V;
        impl<'de> Visitor<'de> for V {
            type Value = Strict;
            fn expecting(&self, f: &mut fmt::Formatter) -> fmt::Result {
                f.write_str("strict JSON")
            }
            fn visit_bool<E: de::Error>(self, v: bool) -> std::result::Result<Strict, E> {
                Ok(Strict(v.into()))
            }
            fn visit_i64<E: de::Error>(self, v: i64) -> std::result::Result<Strict, E> {
                Ok(Strict(v.into()))
            }
            fn visit_u64<E: de::Error>(self, v: u64) -> std::result::Result<Strict, E> {
                Ok(Strict(v.into()))
            }
            fn visit_f64<E: de::Error>(self, v: f64) -> std::result::Result<Strict, E> {
                serde_json::Number::from_f64(v)
                    .map(|n| Strict(Value::Number(n)))
                    .ok_or_else(|| E::custom("nonfinite JSON number"))
            }
            fn visit_str<E: de::Error>(self, v: &str) -> std::result::Result<Strict, E> {
                Ok(Strict(v.into()))
            }
            fn visit_string<E: de::Error>(self, v: String) -> std::result::Result<Strict, E> {
                Ok(Strict(v.into()))
            }
            fn visit_unit<E: de::Error>(self) -> std::result::Result<Strict, E> {
                Ok(Strict(Value::Null))
            }
            fn visit_seq<A: SeqAccess<'de>>(
                self,
                mut a: A,
            ) -> std::result::Result<Strict, A::Error> {
                let mut out = Vec::new();
                while let Some(Strict(v)) = a.next_element()? {
                    out.push(v);
                }
                Ok(Strict(Value::Array(out)))
            }
            fn visit_map<A: MapAccess<'de>>(
                self,
                mut a: A,
            ) -> std::result::Result<Strict, A::Error> {
                let mut out = Map::new();
                while let Some(k) = a.next_key::<String>()? {
                    if out.contains_key(&k) {
                        return Err(de::Error::custom(format!("duplicate JSON key {k}")));
                    }
                    let Strict(v) = a.next_value()?;
                    out.insert(k, v);
                }
                Ok(Strict(Value::Object(out)))
            }
        }
        d.deserialize_any(V)
    }
}
pub fn parse(bytes: &[u8]) -> Result<Value> {
    if bytes.len() > MAX_RECORD {
        return Err("JSON exceeds 128 MiB".into());
    }
    serde_json::from_slice::<Strict>(bytes)
        .map(|v| v.0)
        .map_err(|e| e.to_string())
}
pub fn read_json(path: &Path) -> Result<Value> {
    let f = File::open(path).map_err(|e| format!("{}: {e}", path.display()))?;
    let mut bytes = Vec::new();
    f.take(MAX_RECORD as u64 + 1)
        .read_to_end(&mut bytes)
        .map_err(|e| e.to_string())?;
    parse(&bytes).map_err(|e| format!("{}: {e}", path.display()))
}
pub fn reader(path: &Path) -> Result<BufReader<File>> {
    Ok(BufReader::with_capacity(
        BUFFER,
        File::open(path).map_err(|e| format!("{}: {e}", path.display()))?,
    ))
}
// 不用 read_until，避免超限行在拒绝前无界分配；EOF 允许最后一条合法 JSON 无换行。
pub fn read_line(input: &mut impl BufRead, out: &mut Vec<u8>) -> Result<usize> {
    out.clear();
    loop {
        let b = input.fill_buf().map_err(|e| e.to_string())?;
        if b.is_empty() {
            return Ok(out.len());
        }
        let n = b
            .iter()
            .position(|c| *c == b'\n')
            .map_or(b.len(), |i| i + 1);
        if n > MAX_RECORD - out.len() {
            return Err("JSONL record exceeds 128 MiB".into());
        }
        let done = b[n - 1] == b'\n';
        out.extend_from_slice(&b[..n]);
        input.consume(n);
        if done {
            return Ok(out.len());
        }
    }
}
// 计数只统计真正到 File 的写入；编码器的小块写入由固定 1 MiB 聚合。
pub struct Sink {
    file: File,
    pub writes: u64,
    pub bytes: u64,
}
impl Write for Sink {
    fn write(&mut self, b: &[u8]) -> std::io::Result<usize> {
        let n = self.file.write(b)?;
        self.writes += 1;
        self.bytes += n as u64;
        Ok(n)
    }
    fn flush(&mut self) -> std::io::Result<()> {
        self.file.flush()
    }
}
pub type Output = BufWriter<Sink>;
pub fn output(path: &Path) -> Result<Output> {
    let file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .map_err(|e| format!("{}: {e}", path.display()))?;
    Ok(BufWriter::with_capacity(
        BUFFER,
        Sink {
            file,
            writes: 0,
            bytes: 0,
        },
    ))
}
pub fn line(out: &mut impl Write, value: &impl Serialize) -> Result<()> {
    serde_json::to_writer(&mut *out, value).map_err(|e| e.to_string())?;
    out.write_all(b"\n").map_err(|e| e.to_string())
}
pub fn flush(out: &mut impl Write) -> Result<()> {
    out.flush().map_err(|e| e.to_string())
}
pub fn save(path: &Path, value: &impl Serialize) -> Result<()> {
    let mut out = output(path)?;
    line(&mut out, value)?;
    flush(&mut out)
}
// 有固定 schema 的对象列必须齐全且没有额外字段；diagnostics 自身是开放诊断域。
pub fn keys(value: &Value, required: &[&str], optional: &[&str]) -> Result<()> {
    let map = value.as_object().ok_or("expected JSON object")?;
    if let Some(k) = required.iter().find(|k| !map.contains_key(**k)) {
        return Err(format!("missing field {k}"));
    }
    if let Some(k) = map
        .keys()
        .find(|k| !required.contains(&k.as_str()) && !optional.contains(&k.as_str()))
    {
        return Err(format!("unknown field {k}"));
    }
    Ok(())
}
