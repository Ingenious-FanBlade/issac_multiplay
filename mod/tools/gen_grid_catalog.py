"""从本地 Wiki 转换 Grid 全成员、类型和回调目录，不运行游戏或校验器。"""
from pathlib import Path
import json
import re
from gen_entity_catalog import ROOT, WIKIS, clean, lua

CLASSES = ['GridEntity'] + ['GridEntity'+x for x in ['Decoration','Door','Fire','Gravity','Lock','Pit','Poop','PressurePlate','Rock','Spikes','Stairs','Statue','Teleporter','TNT','TrapDoor','Wall','Web']] + ['GridEntityDesc']

def main():
    entries={}
    for cls in CLASSES:
        for wiki,base in WIKIS:
            path=base/(cls+'.md')
            if not path.exists(): continue
            for no,line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(),1):
                if not line.startswith('#### '): continue
                sig=clean(line[5:]); m=re.match(r'(.+?)\s+(\w+)\s*\((.*)\)\s*$',sig)
                if m:
                    typ,name,args=m.groups()
                    category='cast' if name.startswith('To') else 'query' if name.startswith(('Get','Is','Can','Has')) else 'operation'
                    if category=='query' and args.strip(): category='parameter_query'
                else:
                    m=re.match(r'(.+?)\s+(\w+)\s*$',sig)
                    if not m: continue
                    typ,name=m.groups(); args=None; category='field'
                reason=''
                if name=='IsBreakableRock': typ='boolean'; reason='Wiki void typo; official LuaGridEntity.cpp returns boolean'
                if name=='GetRenderPosition': reason='official Lua wrapper leaks temporary Vector; native stack-buffer adapter'
                canonical='GetSaveState' if cls=='GridEntity' and name=='Desc' else name
                if cls=='GridEntityDoor' and name=='GetExtraSprite': canonical='ExtraSprite'
                if cls=='GridEntityRock' and name=='GetRubbleAnim': canonical='RubbleAnim'
                key=(cls,name,sig)
                e=entries.setdefault(key,dict(id=cls+'.'+canonical,owner=cls,member=name,returns=typ.replace('const ','').replace('static ',''),parameters=args,category=category,reason=reason,sources=[]))
                e['sources'].append(dict(wiki=wiki,path=path.as_posix(),line=no))
    # 官方绑定公开但本地Wiki尚未列出的动作，仍纳入完整目录；采集不得执行。
    source=ROOT.parent/'REPENTOGON-1.1.3/repentogon/LuaInterfaces/GridEntities/LuaGridEntity.cpp'
    entries[('GridEntity','HurtSurroundings','native-public')]=dict(id='GridEntity.HurtSurroundings',owner='GridEntity',member='HurtSurroundings',returns='void',parameters='float enemyDistance, float playerDistance, float enemyDamage, int playerDamage, int damageFlags, boolean ignoreGridCollision',category='operation',reason='official public binding; absent from local Wiki; actual event only',sources=[dict(wiki='official-binding',path=source.as_posix(),line=18)])
    members=sorted(entries.values(),key=lambda e:(e['owner'],e['member']))
    groups={}
    for e in members:
        e.update(schema='isaac.cs.grid',schema_version=2,target='REPENTOGON 1.1.3 / Repentance+ v1.9.7.12.J273',phase='OrdinaryPost / independent callback / explicit query',absence='Value / Null / Empty / NotApplicable / Unavailable / ReadFailed / Nonfinite / Truncated',units='API原单位；计时为游戏更新帧；查询保留参数与默认值')
        e['implementation']={'field':'public_lua_copy','query':'readonly_query','parameter_query':'finite_enum_or_explicit_query','cast':'all_cast_results','operation':'actual_callback_event_only'}[e['category']]
        if e['member']=='Desc': e['implementation']='alias:GridEntity.GetSaveState'
        groups.setdefault(e['owner'],[]).append({k:e[k] for k in ['id','owner','member','returns','parameters','category','reason']})
    enums=[]
    for wiki,base in WIKIS:
        for path in sorted((base/'enums').glob('*.md')):
            if not re.search(r'Grid|Door|Rock|Pit|Poop|PressurePlate|Spikes|Teleporter|WaterClip|Backdrop|RoomType',path.stem): continue
            enums.append(dict(wiki=wiki,path=path.as_posix(),text=path.read_text(encoding='utf-8-sig')))
    callbacks=[]
    path=WIKIS[1][1]/'enums/ModCallbacks.md'
    for no,line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(),1):
        if line.startswith('|') and 'MC_' in line and any(x in line for x in ['GRID_ENTITY','GRID_ROCK','GRID_POOP','GRID_PIT','GRID_TNT','GRID_HURT']):
            callbacks.append(dict(source=path.as_posix(),line=no,signature=line))
    data=dict(shared_resources=dict(catalog='entity-catalog.json',canonical_classes=['Sprite','LayerState','AnimationData','AnimationLayer','AnimationFrame','NullFrame','Image','BlendMode','Shape','Vector','Color','KColor'],semantics='same canonical resource fields; current Grid owns the copied resource state; no second identity'),associated=[dict(owner='EntityEffect',member='GetGridEntityDesc',applicability='EffectVariant.GRID_ENTITY_PROJECTILE_HELPER=136',implementation='grid.lifted_descriptors; Entity identity; does not imply active Grid membership',sources=['REPENTOGON/docs/EntityEffect.md:34','repentogon/LuaInterfaces/Entities/LuaEntityEffect.cpp:52'])],format='isaac.cs.grid.catalog',version=2,classes=CLASSES,members=members,enums=enums,callbacks=callbacks)
    dest=ROOT/'mod/observation/isaac-cs-observation'
    (dest/'domains/grid').mkdir(parents=True,exist_ok=True)
    (dest/'domains/grid/schema.lua').write_text('-- 自动生成；动作仅入目录，不由采集执行。\nreturn '+lua(groups)+'\n',encoding='utf-8')
    out=ROOT/'mod/binding/catalog/grid-catalog.json'
    out.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    (dest/'grid-catalog.json').write_text(out.read_text(encoding='utf-8'),encoding='utf-8')
    lines=['# tx-04-11 Grid 逐成员目录','','Sprite／图层／动画／资源的共用规范字段见[资源成员目录](entity-catalog.md)，本目录JSON的shared_resources声明复用范围。','','继承公共字段由 GridEntity 管理；17 个派生接口分别登记，空的派生接口仍保存转换结果和类型。Desc 与 GetSaveState 共用规范字段。操作不在采集时执行，真实更新／生成／破坏与 native 生命周期分 scope 记录。全参数查询见签名；目录不等于全部场景运行覆盖。','','| 规范成员 | 原成员及参数 | 实现 | 来源 |','| --- | --- | --- | --- |']
    for e in members:
        sources='<br>'.join(f"[{s['wiki']}]({s['path']}) L{s['line']}" for s in e['sources'])
        lines.append(f"| `{e['id']}` | `{e['member']}({e['parameters'] or ''})` | {e['implementation']} {e['reason']} | {sources} |")
    out.with_suffix('.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')

if __name__=='__main__': main()
