"""生成控制信封编码使用的固定名称表。"""
import json
import sys
from pathlib import Path

source, target = map(Path, sys.argv[1:])
names = json.loads(source.read_text(encoding="utf-8"))
rows = [(index * 4, name) for index, name in enumerate(names, 1)]
target.mkdir(parents=True, exist_ok=True)
for order, items in (("text", sorted(rows, key=lambda row: row[1])), ("code", rows)):
    output = "".join(f"{{{code}, {json.dumps(name)}}},\n" for code, name in items)
    (target / f"stream_names_by_{order}.inc").write_text(output, encoding="utf-8")
