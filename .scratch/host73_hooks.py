import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'mod/binding'
def read(n):return json.loads((root/n).read_text(encoding='utf-8'))
def write(n,x):(root/n).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
hooks=read('hooks.json');rows=[]
def add(ident,cls,method,params,call,rva,own=False):
    rows.append([ident,'world',cls,method,'void',params,call,hex(rva),'None',False,'',own,-20000,True])
add('binary_music_play','Music','Play','int musicid, float volume','musicid, volume',0x3d8360)
add('binary_music_crossfade','Music','Crossfade','int musicid, float faderate','musicid, faderate',0x3d8480)
add('binary_music_queue','CsMusic','Queue','int musicid','musicid',0x3d8a80,True)
add('binary_music_fadein','Music','Fadein','unsigned int Id, float Volume, float FadeRate','Id, Volume, FadeRate',0x3d8690)
add('binary_music_fadeout','Music','Fadeout','float FadeRate','FadeRate',0x3d8710)
add('binary_music_pause','Music','Pause','','',0x3d8750)
add('binary_music_resume','Music','Resume','','',0x3d8800)
add('binary_music_jingle','Music','PlayJingle','int musicid, int unusedInt, bool unusedBool','musicid, unusedInt, unusedBool',0x3d8b80)
add('binary_music_layer_on','Music','EnableLayer','int id, bool instant','id, instant',0x3d8890)
add('binary_music_layer_off','Music','DisableLayer','int id','id',0x3d8a10)
add('binary_display_banner','HUD','ShowItemText','Entity_Player* player, ItemConfig_Item* item','player, item',0x57b080)
add('binary_display_custom_banner','HUD','ShowItemTextCustomUTF8','Entity_Player* player, char* title, char* subtitle','player, title, subtitle',0x57b720)
add('binary_display_fortune','HUD','ShowFortuneText','int** param_1','param_1',0x57bee0)
add('binary_display_banner_text','HUD_Message','LoadText','const wchar_t* str, bool asSubText','str, asSubText',0x431a60)
add('binary_display_overlay','ItemOverlay','Show','int eOverlayID, int delay, Entity_Player* player','eOverlayID, delay, player',0x5848b0)
add('binary_display_boss','RoomTransition','StartBossIntro','unsigned int bossID1, unsigned int bossID2','bossID1, bossID2',0x423d10)
add('binary_display_transition_start','CsRoomTransition','Start','int index, int direction, int animation, Entity_Player* player, int dimension','index, direction, animation, player, dimension',0x423990,True)
add('binary_display_fortune_source','CsFortuneRng','ShowFortune','char* category','category',0x558c30,True)
add('binary_display_hud_update','HUD','Update','','',0x57acf0)
add('binary_display_hud_post','HUD','PostUpdate','','',0x57ae90)
add('binary_display_minimap_update','Minimap','Update','','',0x565db0)
add('binary_display_minimap_refresh','Minimap','Refresh','','',0x565d60)
add('binary_display_camera_drag','Camera','UpdateDrag2','','',0x51e5e0)
add('binary_display_camera_smooth','Camera','UpdateUltrasmooth','bool unk','unk',0x51d840)
add('binary_display_fx','CsFXLayers','DisplayUpdate','bool unk','unk',0x56bf40,True)
add('binary_display_overlay_update','ItemOverlay','Update','bool unk','unk',0x584110)
existing={r[0] for r in hooks['hooks']}
hooks['hooks']=[r for r in rows if r[0] not in existing]+hooks['hooks']
write('hooks.json',hooks)
static=read('v1_9_7_12-static.json')
patterns={0x3d8a80:'55 8B EC 56 8B 75 08 85 F6 78 27 8B 91 20 03 00 00',0x423990:'53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 6A FF',0x558c30:'55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 83 EC 48',0x56bf40:'55 8B EC 83 E4 F8 83 EC 28 56 57 8B F9 89 7C 24 20 80 7F 08 00'}
existing={int(r['rva'],0) for r in static['functions']}
for rva,pattern in patterns.items():
    if rva not in existing:static['functions'].append(dict(rva=hex(rva),pattern=pattern,type='void __thiscall',entry=['版本73显示委托Host正常来源；J273 IDA静态核对，原函数恰好一次；不含测试结论。']))
write('v1_9_7_12-static.json',static)
