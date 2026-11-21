"""从两套本地 Wiki 生成产品字段目录及只读 Lua schema；只做资料转换，不运行采集或测试。"""
from pathlib import Path
import json
import re

ROOT=Path(__file__).resolve().parents[2]
WIKIS=[('vanilla',Path('F:/projects_isaac/IsaacDocs/docs')),('repentogon',Path('F:/projects_isaac/REPENTOGON-1.1.3/docs/docs'))]
ENTITY=['Entity','EntityPlayer','EntityNPC','EntityTear','EntityProjectile','EntityBomb','EntityLaser','EntityKnife','EntityPickup','EntityFamiliar','EntityEffect','EntitySlot','EntityDelirium']
RELATED=['renderer/Image','renderer/BlendMode','renderer/Shape','BitSet128','Sprite','LayerState','AnimationData','AnimationLayer','AnimationFrame','NullFrame','Color','KColor','Vector','Capsule','ColorParams','Weapon','TemporaryEffects','TemporaryEffect','PathFinder','TearParams','CostumeSpriteDesc','PocketItem','QueueItemData','EntityDesc','EntityConfigEntity','EntityConfigPlayer','EntityConfigBaby','ItemConfig_Item','ItemConfig_Costume','ItemConfig_Card','ItemConfig_PillEffect','LootList','LootListEntry','History','HistoryItem','PlayerTypes_ActiveItemDesc','PlayerTypes_PosVel','CppContainer_Vector_VectorList','CppContainer_Vector_EffectList','CppContainer_EntityList']
ALIASES={'renderer/Image':'Image','renderer/BlendMode':'BlendMode','renderer/Shape':'Shape','PlayerTypes_ActiveItemDesc':'ActiveItemDesc','PlayerTypes_PosVel':'PosVel','CppContainer_EntityList':'EntityList','CppContainer_Vector_VectorList':'VectorList','CppContainer_Vector_EffectList':'EffectList','ItemConfig_Item':'ItemConfigItem','ItemConfig_Costume':'ItemConfigCostume','ItemConfig_Card':'ItemConfigCard','ItemConfig_PillEffect':'ItemConfigPillEffect'}
# 以下接口不能凭 Get 前缀作为轮询。随机选择、懒创建及玩法派生计算只记录真实发生。
ACTUAL={'GetData':'mod_namespace:tx-04-14','GetRandomPickupVelocity':'rng_consuming_call','GetRandomWisp':'rng_consuming_call','RandomCoinJamAnim':'rng_consuming_call','GetGlyphOfBalanceDrop':'gameplay_drop_selection','GetTearHitParams':'gameplay_attack_parameters','GetBombVariant':'gameplay_bomb_selection','GetMultiShotParams':'gameplay_attack_parameters','GetMultiShotPositionVelocity':'gameplay_attack_parameters','GetLootList':'gameplay_loot_generation','GetFireplaceLoot':'gameplay_loot_generation','GetShopkeeperLoot':'gameplay_loot_generation'}
ACTUAL['GetBombFlags']='gameplay_attack_flags_and_rng_even_when_false:J273-779880'
DERIVED={'ComputeStatusEffectDuration','IgnoreEffectFromFriendly','CalculateEndPoint','CalcTargetPosition','QueryNPCsGroup','QueryNPCsSpawnerType','QueryNPCsType','VoidHasCollectible'}
DEFER={'IsLocalPlayer':'isolated_original_online_locality;use_cs_control_relation','GetPlayerHUD':'presentation:tx-04-14','GetMovingBoxContents':'room_save:tx-04-13','GetHistory':'player_history:tx-04-13','GetGridEntityDesc':'grid:isaac.cs.grid.lifted_descriptors'}

def clean(s):
    return re.sub(r'\[([^]]+)\]\([^)]*\)',r'\1',s).split('{')[0].strip()

def members():
    entries={}
    for cls in ENTITY+RELATED:
        for wiki,base in WIKIS:
            path=base/(cls+'.md')
            if not path.exists(): continue
            lines=path.read_text(encoding='utf-8-sig').splitlines()
            for no,line in enumerate(lines,1):
                if not line.startswith('#### '): continue
                signature=clean(line[5:])
                m=re.match(r'(.+?)\s+(\w+)\s*\((.*)\)\s*$',signature)
                if m:
                    typ,name,args=m.groups(); category='query' if re.match(r'^(Get|Is|Has|Can|Are|Needs|Will|Collides|Exists)',name) and not typ.startswith('void') else 'operation'
                else:
                    m=re.match(r'(.+?)\s+(\w+)\s*$',signature)
                    if not m: continue
                    typ,name=m.groups(); args=None; category='field'
                if 'static ' in typ: category='derived_query' if category=='query' else 'operation'
                if name.startswith('To') and name[2:] in [c.removeprefix('Entity') for c in ENTITY]: category='cast'
                reason=''
                if name in DERIVED: category='derived_query'
                if name in ACTUAL: category='actual_call'; reason=ACTUAL[name]
                elif 'RNG' in typ: category='external_reference'; reason='random:tx-04-14'
                elif typ=='function': category='external_reference'; reason='mod_function:tx-04-14'
                elif name in DEFER: category='external_reference'; reason=DEFER[name]
                elif category=='query' and args and args.strip(): category='parameter_query'
                key=(cls,name,signature)
                e=entries.setdefault(key,{'class':ALIASES.get(cls,cls),'member':name,'signature':signature,'returns':typ.replace('const ','').replace('static ',''),'parameters':args,'category':category,'reason':reason,'sources':[]})
                e['sources'].append({'wiki':wiki,'path':str(path).replace('\\','/'),'line':no})
    for name,typ in [('counter','int'),('duration','int'),('text','string'),('draw_color','native_kcolor20'),('draw_phase','native_render_dependency')]:
        reason='J273 real-current-use-writer-witness;Init-does-not-initialize-buffer' if name=='text' else 'independent-native-text-render-v1;not-ordinary-phase' if name=='draw_phase' else 'J273 completed creation owner;KColor20 includes opaque32;see native supplements'
        entries[('NativeText',name,name)]={'class':'NativeText','member':name,'signature':typ+' '+name,'returns':typ,'parameters':None,'category':'native_field','reason':reason,'sources':[{'wiki':'native-J273','path':str(ROOT/'docs/archive/design/observation-expansion/entity-native-supplements.md').replace('\\','/'),'line':1}]}
    for name,typ in [('sample_mode_raw','int8'),('start_offset_mode_opaque32','int32')]:
        entries[('EntityLaser',name,name)]={'class':'EntityLaser','member':name,'signature':typ+' '+name,'returns':typ,'parameters':None,'category':'native_field','reason':'J273 Laser Init+completed owner; signed sample byte and opaque draw branch; not gameplay setter','sources':[{'wiki':'native-J273','path':str(ROOT/'docs/archive/design/tx-04-19/entity-source-research.md').replace('\\','/'),'line':1}]}
    result=sorted(entries.values(),key=lambda e:(e['class'],e['member'],e['signature']))
    # 字段使用规范名称，重载共用名称但保留完整来源／参数签名。
    fields={(e['class'],e['member']) for e in result if e['category']=='field'}
    for e in result:
        # 只有无参实例getter可对应同名属性；静态GetOrbitDistance(Layer)不是OrbitDistance字段。
        # 二者同ID会让派生查询的NotScheduled遮蔽真实Vector，破坏完整应用来源。
        canonical=e['member'][3:] if e['member'].startswith('Get') and (e['class'],e['member'][3:]) in fields and not e['signature'].startswith('static ') and not (e['parameters'] or '').strip() else e['member']
        # J273 SampleLaser 属性被绑定为 signed-char userdata；公开 IsSampleLaser 读取同一 +1117 字节的布尔语义。
        if e['class']=='EntityLaser' and e['member']=='IsSampleLaser': canonical='SampleLaser'
        e['id']=e['class']+'.'+canonical
        # Wiki此处标题误写int；同页正文及官方Lua wrapper明确返回完整整数数组。
        # 保留原始签名与documented_returns，采用类型按源码修正，不能将真实table强读成标量。
        if e['id']=='EntityLaser.GetHitList':
            e['documented_returns']=e['returns']
            e['returns']='int[]'
            e['reason']='Wiki标题int与正文array冲突；官方Lua_EntityLaserGetHitList逐项返回整数table，采用int[]并保留原签名'
            e['sources'].append({'wiki':'repentogon-source','path':str(WIKIS[1][1].parent.parent/'repentogon/LuaInterfaces/Entities/LuaEntityLaser.cpp').replace('\\','/'),'line':114})
        # 官方wrapper用lua_pushnumber返回double缓存值；Wiki的int标题不能截断小数或拒绝真实来源。
        if e['id']=='EntityPlayer.GetTearsCap':
            e['documented_returns']=e['returns'];e['returns']='double'
            e['reason']='官方1.1.3 Lua_PlayerGetTearsCap返回GetCustomCacheValue的double；保留Wiki原始int签名'
            e['sources'].append({'wiki':'repentogon-source','path':str(WIKIS[1][1].parent.parent/'repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp').replace('\\','/'),'line':2857})
        # Angle标题int8的正文明确0..255，官方getter也零扩展uint8；保留原文而修正采用范围。
        if e['id']=='EntityDelirium.Angle':
            e['documented_returns']=e['returns'];e['returns']='uint8'
            e['reason']='Wiki正文0..255与官方uint8 getter一致；标题int8保留为原始签名，采用无符号8位'
            e['sources'].append({'wiki':'repentogon-source','path':str(WIKIS[1][1].parent.parent/'repentogon/LuaInterfaces/LuaDelirium.cpp').replace('\\','/'),'line':79})
        e['schema']='isaac.cs.entity'; e['schema_version']=2
        e['target']='REPENTOGON 1.1.3 / Repentance+ v1.9.7.12.J273'
        e['phase']='OrdinaryPost / Lifecycle / explicit query scope'
        if e['id']=='NativeText.draw_phase': e['phase']='ordinary owner-qualified dependency / independent Render entry inputs'
        e['absence']='Value / Null / Empty / NotApplicable / Unavailable / NotCaptured / ReadFailed / Nonfinite / Truncated'
        e['units']='API原单位；持续时间为游戏更新帧，角度／坐标依原成员语义'
        e['implementation']={'field':'public_lua_copy','query':'public_lua_copy','parameter_query':'bounded_parameter_query','derived_query':'explicit_readonly_query','operation':'actual_event_only','actual_call':'actual_event_only','external_reference':'owner_qualified_reference','cast':'classification','native_field':'native_copy'}[e['category']]
    return result

def lua(v):
    if v is None:return 'nil'
    if isinstance(v,str):return json.dumps(v,ensure_ascii=False)
    if isinstance(v,bool):return 'true' if v else 'false'
    if isinstance(v,(int,float)):return str(v)
    if isinstance(v,list):return '{'+','.join(lua(x) for x in v)+'}'
    if isinstance(v,dict):return '{'+','.join('['+lua(k)+']='+lua(x) for k,x in v.items())+'}'
    raise TypeError(type(v))

def main():
    data=members()
    dest=ROOT/'mod/observation/isaac-cs-observation/domains/entity'; dest.mkdir(parents=True,exist_ok=True)
    groups={}
    for e in data:
        groups.setdefault(e['class'],[]).append({**{k:e[k] for k in ['id','member','returns','parameters','category','reason']},'owner':e['class'],'static':e['signature'].startswith('static ')})
    (dest/'schema.lua').write_text('-- 由 gen_entity_catalog.py 生成；此表只声明读能力，不动态执行动作。\nreturn '+lua(groups)+'\n',encoding='utf-8')
    out=ROOT/'mod/binding/catalog/entity-catalog.json'
    out.write_text(json.dumps({'format':'isaac.cs.entity.catalog','version':2,'members':data},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    (dest.parents[1]/'entity-catalog.json').write_text(out.read_text(encoding='utf-8'),encoding='utf-8')
    lines=['# tx-04-10 Entity 逐成员来源目录','','由 `mod/tools/gen_entity_catalog.py` 从两套本地 Wiki 生成。实现与实机覆盖分别记录；目录行不等于运行通过。继承字段由 Entity 统一管理。参数查询保留完整签名与默认值；无界向量／字符串查询按显式请求采集，不假称遍历全部参数。','','| 规范字段／接口 | 签名 | 分类及实现 | 来源 |','| --- | --- | --- | --- |']
    for e in data:
        sources='<br>'.join(f"[{s['wiki']}]({s['path']})（行{s['line']}）" for s in e['sources'])
        lines.append(f"| `{e['id']}` | `{e['signature'].replace('|','&#124;')}` | {e['implementation']} {e['reason']} | {sources} |")
    (out.with_suffix('.md')).write_text('\n'.join(lines)+'\n',encoding='utf-8')

if __name__=='__main__': main()
