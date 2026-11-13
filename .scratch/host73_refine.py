import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'mod/binding/state'
def read(n):return json.loads((root/n).read_text(encoding='utf-8'))
def write(n,x):(root/n).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
level=read('level.json')
def walk(x):
    if isinstance(x,dict):
        if x.get('type')=='RoomDescriptor' and 'ranges' in x:
            seed=next(r for r in x['ranges'] if r['name']=='spawn_seed')
            seed['wire']=dict(kind='convert',type='u32',host='room_descriptor_spawn_seed_read',client='room_descriptor_spawn_seed_install',when=dict(field='spawn_seed_current',predicate='room_descriptor_is_current',test=dict(kind='equal',value=1)))
            x.setdefault('synthetic_fields',[])
            if not any(r['name']=='spawn_seed_current' for r in x['synthetic_fields']):x['synthetic_fields'].append(dict(name='spawn_seed_current',type='u8',semantics='仅当前描述为1；其它描述不读取／不发送seed',evidence='Q9仅同步当前房间描述的SpawnSeed',host='room_descriptor_current_read',client='room_descriptor_current_install'))
            for b in x['blocks']:
                if 'spawn_seed' in b['fields'] and 'spawn_seed_current' not in b['fields']:b['fields'].insert(b['fields'].index('spawn_seed'),'spawn_seed_current')
            x['conversions'].update(room_descriptor_current_read='同world比较已核描述owner与当前Room+4，输出0／1，不读取非当前seed',room_descriptor_current_install='只作当前seed的presence选择器，不能用它切房或选路',room_descriptor_spawn_seed_read='仅current选择器为真时读取当前描述+5C原值；按变化同步，不读取其它房间seed',room_descriptor_spawn_seed_install='仅随当前Room事务安装非零Host SpawnSeed，不写其它描述的seed')
            x['validators']['spawn_seed_current']=dict(kind='range',scalar='u8',min=0,max=1)
            x['conversions']['room_descriptor_is_current']='spawn_seed_current == 1；只允许当前描述的seed负载'
        for v in x.values():walk(v)
    elif isinstance(x,list):
        for v in x:walk(v)
walk(level);write('level.json',level)
display=read('display.json');wire=read('wire.json')
headers=[t for t in display['types'] if t['type'] in ('StateEventQueueHeader','StateEventHeader')]
display['types']=[t for t in display['types'] if t not in headers]
for t in headers:
    if not any(x['type']==t['type'] for x in wire['types']):wire['types'].append(t)
injection=next(x for x in display['types'] if x['type']=='DisplayInjectionState')
if not any(f['name']=='map_focus_large' for f in injection['synthetic_fields']):
    original=next(f for f in injection['synthetic_fields'] if f['name']=='map_focus')
    extra=dict(original);extra['name']='map_focus_large';extra['semantics']='挑战32大地图的实际SafeGridIndex；分别观察，不假定与小地图相同'
    injection['synthetic_fields'].append(extra);injection['blocks'][0]['fields'].append('map_focus_large')
for key in ('map_focus','map_focus_large'):injection['validators'][key]=dict(kind='range',scalar='i32',min=-1,max=169)
write('display.json',display);write('wire.json',wire)
