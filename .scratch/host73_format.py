import json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'mod/binding'
for filename,key in [('hooks.json','hooks'),('v1_9_7_12-static.json','functions')]:
    path=root/filename;value=json.loads(path.read_text(encoding='utf-8'));text=json.dumps(value,ensure_ascii=False,indent=2)
    rows='  "'+key+'": [\n'+',\n'.join('    '+json.dumps(row,ensure_ascii=False,separators=(',',':')) for row in value[key])+'\n  ]'
    text=re.sub(r'  "'+key+r'": \[\n.*?\n  \]',lambda _:rows,text,count=1,flags=re.S)
    path.write_text(text+'\n',encoding='utf-8')
