#include "world/collector.h"
#include "binding/world_layout.h"
#include <windows.h>
#include <climits>
namespace cs::world {
std::uint32_t Collector::BeforePlayerReplaceSource(std::uint32_t manager,std::uint32_t old_body,std::uint32_t replacement) noexcept {
  if(!Recording()||depth_>=calls_.size())return 0;
  Call pending{};pending.object=manager;pending.count=2;pending.args[0]=old_body;pending.args[1]=replacement;
  RememberPlayerReplaceState(pending);player_replace_before_control_[depth_]=pending.player_replace;
  RememberPlayerEsauReplacement(manager,old_body,replacement);
  const auto *old=identities_.Find(ObjectDomain::Entity,old_body),*next=identities_.Find(ObjectDomain::Entity,replacement);
  return (old?old->readiness:0)|((next?next->readiness:0)<<8)|(pending.player_replace.old_body?1u<<16:0);
}
// J273 ReplacePlayer令旧体离场/StoreGameState并清理武器、子引用及动态成员，不析构其Player。
// 证言只允许读取清理后的当前Null/空容器/值；从不复用旧字段或直接声明来源完整。
// 前后都核对真实manager/roster与双向backup；任一旧体Init/Restore/物理生命周期使证言失效。
bool Collector::PlayerReplaceRoster(const Call &call,bool completed) noexcept {
  const auto &w=call.player_replace;const auto &l=layout::LayoutConstants();std::uint32_t game=0,first=0,last=0,cap=0;
  if(!Value(std::uint64_t(memory_.image_base)+l.game_root_rva,game)||!game||call.object!=std::uint64_t(game)+l.game_player_manager_offset)return false;
  const auto roster=std::uint64_t(game)+l.game_player_manager_offset+l.player_manager_roster_offset;
  if(!Value(roster,first)||!Value(roster+4,last)||!Value(roster+8,cap)||last<first||cap<last||(last-first)%4||(cap-first)/4>256)return false;
  unsigned old_count=0,new_count=0;
  for(auto at=std::uint64_t(first);at<last;at+=4){std::uint32_t member=0;if(!Value(at,member))return false;old_count+=member==w.old_body;new_count+=member==w.new_body;}
  return completed?old_count==0&&new_count==1:old_count==1&&new_count==0;
}
void Collector::RememberPlayerReplaceState(Call &call) noexcept {
  auto *old=identities_.Find(ObjectDomain::Entity,call.args[0]);const auto *next=identities_.Find(ObjectDomain::Entity,call.args[1]);
  if(!old||!next||old->Retired()||next->Retired()||old->readiness<2||!old->current_public_ready||next->readiness<2||!next->current_public_ready)return;
  const auto &l=layout::LayoutConstants();int ot=0,nt=0,oi=0,ni=0;std::uint32_t forward=0,back=0,pv=0;bool base=false;
  if(Classify(ObjectDomain::Entity,old->address,pv,base)!=Class::Player||Classify(ObjectDomain::Entity,next->address,pv,base)!=Class::Player)return;
  if(!Value(std::uint64_t(old->address)+l.player_type_offset,ot)||(ot!=29&&ot!=38)||!Value(std::uint64_t(next->address)+l.player_type_offset,nt)||nt!=67-ot||
     !Value(std::uint64_t(old->address)+l.player_index_offset,oi)||oi<0||!Value(std::uint64_t(next->address)+l.player_index_offset,ni)||ni>=0||
     !Value(std::uint64_t(old->address)+l.player_backup_storage_offset,forward)||forward!=next->address||!Value(std::uint64_t(next->address)+l.player_backup_storage_offset,back)||back!=old->address)return;
  auto &w=call.player_replace;w.manager=call.object;w.old_body=old->address;w.new_body=next->address;w.old_type=unsigned(ot);w.old_local=old->local_id;w.old_use=old->use;w.new_local=next->local_id;w.new_use=next->use;w.run=run_;
  if(!PlayerReplaceRoster(call,false))w={};
}
void Collector::CompletePlayerReplaceState(const Call &call) noexcept {
  const auto &w=call.player_replace;if(!w.old_body||w.invalid||w.run!=run_||call.args[0]!=w.old_body||call.args[1]!=w.new_body||!Recording())return;
  auto *old=identities_.Find(ObjectDomain::Entity,w.old_body);const auto *next=identities_.Find(ObjectDomain::Entity,w.new_body);
  if(!old||!next||old->Retired()||next->Retired()||old->local_id!=w.old_local||old->use!=w.old_use||next->local_id!=w.new_local||next->use!=w.new_use||!PlayerReplaceRoster(call,true))return;
  const auto &l=layout::LayoutConstants();int ot=0,nt=0,oi=0;std::uint32_t forward=0,back=0;
  if(!Value(std::uint64_t(w.old_body)+l.player_type_offset,ot)||ot!=int(w.old_type)||!Value(std::uint64_t(w.new_body)+l.player_type_offset,nt)||nt!=67-ot||
     !Value(std::uint64_t(w.old_body)+l.player_index_offset,oi)||oi>=0||!Value(std::uint64_t(w.old_body)+l.player_backup_storage_offset,forward)||forward!=w.new_body||
     !Value(std::uint64_t(w.new_body)+l.player_backup_storage_offset,back)||back!=w.old_body)return;
  old->readiness=unsigned(Readiness::PlayerStateOwnerCompleted);old->current_public_ready=true;
}
// Source只观察真实owner，不调用创建／Init或写游戏。失败只撤销本次证言，不补birth。
std::uint32_t Collector::BeginPlayerChildSource(std::uint32_t object,unsigned kind,unsigned slot) noexcept {
  if(GetCurrentThreadId()!=memory_.main_thread||!Recording()||kind>4||(kind==4&&slot>3))return 0;
  if(kind==1||kind==2){
    for(unsigned i=0;i<depth_;++i)if(calls_[i].player_replace.old_body==object)calls_[i].player_replace.invalid=true;
    for(unsigned i=0;i<player_child_count_;++i){auto &w=player_child_witnesses_[i];if(w.state_owner&&(w.object==object||(w.pair_owner&&w.parent==object)))w.invalid=true;}
  }
  auto *id=identities_.Find(ObjectDomain::Entity,object);if(!id||id->Retired())return 0;
  Identity *former_hologram=nullptr;int source_type=0;
  if(kind==3){
    // 只记录真实29/38活动根及其较高索引hologram的既有资格；普通Update不自动授予备用体资格。
    const auto &l=layout::LayoutConstants();std::uint32_t twin=0,back=0,game=0,first=0,last=0,cap=0;int other_type=0,main_index=0,other_index=0;std::uint8_t valid=0,exists=0;
    if(id->readiness<2||!id->current_public_ready||!Value(std::uint64_t(object)+l.player_type_offset,source_type)||(source_type!=29&&source_type!=38)||
       !Value(std::uint64_t(object)+l.player_twin_storage_offset,twin)||!twin)return 0;
    former_hologram=identities_.Find(ObjectDomain::Entity,twin);
    if(!former_hologram||former_hologram->Retired()||former_hologram->readiness<2||!former_hologram->current_public_ready||
       !Value(std::uint64_t(twin)+l.player_type_offset,other_type)||other_type!=67-source_type||!Value(std::uint64_t(twin)+l.player_twin_storage_offset,back)||back!=object||
       !Value(std::uint64_t(object)+l.player_index_offset,main_index)||main_index<0||!Value(std::uint64_t(twin)+l.player_index_offset,other_index)||other_index<=main_index||
       !Value(std::uint64_t(twin)+l.entity_valid_offset,valid)||!valid||!Value(std::uint64_t(twin)+l.entity_exists_offset,exists)||!exists||
       !Value(std::uint64_t(memory_.image_base)+l.game_root_rva,game)||!game)return 0;
    const auto roster=std::uint64_t(game)+l.game_player_manager_offset+l.player_manager_roster_offset;
    if(!Value(roster,first)||!Value(roster+4,last)||!Value(roster+8,cap)||last<first||cap<last||(last-first)%4||(cap-first)/4>256)return 0;
    unsigned a=0,b=0;for(auto at=std::uint64_t(first);at<last;at+=4){std::uint32_t member=0;if(!Value(at,member))return 0;a+=member==object;b+=member==twin;}if(a!=1||b!=1)return 0;
  }
  if(player_child_depth_==player_child_owners_.size()||next_player_child_==UINT32_MAX){Fault(Reason::Budget,"player_child_owner_capacity");return 0;}
  if(!player_child_depth_){player_child_count_=0;player_child_failed_=false;}
  auto &o=player_child_owners_[player_child_depth_++];o={};o.token=++next_player_child_;o.object=object;o.kind=kind;o.slot=slot;o.call_depth=depth_;o.first=player_child_count_;o.local=id->local_id;o.use=id->use;o.run=run_;
  o.source_type=unsigned(source_type);
  if(former_hologram){RememberPlayerChildCtor(*former_hologram);if(player_child_count_>o.first){auto &w=player_child_witnesses_[o.first];w.state_owner=true;w.pair_owner=true;w.parent=object;}}
  // 创建完成依据2是存储初始化历史，逻辑撤锚的public标志不抹去它；当前更新依据3仍须有效。
  // 0/1不进入证言，旧体Init/Restore/Acquire/Destroy会使pending失效；结束仍核对真实槽/Replace。
  const bool initialized=id->readiness==unsigned(Readiness::CreationOwnerCompleted)||(id->readiness>=2&&id->current_public_ready);
  if(kind==4&&initialized){
    const auto &l=layout::LayoutConstants();int item=0;
    if(Value(std::uint64_t(object)+l.player_active_desc_first_offset+slot*l.player_active_desc_stride,item)&&item==703){
      RememberPlayerChildCtor(*id);if(player_child_count_>o.first)player_child_witnesses_[o.first].state_owner=true;
    }
  }
  storage_.Diag().callbacks.fetch_add(1,std::memory_order_relaxed);return o.token;
}
void Collector::RememberPlayerChildCtor(const Identity &id) noexcept {
  if(!player_child_depth_)return;
  if(player_child_count_==player_child_witnesses_.size()){player_child_failed_=true;Fault(Reason::Budget,"player_child_ctor_capacity");return;}
  auto &w=player_child_witnesses_[player_child_count_++];w={};w.object=id.address;w.local=id.local_id;w.use=id.use;w.ctor_seen=true;
}
// 只在真实703外层内、Replace撤锚之前保存新体的独立初始化事实；0/1本身从不作依据。
// 新分配必须先有本次Ctor和完整Player Init正常返回；复用必须已有2/4/5，不能借旧体的资格。
void Collector::RememberPlayerEsauReplacement(std::uint32_t manager,std::uint32_t old_body,std::uint32_t replacement) noexcept {
  auto *next=identities_.Find(ObjectDomain::Entity,replacement);if(!next||next->Retired())return;
  const auto &l=layout::LayoutConstants();std::uint32_t game=0,pv=0;bool base=false;
  if(!Value(std::uint64_t(memory_.image_base)+l.game_root_rva,game)||!game||manager!=std::uint64_t(game)+l.game_player_manager_offset||Classify(ObjectDomain::Entity,replacement,pv,base)!=Class::Player)return;
  for(unsigned n=0;n<player_child_depth_;++n){auto &o=player_child_owners_[n];if(o.kind!=4||o.object!=old_body||o.run!=run_)continue;
    PlayerChildWitness *old=nullptr,*candidate=nullptr;
    for(unsigned i=o.first;i<player_child_count_;++i){auto &w=player_child_witnesses_[i];
      if(w.object==old_body&&w.state_owner&&!w.invalid)old=&w;
      if(w.object==replacement&&w.local==next->local_id&&w.use==next->use&&w.initialized&&!w.invalid)candidate=&w;
    }
    if(!old)continue;
    const bool stored=next->readiness==unsigned(Readiness::CreationOwnerCompleted)||next->readiness==unsigned(Readiness::PlayerChildInitCompleted)||next->readiness==unsigned(Readiness::PlayerStateOwnerCompleted);
    if(!candidate&&!stored)continue;
    Call before{};before.object=manager;before.player_replace.old_body=old_body;before.player_replace.new_body=replacement;
    if(!PlayerReplaceRoster(before,false))continue;
    if(!candidate){const auto before_count=player_child_count_;RememberPlayerChildCtor(*next);if(player_child_count_!=before_count+1)continue;candidate=&player_child_witnesses_[player_child_count_-1];candidate->ctor_seen=false;candidate->initialized=true;}
    candidate->esau_active=true;candidate->peer=old_body;candidate->parent_local=old->local;candidate->parent_use=old->use;
    o.replacement=replacement;o.replacement_local=next->local_id;o.replacement_use=next->use;
  }
}
// 7A5660在首个调用前检查+267C并立即退出。只认同次995E10的真实双向临时互指，
// 此处不称GameState恢复成功；真正Init/Restore及其它物理变化照常使候选失效。
bool Collector::PlayerEsauRestoreNoop(std::uint32_t object) noexcept {
  const auto &l=layout::LayoutConstants();std::uint32_t game=0;
  if(!Value(std::uint64_t(memory_.image_base)+l.game_root_rva,game)||!game)return false;
  for(unsigned i=0;i<player_child_count_;++i){const auto &w=player_child_witnesses_[i];if(!w.esau_active||!w.initialized||w.invalid||w.object!=object)continue;
    const auto *id=identities_.Find(ObjectDomain::Entity,object);if(!id||id->Retired()||id->local_id!=w.local||id->use!=w.use)continue;
    std::uint32_t other=0,back=0;if(!Value(std::uint64_t(object)+l.player_replaced_storage_offset,other)||other!=w.peer||!Value(std::uint64_t(other)+l.player_replaced_storage_offset,back)||back!=object)continue;
    for(unsigned d=0;d<depth_;++d)if(calls_[d].observation==Observation::Replace&&calls_[d].object==std::uint64_t(game)+l.game_player_manager_offset&&calls_[d].count==2&&calls_[d].args[0]==other&&calls_[d].args[1]==object)return true;
  }return false;
}
// 先核旧体的真实备份槽证言，再封闭同次新活动体的独立初始化与替换后态。
void Collector::CompletePlayerEsauReplacement(const PlayerChildOwner &o) noexcept {
  const PlayerChildWitness *old=nullptr;
  for(unsigned i=o.first;i<player_child_count_;++i){const auto &w=player_child_witnesses_[i];if(w.object==o.object&&w.state_owner&&!w.pair_owner&&w.complete&&!w.invalid)old=&w;}
  if(!old)return;
  for(unsigned i=o.first;i<player_child_count_;++i){auto &w=player_child_witnesses_[i];if(!w.esau_active||!w.initialized||w.invalid||w.object!=o.replacement||w.local!=o.replacement_local||w.use!=o.replacement_use)continue;
    w.parent=old->parent;w.offset=old->offset;w.complete=true;w.state_owner=true;
  }
}
bool Collector::PlayerChildStillCurrent(const PlayerChildWitness &w) noexcept {
  const auto *c=identities_.Find(ObjectDomain::Entity,w.object);
  if(w.invalid)return false;
  if(w.esau_active){
    const auto *peer=identities_.Find(ObjectDomain::Entity,w.peer);if(!c||!peer||c->Retired()||peer->Retired()||c->local_id!=w.local||c->use!=w.use||peer->local_id!=w.parent_local||peer->use!=w.parent_use)return false;
    const auto &l=layout::LayoutConstants();std::uint32_t game=0,member=0;
    if(!Value(std::uint64_t(memory_.image_base)+l.game_root_rva,game)||game!=w.parent||!Value(std::uint64_t(game)+w.offset,member)||member!=w.peer)return false;
    Call after{};after.object=game+l.game_player_manager_offset;after.player_replace.old_body=w.peer;after.player_replace.new_body=w.object;return PlayerReplaceRoster(after,true);
  }
  if(w.state_owner&&!w.pair_owner){std::uint32_t game=0,member=0;const auto &l=layout::LayoutConstants();return c&&!c->Retired()&&c->local_id==w.local&&c->use==w.use&&Value(std::uint64_t(memory_.image_base)+l.game_root_rva,game)&&game==w.parent&&Value(std::uint64_t(game)+w.offset,member)&&member==w.object;}
  const auto *p=identities_.Find(ObjectDomain::Entity,w.parent);
  if(!p||!c||p->Retired()||c->Retired()||p->local_id!=w.parent_local||p->use!=w.parent_use||c->local_id!=w.local||c->use!=w.use)return false;
  std::uint32_t child=0;return Value(std::uint64_t(w.parent)+w.offset,child)&&child==w.object;
}
// 703确实把具有当前字段依据的旧active换入实际四槽之后，才形成新的状态owner证言。
// 不从Controller/ordinal猜槽，不将新active的更新资格借给旧backup，也不清任何原生槽。
bool Collector::PlayerEsauSlotCompleted(const PlayerChildOwner &o,PlayerChildWitness &w) noexcept {
  if(!w.state_owner||w.object!=o.object||!o.replacement)return false;
  const auto *id=identities_.Find(ObjectDomain::Entity,w.object);if(!id||id->Retired()||id->local_id!=w.local||id->use!=w.use)return false;
  const auto *replacement=identities_.Find(ObjectDomain::Entity,o.replacement);
  if(!replacement||replacement->Retired()||replacement->local_id!=o.replacement_local||replacement->use!=o.replacement_use)return false;
  const auto &l=layout::LayoutConstants();int var=0;std::uint32_t game=0,member=0;
  // 临时替换互指必须已由正常Replace收尾清除，不能在中途或另一分支授证。
  if(!Value(std::uint64_t(o.object)+l.player_replaced_storage_offset,member)||member||!Value(std::uint64_t(o.replacement)+l.player_replaced_storage_offset,member)||member)return false;
  if(!Value(std::uint64_t(o.object)+l.player_active_desc_first_offset+o.slot*l.player_active_desc_stride+l.player_active_var_data_offset,var)||var<1||var>4||!Value(std::uint64_t(memory_.image_base)+l.game_root_rva,game)||!game)return false;
  const auto offset=l.game_player_manager_offset+l.player_manager_esau_offset+4*unsigned(var-1);
  if(!Value(std::uint64_t(game)+offset,member)||member!=o.object)return false;
  std::uint32_t first=0,last=0,cap=0;const auto roster=std::uint64_t(game)+l.game_player_manager_offset+l.player_manager_roster_offset;
  if(!Value(roster,first)||!Value(roster+4,last)||!Value(roster+8,cap)||last<first||cap<last||(last-first)%4||(cap-first)/4>256)return false;
  unsigned replacements=0;for(auto at=std::uint64_t(first);at<last;at+=4){if(!Value(at,member)||member==o.object)return false;replacements+=member==o.replacement;}
  if(replacements!=1)return false;
  w.parent=game;w.offset=offset;w.complete=true;return true;
}
// 77C10C..77C29A正常Update收尾：原hologram同一实例变为非活动backup，不补其PlayerUpdate历史。
bool Collector::PlayerLazarusRetired(const PlayerChildOwner &o,PlayerChildWitness &w) noexcept {
  if(!w.state_owner||!w.pair_owner||w.invalid)return false;
  const auto &l=layout::LayoutConstants();int type=0;std::uint32_t first_twin=0,second_twin=0;std::uint8_t valid=0,exists=0;
  if(!Value(std::uint64_t(o.object)+l.player_type_offset,type)||type!=int(o.source_type)||
     !Value(std::uint64_t(o.object)+l.player_twin_storage_offset,first_twin)||first_twin||!Value(std::uint64_t(w.object)+l.player_twin_storage_offset,second_twin)||second_twin||
     !Value(std::uint64_t(w.object)+l.entity_valid_offset,valid)||valid||!Value(std::uint64_t(w.object)+l.entity_exists_offset,exists)||exists)return false;
  return PlayerChildRelation(o,w);
}
bool Collector::PlayerChildRelation(const PlayerChildOwner &o,PlayerChildWitness &w) noexcept {
  const auto *parent=identities_.Find(ObjectDomain::Entity,o.object),*child=identities_.Find(ObjectDomain::Entity,w.object);
  if(!parent||!child||parent->Retired()||child->Retired()||parent->local_id!=o.local||parent->use!=o.use||child->local_id!=w.local||child->use!=w.use||o.run!=run_)return false;
  const auto &l=layout::LayoutConstants();std::uint32_t pv=0,cv=0;bool base=false;
  if(Classify(ObjectDomain::Entity,o.object,pv,base)!=Class::Player||Classify(ObjectDomain::Entity,w.object,cv,base)!=Class::Player)return false;
  int pt=0,ct=0,pi=0,ci=0;std::uint32_t sub=0,backup=0,back=0;
  if(!Value(std::uint64_t(o.object)+l.player_type_offset,pt)||!Value(std::uint64_t(w.object)+l.player_type_offset,ct)||!Value(std::uint64_t(o.object)+l.player_index_offset,pi)||!Value(std::uint64_t(w.object)+l.player_index_offset,ci)||ci>=0)return false;
  unsigned offset=0;
  if(pi>=0&&(pt==16||pt==17)&&ct==33-pt&&Value(std::uint64_t(o.object)+l.player_sub_storage_offset,sub)&&sub==w.object)offset=l.player_sub_storage_offset;
  else if(o.kind!=0&&(pt==29||pt==38)&&ct==67-pt&&Value(std::uint64_t(o.object)+l.player_backup_storage_offset,backup)&&backup==w.object&&Value(std::uint64_t(w.object)+l.player_backup_storage_offset,back)&&back==o.object)offset=l.player_backup_storage_offset;
  if(!offset)return false;
  // 非活动身体不能出现在主roster；真实vector头和每个元素只读、有界核对。
  std::uint32_t game=0,first=0,last=0,capacity=0;
  if(!Value(std::uint64_t(memory_.image_base)+l.game_root_rva,game)||!game)return false;
  const auto roster=std::uint64_t(game)+l.game_player_manager_offset+l.player_manager_roster_offset;
  if(!Value(roster,first)||!Value(roster+4,last)||!Value(roster+8,capacity)||last<first||capacity<last||(last-first)%4||(capacity-first)/4>256)return false;
  unsigned parents=0;for(auto at=std::uint64_t(first);at<last;at+=4){std::uint32_t member=0;if(!Value(at,member)||member==w.object)return false;parents+=member==o.object;}
  if(o.kind==3&&(pi<0||parents!=1))return false;
  w.parent=o.object;w.parent_local=o.local;w.parent_use=o.use;w.offset=offset;w.complete=true;return true;
}
void Collector::EndPlayerChildSource(std::uint32_t token,bool normal,bool body_reached) noexcept {
  if(!token)return;
  if(!player_child_depth_||player_child_owners_[player_child_depth_-1].token!=token){player_child_failed_=true;Fault(Reason::UnpairedScope,"player_child_owner_pair");return;}
  const auto o=player_child_owners_[player_child_depth_-1];
  const bool complete=normal&&body_reached&&Recording()&&o.call_depth==depth_&&o.run==run_;
  if(!complete)player_child_failed_=true;
  // 完整74F6A0返回确认本次Ctor的存储初始化；仍等外层正常收口，不因进入/基Init返回而授证。
  if(complete&&o.kind==2)for(unsigned i=0;i<player_child_count_;++i){auto &w=player_child_witnesses_[i];if(w.ctor_seen&&!w.invalid&&w.object==o.object&&w.local==o.local&&w.use==o.use)w.initialized=true;}
  if(complete)for(unsigned i=o.first;i<player_child_count_;++i){auto &w=player_child_witnesses_[i];if(o.kind==3&&w.pair_owner)PlayerLazarusRetired(o,w);else if(o.kind==4&&w.state_owner&&!w.pair_owner)PlayerEsauSlotCompleted(o,w);else if(o.kind!=3&&!w.state_owner)PlayerChildRelation(o,w);}
  if(complete&&o.kind==4)CompletePlayerEsauReplacement(o);
  --player_child_depth_;storage_.Diag().callbacks.fetch_sub(1,std::memory_order_relaxed);
  // 703异常前缀可能留下已写槽但已被EH释放的指针；停相关采集，不能继续解引用它。
  if(!normal||o.call_depth!=depth_)Fault(Reason::UnpairedScope,"player_child_source_abnormal");
  if(!player_child_depth_){
    if(!player_child_failed_&&Recording())for(unsigned i=0;i<player_child_count_;++i){const auto &w=player_child_witnesses_[i];if(!w.complete||!PlayerChildStillCurrent(w))continue;
      auto *id=identities_.Find(ObjectDomain::Entity,w.object);if(!id||(!w.state_owner&&id->readiness!=unsigned(Readiness::BaseInitReturned)))continue;
      id->readiness=unsigned(w.state_owner?Readiness::PlayerStateOwnerCompleted:Readiness::PlayerChildInitCompleted);id->current_public_ready=true;
      // 没有创建owner的完整出生历史仍缺失；结束旧pending，不制造出生号。
      if(!id->owner_scope)id->candidate=false;
    }
    player_child_count_=0;CloseIfDrained();
  }
}
}
