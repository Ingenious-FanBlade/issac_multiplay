"""临时诊断报告：解析同一构建PDB的采样候选，栈扫描不当作可靠展开。"""
import json,sys,subprocess,collections
from pathlib import Path
profile=json.loads(Path(sys.argv[1]).read_text(encoding='utf-8'))
addresses={a for chain,n in profile['chains'] for a in chain if int(a,16)<0x1500000}
addresses.update(a for a,n in profile['first_dll_stack_candidates'] if int(a,16)<0x1500000)
lines=subprocess.check_output([sys.executable,'.scratch/step15_symbols.py',*sorted(addresses)],text=True,encoding='utf-8').splitlines()
names={line.split(' ',1)[0]:line.split(' ',1)[1].rsplit(' ',1)[0] for line in lines}
print('First candidates:')
for a,n in profile['first_dll_stack_candidates']:
    if a in names:print(n,names[a])
chains=collections.Counter()
for chain,n in profile['chains']:
    names_chain=tuple(dict.fromkeys(names[a] for a in chain if a in names))
    if names_chain:chains[names_chain]+=n
print('Candidate chains (not unwound):')
for chain,n in chains.most_common(35):print(n,' -> '.join(chain))
