"""tx-04-14：转换本地Wiki为四域成员目录；不执行游戏、Getter或测试。"""
from gen_entity_catalog import ROOT,WIKIS,clean,lua
import re,json

CLASSES={**dict.fromkeys(['Game','Seeds','ChallengeParam','DailyChallenge','Ambush'],'run'),**dict.fromkeys(['RNG','ItemPool'],'random'),**dict.fromkeys(['HUD','PlayerHUD','PlayerHUDHeart','HistoryHUD','HistoryHUDItem','MusicManager','SFXManager','GenericPrompt','ColorModifier','ItemOverlay','Font'],'presentation'),**dict.fromkeys(['ModReference','EntitySaveStateManager'],'mod')}
# 名称中的Get不保证只读；真实抽取、随机及懒创建必须走实际调用。
ACTUAL={'GetCollectible','GetCollectibleFromList','GetCard','GetCardEx','GetPill','GetTrinket','GetRandomPool','PickCollectible','GetNextSeed','GetPlayerInitSeed','GetNextWave','GetNextWaves','GetEntityData','GetEntitySaveStateData','RandomInt','RandomFloat','RandomVector','Next','Previous'}
EXTERNAL={'GetRoom':'room','GetLevel':'level','GetPlayer':'entity','GetNearestPlayer':'entity','GetRandomPlayer':'random','GetItemPool':'random','GetSeeds':'run','GetHUD':'presentation','GetAmbush':'run','GetItemOverlay':'presentation'}

def main():
    entries={}
    for cls,domain in CLASSES.items():
        for wiki,base in WIKIS:
            p=base/(cls+'.md')
            if not p.exists():continue
            for no,line in enumerate(p.read_text(encoding='utf-8-sig').splitlines(),1):
                if not line.startswith('#### '):continue
                sig=clean(line[5:]);m=re.match(r'(.+?)\s+(\w+)\s*\((.*)\)\s*$',sig)
                if m:
                    typ,name,args=m.groups();cat='query' if name.startswith(('Get','Is','Has','Can','Count','Achievement','Phantom')) and typ!='void' else 'operation'
                    if cat=='query' and args.strip():cat='parameter_query'
                else:
                    m=re.match(r'(.+?)\s+(\w+)\s*$',sig)
                    if not m:continue
                    typ,name=m.groups();args=None;cat='field'
                reason=''
                if name in ACTUAL or 'Random' in name and not name.startswith('Phantom'):cat='actual_call';reason='real_call_only;never_poll'
                if cls=='Game' and name in EXTERNAL:cat='external_reference';reason='canonical:'+EXTERNAL[name]
                if cls=='PlayerHUD' and name=='GetHUD':cat='external_reference';reason='presentation.hud;avoid_cycle'
                if name in ('__len','__index','__newindex'):cat='operation'
                key=(cls,sig);e=entries.setdefault(key,dict(id=cls+'.'+name,owner=cls,member=name,returns=typ.replace('static ','').replace('const ',''),parameters=args,category=cat,reason=reason,static=cls in ('DailyChallenge','EntitySaveStateManager','Ambush','ItemOverlay') or typ.startswith('static '),domain=domain,sources=[]))
                e['sources'].append(dict(wiki=wiki,path=p.as_posix(),line=no))
    members=sorted(entries.values(),key=lambda e:(e['owner'],e['member'],e['parameters'] or ''))
    dest=ROOT/'mod/observation/isaac-cs-observation';groups={d:{} for d in ('run','random','presentation','mod')}
    for e in members:
        groups[e['domain']].setdefault(e['owner'],[]).append({k:e[k] for k in ('id','owner','member','returns','parameters','category','reason','static')})
        e.update(schema='isaac.cs.'+e['domain'],version=2,target='REPENTOGON 1.1.3 / v1.9.7.12.J273',phase='OrdinaryPost / independent actual call / Render',absence='Null/Empty/Unavailable/NotCaptured/ReadFailed/Nonfinite/Truncated',units='API原单位；各时钟不合并；概率修正与最终值分开')
    for d,g in groups.items():
        p=dest/'domains'/d;p.mkdir(parents=True,exist_ok=True);(p/'schema.lua').write_text('-- 自动生成：动作与真实调用不进入轮询器。\nreturn '+lua(g)+'\n',encoding='utf-8')
    callbacks=[];enums=[]
    for wiki,base in WIKIS:
        for p in (base/'enums').glob('*.md'):
            if re.search('SeedEffect|GameState|LevelState|ItemPool|SoundEffect|Music|PlayerHUD|Challenge|PillColor',p.stem):enums.append(dict(wiki=wiki,path=p.as_posix(),text=p.read_text(encoding='utf-8-sig')))
        p=base/'enums/ModCallbacks.md'
        for no,line in enumerate(p.read_text(encoding='utf-8-sig').splitlines(),1):
            if line.startswith('|') and 'MC_' in line and any(x in line for x in ('RNG','COLLECTIBLE','PILL','CARD','SOUND','MUSIC','HUD','GAME','RENDER')):callbacks.append(dict(path=p.as_posix(),line=no,signature=line))
    data=dict(format='isaac.cs.run-random-presentation-mod.catalog',version=2,members=members,enums=enums,callbacks=callbacks,unexposed=['Native random consumers bypass Lua wrappers: generation/combat/AI/drop internal calls; only current named state and public result callbacks are visible','GPU pixels and sound-card samples are not captured','Unregistered mods/private closures/native opaque objects require explicit adapters'])
    # 本地装载绑定的新HUDMessage入口替代Wiki旧Sprite别名；仅目录补充，不扩大轮询表。
    binding='F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/HUD/LuaHUD.cpp'
    data['binding_supplements']=[dict(id=name,source=binding,line=line,implementation='domains/presentation/collect.lua:messages',semantics='current_message_or_public_getter;no_update_or_render') for name,line in [('HUD.GetMainMessage',264),('HUD.GetStackedMessage',265),('HUD.GetCoopPlayerMessage',266),('HUDMessage.GetSprite',239),('HUDMessage.IsShowing',240),('HUDMessage.GetMainText',243),('HUDMessage.GetSubText',245)]]
    data['binding_supplements'].append(dict(id='Game.GetGenericPrompt',source='F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaGame.cpp',implementation='IsaacCSObservationNative.PromptState',semantics='direct_main_thread_live_root;avoid_unconstructed_cpp_ANM2_copy;public_fields_and_sprite_only'))
    for p in (dest,ROOT/'docs/design/observation-expansion'):(p/'run-catalog.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    lines=['# tx-04-14 四域逐成员目录','','本地Wiki逐成员分类；目录不等于实机覆盖。真实调用不额外执行；无界参数查询按显式请求。','','| 成员 | 分类 | 参数／来源 |','| --- | --- | --- |']
    for e in members:lines.append('| `'+e['id']+'` | '+e['category']+' '+e['reason']+' | '+str(e['parameters'] or '').replace('|','&#124;')+' '+'<br>'.join(f"[{s['wiki']}]({s['path']}) L{s['line']}" for s in e['sources'])+' |')
    lines+=['','## 当前绑定补充','','HUD旧Sprite别名由当前HUDMessage的真实Getter提供；通用Prompt通过安全纯值桥接读取，不复制C++对象。','']
    for e in data['binding_supplements']:lines.append('- `'+e['id']+'`：'+e['implementation']+'；来源 '+e['source'])
    (ROOT/'docs/design/observation-expansion/run-catalog.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
if __name__=='__main__':main()
