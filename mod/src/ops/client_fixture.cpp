#include "ops/client_fixture.h"
#include "game/world_bridge.h"
#include "game/context.h"
#include "replica/model.h"
#include "replica/entity_target.h"
#include "replica/codec.h"
#include "game/bridge.h"
#include "game/client.h"
#include "game/client_entity_events.h"
#include "game/world_sources/lua_entity.h"
#include "runtime.h"
#include <filesystem>
#include <fstream>
#include <set>

namespace cs {
// 续测准备期降低纯值采样频率；生命周期Hook/身份表持续工作，Authority从不等待采样。
std::string ClientFixtureSampling(const std::string &token,const std::string &mode) noexcept {
  if(!IsMain()||game::TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||(mode!="prepare"&&mode!="capture"&&mode!="capture19"))return "ok=0\nerror=fixture_sampling_arguments\n";
  const auto c=game::CurrentWorldRecording();if(!c||!c->formal_consumer||c->recorder_consumer)return "ok=0\nerror=fixture_sampling_consumer\n";
  try{if(mode=="capture"||mode=="capture19"){c->e2e_entity_independent.clear();c->e2e_entity_bytes=0;c->e2e_entity_loss=false;}
    auto plan=c->observation.Plan();for(auto &d:plan.domains)d.interval=mode=="prepare"?1000000:mode=="capture19"?1:30;
    const auto r=c->observation.UpdateCapturePlan(1,std::move(plan));return r.accepted?"ok=1\nsampling_updated=1\n":"ok=0\nerror="+std::string(r.reason)+"\n";
  }catch(...){return "ok=0\nerror=fixture_sampling_allocation\n";}
}
namespace {
// 仅选择17明确支持的Entity字段；语义去冗余由正式Registry投影负责。
// source-full是全域复制模型（已执行模型v2发送选择），并非完整观测原文；Room/Grid不另作夹具裁剪。
// 新类别在Source清理后仍存在时明确失败，不能把它从成功目标中悄悄滤掉。
void ProjectEntity(replica::Json &m) {
  using V=replica::Value;auto &a=m.GetAllocator();
  static const std::set<std::string> common={"Entity.Type","Entity.Variant","Entity.SubType","Entity.InitSeed","Entity.Position","Entity.Velocity","Entity.PositionOffset","Entity.DepthOffset","Entity.Size","Entity.Mass","Entity.CollisionDamage","Entity.EntityCollisionClass","Entity.GridCollisionClass","Entity.Visible","Entity.Parent","Entity.Child","Entity.Target","Entity.SpawnerEntity","Entity.GetSprite"};
  static const std::set<std::string> player={"EntityPlayer.GetPlayerType","EntityPlayer.Damage","EntityPlayer.MoveSpeed","EntityPlayer.MaxFireDelay","EntityPlayer.ShotSpeed","EntityPlayer.TearRange","EntityPlayer.Luck","EntityPlayer.GetHearts","EntityPlayer.GetMaxHearts","EntityPlayer.GetSoulHearts","EntityPlayer.GetBlackHearts","EntityPlayer.GetEternalHearts","EntityPlayer.GetNumCoins","EntityPlayer.GetNumKeys","EntityPlayer.GetNumBombs"};
  unsigned players=0;V retired(rapidjson::kArrayType);auto &objects=m["components"]["entity"]["state"]["objects"];
  // 普通后态仍可枚举尚未从EL摘表、Exists=false的退休前缀；它不是待构建可见Entity。
  // 仅Entity代表投影显式登记这些墓碑，完整源及Room/Grid保持原样；存活未知类别仍拒绝。
  for(auto i=objects.Begin();i!=objects.End();){const auto &f=(*i)["common"]["fields"];
    if(std::string((*i)["class"].GetString())!="EntityPlayer"&&f.HasMember("Entity.Exists")&&f["Entity.Exists"].IsBool()&&!f["Entity.Exists"].GetBool()){
      V row(rapidjson::kObjectType),subject;subject.CopyFrom((*i)["subject"],a);replica::Put(row,"subject",std::move(subject),a);
      replica::Put(row,"reason","source Entity.Exists=false;retired EL prefix;not constructed",a);retired.PushBack(std::move(row),a);i=objects.Erase(i);
    }else ++i;
  }
  for(auto &o:objects.GetArray()){
    const std::string cl=o["class"].GetString();const bool p=cl=="EntityPlayer";
    if(p)++players;
    else {
      const auto &variant=o["common"]["fields"]["Entity.Variant"];
      if(cl!="EntityEffect"||replica::Unsigned(variant.IsObject()?variant["value"]:variant)!=57)throw replica::ContractError("fixture_entity19_required");
    }
    for(const auto name:{"common","body"}){auto &f=o[name]["fields"];const auto &allow=std::string(name)=="common"?common:player;
      for(auto i=f.MemberBegin();i!=f.MemberEnd();)if((std::string(name)=="body"&&!p)||!allow.count(std::string(i->name.GetString(),i->name.GetStringLength())))i=f.EraseMember(i);else ++i;}
  }
  if(players!=1)throw replica::ContractError("fixture_exact_player_required");
  V profile(rapidjson::kObjectType);replica::Put(profile,"id","tx-04-17-representative-v1",a);
  replica::Put(profile,"source","tx18 formal consumer;source-full-<label>.json",a);
  replica::Put(profile,"scope","Room/Grid replica projection v2;explicit tx17 Entity field whitelist only;not full observation or full-world Applied",a);
  replica::Put(profile,"retired_entity_prefixes",std::move(retired),a);
  V fields(rapidjson::kArrayType);for(const auto &s:common)fields.PushBack(V(s.c_str(),a),a);for(const auto &s:player)fields.PushBack(V(s.c_str(),a),a);
  replica::Put(profile,"selected_fields",std::move(fields),a);replica::Put(m,"client_profile",std::move(profile),a);
}
void Save(const std::filesystem::path &path,const replica::Value &v){
  std::ofstream f(path,std::ios::binary|std::ios::trunc);const auto text=replica::Text(v);f.write(text.data(),text.size());f.close();if(!f)throw replica::ContractError("fixture_write_failed");
}
}
// 失败模型：关闭录制、混采样窗、Partial、旧epoch、未支持Entity及写盘失败均可区分。
// 此同步文件出口仅在显式E2E token下运行；生产consumer不依赖它，后台不持有游戏指针。
std::string ExportClientFixture(const std::string &token,const std::string &label,bool entity19) noexcept {
  if(!IsMain()||game::TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||label.empty()||label.size()>48||label.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_")!=std::string::npos)return "ok=0\nerror=fixture_export_arguments\n";
  try {
    const auto c=game::CurrentWorldRecording();if(!c||!c->formal_consumer)throw replica::ContractError("fixture_formal_consumer_unavailable");
    const auto token_at_export=game::ObservationSourceSnapshotToken();
    if(entity19&&token_at_export.empty())throw replica::ContractError("fixture_same_window_unavailable");
    const auto &ctx=c->latest_room.context;
    if(ctx.phase!=world::observation::Phase::OrdinaryPost||!ctx.run.value.local||!ctx.room.value.local||!ctx.floor.value.local||!ctx.dimension.value.local)throw replica::ContractError("fixture_scope_unavailable");
    replica::Scope scope{ctx.session,1,ctx.run.value.local,ctx.floor.value.local,ctx.dimension.value.local,ctx.room.value.local,ctx.room.value.local};
    static std::unique_ptr<replica::Registry> registry;static std::string owner;static std::uint64_t revision=0;
    const auto identity=token+":"+std::to_string(ctx.epoch)+":"+std::to_string(scope.run);
    if(owner!=identity){registry=std::make_unique<replica::Registry>(scope.session,scope.run);owner=identity;revision=0;}
    if(revision==UINT64_MAX)throw replica::ContractError("fixture_revision_exhausted");
    std::vector<replica::Json> rows;std::size_t raw_bytes=0;
    const auto envelope=[&](const world::observation::ObservationEnvelope &e){const auto raw=world::observation::EncodeEnvelope(e);
      try{raw_bytes+=raw.size();rows.push_back(replica::Parse(raw));}catch(const std::exception &error){
        const auto root=std::filesystem::path(Settings.world_output_dir)/"client-targets";std::filesystem::create_directories(root);
        std::ofstream f(root/("source-envelope-"+label+"-"+std::to_string(unsigned(e.domain))+".json"),std::ios::binary);f.write(raw.data(),raw.size());
        throw replica::ContractError("fixture_envelope_domain="+std::to_string(unsigned(e.domain))+":bytes="+std::to_string(raw.size())+":"+error.what());
      }};
    for(const auto *e:{&c->latest_entity,&c->latest_grid,&c->latest_room,&c->latest_level})envelope(*e);
    for(const auto &e:c->latest_continuity)envelope(e);
    auto model=replica::CaptureRows(scope,revision+1,rows,*registry);
    const auto root=std::filesystem::path(Settings.world_output_dir)/"client-targets";std::filesystem::create_directories(root);
    if(model["publishable"]!=true){
      // 中断材料保留具体域/字段原因，不将Partial说成空集合；仅E2E故障续测使用。
      Save(root/("source-incomplete-"+label+".json"),model);
      if(!c->e2e_grid_faults.empty()){std::ofstream diagnosis(root/("source-grid-identities-"+label+".json"),std::ios::binary);diagnosis<<c->e2e_grid_faults;}
      std::string error="fixture_source_not_complete";
      for(auto i=model["components"].MemberBegin();i!=model["components"].MemberEnd();++i)
        if(std::string(i->value["coverage"].GetString())!="Complete"&&std::string(i->value["coverage"].GetString())!="NotApplicable")error+=":"+std::string(i->name.GetString())+":"+replica::Text(i->value["reasons"]);
      throw replica::ContractError(error);
    }
    Save(root/("source-full-"+label+".json"),model);
    const auto projection_bytes=replica::Text(model).size();
    if(entity19){
      // 不删墓碑、不筛活对象、不裁非Player字段；所有19消费者使用同一正式目标。
      const auto target=replica::ParseEntityTarget(model);
      replica::Value profile(rapidjson::kObjectType);auto &a=model.GetAllocator();
      replica::Put(profile,"id","tx-04-19-non-player-v1",a);
      replica::Put(profile,"source","same-window formal CaptureRows;complete entity projection retained",a);
      replica::Put(profile,"scope","all non-Player;ordinary Isaac support only;full Player=tx-04-20;not all-domain Applied",a);
      replica::Put(profile,"source_members",replica::Value(target.source_member_count),a);
      replica::Put(profile,"player_members",replica::Value(unsigned(target.player_keys.size())),a);
      replica::Put(model,"client_profile",std::move(profile),a);
      replica::Json original(rapidjson::kArrayType);for(const auto &row:rows){replica::Value v;v.CopyFrom(row,original.GetAllocator());original.PushBack(std::move(v),original.GetAllocator());}
      Save(root/("observation-"+label+".json"),original);
      replica::Json metric(rapidjson::kObjectType);auto &ma=metric.GetAllocator();
      replica::Put(metric,"capture_token",token_at_export,ma);
      replica::Put(metric,"raw_observation_bytes",replica::Decimal(raw_bytes,ma),ma);
      replica::Put(metric,"replica_projection_bytes",replica::Decimal(projection_bytes,ma),ma);
      replica::Put(metric,"comparison","same source raw vs current projection;not P1-P6 net improvement",ma);
      Save(root/("metrics-"+label+".json"),metric);
      if(c->e2e_entity_loss)throw replica::ContractError("fixture_independent_buffer_loss");
      replica::Json independent(rapidjson::kArrayType);
      for(const auto &e:c->e2e_entity_independent){
        if(e.context.room.value.local!=ctx.room.value.local||e.context.run.value.local!=ctx.run.value.local)continue;
        auto message=replica::Independent(scope,replica::Parse(world::observation::EncodeEnvelope(e)),*registry);
        // 只导出真实独立scope，required_state指本次T，绝不把独立值合并进普通快照。
        replica::Put(message.body,"required_state",replica::Decimal(revision+1,message.body.GetAllocator()),message.body.GetAllocator());
        replica::Value item(rapidjson::kObjectType),body;auto &ia=independent.GetAllocator();body.CopyFrom(message.body,ia);
        replica::Put(item,"kind",replica::Value(unsigned(message.kind)),ia);replica::Put(item,"sequence",replica::Decimal(message.sequence,ia),ia);
        replica::Put(item,"body",std::move(body),ia);independent.PushBack(std::move(item),ia);
      }
      Save(root/("independent-"+label+".json"),independent);
      c->e2e_entity_independent.clear();c->e2e_entity_bytes=0;
    }else ProjectEntity(model);
    replica::ValidateModel(model);
    Save(root/(label+".json"),model);++revision;
    return "ok=1\nexported=1\nrevision="+std::to_string(revision)+"\nsource_tick="+std::to_string(ctx.authority_tick.value)+"\ncapture_token="+token_at_export+"\n";
  }catch(const std::exception &e){return "ok=1\nexported=0\nerror="+std::string(e.what())+"\n";}catch(...){return "ok=0\nerror=fixture_export_allocation\n";}
}
std::string ImportClientIndependent(const std::string &token,const std::string &label) noexcept {
  if(!IsMain()||game::TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||
     label.empty()||label.size()>48||label.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_")!=std::string::npos)return "ok=0\nerror=independent_arguments\n";
  try {
    const auto root=std::filesystem::canonical(std::filesystem::path(Settings.world_output_dir)/"client-targets");
    const auto path=std::filesystem::canonical(root/("independent-"+label+".json"));
    if(path.parent_path()!=root||std::filesystem::file_size(path)>(8u<<20))throw replica::ContractError("independent_path_or_budget");
    std::ifstream f(path,std::ios::binary);auto rows=replica::Parse(std::string(std::istreambuf_iterator<char>(f),{}));
    if(!rows.IsArray()||rows.Size()>4096)throw replica::ContractError("independent_array");
    unsigned accepted=0;const char *reason="";
    for(const auto &row:rows.GetArray()){
      replica::Message m;m.kind=static_cast<replica::Kind>(replica::Unsigned(row["kind"]));m.sequence=replica::Unsigned(row["sequence"]);m.body=replica::Copy(row["body"]);
      m=replica::Decode(replica::Encode(m));
      if(!game::ClientAdmitEntityIndependent(m,reason))return "ok=1\naccepted=0\nrecords="+std::to_string(accepted)+"\ndeferred="+std::to_string(rows.Size()-accepted)+"\nreason="+std::string(reason)+"\n";
      ++accepted;
    }
    return "ok=1\naccepted=1\nrecords="+std::to_string(accepted)+"\n";
  }catch(const std::exception &e){return "ok=0\nerror="+std::string(e.what())+"\n";}
}

}
