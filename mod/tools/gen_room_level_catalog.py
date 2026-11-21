"""tx-04-12 本地 Wiki 转为房间/楼层完整目录与只读 schema；不运行游戏。"""
from pathlib import Path
import re,json
from gen_entity_catalog import ROOT,WIKIS,clean,lua
CLASSES={'Room':'room','RoomDescriptor':'level','Level':'level','RoomConfig_Room':'room','RoomConfigRoom':'room','RoomConfig_Spawn':'room','RoomConfig_Entry':'room','RoomConfigStage':'room','RoomConfig':'room','RoomTransition':'room','Backdrop':'room','FXParams':'room','FXLayers':'room','ColorModifier':'room','LRoomAreaDesc':'room','LRoomTileDesc':'room','RailManager':'room','Camera':'room'}
ALIASES={'RoomConfig_Room':'RoomConfigRoom','RoomConfig_Spawn':'RoomConfigSpawn','RoomConfig_Entry':'RoomConfigEntry'}
UNSAFE={'GetRandomPosition','GetRandomTileIndex','GetSeededCollectible','GetItemPool','GetRandomRoomIndex','QueryRoomTypeIndex','GetRandomRoom','GetRandomTile','ForceHorsemanBoss','GetRoomSet'}
EXTERNAL={'GetDecoSaveState':'room_save:tx-04-13','GetEntitiesSaveState':'room_save:tx-04-13','GetGridEntitiesSaveState':'room_save:tx-04-13','GetMyosotisPickups':'room_save:tx-04-13','GetDevilRoomChance':'run:tx-04-14','GetAngelRoomChance':'run:tx-04-14','GetPlanetariumChance':'run:tx-04-14'}
CUSTOM={'GetEntities','GetGridEntity','GetGridEntityFromPos','GetDoor','GetRooms','GetCurrentRoom','GetCurrentRoomDesc','GetLastRoomDesc','GetRoomByIdx','GetNeighboringRooms','Doors'}
def main():
    entries={}
    for cls,domain in CLASSES.items():
        for wiki,base in WIKIS:
            p=base/(cls+'.md')
            if not p.exists():continue
            for no,line in enumerate(p.read_text(encoding='utf-8-sig').splitlines(),1):
                if not line.startswith('#### '):continue
                sig=clean(line[5:]); m=re.match(r'(.+?)\s+(\w+)\s*\((.*)\)\s*$',sig)
                if m:
                    typ,name,args=m.groups(); cat='query' if name.startswith(('Get','Is','Has','Can')) and typ!='void' else 'operation'
                    if args.strip() and cat=='query':cat='parameter_query'
                else:
                    m=re.match(r'(.+?)\s+(\w+)\s*$',sig)
                    if not m:continue
                    typ,name=m.groups();args=None;cat='field'
                reason=''
                if name.startswith('__') or (cls=='ColorModifier' and name=='ColorModifier'):cat='operation'
                if name in ('CheckLine','ScreenWrapPosition','WorldToScreenPosition','FindFreeTilePosition'):cat='derived_query'
                if name in UNSAFE:cat='actual_call';reason='random_selection_or_lazy_resource_load; never_poll'
                elif name in EXTERNAL:cat='external_reference';reason=EXTERNAL[name]
                elif name in CUSTOM:cat='domain_value';reason='explicit_owned_membership_or_subject_reference'
                if cls=='RoomConfig' and cat in ('query','parameter_query'):cat='derived_query';reason='explicit_resource_lookup'
                owner=ALIASES.get(cls,cls); typ=typ.replace('const ','').replace('static ','')
                if owner=='RoomConfigRoom' and name=='Mode':typ='int'
                key=(owner,name,sig)
                e=entries.setdefault(key,dict(id=owner+'.'+name,owner=owner,member=name,returns=typ,parameters=args,category=cat,reason=reason,static='static ' in sig or cls in ('RoomConfig','RoomTransition'),sources=[],domain=domain))
                e['sources'].append(dict(wiki=wiki,path=p.as_posix(),line=no))
    members=sorted(entries.values(),key=lambda x:(x['owner'],x['member'],x['parameters'] or ''))
    groups={}
    for e in members:
        e.update(schema='isaac.cs.'+e['domain'],schema_version=2,phase='OrdinaryPost / Transition / actual callback / explicit query',absence='Value / Null / Empty / NotApplicable / NotScheduled / Unavailable / ReadFailed / Nonfinite / Truncated',units='API原单位；时钟保留局部作用域；种子按用途命名')
        groups.setdefault(e['domain'],{}).setdefault(e['owner'],[]).append({k:e[k] for k in ['id','owner','member','returns','parameters','category','reason','static']})
    dest=ROOT/'mod/observation/isaac-cs-observation'
    for domain,g in groups.items():
        p=dest/'domains'/domain;p.mkdir(exist_ok=True)
        (p/'schema.lua').write_text('-- 自动生成；玩法动作与随机/懒加载查询不得由轮询执行。\nreturn '+lua(g)+'\n',encoding='utf-8')
    enums=[]; callbacks=[]
    for wiki,base in WIKIS:
        for path in sorted((base/'enums').glob('*.md')):
            if re.search(r'Room|Level|Stage|Stb|Door|Dimension|Backdrop|Direction|RenderMode|Linecheck',path.stem):
                enums.append(dict(wiki=wiki,path=path.as_posix(),text=path.read_text(encoding='utf-8-sig')))
        path=base/'enums/ModCallbacks.md'
        if path.exists():
            for no,line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(),1):
                if line.startswith('|') and 'MC_' in line and any(x in line for x in ['ROOM','LEVEL','AMBUSH','GREED_WAVE','CLEAN_AWARD']):callbacks.append(dict(source=path.as_posix(),line=no,signature=line))
    data=dict(enums=enums,callbacks=callbacks,format='isaac.cs.room-level.catalog',version=2,members=members,identity='run/floor/dimension/descriptor/room-instance; descriptor construction_revision is current config/seed witness, not native birth; bounded allocation; no addresses',domain_fields={'level.off_grid':'19 applicable initialized/uninitialized native off-grid slots; excludes Online -19; SDK total=527','room.descriptor.construction_revision':'decimal u64; actual Data/Override config and named-seed revision','room.doors.actual_connection':'map/off-grid/DimensionRoute/no-target; -100/-101 are not descriptor indices','room.room_index':'actual Level.GetCurrentRoomIndex; independent of GridIndex/ListIndex'},shared_resources='entity-catalog.json',unexposed=[dict(member='engine_private_environment_storage',status='Unavailable',reason='FXLayers exposes only AddPoopFx; no readonly layer/container API; active effects reference Entity domain')])
    for base in [dest,ROOT/'docs/archive/design/observation-expansion']:(base/'room-level-catalog.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    lines=['# tx-04-12 房间与楼层逐成员目录','','由两套本地Wiki生成。目录与实际E2E覆盖分别报告。操作/随机/懒加载不轮询；显式参数查询保留签名，有限网格和门槽由域遍历。资源复用Entity公共值编码。','','| 成员 | 分类 | 参数／原因 | 来源 |','| --- | --- | --- | --- |']
    for e in members:lines.append('| `'+e['id']+'` | '+e['category']+' | '+str(e['parameters'] or '')+' '+e['reason']+' | '+'<br>'.join(f"[{s['wiki']}]({s['path']}) L{s['line']}" for s in e['sources'])+' |')
    (ROOT/'mod/binding/catalog/room-level-catalog.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
if __name__=='__main__':main()
