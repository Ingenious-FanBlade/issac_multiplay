// J273的Factory实际分配为0x6AA660，不能采用SDK过期的sizeof(EntityFactory)。
// 原生Game构造6ED4B0→6EEE60与析构6EF470的十二池表经静态复读；所有偏移为本方owner内相对值。
static_assert(sizeof(void *)==4&&sizeof(Vector)==8&&sizeof(ANM2)==0x114,"J273 passive resource ABI");
struct NativeEntityPool final {unsigned offset,count,stride,dtor;};
constexpr NativeEntityPool entity_pools[]={
 {0,1024,0x4c0,0x1ccfd0},{0x130008,64,0xf30,0x2070f0},
 {0x16cc10,64,0x9d8,0x29d630},{0x194218,512,0x478,0x24f4a0},
 {0x223220,512,0x848,0x262e20},{0x32c228,128,0x568,0x545c50},
 {0x357630,512,0x6f0,0x2dd6a0},{0x435638,128,0x550,0x259a70},
 {0x45fe40,512,0xf60,0x2b48a0},{0x64be48,128,0x530,0x1cc250},
 {0x675650,128,0x410,0x2a35c0},{0x695e58,64,0x520,0x539c20}};
void *EntityAllocate(std::size_t bytes){
 auto *p=reinterpret_cast<void *(__cdecl *)(unsigned)>(Base+0x686de5)(unsigned(bytes));
 if(!p)Fail("native_entity_allocation");return p;
}
void EntityFree(void *p){reinterpret_cast<void (__cdecl *)(void *)>(Base+0x68717b)(p);}
// 一次票据仅授权指定私有Factory的一次force=true取槽；Init、Render或Lua不能借用后续生成。
struct FactoryTicket final {
 void *owner=nullptr;unsigned type=0;bool available=false;
};
thread_local FactoryTicket factory_ticket;
bool ConsumeFactoryTicket(const void *owner,unsigned type,bool force){
 if(!factory_ticket.available||owner!=factory_ticket.owner||type!=factory_ticket.type||!force)return false;
 factory_ticket.available=false;return true;
}
struct NativeEntityFactory final {
 void *root=nullptr;bool constructed=false;
 // 申请与构造分开，使Binding能在首个原生构造之前接上本方owner及回调隔离。
 void Allocate(){if(!root){root=EntityAllocate(0x6aa660);std::memset(root,0,0x6aa660);}}
 void Construct(){if(constructed)return;if(!root||!NativeRoomGridContextActive()||g_Game->_entityFactory!=root)Fail("native_entity_factory_context");
  reinterpret_cast<void *(__thiscall *)(void *)>(Base+0x2eee60)(root);constructed=true;}
 const NativeEntityPool &Pool(const std::string &category)const{
  if(category=="EntityEffect")return entity_pools[0];if(category=="EntityFamiliar")return entity_pools[1];
  if(category=="EntityBomb")return entity_pools[2];if(category=="EntityProjectile")return entity_pools[3];
  if(category=="EntityTear")return entity_pools[4];if(category=="EntityLaser")return entity_pools[5];
  if(category=="EntityPickup")return entity_pools[6];if(category=="EntitySlot")return entity_pools[7];
  if(category=="EntityNPC")return entity_pools[8];if(category=="NativeText")return entity_pools[9];
  if(category=="NativeDummy")return entity_pools[10];if(category=="EntityKnife")return entity_pools[11];
  Fail("native_entity_pool_category");
 }
 bool Contains(const Entity *e,const NativeEntityPool &pool)const{
  const auto start=reinterpret_cast<std::uintptr_t>(root)+pool.offset,address=reinterpret_cast<std::uintptr_t>(e);
  return address>=start&&address<start+pool.count*pool.stride&&(address-start)%pool.stride==0;
 }
 Entity *Take(const std::string &category,unsigned type){
  if(!constructed||!NativeRoomGridContextActive()||g_Game->_entityFactory!=root)Fail("native_entity_factory_context");
  const auto &pool=Pool(category);Entity *entity=nullptr;
  if(category=="NativeDummy"){
   // 原生Create没有Dummy分支；使用已由原生Factory构造的专用池，而不请求错误的NPC载体。
   for(unsigned i=0;i<pool.count;++i){auto *e=reinterpret_cast<Entity *>(static_cast<char *>(root)+pool.offset+i*pool.stride);if(!e->_valid){entity=e;break;}}
  }else{
   if(factory_ticket.available)Fail("native_entity_nested_factory_ticket");factory_ticket={root,type,true};
   try{entity=static_cast<EntityFactory *>(root)->Create(type,true);factory_ticket={};}catch(...){factory_ticket={};throw;}
  }
  if(!entity)Fail("native_entity_pool_exhausted");if(!Contains(entity,pool)||entity->_valid)Fail("native_entity_factory_binding");
  // 发布之前即预留槽位，后续主体创建不能取得同一个尚未发布的地址。
  entity->_valid=true;return entity;
 }
 void Destroy(){if(!root)return;
  if(constructed){if(!NativeRoomGridContextActive()||g_Game->_entityFactory!=root)Fail("native_entity_factory_retire_context");
   // 复用原生Game析构的逆序pool析构；保留Entity的内部ESSM通知及所有原生资源析构。
   for(unsigned p=unsigned(std::size(entity_pools));p-->0;){const auto &pool=entity_pools[p];
    const auto destroy=reinterpret_cast<void (__thiscall *)(void *)>(Base+pool.dtor);
    for(unsigned i=pool.count;i-->0;)destroy(static_cast<char *>(root)+pool.offset+i*pool.stride);}
  }
  EntityFree(root);root=nullptr;constructed=false;
 }
};
