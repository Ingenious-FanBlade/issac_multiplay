// 原生执行器只借用当前Binding的对象；协议/后台从不持有这些地址或Lua userdata。
const V &EntityValues(const J &model){return At(At(At(model,"components"),"entity"),"state");}
const V &EntityObject(const Scene &scene,std::uint64_t id){
 const auto found=scene.entity_values.find(id);if(found==scene.entity_values.end())Fail("native_entity_object_missing");return *found->second;
}
std::uint64_t EntityKeyNumber(const V &key,const char *name){return std::stoull(Str(At(key,name)));}
Entity *EntityPointer(Scene &s,std::uint64_t id){
 if(auto i=s.entities.find(id);i!=s.entities.end())return i->second;
 if(auto i=s.entity_players.find(id);i!=s.entity_players.end())return i->second;
 Fail("native_entity_reference_missing");
}
lua::Metatables EntityMetatable(const std::string &category){
 if(category=="EntityPlayer")return lua::Metatables::ENTITY_PLAYER;if(category=="EntityNPC")return lua::Metatables::ENTITY_NPC;
 if(category=="EntityTear")return lua::Metatables::ENTITY_TEAR;if(category=="EntityProjectile")return lua::Metatables::ENTITY_PROJECTILE;
 if(category=="EntityFamiliar")return lua::Metatables::ENTITY_FAMILIAR;if(category=="EntityPickup")return lua::Metatables::ENTITY_PICKUP;
 if(category=="EntityBomb")return lua::Metatables::ENTITY_BOMB;if(category=="EntityLaser")return lua::Metatables::ENTITY_LASER;
 if(category=="EntityKnife")return lua::Metatables::ENTITY_KNIFE;if(category=="EntityEffect")return lua::Metatables::ENTITY_EFFECT;
 return lua::Metatables::ENTITY;
}
// 解析闭包只在本次Lua调用期间存在，检查scope/shape后返回本方拥有的当前对象。
int ResolveNativeEntityLua(lua_State *L){
 const bool comparing=lua_gettop(L)>=2;
 auto *s=static_cast<Scene *>(lua_touserdata(L,lua_upvalueindex(1)));void *object=nullptr;bool grid=false,nil=false,failed=false;const std::string *category=nullptr;
 try{
  if(!s||s!=bound||!IsMain()||!lua_istable(L,1))Fail("native_entity_resolver_context");
  lua_getfield(L,1,"status");const std::string status=lua_tostring(L,-1)?lua_tostring(L,-1):"";lua_pop(L,1);
  if(status=="Null")nil=true;
  else{
   if(status!="KnownCurrent")Fail("native_entity_resolver_status");lua_getfield(L,1,"target");
   auto number=[&](const char *key){lua_getfield(L,-1,key);const char *p=lua_tostring(L,-1);if(!p)Fail("native_entity_resolver_key");const auto n=std::stoull(p);lua_pop(L,1);return n;};
   const auto run=number("run"),space=number("space"),id=number("id");lua_pop(L,1);
   if(run!=EntityKeyNumber(s->target["scope"],"run"))Fail("native_entity_resolver_scope");
   lua_getfield(L,1,"shape_revision");const char *p=lua_tostring(L,-1);if(!p)Fail("native_entity_resolver_shape");const auto shape=std::stoull(p);lua_pop(L,1);
   if(space==1){if(s->entity_shapes.at(id)!=shape)Fail("native_entity_resolver_shape");object=EntityPointer(*s,id);category=&s->entity_categories.at(id);}
   else if(space==2){const auto &o=Object(s->target,id);if(EntityKeyNumber(At(o,"subject"),"shape_revision")!=shape)Fail("native_entity_resolver_grid_shape");object=s->grids.at(id);grid=true;}
   else Fail("native_entity_resolver_space");
  }
 }catch(...){failed=true;}
 // lua_error会longjmp；这里没有跨越该调用的C++拥有型临时对象。
 if(failed){lua_pushliteral(L,"native_entity_reference_unavailable");return lua_error(L);}
 if(comparing){void *actual=nullptr;bool comparable=lua_isnil(L,2);
  if(lua_type(L,2)==LUA_TUSERDATA&&lua_rawlen(L,2)==sizeof(lua::luabridge::UserdataPtr)){actual=lua::UserdataToData<void *>(lua_touserdata(L,2));comparable=true;}
  lua_pushboolean(L,comparable&&actual==object);return 1;}
 if(nil)lua_pushnil(L);else lua::luabridge::UserdataPtr::push(L,object,grid?lua::Metatables::GRID_ENTITY:EntityMetatable(*category));
 return 1;
}
int entity_adapter_ref=LUA_NOREF;
void EntityAdapter(lua_State *L,const char *method){
 if(entity_adapter_ref==LUA_NOREF){const char *code=
#include "native_entity_lua.inc"
  ;if(luaL_loadbuffer(L,code,std::strlen(code),"IsaacCS.NativeEntity")!=LUA_OK||lua_pcall(L,0,1,0)!=LUA_OK)Fail("native_entity_adapter_load");entity_adapter_ref=luaL_ref(L,LUA_REGISTRYINDEX);}
 lua_rawgeti(L,LUA_REGISTRYINDEX,entity_adapter_ref);lua_getfield(L,-1,method);lua_remove(L,-2);
 if(!lua_isfunction(L,-1))Fail("native_entity_adapter_method");
}
// 整个component只压入Lua一次建立graph索引，避免每个实体重复复制整张世界值图。
struct EntityLuaValues final {
 lua_State *L=nullptr;int graph=LUA_NOREF;
 explicit EntityLuaValues(Scene &s){L=g_LuaEngine?g_LuaEngine->_state:nullptr;if(!L)Fail("native_entity_lua_state");const int top=lua_gettop(L);
  try{EntityAdapter(L,"Index");Push(L,EntityValues(s.target));if(lua_pcall(L,1,1,0)!=LUA_OK)Fail("native_entity_graph_index");graph=luaL_ref(L,LUA_REGISTRYINDEX);lua_settop(L,top);}
  catch(...){lua_settop(L,top);throw;}}
 ~EntityLuaValues(){if(graph!=LUA_NOREF)luaL_unref(L,LUA_REGISTRYINDEX,graph);}
 void Invoke(Scene &s,Entity *entity,const V &o,const char *method){const int top=lua_gettop(L);
  // common必须使用Entity元表；Knife.IsFlying是独立投掷状态，不能遮蔽公共Entity.IsFlying的碰撞类别语义。
  try{EntityAdapter(L,method);lua::luabridge::UserdataPtr::push(L,entity,lua::Metatables::ENTITY);Push(L,o);
   lua_pushlightuserdata(L,&s);lua_pushcclosure(L,ResolveNativeEntityLua,1);lua_rawgeti(L,LUA_REGISTRYINDEX,graph);
   if(lua_pcall(L,4,0,0)!=LUA_OK){const char *p=lua_tostring(L,-1);const std::string error=p?p:"native_entity_state";lua_settop(L,top);throw std::runtime_error(error);}lua_settop(L,top);
  }catch(...){lua_settop(L,top);throw;}}
 void Apply(Scene &s,Entity *entity,const V &o){Invoke(s,entity,o,"State");}
 void Verify(Scene &s,Entity *entity,const V &o){Invoke(s,entity,o,"Verify");}
};
void EntitySprite(Entity *e,const std::string &category,const char *member,const V &pose,int parameter=-1){
 auto *L=g_LuaEngine->_state;const int top=lua_gettop(L);ANM2 *sprite=nullptr;
 try{EntityAdapter(L,"Sprite");lua::luabridge::UserdataPtr::push(L,e,EntityMetatable(category));lua_newtable(L);
  lua_pushlstring(L,category.data(),category.size());lua_setfield(L,-2,"class");lua_pushstring(L,member);lua_setfield(L,-2,"member");
  if(parameter>=0){lua_pushinteger(L,parameter);lua_setfield(L,-2,"index");}
  if(lua_pcall(L,2,1,0)!=LUA_OK)Fail("native_entity_sprite_getter");sprite=lua::GetLuabridgeUserdata<ANM2 *>(L,-1,lua::Metatables::SPRITE,"Sprite");lua_settop(L,top);
  if(!sprite)Fail("native_entity_sprite_missing");Sprite(*sprite,pose);
 }catch(...){lua_settop(L,top);throw;}
}
// Familiar.Init 60921A内联构造的Body只含原生vptr、五份ANM2和四个层ID。
// 复现这一已核定构造序列并交给430E40原生智能引用owner；不进入Familiar.Init的服装/玩法选择。
void EntityBody(Entity *e,const V &native){
 if(e->_type!=3||e->_variant!=48)return;void *body=nullptr;std::memcpy(&body,reinterpret_cast<char *>(e)+0xecc,4);
 if(!Flag(At(native,"isaacs_body"))){if(body)Fail("native_familiar_body_rebuild_required");return;}
 if(!body){body=EntityAllocate(0x578);std::memset(body,0,0x578);*static_cast<std::uintptr_t *>(body)=Base+0x714f84;unsigned constructed=0;
  try{for(;constructed<5;++constructed){auto *sprite=reinterpret_cast<ANM2 *>(static_cast<char *>(body)+(constructed?0x118+(constructed-1)*0x114:4));sprite->constructor();}
   reinterpret_cast<void *(__thiscall *)(void *,void *)>(Base+0x30e40)(reinterpret_cast<char *>(e)+0xecc,body);
  }catch(...){while(constructed){--constructed;reinterpret_cast<ANM2 *>(static_cast<char *>(body)+(constructed?0x118+(constructed-1)*0x114:4))->destructor();}EntityFree(body);throw;}
 }
 if(*static_cast<std::uintptr_t *>(body)!=Base+0x714f84)Fail("native_familiar_body_class");
 const auto &ids=At(native,"body_ids");if(!ids.IsArray()||ids.Size()!=4)Fail("native_familiar_body_layers");
 for(unsigned i=0;i<5;++i){const auto name=std::string("isaacs_body_")+std::to_string(i);
  Sprite(*reinterpret_cast<ANM2 *>(static_cast<char *>(body)+(i?0x118+(i-1)*0x114:4)),At(At(native,"sprites"),name.c_str()));}
 for(unsigned i=0;i<4;++i){const auto id=Int(ids[i]);const auto *sprite=reinterpret_cast<ANM2 *>(static_cast<char *>(body)+0x118+i*0x114);
  if(id<-1||id>=std::int64_t(sprite->_layerCount))Fail("native_familiar_body_layer");const int value=int(id);std::memcpy(static_cast<char *>(body)+0x568+i*4,&value,4);}
}
void EntityAbyss(Scene &s,Entity *e,const V &native);
void EntityPoses(Scene &s,Entity *e,const V &o){
 EntitySprite(e,"Entity","GetSprite",o["common"]["fields"]["Entity.GetSprite"]);
 if(o.HasMember("native_render")){const auto &native=At(o,"native_render");if(Int(At(native,"version"))!=1)Fail("native_entity_render_version");
  if(e->_type==5)Sprite(static_cast<Entity_Pickup *>(e)->_altPedestalANM2,At(At(native,"sprites"),"alternate_pedestal"));}
 if(o.HasMember("native_render"))EntityBody(e,o["native_render"]);
 if(o.HasMember("native_render"))EntityAbyss(s,e,o["native_render"]);
 const auto cls=Str(o["class"]);if(cls=="EntityPlayer"||cls=="NativeText"||cls=="NativeDummy")return;
 const auto &f=o["body"]["fields"];
 const std::map<std::string,std::vector<const char *>> sprites={{"EntityTear",{"GetDeadEyeSprite","GetTearEffectSprite","GetTearHaloSprite"}},{"EntityPickup",{"GetPriceSprite"}},{"EntitySlot",{"GetPrizeSprite"}}};
 if(auto i=sprites.find(cls);i!=sprites.end())for(auto member:i->second)EntitySprite(e,cls,member,At(f,(cls+"."+member).c_str()));
 if(cls=="EntityBomb")for(const auto &value:At(At(f,"EntityBomb.GetCostumeLayerSprite"),"values").GetArray()){
  const auto &p=At(value,"parameters");const int index=p.IsArray()?int(Int(p[0])):int(Int(At(At(p,"entries")[0],"value")));
  if(index<0||index>=5)Fail("native_bomb_costume_layer");EntitySprite(e,cls,"GetCostumeLayerSprite",At(value,"value"),index);}
}
// 只写已核定只读成员的真实决定状态；不调用Update、Add状态效果、随机或Morph。
template<class T> void EntityStore(Entity *e,unsigned offset,const T &value){std::memcpy(reinterpret_cast<char *>(e)+offset,&value,sizeof(value));}
Entity *NativeEntityReference(Scene &s,const V &v);
void NativeEntityCommon(Entity *e,const V &o){const auto &f=o["common"]["fields"];
 const auto frame=std::int64_t(g_Game->_frameCount)-Int(f["Entity.FrameCount"]);if(frame<INT32_MIN||frame>INT32_MAX)Fail("native_entity_birth_frame");
 EntityStore(e,0x328,int(frame));EntityStore(e,0x32c,int(Int(f["Entity.SpawnGridIndex"])));
 EntityStore(e,0x3dc,std::uint32_t(Int(f["Entity.DropSeed"])));e->_posOffset=Vec(f["Entity.PositionOffset"]);
 e->_exists=Flag(f["Entity.Exists"]);e->_dead=Flag(f["Entity.IsDead"]);e->_visible=Flag(f["Entity.Visible"]);
 // Scale等Setter可能维护公共碰撞量；最后再覆盖权威当前值，普通change不重跑Init。
 e->_pos=Vec(f["Entity.Position"]);e->_velocity=Vec(f["Entity.Velocity"]);e->_size=Num(f["Entity.Size"]);
 e->_mass=Num(f["Entity.Mass"]);e->_collisionDamage=Num(f["Entity.CollisionDamage"]);
 e->_entityGridCollisionClass=int(Int(f["Entity.GridCollisionClass"]));e->_entityCollisionClass=int(Int(f["Entity.EntityCollisionClass"]));
 e->_collidesWithGrid=Flag(At(f,"Entity.CollidesWithGrid"));
 if(Str(o["class"])=="EntityTear"){
  const auto &t=o["body"]["fields"];auto *tear=static_cast<Entity_Tear *>(e);
  tear->_baseDamage=Num(t["EntityTear.BaseDamage"]);tear->_baseScale=Num(t["EntityTear.BaseScale"]);tear->_tearIdx=int(Int(t["EntityTear.TearIndex"]));
  tear->_deadEyeIntensity=Num(At(t,"EntityTear.GetDeadEyeIntensity"));
  EntityStore(e,0x7ec,Vec(t["EntityTear.PosDisplacement"]));
 }
}
void RefreshEntityPlayerContext(Scene &s){
 if(bound!=&s||g_Game->_entityFactory!=s.entity_factory.root)Fail("native_player_context_owner");
 auto **first=s.player_roster.empty()?nullptr:s.player_roster.data();auto **last=first?first+s.player_roster.size():nullptr;
 Entity_Player **header[]={first,last,last};std::memcpy(&g_Game->_playerManager._playerList,header,sizeof(header));std::memcpy(g_Game->_playerManager._esauJrState,s.player_esau,sizeof(s.player_esau));
}
// 未发布的最小Player是完整原生构造对象；只作必要Owner载体，完整角色状态由20扩展同一key。
void CreateEntityPlayer(Scene &s,std::uint64_t id,const V &o){
 auto *p=static_cast<Entity_Player *>(EntityAllocate(0x2ea0));std::memset(p,0,0x2ea0);
 try{p->constructor();}catch(...){EntityFree(p);throw;}
 try{s.entity_players.emplace(id,p);}catch(...){p->_valid=p->_exists=false;p->destructor();EntityFree(p);throw;}
 s.entity_shapes[id]=EntityKeyNumber(o["subject"],"shape_revision");s.entity_categories[id]="EntityPlayer";
 const auto &f=o["common"]["fields"];p->Entity::Original_Init(unsigned(Int(f["Entity.Type"])),unsigned(Int(f["Entity.Variant"])),unsigned(Int(f["Entity.SubType"])),unsigned(Int(f["Entity.InitSeed"])));
}
// 游戏MSVC vector的大块分配使用32字节对齐及前置原始指针；不能把DLL vector缓冲交给游戏析构。
void *EntityVectorAllocate(std::size_t bytes){if(!bytes)return nullptr;if(bytes<4096)return EntityAllocate(bytes);
 auto *raw=EntityAllocate(bytes+35);const auto address=(reinterpret_cast<std::uintptr_t>(raw)+35)&~std::uintptr_t(31);
 auto *out=reinterpret_cast<void *>(address);reinterpret_cast<void **>(out)[-1]=raw;return out;
}
void EntityVectorFree(void *buffer,std::size_t bytes){if(!buffer)return;void *raw=buffer;
 if(bytes>=4096){raw=reinterpret_cast<void **>(buffer)[-1];const auto a=reinterpret_cast<std::uintptr_t>(buffer),b=reinterpret_cast<std::uintptr_t>(raw);
  if(a<b+4||a-b-4>31)Fail("native_entity_vector_allocator");}EntityFree(raw);
}
template<class T> void EntityVector(Scene &s,void *member,const std::vector<T> &values){
 T *old[3]{};std::memcpy(old,member,sizeof(old));const auto begin=reinterpret_cast<std::uintptr_t>(old[0]),end=reinterpret_cast<std::uintptr_t>(old[1]),cap=reinterpret_cast<std::uintptr_t>(old[2]);
 if(end<begin||cap<end||(end-begin)%sizeof(T)||(cap-begin)%sizeof(T)||cap-begin>32u<<20)Fail("native_entity_vector_header");
 const auto bytes=values.size()*sizeof(T),previous=cap-begin;
 const auto ledger=s.entity_vector_capacities.find(member);const auto accounted=ledger==s.entity_vector_capacities.end()?0:ledger->second;
 if(bytes>32u<<20||s.entity_vector_bytes-accounted+bytes>32u<<20)Fail("native_entity_vector_budget");
 auto *data=static_cast<T *>(EntityVectorAllocate(bytes));if(bytes)std::memcpy(data,values.data(),bytes);
 T *header[]={data,data?data+values.size():nullptr,data?data+values.size():nullptr};std::memcpy(member,header,sizeof(header));
 EntityVectorFree(old[0],previous);s.entity_vector_bytes=s.entity_vector_bytes-accounted+bytes;s.entity_vector_capacities[member]=bytes;
}
// AbyssLocustData原生内联构造60904F；Client仅恢复Renderer读取的前/后两组Sprite。
// 原生玩法关系树保持合法空sentinel，未借用Authority owner或本端玩家的链表。
void EntityAbyss(Scene &s,Entity *e,const V &native){if(e->_type!=3||e->_variant!=231)return;
 void *data=nullptr;std::memcpy(&data,reinterpret_cast<char *>(e)+0xecc,4);
 if(!data){data=EntityAllocate(0x58);std::memset(data,0,0x58);*static_cast<std::uintptr_t *>(data)=Base+0x71510c;void *sentinel=nullptr;
  try{sentinel=EntityAllocate(0x18);std::memset(sentinel,0,0x18);for(unsigned i=0;i<3;++i)static_cast<void **>(sentinel)[i]=sentinel;
   const unsigned short flags=0x101;std::memcpy(static_cast<char *>(sentinel)+12,&flags,2);std::memcpy(static_cast<char *>(data)+0x1c,&sentinel,4);
   reinterpret_cast<void *(__thiscall *)(void *,void *)>(Base+0x30e40)(reinterpret_cast<char *>(e)+0xecc,data);
  }catch(...){EntityFree(sentinel);EntityFree(data);throw;}
 }
 if(*static_cast<std::uintptr_t *>(data)!=Base+0x71510c)Fail("native_familiar_abyss_class");
 for(unsigned side=0;side<2;++side){const auto count=Int(At(native,side?"abyss_after":"abyss_before"));if(count<0||count>256)Fail("native_familiar_abyss_budget");
  auto *member=static_cast<char *>(data)+0x24+side*12;ANM2 *old[3]{};std::memcpy(old,member,sizeof(old));
  const auto begin=reinterpret_cast<std::uintptr_t>(old[0]),end=reinterpret_cast<std::uintptr_t>(old[1]),cap=reinterpret_cast<std::uintptr_t>(old[2]);
  if(end<begin||cap<end||(end-begin)%0x114||(cap-begin)%0x114||(cap-begin)/0x114>256)Fail("native_familiar_abyss_vector");
  const auto ledger=s.entity_vector_capacities.find(member);const auto previous=ledger==s.entity_vector_capacities.end()?0:ledger->second;
  const auto bytes=std::size_t(count)*0x114;if(s.entity_vector_bytes-previous+bytes>32u<<20)Fail("native_entity_vector_budget");
  if(end-begin==bytes){for(unsigned i=0;i<unsigned(count);++i){const auto name=std::string(side?"abyss_after_":"abyss_before_")+std::to_string(i);
    Sprite(*reinterpret_cast<ANM2 *>(begin+i*0x114),At(At(native,"sprites"),name.c_str()));}continue;}
  auto *next=static_cast<char *>(EntityVectorAllocate(bytes));unsigned constructed=0;
  try{for(;constructed<unsigned(count);++constructed)reinterpret_cast<ANM2 *>(next+constructed*0x114)->constructor();
   for(unsigned i=0;i<unsigned(count);++i){const auto name=std::string(side?"abyss_after_":"abyss_before_")+std::to_string(i);Sprite(*reinterpret_cast<ANM2 *>(next+i*0x114),At(At(native,"sprites"),name.c_str()));}
  }catch(...){while(constructed)reinterpret_cast<ANM2 *>(next+--constructed*0x114)->destructor();EntityVectorFree(next,bytes);throw;}
  ANM2 *header[]={reinterpret_cast<ANM2 *>(next),reinterpret_cast<ANM2 *>(next?next+bytes:nullptr),reinterpret_cast<ANM2 *>(next?next+bytes:nullptr)};std::memcpy(member,header,sizeof(header));
  for(auto cursor=end;cursor>begin;){cursor-=0x114;reinterpret_cast<ANM2 *>(cursor)->destructor();}EntityVectorFree(old[0],cap-begin);
  s.entity_vector_bytes=s.entity_vector_bytes-previous+bytes;s.entity_vector_capacities[member]=bytes;
 }
}
const V *GraphNode(const V &v,std::int64_t node){
 if(v.IsObject()){if(v.HasMember("type")&&v["type"]=="table"&&Int(At(v,"node"))==node)return &v;
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)if(auto *found=GraphNode(i->value,node))return found;}
 if(v.IsArray())for(const auto &e:v.GetArray())if(auto *found=GraphNode(e,node))return found;return nullptr;
}
const V &EntityTable(const Scene &s,const V &v){
 if(At(v,"type")=="table")return At(v,"entries");
 if(v["type"]=="graph_ref"){auto *found=GraphNode(EntityValues(s.target),Int(At(v,"node")));if(found)return At(*found,"entries");}
 Fail("native_entity_table");
}
void NativeEntityPlayer(Scene &s,Entity *e,const V &o){const auto &f=o["body"]["fields"];
 // J273原生GetPlayerType=4931A0读取+1368，SDK旧头的+13BC在本目标不适用。
 EntityStore(e,0x1368,int(Int(At(f,"EntityPlayer.GetPlayerType"))));
 const auto &entries=EntityTable(s,At(f,"EntityPlayer.GetCollectiblesList"));std::vector<int> inventory(1,0);
 for(const auto &entry:entries.GetArray()){const auto id=Int(At(entry,"key")),count=Int(At(entry,"value"));
  if(id<1||id>=8192||count<0||count>INT32_MAX)Fail("native_player_inventory_budget");if(inventory.size()<=std::size_t(id))inventory.resize(std::size_t(id)+1);inventory[std::size_t(id)]=int(count);}
 EntityVector(s,reinterpret_cast<char *>(e)+0x16b8,inventory);
}
void NativeEntityText(Entity *e,const V &o){const auto &f=o["body"]["fields"];
 EntityStore(e,0x410,std::uint32_t(Int(At(f,"NativeText.counter"))));EntityStore(e,0x414,std::uint32_t(Int(At(f,"NativeText.duration"))));
 std::string text;const auto &value=At(f,"NativeText.text");if(value.IsString())text=Str(value);
 else{const auto hex=Str(At(value,"hex"));for(std::size_t i=0;i<hex.size();i+=2)text.push_back(char(std::stoul(hex.substr(i,2),nullptr,16)));}
 if(text.size()>255)Fail("native_text_size");std::memcpy(reinterpret_cast<char *>(e)+0x418,text.data(),text.size());reinterpret_cast<char *>(e)[0x418+text.size()]=0;
 KColor color(Num(At(f,"NativeText.red")),Num(At(f,"NativeText.green")),Num(At(f,"NativeText.blue")),Num(At(f,"NativeText.alpha")),int(Int(At(f,"NativeText.color_word"))));EntityStore(e,0x518,color);
}
// 公共Lua没有Setter的最终后态使用官方接口实现中的具名SDK成员，不调用派生Init或玩法维护。
Entity *NativeEntityReference(Scene &s,const V &v){
 if(At(v,"status")=="Null")return nullptr;if(v["status"]!="KnownCurrent")Fail("native_entity_reference_state");
 return EntityPointer(s,EntityKeyNumber(At(v,"target"),"id"));
}
bool NativeEntityOwned(const Scene &s,const Entity *entity){
 for(const auto &entry:s.entities)if(entry.second==entity)return true;for(const auto &entry:s.entity_players)if(entry.second==entity)return true;return false;
}
// ClearReferences会Remove子实体；先使用原生EntityPtr接口解除全部槽，销毁weapon时不重演该玩法动作。
void DestroyEntityWeapon(Scene &s,Weapon **slot){if(!*slot)return;auto *weapon=*slot;const auto layout=EntityWeaponLayout(weapon);
 if(!NativeRoomGridContextActive()||!NativeEntityOwned(s,weapon->_owner)||(weapon->_owner->_type!=1&&weapon->_owner->_type!=3))Fail("native_weapon_owner");
 for(unsigned i=0;i<layout.count;++i){auto *reference=reinterpret_cast<EntityPtr *>(reinterpret_cast<char *>(weapon)+layout.offsets[i]);
  if(reference->GetReference()&&!NativeEntityOwned(s,reference->GetReference()))Fail("native_weapon_foreign_reference");reference->SetReference(nullptr);}
 Isaac::DestoryWeapon(slot);
}
void NativeEntityWeapon(Scene &s,Entity_Familiar *familiar,const V &o){const auto &f=o["body"]["fields"],&state=At(f,"EntityFamiliar.GetWeapon");
 const auto &native=At(At(o,"native_render"),"weapon");const bool present=Flag(At(native,"present"));
 if(!present){if(!state.IsObject()||At(state,"status")!="Null")Fail("native_weapon_source_presence");DestroyEntityWeapon(s,&familiar->_weapon);return;}
 const auto &w=At(state,"fields");const auto type=Int(At(w,"Weapon.GetWeaponType"));if(type<0||type>=16)Fail("native_weapon_type");
 if(familiar->_weapon&&familiar->_weapon->_weaponType!=type)DestroyEntityWeapon(s,&familiar->_weapon);
 if(!familiar->_weapon){familiar->_weapon=Isaac::CreateWeapon(static_cast<WeaponType>(type),familiar);if(!familiar->_weapon)Fail("native_weapon_factory_unavailable");}
 auto *weapon=familiar->_weapon;if(weapon->_weaponType!=type)Fail("native_weapon_factory_binding");const auto layout=EntityWeaponLayout(weapon);
 const auto &references=At(native,"refs");if(Str(At(native,"layout"))!=layout.name||!references.IsArray()||references.Size()!=layout.count)Fail("native_weapon_reference_shape");
 weapon->_owner=NativeEntityReference(s,At(w,"Weapon.GetOwner"));if(!weapon->_owner||(weapon->_owner->_type!=1&&weapon->_owner->_type!=3))Fail("native_weapon_owner_class");
 weapon->_fireDelay=Num(At(w,"Weapon.GetFireDelay"));weapon->_maxFireDelay=Num(At(w,"Weapon.GetMaxFireDelay"));weapon->_charge=Num(At(w,"Weapon.GetCharge"));
 weapon->_direction=Vec(At(w,"Weapon.GetDirection"));weapon->_modifiers=unsigned(Int(At(w,"Weapon.GetModifiers")));weapon->_numFired=int(Int(At(w,"Weapon.GetNumFired")));
 for(unsigned i=0;i<layout.count;++i)reinterpret_cast<EntityPtr *>(reinterpret_cast<char *>(weapon)+layout.offsets[i])->SetReference(NativeEntityReference(s,references[i]));
 if(weapon->GetMainEntity()!=NativeEntityReference(s,At(w,"Weapon.GetMainEntity")))Fail("native_weapon_main_reference");
}
void NativeEntityDerived(Scene &s,Entity *e,const V &o){const auto cls=Str(o["class"]);
 if(cls=="NativeText"){NativeEntityText(e,o);return;}if(cls=="NativeDummy")return;
 const auto &f=o["body"]["fields"];
 if(cls=="EntityBomb"){auto *b=static_cast<Entity_Bomb *>(e);
  b->_explosionCountdown1=int(Int(At(f,"EntityBomb.GetExplosionCountdown")));
  b->_loadCostumes=Flag(At(f,"EntityBomb.IsLoadingCostumes"));b->_prismTouched=Flag(At(f,"EntityBomb.IsPrismTouched"));
 }else if(cls=="EntityKnife"){auto *k=static_cast<Entity_Knife *>(e);
  EntityStore(e,0x434,Num(At(At(o,"native_render"),"knife_sort_z")));
  // 原版Knife Lua表851D8E绑定606C10，读+464；与Entity.IsFlying/相邻+465 swinging分别恢复。
  EntityStore(e,0x464,Flag(At(f,"EntityKnife.IsFlying")));
  k->_prismApplied=Flag(At(f,"EntityKnife.IsPrismTouched"));k->_multidimensionalApplied=Flag(At(f,"EntityKnife.IsMultidimensionalTouched"));
 }else if(cls=="EntityLaser"){auto *l=static_cast<Entity_Laser *>(e);
  l->_sampleLaser=Flag(At(f,"EntityLaser.IsSampleLaser"));l->_multidimensionalTouched=Flag(At(f,"EntityLaser.IsMultidimensionalTouched"));l->_prismTouched=Flag(At(f,"EntityLaser.IsPrismTouched"));
  auto samples=[&](const char *name,void *member){const auto &source=At(f,name);const auto &v=source.IsArray()?source:At(source,"values");if(!v.IsArray()||v.Size()>4096)Fail("native_laser_samples_budget");
   std::vector<Vector> points;points.reserve(v.Size());for(const auto &point:v.GetArray())points.push_back(Vec(point));EntityVector(s,member,points);};
  samples("EntityLaser.GetSamples",reinterpret_cast<char *>(l)+0x480);
  samples("EntityLaser.GetNonOptimizedSamples",reinterpret_cast<char *>(l)+0x48c);
  l->_homingLaser._entity=e;l->_homingLaser._origin=e->_pos;l->_homingLaser._endPoint=Vec(At(f,"EntityLaser.GetEndPoint"));
 }else if(cls=="EntityFamiliar"){auto *fam=static_cast<Entity_Familiar *>(e);
  fam->_isLilDelirium=Flag(At(f,"EntityFamiliar.IsLilDelirium"));fam->_dirtColor=Color(At(f,"EntityFamiliar.GetDirtColor"));
  const auto &config=At(f,"EntityFamiliar.GetItemConfig");
  if(config.IsObject()&&config.HasMember("status")&&config["status"]=="Null")fam->_item=nullptr;
  else{const auto &item=At(config,"fields");const auto id=int(Int(At(item,"ItemConfigItem.ID")));
   fam->_item=Flag(At(item,"ItemConfigItem.IsTrinket"))?g_Manager->_itemConfig.GetTrinket(id):g_Manager->_itemConfig.GetCollectible(id);if(!fam->_item)Fail("native_familiar_item_config");}
  if(e->_variant==240)reinterpret_cast<EntityPtr *>(reinterpret_cast<char *>(e)+0xdac)->SetReference(NativeEntityReference(s,At(At(o,"native_render"),"familiar_child")));
  // 官方1.1.3 Lua Getter直接读这个真实缓存；恢复缓存后态，不重算Owner装备/伤害或重新执行GetMultiplier。
  // EntityPlusHolder由REPENTOGON Entity ctor/Init拥有，成员只有一个unique_ptr；此处仅在本方Binding内借用。
  // Win32官方EntityPlus前缀：vptr、optional<bool>、optional<uint32_t>，派生缓存位于+10/+18。
  // 只借用缓存字节，避免引入ItemSpoofSystem头所携带的全局配置初始化；不构造/析构跨模块C++对象。
  struct FamiliarPlusView {void *vtable;std::optional<bool> flying;std::optional<std::uint32_t> water;std::optional<float> multiplier;std::uintptr_t player;};
  static_assert(offsetof(FamiliarPlusView,multiplier)==0x10&&offsetof(FamiliarPlusView,player)==0x18&&sizeof(FamiliarPlusView)==0x1c,"REPENTOGON familiar cache ABI");
  const auto holder=(std::uintptr_t(e->_entityExtraDataPtrUpperBytes)<<16)|e->_entityExtraDataPtrLowerBytes;void *plus=nullptr;
  if(!holder)Fail("native_familiar_plus_holder");std::memcpy(&plus,reinterpret_cast<const void *>(holder),sizeof(plus));
  if(!plus||e->_type!=3)Fail("native_familiar_plus_class");
  const std::optional<float> multiplier=Num(At(f,"EntityFamiliar.GetMultiplier"));const auto player=reinterpret_cast<std::uintptr_t>(fam->_player);
  std::memcpy(static_cast<char *>(plus)+offsetof(FamiliarPlusView,multiplier),&multiplier,sizeof(multiplier));
  std::memcpy(static_cast<char *>(plus)+offsetof(FamiliarPlusView,player),&player,sizeof(player));
 }else if(cls=="EntityNPC"){auto *npc=static_cast<Entity_NPC *>(e);
  EntityStore(e,0xc63,Flag(At(f,"EntityNPC.IsChampion")));EntityStore(e,0xc64,int(Int(At(f,"EntityNPC.GetChampionColorIdx"))));
  npc->_dirtColor=Color(At(f,"EntityNPC.GetDirtColor"));npc->_championRegenTimer=static_cast<unsigned short>(Int(At(f,"EntityNPC.GetDarkRedChampionRegenTimer")));
  npc->_bosscoloridx=int(Int(At(f,"EntityNPC.GetBossColorIdx")));npc->_isBoss=Flag(At(f,"EntityNPC.IsBoss"));
  npc->_sirenPlayerEntity=static_cast<Entity_Player *>(NativeEntityReference(s,At(f,"EntityNPC.GetSirenPlayerEntity")));
  if(o.HasMember("special")){const auto &d=At(o["special"],"fields");npc->_deliriumBossType=unsigned(Int(At(d,"EntityDelirium.BossType")));npc->_deliriumBossVariant=unsigned(Int(At(d,"EntityDelirium.BossVariant")));}
 }else if(cls=="EntityPickup"){auto *p=static_cast<Entity_Pickup *>(e);
  p->_pickupGhost=static_cast<Entity_Effect *>(NativeEntityReference(s,At(f,"EntityPickup.GetPickupGhost")));
  p->_megaChestCollectible[0]=static_cast<Entity_Pickup *>(NativeEntityReference(s,At(f,"EntityPickup.GetMegaChestLeftCollectible")));
  p->_megaChestCollectible[1]=static_cast<Entity_Pickup *>(NativeEntityReference(s,At(f,"EntityPickup.GetMegaChestRightCollectible")));
  const auto &cycle=EntityTable(s,At(f,"EntityPickup.GetCollectibleCycle"));if(cycle.Size()>8)Fail("native_pickup_cycle_budget");
  std::fill(std::begin(p->_cycleCollectibleList),std::end(p->_cycleCollectibleList),0);p->_cycleCollectibleCount=cycle.Size();
  for(const auto &entry:cycle.GetArray()){const auto index=Int(At(entry,"key"));if(index<1||index>cycle.Size())Fail("native_pickup_cycle_index");p->_cycleCollectibleList[index-1]=unsigned(Int(At(entry,"value")));}
  const bool flip=Flag(At(f,"EntityPickup.HasFlipData"));
  if(!flip)p->_flipSaveState.SetP(nullptr);
  else{if(!p->_flipSaveState.saveState){auto *state=static_cast<EntitySaveState *>(EntityAllocate(sizeof(EntitySaveState)));std::memset(state,0,sizeof(EntitySaveState));
    state->constructor();p->_flipSaveState.SetP(state);}
   auto *state=p->_flipSaveState.saveState;state->type=e->_type;state->variant=e->_variant;state->subtype=int(Int(At(f,"EntityPickup.GetFlipCollectible")));}
 }else if(cls=="EntityEffect"&&e->_variant==136){
  // 搬起Grid描述归Effect所有；同窗口Grid component已携带完整值，不能伪造来源活动槽。
  const auto &lifted=At(At(At(s.target["components"],"grid"),"state"),"lifted_descriptors");bool found=false;
  for(const auto &v:lifted.GetArray())if(EntityKeyNumber(At(At(v,"owner"),"target"),"id")==ObjectId(o)){
   const auto &d=At(At(v,"descriptor"),"fields");GridEntityDesc desc{};
   desc._type=int(Int(At(d,"GridEntityDesc.Type")));desc._variant=int(Int(At(d,"GridEntityDesc.Variant")));desc._state=int(Int(At(d,"GridEntityDesc.State")));
   desc._spawnCount=int(Int(At(d,"GridEntityDesc.SpawnCount")));desc._spawnSeed=unsigned(Int(At(d,"GridEntityDesc.SpawnSeed")));desc._varData=int(Int(At(d,"GridEntityDesc.VarData")));
   desc._variableSeed=unsigned(Int(At(d,"GridEntityDesc.VariableSeed")));desc._initialized=Flag(At(d,"GridEntityDesc.Initialized"));EntityStore(e,0x420,desc);found=true;break;}
  if(!found)Fail("native_effect_lifted_descriptor");
 }
}
// Text没有Lua派生元表，正文及五个KColor分量须从实际私有对象读回。
void CheckNativeText(Entity *e,const V &o){if(o["class"]!="NativeText")return;const auto &f=o["body"]["fields"];
 auto word=[&](unsigned offset){std::uint32_t value=0;std::memcpy(&value,reinterpret_cast<char *>(e)+offset,4);return value;};
 if(word(0x410)!=std::uint32_t(Int(At(f,"NativeText.counter")))||word(0x414)!=std::uint32_t(Int(At(f,"NativeText.duration"))))Fail("native_text_readback_clock");
 std::string expected;const auto &v=At(f,"NativeText.text");if(v.IsString())expected=Str(v);else{const auto hex=Str(At(v,"hex"));for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(char(std::stoul(hex.substr(i,2),nullptr,16)));}
 const char *text=reinterpret_cast<char *>(e)+0x418;if(std::memcmp(text,expected.data(),expected.size())||text[expected.size()]!=0)Fail("native_text_readback_text");
 KColor color;std::memcpy(&color,reinterpret_cast<char *>(e)+0x518,sizeof(color));
 if(color._red!=Num(At(f,"NativeText.red"))||color._green!=Num(At(f,"NativeText.green"))||color._blue!=Num(At(f,"NativeText.blue"))||color._alpha!=Num(At(f,"NativeText.alpha"))||color._unk!=Int(At(f,"NativeText.color_word")))Fail("native_text_readback_color");
}
void CheckEntityLists(Scene &s,bool complete){
 std::set<Entity *> owned,owners,render;for(const auto &e:s.entities)owned.insert(e.second);
 for(unsigned i=0;i<8;++i){const auto &list=*reinterpret_cast<EntityList_EL *>(reinterpret_cast<char *>(&s.room->_entityList)+0x20+i*0x10);
  if(list._size>list._capacity||list._size>3776||(list._size&&!list._data))Fail("native_entity_list_budget");std::set<Entity *> seen;
  for(unsigned j=0;j<list._size;++j){auto *e=list._data[j];if(!owned.count(e)||!seen.insert(e).second)Fail("native_entity_foreign_list_member");
   if(i<2&&!owners.insert(e).second)Fail("native_entity_duplicate_owner");if(i==3)render.insert(e);}}
 if(complete&&(owners!=owned||render!=owned))Fail("native_entity_actual_membership");
}
void PublishNativeEntity(Scene &s,Entity *e){auto &list=s.room->_entityList;
 auto capacity=[](const EntityList_EL &v){if(v._size>=v._capacity)Fail("native_entity_publish_capacity");};
 const bool persistent=e->_type==1||e->_type==3||(e->_type==8&&!e->_subtype);capacity(persistent?list._persistentEL:list._mainEL);capacity(list._renderEL);
 if(!(e->_flags&8))capacity(list._effectEL);if(e->_type==3&&e->_variant==206)capacity(list._wispEL);
 // 取槽期间用valid预留；原生Add以valid区分首次发布，必须只在这一个确知窗口撤销预留。
 e->_valid=false;reinterpret_cast<std::uint32_t (__thiscall *)(EntityList *,Entity *)>(Base+0x13d00)(&list,e);
 if(!e->_valid)Fail("native_entity_publish_incomplete");
}
void ApplyNativeEntities(Scene &s,const NativeRoomGridTransaction &tx){if(!tx.has_entities)return;
 s.entity_values.clear();for(const auto &object:At(EntityValues(s.target),"objects").GetArray())s.entity_values.emplace(ObjectId(object),&object);
 s.entity_factory.Construct();
 for(auto id:tx.entity_player_dependencies)if(!s.entity_players.count(id))CreateEntityPlayer(s,id,EntityObject(s,id));
 s.player_roster.clear();for(auto id:tx.entity_player_roster)s.player_roster.push_back(s.entity_players.at(id));
 for(unsigned i=0;i<4;++i)s.player_esau[i]=tx.entity_player_esau[i]?s.entity_players.at(tx.entity_player_esau[i]):nullptr;RefreshEntityPlayerContext(s);
 std::vector<std::uint64_t> publish;
 for(auto id:tx.entity_order){const auto &o=EntityObject(s,id);const auto cls=Str(o["class"]);if(cls=="EntityPlayer")continue;
  if(s.entities.count(id))continue;const auto &f=o["common"]["fields"];auto *e=s.entity_factory.Take(cls,unsigned(Int(f["Entity.Type"])));
  s.entities.emplace(id,e);s.entity_shapes[id]=EntityKeyNumber(o["subject"],"shape_revision");s.entity_categories[id]=cls;
  // SDK Original_Init是非虚入口stub，指向被常驻Hook的6A4A20；它仍经过REPENTOGON EntityPlus.Init，不是super trampoline。
  e->Original_Init(unsigned(Int(f["Entity.Type"])),unsigned(Int(f["Entity.Variant"])),unsigned(Int(f["Entity.SubType"])),unsigned(Int(f["Entity.InitSeed"])));
  // 只分配本端Room ordinal；远端Index不进入本地池或排序容器。
  auto &ordinal=*reinterpret_cast<unsigned *>(reinterpret_cast<char *>(s.room)+0x1214);if(ordinal==UINT32_MAX)Fail("native_entity_ordinal_exhausted");e->_index=ordinal++;publish.push_back(id);
 }
 // Weapon.Free可能清本方Owner的显示计时；所有weapon维护先完成，再统一覆盖各身体/实体最终后态。
 for(auto id:tx.entity_order){const auto &o=EntityObject(s,id);if(o["class"]=="EntityFamiliar")NativeEntityWeapon(s,static_cast<Entity_Familiar *>(s.entities.at(id)),o);}
 EntityLuaValues values(s);
 for(auto id:tx.entity_player_dependencies){auto *e=s.entity_players.at(id);const auto &o=EntityObject(s,id);values.Apply(s,e,o);NativeEntityCommon(e,o);e->_minecart.SetReference(NativeEntityReference(s,At(o["common"]["fields"],"Entity.GetMinecart")));NativeEntityPlayer(s,e,o);EntityPoses(s,e,o);}
 for(auto id:tx.entity_order){const auto &o=EntityObject(s,id);if(o["class"]=="EntityPlayer")continue;auto *e=s.entities.at(id);
  if(o.HasMember("special")){const auto &d=At(o["special"],"fields");auto *npc=static_cast<Entity_NPC *>(e);
   // ToDelirium以这两个真实字段识别变形后的NPC，必须先于公开Delirium字段恢复成立。
   npc->_deliriumBossType=unsigned(Int(At(d,"EntityDelirium.BossType")));npc->_deliriumBossVariant=unsigned(Int(At(d,"EntityDelirium.BossVariant")));}
  values.Apply(s,e,o);NativeEntityCommon(e,o);e->_minecart.SetReference(NativeEntityReference(s,At(o["common"]["fields"],"Entity.GetMinecart")));NativeEntityDerived(s,e,o);EntityPoses(s,e,o);InjectFailure("entity_state");}
 for(const auto &entry:s.entities)s.entity_order_fragments[entry.second]=unsigned(Int(At(At(EntityObject(s,entry.first),"native_render"),"order_fragment")));
 for(auto id:publish)PublishNativeEntity(s,s.entities.at(id));CheckEntityLists(s,true);
 auto &render=s.room->_entityList._renderEL;if(render._size!=tx.entity_render_order.size())Fail("native_entity_render_order_membership");
 for(unsigned i=0;i<render._size;++i)render._data[i]=s.entities.at(tx.entity_render_order[i]);
 // 原生分区/排序/多pass保持游戏实现；仅补其纯数值键所需的源排序片段及输入次序。
 s.room->_entityList.render_sort();CheckEntityLists(s,true);
 // Statue自身Render为空；显示Entity只在Room列表绘制一次，Grid保留原生受管借用供生命周期使用。
 for(const auto &o:GridValues(s.target).GetArray())if(o["class"]=="GridEntityStatue"){
  auto *grid=s.grids.at(ObjectId(o));const auto &relations=At(o,"native_relations");if(Int(At(relations,"version"))!=1)Fail("native_grid_relation_version");
  reinterpret_cast<EntityPtr *>(reinterpret_cast<char *>(grid)+0x154)->SetReference(NativeEntityReference(s,At(relations,"display_entity")));
 }
}
void RetireNativeEntities(Scene &s){
 if(!s.entity_factory.root)return;
 CheckEntityLists(s,false);
 for(const auto &entry:s.entities)if(s.entity_categories.at(entry.first)=="EntityFamiliar")DestroyEntityWeapon(s,&static_cast<Entity_Familiar *>(entry.second)->_weapon);
 // 所有成员均属于本方私有Room/Factory；先撤销借用和显示资格，再由原生池析构完整收资源。
 if(s.entity_factory.constructed)for(const auto &pool:entity_pools)for(unsigned i=0;i<pool.count;++i){
  auto *entity=reinterpret_cast<Entity *>(static_cast<char *>(s.entity_factory.root)+pool.offset+i*pool.stride);
  // 也收回Take成功后、映射登记分配失败的预留槽；它仍是本方原生池的确定成员。
  if(entity->_valid){entity->ClearReferences();entity->_exists=false;entity->_valid=false;entity->_dead=true;}}
 for(const auto &entry:s.entity_players){entry.second->ClearReferences();entry.second->_exists=false;entry.second->_valid=false;}
 // 不重演派生Remove的拾取/效果逻辑。整体owner退出时撤销八个本方借用视图，缓冲仍交原生Room析构。
 for(unsigned i=0;i<8;++i){auto &list=*reinterpret_cast<EntityList_EL *>(reinterpret_cast<char *>(&s.room->_entityList)+0x20+i*0x10);
  list._size=0;
 }
 s.entity_shapes.clear();s.entity_categories.clear();s.entities.clear();
 s.entity_order_fragments.clear();
 s.player_roster.clear();std::fill(std::begin(s.player_esau),std::end(s.player_esau),nullptr);RefreshEntityPlayerContext(s);
}
