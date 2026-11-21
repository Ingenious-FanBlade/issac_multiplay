"""tx-04-12补全清单：读取本地Wiki与原版STB静态布局，生成必测场景；不运行游戏。"""
from pathlib import Path
import json,re,struct
from collections import Counter
from gen_entity_catalog import ROOT,lua
RES=Path('F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/extracted_resources/resources/rooms')
def layouts(root=RES):
 out=[]
 for p in sorted(root.glob('*.stb')):
  b=p.read_bytes(); at=8
  if b[:4]!=b'STB1':continue
  for _ in range(struct.unpack_from('<I',b,4)[0]):
   typ,var,sub,dif,size=struct.unpack_from('<IIIBH',b,at);at+=15
   name=b[at:at+size].decode('utf-8','replace');at+=size
   weight,w,h,shape,nd,ne=struct.unpack_from('<fBBBBH',b,at);at+=10+nd*5
   for _ in range(ne):
    x,y,count=struct.unpack_from('<hhB',b,at);at+=5+count*10
   out.append(dict(stage=int(p.name[:2]),type=typ,variant=var,subtype=sub,shape=shape,name=name,source=p.as_posix()))
 return out

def main():
 stock=layouts(); greed_stock=layouts(RES/'greed'); cases=[]
 def select(typ,sub=None,shape=None,stage=None,variant=None):
  candidates=[r for r in stock if r['type']==typ and (sub is None or r['subtype']==sub) and (shape is None or r['shape']==shape) and (stage is None or r['stage']==stage) and (variant is None or r['variant']==variant)]
  candidates.sort(key=lambda r:(r['stage']!=0,r['variant'],r['stage']))
  if not candidates:raise ValueError(('no static layout',typ,sub,shape,stage,variant))
  return candidates[0]
 def add(id,group,layout=None,**kw):
  c=dict(id=id,group=group,method='controlled-config',return_required=True)
  if layout:c.update(layout=layout,expected_type=layout['type'],expected_subtype=layout['subtype'],expected_shape=layout['shape'])
  c.update(kw);cases.append(c)
 wiki=Path('F:/projects_isaac/IsaacDocs/docs/enums/RoomType.md').read_text(encoding='utf-8-sig')
 for typ in range(1,30):
  name=re.search(r'\|'+str(typ)+r' \|(ROOM_\w+)',wiki).group(1)
  add('type_'+name,'room_type',select(typ),expected_grid=-3)
 slots=[(-1,14,0),(-2,3,0),(-3,1,0),(-4,16,0),(-5,17,0),(-6,22,0),(-7,5,55),(-8,5,63),(-9,5,70),(-10,27,1),(-11,16,1),(-12,18,99),(-13,2,0),(-14,16,2),(-15,16,3),(-16,28,0),(-17,5,0),(-18,15,1),(-20,1,0)]
 # Hush/Delirium的实际Boss ID以STB语义选择，不把槽名直接当RoomType。
 for slot,typ,sub in slots:
  add('slot_'+str(-slot),'off_grid_slot',select(typ,sub),expected_grid=slot,method='controlled-slot',reenter_required=True)
 add('slot_devil_native','native_entry',select(14,0),expected_grid=-1,method='devil',reenter_required=True)
 add('slot_angel_native','native_entry',select(15,0),expected_grid=-1,method='angel',reenter_required=True)
 add('genesis_native','native_entry',select(18,99),expected_grid=-12,method='genesis',reenter_required=True)
 add('crawlspace_native','native_entry',select(16,0),expected_grid=-4,method='crawlspace',reenter_required=True)
 for c in cases:
  if c['group']=='native_entry':c['expected_room']=c.pop('expected_grid')
  if c['method'] in ['angel','devil']:
   c.pop('expected_shape',None);c.pop('expected_subtype',None)
 # 十二房形均取Basement集合；Void磁盘候选曾在实机集合中找不到，不能把静态布局当可加载见证。
 for shape in range(1,13):add('shape_'+str(shape),'room_shape',select(1,shape=shape,stage=1),expected_grid=-3)
 pairs={2:[0,1,2,3,4,10,11,100,101,102,103,104,110,111],4:[0,1,2,3,34],6:list(range(16)),9:[0,1],10:[0,1],11:[0,1,10,11,12],12:list(range(5)),14:[0,1],15:[0,1],16:list(range(5)),18:[0,99],27:[1,2,3]}
 for typ,subtypes in pairs.items():
  for sub in subtypes:
   if typ==16 and sub==3:
    # Rotgut心脏需专用槽及先前保存的实体；离图槽项已完成构建，公开子类项复访该真实房间。
    add(f'subtype_{typ}_{sub}','public_subtype',select(typ,sub),expected_grid=-15,method='existing-slot')
   else:add(f'subtype_{typ}_{sub}','public_subtype',select(typ,sub),expected_grid=-3)
 # 同一默认RoomType里的特殊布局不能被普通房间替代：按STB/公开子类逐项列出。
 for stage,subs in {7:[1],27:[1,34],29:[1,10,11,20,30,31],35:[0,1,2,3,10,11,33,34],36:[0,1,4,7,27,29,31]}.items():
  for sub in subs:add(f'default_special_{stage}_{sub}','special_default_layout',select(1,sub,stage=stage),expected_grid=-3)
 # 全维度与关键特殊路线，分别验证入口房及实际内部，不用类型枚举代替路线。
 routes=[('downpour','stage 2c',0),('dross','stage 2d',0),('mines','stage 4c',0),('ashpit','stage 4d',0),('mausoleum','stage 6c',0),('gehenna','stage 6d',0),('corpse','stage 8c',0),('blue_womb','stage 9',0),('sheol','stage 10',0),('cathedral','stage 10a',0),('dark_room','stage 11',0),('chest','stage 11a',0),('void','stage 12',0),('home','stage 13',0)]
 for name,cmd,dim in routes:add('route_'+name,'route',method='stage',command=cmd,expected_dimension=dim,return_required=False)
 add('mirror_entrance','route',method='route-room',command='stage 2c',descriptor_subtype=34,expected_dimension=0,return_required=False)
 add('mirror_inside','dimension',method='dimension',dimension=1,expected_dimension=1,return_required=False)
 add('mineshaft_entrance','route',method='route-room',command='stage 4c',descriptor_subtype=10,expected_dimension=0,return_required=False)
 add('mineshaft_inside','dimension',method='dimension',dimension=1,expected_dimension=1,return_required=False)
 add('death_certificate','dimension',method='death-certificate',expected_dimension=2,return_required=False)
 add('red_room','structure_change',method='red-room',return_required=False)
 add('ascent','route',method='ascent',expected_ascent=True,return_required=False)
 add('greed','progress',method='greed',expected_greed=True,return_required=False)
 # Greed资源集与普通模式独立；逐类型及最终Boss列项，不能用普通模式同类型代替。
 for typ in sorted({r['type'] for r in greed_stock}):
  chosen=next(r for r in greed_stock if r['type']==typ)
  add(f'greed_layout_{typ}','greed_room',dict(chosen,mode=1),expected_grid=-3,expected_greed=True)
 ultra=next(r for r in greed_stock if r['type']==5 and r['subtype']==62)
 add('greed_ultra_layout','greed_room',dict(ultra,mode=1),expected_grid=-3,expected_greed=True)
 for typ in [1,2,4,8,10,23]:
  add(f'greed_generated_{typ}','greed_room',method='greed-room',room_type=typ,expected_type=typ,expected_greed=True)
 for method,typ in [('angel',15),('devil',14)]:
  add('greed_native_'+method,'greed_room',method=method,expected_room=-1,expected_type=typ,expected_greed=True,reenter_required=True)
 add('greed_arena','greed_room',method='greed-room',room_type=1,room_shape=4,expected_type=1,expected_shape=4,expected_greed=True,return_required=False)
 add('greed_wave','progress',method='greed-wave',expected_greed=True,expected_wave=True,return_required=False)
 # Greed出口使用实际Greed上下文；资源文件18对应special set的mode=1，不是可加载Stage18。
 greed_exit=next(c for c in cases if c['id']=='type_ROOM_GREED_EXIT'); cases.remove(greed_exit); cases.append(greed_exit)
 for stage in range(2,8):add(f'greed_floor_{stage}','greed_floor',method='greed-stage',stage=stage,expected_stage=stage,expected_greed=True,return_required=False)
 add('greed_ultra_native','greed_room',method='greed-room',room_type=5,room_subtype=62,expected_type=5,expected_subtype=62,expected_greed=True,return_required=False)
 add('greedier','progress',method='greedier',expected_greed=True,expected_difficulty=3,return_required=False)
 add('greedier_arena','greed_room',method='greed-room',room_type=1,room_shape=4,expected_type=1,expected_shape=4,expected_greed=True,return_required=False)
 add('greedier_wave','progress',method='greed-wave',expected_greed=True,expected_wave=True,expected_difficulty=3,return_required=False)
 data=dict(format='isaac.cs.tx-04-12.coverage',version=1,sources=['IsaacDocs/docs/enums/RoomType.md','IsaacDocs/docs/enums/GridRooms.md','IsaacDocs/docs/enums/RoomShape.md','REPENTOGON/docs/docs/enums/RoomSubType.md','REPENTOGON/include/IsaacRepentance.h:eGridRooms','REPENTOGON/stbParser/stbParser.cpp'],cases=cases,exclusions=[dict(id='ROOM_NULL',reason='0 is absence, not a constructible room'),dict(id='NUM_ROOMTYPES/NUM_ROOMSHAPES/NO_ROOM_IDX',reason='bounds/sentinel, never enter'),dict(id='ROOM_DEATHMATCH/ROOM_DEATHMATCH_IDX',reason='original Online isolation; do not enter or initialize'),dict(id='ROOM_MIRROR_IDX/ROOM_MINESHAFT_IDX',reason='-100/-101 are door routing sentinels, not descriptor slots; observe target semantics and enter real dimensions')],limits='Each public category/path above is required; not a Cartesian product of every XML variant, seed, character or gameplay ending. Controlled native-slot construction is distinguished from actual item/entrance flow; unused teleporter types use stock layouts without claiming natural reachability.')
 out=ROOT/'tools/harness/scenarios/tx-04-12-special-coverage.json';out.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 (ROOT/'tests/e2e-mods/isaac-cs-test/tx12_cases.lua').write_text('-- 自动生成的必测清单；只供E2E夹具，不进入产品。\nreturn '+lua(cases)+'\n',encoding='utf-8')
 lines=['# tx-04-12 特殊房间补全覆盖清单','','状态：清单已实现并通过同tx统一E2E，当前结果与边界见[补充实现及验收](../../../docs/archive/design/observation-expansion/tx-04-12-supplement.md)。历史5/5仅证明旧场景。','','来自两套本地Wiki、目标SDK及原版STB静态布局（包括独立greed资源集）。按房间类型、原生离图槽位、公开结构子类、全部房形、维度和路线分别列项；每项必须有Complete的正式Room/Level样本，不能仅有夹具请求或debug类型计数。','','隔离排除：RoomType30/槽-19属于原版Online，禁止进入/初始化；0/数量常量/NO_ROOM_IDX不是真实房间；-100/-101仅为门路由哨兵，禁止按房号进入。目标SDK实际有20个离图槽（总527），本地Wiki的MAX_ROOMS=525为旧值。','',f'{len(cases)}项是按不同风险维度计数的场景，不是{len(cases)}种RoomType。29个类型项只测试各类型；12个形状项统一使用ROOM_DEFAULT，没有做29×12的组合。槽位、公开子类、原生入口、路线各有独立判据，同一类型在这些项中可以重复。',
    '29种单机适用类型含当前未自然使用的传送器入口/出口25/26，使用其原版Mausoleum布局进行受控构建，不宣称自然触发。离图槽的受控配置构建、原生物品/入口流程以及调试槽分别标明。Genesis、天使/恶魔、夹层必须另外走真实原生入口。所有离图槽逐项退出/回访；原生slot -1的天使/恶魔和slot -3多配置重建不能被合并成同一构建版本。','','| 必测ID | 分类/方式 | 实际要求 | 来源布局 |','| --- | --- | --- | --- |']
 for c in cases:
  req={k:v for k,v in c.items() if k.startswith('expected_')};r=c.get('layout',{})
  lines.append(f"| `{c['id']}` | {c['group']} / {c['method']} | {json.dumps(req,ensure_ascii=False)} | {r.get('stage','')} / {r.get('type','')}.{r.get('variant','')}.{r.get('subtype','')} {r.get('name','')} |")
 lines+=['','所有原进度/环境/查询/停录/暂停/退出/隔离/部署恢复用例一起保留，统一入口连续执行。公开RoomSubType的商店等级/稀有/店主、宝物选择/付费/刀片、小Boss、街机/诅咒、挑战/波次/Gideon、图书馆、恶魔/天使变体、Gideon/Rotgut/Beast夹层、Genesis卧室及三种支线出口分别列项。其余公开默认房子类由路线中的实际描述另列，不能以一次Stage命令替代房间内样本。','']
 counts=Counter(c['group'] for c in cases)
 lines+=['## 用例计数（不同维度可重复覆盖同一RoomType）','','| 维度 | 用例数 |','| --- | --- |']+[f'| {group} | {count} |' for group,count in counts.items()]+['',f'合计{len(cases)}项；退出/回访是各项下的额外阶段，另行记录。','']
 (ROOT/'mod/binding/catalog/tx-04-12-special-room-coverage.md').write_text('\n'.join(lines),encoding='utf-8')
 print('coverage cases',len(cases))
if __name__=='__main__':main()
