#include "ops/client_fixture.h"
#include "game/world_bridge.h"
#include "game/context.h"
#include "replica/model.h"
#include "runtime.h"
#include <filesystem>
#include <fstream>
#include <set>

namespace cs {
// 续测准备期降低纯值采样频率；生命周期Hook/身份表持续工作，Authority从不等待采样。
std::string ClientFixtureSampling(const std::string &token,const std::string &mode) noexcept {
  if(!IsMain()||game::TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||(mode!="prepare"&&mode!="capture"))return "ok=0\nerror=fixture_sampling_arguments\n";
  const auto c=game::CurrentWorldRecording();if(!c||!c->formal_consumer||c->recorder_consumer)return "ok=0\nerror=fixture_sampling_consumer\n";
  try{auto plan=c->observation.Plan();for(auto &d:plan.domains)d.interval=mode=="prepare"?1000000:30;
    const auto r=c->observation.UpdateCapturePlan(1,std::move(plan));return r.accepted?"ok=1\nsampling_updated=1\n":"ok=0\nerror="+std::string(r.reason)+"\n";
  }catch(...){return "ok=0\nerror=fixture_sampling_allocation\n";}
}
namespace {
// 仅裁剪17明确支持的Entity字段；完整源独立保存，Room/Grid一项也不删除。
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
  replica::Put(profile,"scope","complete Room/Grid;explicit tx17 Entity field projection;not full-world Applied",a);
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
std::string ExportClientFixture(const std::string &token,const std::string &label) noexcept {
  if(!IsMain()||game::TopOwner()!=Owner::Manager||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token||label.empty()||label.size()>48||label.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_")!=std::string::npos)return "ok=0\nerror=fixture_export_arguments\n";
  try {
    const auto c=game::CurrentWorldRecording();if(!c||!c->formal_consumer)throw replica::ContractError("fixture_formal_consumer_unavailable");
    const auto &ctx=c->latest_room.context;
    if(ctx.phase!=world::observation::Phase::OrdinaryPost||!ctx.run.value.local||!ctx.room.value.local||!ctx.floor.value.local||!ctx.dimension.value.local)throw replica::ContractError("fixture_scope_unavailable");
    replica::Scope scope{ctx.session,1,ctx.run.value.local,ctx.floor.value.local,ctx.dimension.value.local,ctx.room.value.local,ctx.room.value.local};
    static std::unique_ptr<replica::Registry> registry;static std::string owner;static std::uint64_t revision=0;
    const auto identity=token+":"+std::to_string(ctx.epoch)+":"+std::to_string(scope.run);
    if(owner!=identity){registry=std::make_unique<replica::Registry>(scope.session,scope.run);owner=identity;revision=0;}
    if(revision==UINT64_MAX)throw replica::ContractError("fixture_revision_exhausted");
    std::vector<replica::Json> rows;
    const auto envelope=[&](const world::observation::ObservationEnvelope &e){const auto raw=world::observation::EncodeEnvelope(e);
      try{rows.push_back(replica::Parse(raw));}catch(const std::exception &error){
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
    Save(root/("source-full-"+label+".json"),model);ProjectEntity(model);replica::ValidateModel(model);
    Save(root/(label+".json"),model);++revision;
    return "ok=1\nexported=1\nrevision="+std::to_string(revision)+"\nsource_tick="+std::to_string(ctx.authority_tick.value)+"\n";
  }catch(const std::exception &e){return "ok=1\nexported=0\nerror="+std::string(e.what())+"\n";}catch(...){return "ok=0\nerror=fixture_export_allocation\n";}
}
}
