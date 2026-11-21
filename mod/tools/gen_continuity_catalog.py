"""tx-04-13：从本地Wiki转换完整成员目录；分类与来源保留，不执行游戏或验收。"""
from pathlib import Path
import json,re
from gen_entity_catalog import ROOT,WIKIS,clean,lua

CLASSES={'Minimap':'exploration','RoomDescriptor':'room_save','Level':'exploration','EntitiesSaveState':'room_save','EntitiesSaveStateVector':'room_save','GridEntitiesSaveStateVector':'room_save','GridEntityDesc':'room_save','PlayerManager':'player','EntityPlayer':'player','History':'player','HistoryItem':'player','TemporaryEffects':'player','TemporaryEffect':'player'}
def main():
    entries={}
    for cls,domain in CLASSES.items():
        for wiki,base in WIKIS:
            path=base/(cls+'.md')
            if not path.exists():continue
            for no,line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(),1):
                if not line.startswith('#### '):continue
                sig=clean(line[5:]);m=re.match(r'(.+?)\s+(\w+)\s*\((.*)\)\s*$',sig)
                if m:
                    typ,name,args=m.groups();cat='query' if name.startswith(('Get','Is','Has','Can','Anyone','AnyPlayer','First','Search')) and typ!='void' else 'operation'
                    if args.strip() and cat=='query':cat='parameter_query'
                else:
                    m=re.match(r'(.+?)\s+(\w+)\s*$',sig)
                    if not m:continue
                    typ,name=m.groups();args=None;cat='field'
                reason=''
                if name.startswith('__'):cat='container_operator' if name=='__len' else 'operation'
                if 'Random' in name:cat='actual_call';reason='random_selection;never_poll'
                if cls in ('EntityPlayer','TemporaryEffects','TemporaryEffect') and cat not in ('operation','actual_call'):
                    cat='canonical_reference';reason='entity:isaac.cs.entity;single_body_state_owner'
                    if cls=='EntityPlayer' and name=='GetHistory':cat='domain_value';reason='player.history;current_scope'
                    if cls=='EntityPlayer' and name=='GetMovingBoxContents':cat='domain_value';reason='room_save.carried;owner_qualified'
                if cls in ('Level','RoomDescriptor') and cat not in ('operation','actual_call'):
                    cat='canonical_reference';reason='level:isaac.cs.level'
                    if name in ('GetEntitiesSaveState','GetGridEntitiesSaveState','GetDecoSaveState','GetMyosotisPickups'):cat='domain_value';reason='room_save.collections;full_membership'
                    if name in ('VisitedCount','DisplayFlags','Clear','ClearCount','Flags','GetCurses','GetStateFlag'):cat='domain_value';reason='exploration:descriptor_knowledge_and_changes;raw_source_in_level'
                if cls.endswith('SaveStateVector') and name in ('Get','GetByType'):cat='container_query';reason='full_indexed_membership;GetByType_derived_from_copied_fields'
                if cls=='PlayerManager' and name in ('GetPlayers','GetEsauJrState'):cat='domain_value';reason='qualified_native_sources_and_four_explicit_backup_slots'
                e=entries.setdefault((cls,name,sig),dict(id=cls+'.'+name,owner=cls,member=name,returns=typ.replace('const ','').replace('static ',''),parameters=args,category=cat,reason=reason,static=cls in ('Minimap','PlayerManager'),domain=domain,sources=[]))
                e['sources'].append(dict(wiki=wiki,path=path.as_posix(),line=no))
    members=sorted(entries.values(),key=lambda x:(x['owner'],x['member'],x['parameters'] or ''))
    dest=ROOT/'mod/observation/isaac-cs-observation'
    groups={d:{} for d in ('exploration','room_save','player')}
    for e in members:
        e.update(schema='isaac.cs.'+e['domain'],schema_version=2,target='REPENTOGON 1.1.3 / v1.9.7.12.J273',absence='Null / Empty / GeneratedUnvisited / Unavailable / NotCaptured / ReadFailed / Truncated',phase='OrdinaryPost; actual lifecycle/transition has independent scope',units='API原单位；保存字段I/F/B/U不猜测私有含义；History时间为原Getter值')
        if e['owner'] in ('Minimap','EntitiesSaveState','GridEntityDesc','History','HistoryItem','PlayerManager'):
            groups[e['domain']].setdefault(e['owner'],[]).append({k:e[k] for k in ('id','owner','member','returns','parameters','category','reason','static')})
    for d,g in groups.items():
        path=dest/'domains'/d;path.mkdir(parents=True,exist_ok=True)
        (path/'schema.lua').write_text('-- 自动生成；只读状态与动作分类，不轮询随机/玩法方法。\nreturn '+lua(g)+'\n',encoding='utf-8')
    enums=[];callbacks=[]
    for wiki,base in WIKIS:
        for path in sorted((base/'enums').glob('*.md')):
            if re.search(r'Minimap|RoomDescriptor|LevelStateFlag|Curse|PlayerType|ActiveSlot',path.stem):enums.append(dict(wiki=wiki,path=path.as_posix(),text=path.read_text(encoding='utf-8-sig')))
        path=base/'enums/ModCallbacks.md'
        for no,line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(),1):
            if line.startswith('|') and 'MC_' in line and any(x in line for x in ('PLAYER','NEW_ROOM','NEW_LEVEL','GAME_EXIT','GAME_END','SAVE','RESTORE','GLOWING_HOURGLASS','USE_ITEM','USE_CARD','USE_PILL')):callbacks.append(dict(source=path.as_posix(),line=no,signature=line))
    data=dict(format='isaac.cs.continuity.catalog',version=2,members=members,enums=enums,callbacks=callbacks,identity='Entity bodies reuse entity subject/use; SaveEntry is immutable collection revision, not native identity',unexposed=[dict(field='SavedFrom/RestoredFrom native per-object token',status='Unavailable',reason='public containers expose values and ordinal only; temporal scope edges remain distinct from identity'),dict(field='complete native private save/inactive player internals',status='Unavailable',reason='copy only public fields under native read qualification; no guessed restoration')],implementation=dict(exploration='descriptor knowledge delta plus Minimap local display/resource projection',room_save='all initialized descriptions; three full save containers; MovingBox and Myosotis; Grid columns-v2 declares eight columns/types and per-row SaveEntry local_id; namespace/provenance belong to collection; objects counts containers, nodes counts row values; query budget=262144',player='qualified public/native sources, canonical body scope reference, four backup slots, History, relation/life-state deltas; cached native control_group_membership includes inactive role and 703 association owner; control_relation is active anchor only',history='bounded current/floor pure values; only committed windows; streaming changes retain previous scope'))
    for path in (dest,ROOT/'docs/archive/design/observation-expansion'):(path/'continuity-catalog.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    lines=['# tx-04-13 探索、保存与玩家逐成员目录','','由两套本地Wiki生成；分类不是运行覆盖。Entity本体/Level原值沿用规范来源，新增域负责版本与历史。参数查询不假称默认值代表所有语义。','','| 成员 | 分类 | 签名／归属 | 来源 |','| --- | --- | --- | --- |']
    for e in members:lines.append('| `'+e['id']+'` | '+e['category']+' | '+str(e['parameters'] or '')+' '+e['reason']+' | '+'<br>'.join(f"[{s['wiki']}]({s['path']}) L{s['line']}" for s in e['sources'])+' |')
    (ROOT/'mod/binding/catalog/continuity-catalog.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
if __name__=='__main__':main()
