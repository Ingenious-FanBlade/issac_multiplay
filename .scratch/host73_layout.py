import json
from pathlib import Path

root=Path(__file__).resolve().parents[1]/'mod/binding/state'
def read(name): return json.loads((root/name).read_text(encoding='utf-8'))
def write(name,value): (root/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

run=read('run.json')
game=next(x for x in run['types'] if x['type']=='GameRun')
companions=[('display_injections','DisplayInjectionState'),('music_request','MusicRequestState'),('loop_sounds','LoopSoundSet'),('item_overlays','OverlayDisplaySet'),('procedural_display','ProceduralDisplaySet')]
game.setdefault('companions',[])
for name,typ in companions:
    if not any(x['name']==name for x in game['companions']):
        game['companions'].append(dict(name=name,type=typ,semantics='版本73 Host同world显示需求；真实来源失败不发布',evidence='client-display-delegation 6.3／6.4',status='已核',host='display_value_read',client='display_value_install'))
    if not any(name in b['fields'] for b in game['blocks']):
        game['blocks'].append(dict(id=len(game['blocks']),name='版本73 '+name,sequence='world',fields=[name]))
for item in game['companions']: item.setdefault('status','已核')
game['conversions']['display_value_read']='Host正常来源封存的同world自有值，逐字段按生成索引读取；不在后台读取原生对象，不从旧显示缓存补值'
game['conversions']['display_value_install']='world、Player库存与资源成功安装后交具名显示／注入owner；不运行玩法Update，不写PGD'
write('run.json',run)
demand=read('demand.json')
row=next(x for x in demand['fields'] if x['type']=='GameRun')
for name,_ in companions: row['fields'][name]='第二批6.3／6.4显示需求；同world事务，不恢复显示结果批次'
for name in ('display.json','overlay-display.json'):
    data=read(name)
    for typ in data['types']:
        if not any(x['type']==typ['type'] for x in demand['all']+demand['fields']):
            demand['all'].append(dict(type=typ['type'],reason='版本73事件队列／表现状态／注入值；RE-7／RE-8／RE-9实际来源'))
        for f in typ.get('synthetic_fields',[]):
            scalar=f['type'].split('[')[0]
            if scalar=='f32': typ['validators'][f['name']]=dict(kind='finite',scalar='f32')
        if typ['type']=='DisplayInjectionState':
            for key in ('deal_qualified','progress_blocked','pill_identified'):typ['validators'][key]=dict(kind='range',scalar='u8',min=0,max=1)
            typ['validators']['angel_share']=dict(kind='range',scalar='f32',min=0,max=1)
        if typ['type']=='OverlayDisplayState':typ['validators']['state']=dict(kind='range',scalar='i32',min=0,max=2)
    write(name,data)
write('demand.json',demand)
