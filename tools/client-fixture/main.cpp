// tx17统一E2E辅助：实际11域采样经正式CaptureRows形成模型，再明确选取17代表profile。
// 不创建原生对象，不用离线输出代替真实Client；source-full为正式发送投影，完整观测在records中。
// 这里只选择代表字段，所有资源/参数查询去冗余均由正式Registry投影统一负责。
#include "replica/model.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
namespace r=cs::replica;
namespace {
using J=r::Json;using V=r::Value;
const std::set<std::string> common={"Entity.Type","Entity.Variant","Entity.SubType","Entity.InitSeed","Entity.Position","Entity.Velocity","Entity.PositionOffset","Entity.DepthOffset","Entity.Size","Entity.Mass","Entity.CollisionDamage","Entity.EntityCollisionClass","Entity.GridCollisionClass","Entity.Visible","Entity.Parent","Entity.Child","Entity.Target","Entity.SpawnerEntity","Entity.GetSprite"};
const std::set<std::string> player={"EntityPlayer.GetPlayerType","EntityPlayer.Damage","EntityPlayer.MoveSpeed","EntityPlayer.MaxFireDelay","EntityPlayer.ShotSpeed","EntityPlayer.TearRange","EntityPlayer.Luck","EntityPlayer.GetHearts","EntityPlayer.GetMaxHearts","EntityPlayer.GetSoulHearts","EntityPlayer.GetBlackHearts","EntityPlayer.GetEternalHearts","EntityPlayer.GetNumCoins","EntityPlayer.GetNumKeys","EntityPlayer.GetNumBombs"};
std::string Str(const V &v){if(!v.IsString())throw std::runtime_error("string_required");return {v.GetString(),v.GetStringLength()};}
std::uint64_t Num(const V &v){return v.IsObject()?r::Unsigned(v["value"]):r::Unsigned(v);}
void Select(V &fields,const std::set<std::string> &names){for(auto i=fields.MemberBegin();i!=fields.MemberEnd();)if(!names.count(Str(i->name)))i=fields.EraseMember(i);else ++i;}
void Revision(J &model,unsigned revision){auto &a=model.GetAllocator();r::Put(model,"revision",r::Decimal(revision,a),a);
  for(auto i=model["components"].MemberBegin();i!=model["components"].MemberEnd();++i)r::Put(i->value,"version",r::Decimal(revision,a),a);
  const auto dependency=[&](auto &&self,V &v)->void{if(v.IsArray())for(auto &x:v.GetArray())self(self,x);if(v.IsObject()){if(v.HasMember("required_revision"))r::Put(v,"required_revision",r::Decimal(revision,a),a);for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)self(self,i->value);}};dependency(dependency,model["components"]);
}
void Save(const std::filesystem::path &root,const char *name,const J &model){r::ValidateModel(model);std::ofstream file(root/(std::string(name)+".json"),std::ios::binary);file<<r::Text(model);if(!file)throw std::runtime_error("write_failed");}
V &Effect(J &m){for(auto &e:m["components"]["entity"]["state"]["objects"].GetArray())if(Str(e["class"])=="EntityEffect")return e;throw std::runtime_error("effect_missing");}
// 受控目标变体仍走正式完整模型与公共applier；值来自实际采样资源，明确区分源与变体。
V &Sprite(J &m){return Effect(m)["common"]["fields"]["Entity.GetSprite"];}
V &Player(J &m){for(auto &e:m["components"]["entity"]["state"]["objects"].GetArray())if(Str(e["class"])=="EntityPlayer")return e;throw std::runtime_error("player_missing");}
J Project(const J &source){auto m=r::Copy(source);auto &a=m.GetAllocator();V profile(rapidjson::kObjectType);r::Put(profile,"id","tx-04-17-representative-v1",a);
  r::Put(profile,"source","source-full.json;formal CaptureRows OrdinaryPost;replica projection v2",a);r::Put(profile,"scope","tx17 Entity whitelist plus formal replica scene/resources/presentation;not full observation or all-domain Applied",a);
  V selected(rapidjson::kArrayType);for(const auto &name:common)selected.PushBack(V(name.c_str(),a),a);for(const auto &name:player)selected.PushBack(V(name.c_str(),a),a);r::Put(profile,"selected_fields",std::move(selected),a);r::Put(m,"client_profile",std::move(profile),a);
  auto &objects=m["components"]["entity"]["state"]["objects"];if(objects.Size()!=2)throw std::runtime_error("representative_source_requires_exact_player_effect");
  for(auto &e:objects.GetArray()){const auto cl=Str(e["class"]);if(cl!="EntityPlayer"&&cl!="EntityEffect")throw std::runtime_error("source_category");Select(e["common"]["fields"],common);Select(e["body"]["fields"],cl=="EntityPlayer"?player:std::set<std::string>{});}
  Revision(m,1);return m;
}
}
int main(int argc,char **argv){try{
  if(argc==3&&std::string(argv[1])=="--capabilities"){
    // 仅导出正式能力值，供同一实机E2E通过真实Negotiate验证版本拒绝。
    std::ofstream output(argv[2],std::ios::binary);output<<r::Text(r::Capabilities());
    if(!output)throw std::runtime_error("capability_write_failed");return 0;
  }
  if(argc!=3)throw std::runtime_error("client-fixture <records.jsonl> <client-targets>");
  const std::filesystem::path root=argv[2];std::filesystem::create_directories(root);std::ifstream file(argv[1]);if(!file)throw std::runtime_error("capture_missing");
  std::vector<J> rows;J source;std::string window,line;std::unique_ptr<r::Registry> registry;r::Scope scope{};unsigned complete=0;
  const auto flush=[&]{if(rows.empty())return;auto m=r::CaptureRows(scope,1,rows,*registry);if(m["publishable"]==true){
    const auto &objects=m["components"]["entity"]["state"]["objects"];
    if(objects.IsArray()&&objects.Size()==2){bool p=false,e=false;for(const auto &o:objects.GetArray()){const auto cl=Str(o["class"]);p|=cl=="EntityPlayer";e|=cl=="EntityEffect"&&Num(o["common"]["fields"]["Entity.Variant"])==57;}
      if(p&&e){source=std::move(m);++complete;}}
  }rows.clear();};
  constexpr const char *domains[]={"","entity","grid","room","level","exploration","room_save","player","run","random","presentation","mod"};
  while(std::getline(file,line)){auto row=r::Parse(line);const auto &c=row["context"];const auto d=Num(row["domain"]);
    if(Str(c["phase"])!="OrdinaryPost"||d<1||d>11||Str(row["schema"])!="isaac.cs."+std::string(domains[d]))continue;
    if(!registry){scope={r::Unsigned(c["session"]),1,1,1,1,1,1};registry=std::make_unique<r::Registry>(scope.session,scope.run);}
    const auto w=r::Text(c["authority_tick"])+":"+r::Text(c["outer"])+":"+r::Text(c["owner"]);if(!window.empty()&&window!=w)flush();window=w;rows.push_back(std::move(row));
  }flush();if(!complete)throw std::runtime_error("complete_representative_source_missing");Save(root,"source-full",source);
  auto baseline=Project(source);Save(root,"baseline",baseline);
  auto change=r::Copy(baseline);Revision(change,2);Effect(change)["common"]["fields"]["Entity.Position"]["x"].SetDouble(240);Save(root,"change",change);
  // 复用15的component最终值Delta合同；每个component版本仍须等于目标revision。
  // T回到基线时，实际Client必须从已显示A恢复这些值，不能只把B当作当前世界。
  auto back_target=r::Copy(baseline);Revision(back_target,3);
  auto back_delta=r::Difference(baseline,back_target);auto back=r::Reconstruct(baseline,back_delta);Save(root,"back-to-baseline",back);
  auto replace=r::Copy(back);Revision(replace,4);auto &effect=Effect(replace);r::Put(effect["common"]["fields"],"Entity.SubType",V(1u),replace.GetAllocator());r::Put(effect["subject"],"shape_revision",r::Decimal(Num(effect["subject"]["shape_revision"])+1,replace.GetAllocator()),replace.GetAllocator());Save(root,"replace",replace);
  auto remove=r::Copy(replace);Revision(remove,5);auto &objects=remove["components"]["entity"]["state"]["objects"];for(auto i=objects.Begin();i!=objects.End();)if(Str((*i)["class"])=="EntityEffect")i=objects.Erase(i);else ++i;Save(root,"remove",remove);
  auto missing=r::Copy(baseline);Revision(missing,6);r::Put(Effect(missing)["common"]["fields"]["Entity.GetSprite"]["fields"],"Sprite.GetFilename","gfx/tx17-resource-does-not-exist.anm2",missing.GetAllocator());Save(root,"missing-resource",missing);
  auto recover=r::Copy(baseline);Revision(recover,7);Save(root,"recover",recover);
  auto invalid=r::Copy(baseline);Revision(invalid,8);r::Put(Effect(invalid)["common"]["fields"]["Entity.GetSprite"]["fields"],"Sprite.GetFrame",V(-1),invalid.GetAllocator());Save(root,"invalid-pose",invalid);
  auto pose=r::Copy(baseline);Revision(pose,8);auto &ps=Sprite(pose);auto &pf=ps["fields"];auto &pa=pose.GetAllocator();
  // 来源已捕获真实ray成员；受控变体激活第一组，确保统一实机覆盖实际96CE20绘制。
  auto &fx=pose["components"]["room"]["state"]["presentation_runtime"]["room_fx"];
  if(Num(fx["ray_groups"])!=2)throw std::runtime_error("basement_ray_source_missing");fx["ray_group"].SetInt(0);
  V overlay;overlay.CopyFrom(pf["Sprite.GetAnimation"],pa);r::Put(pf,"Sprite.GetOverlayAnimation",std::move(overlay),pa);r::Put(pf,"Sprite.GetOverlayFrame",V(0),pa);
  V overlay_runtime;overlay_runtime.CopyFrom(ps["pose_runtime"]["main"],pa);overlay_runtime["phase"].SetDouble(0.25);r::Put(ps["pose_runtime"],"overlay",std::move(overlay_runtime),pa);
  ps["pose_runtime"]["main"]["phase"].SetDouble(0.5);ps["pose_runtime"]["overlay_first"].SetBool(true);
  auto &layers=pf["Sprite.GetAllLayers"]["entries"];std::string ghost;
  for(auto &row:layers.GetArray())if(Num(row["value"]["fields"]["LayerState.GetLayerID"])==13)ghost=Str(row["value"]["fields"]["LayerState.GetSpritesheetPath"]);
  if(ghost.empty())throw std::runtime_error("actual_dynamic_sheet_missing");
  for(auto &row:layers.GetArray())if(Num(row["value"]["fields"]["LayerState.GetLayerID"])==0){auto &lf=row["value"]["fields"];r::Put(lf,"LayerState.GetSpritesheetPath",ghost.c_str(),pa);lf["LayerState.GetRotation"].SetDouble(15);lf["LayerState.GetColor"]["G"].SetDouble(0.6);}
  V parent;parent.CopyFrom(Player(pose)["subject"],pa);r::Put(parent,"required_revision",r::Decimal(8,pa),pa);r::Put(Effect(pose)["common"]["fields"],"Entity.Parent",std::move(parent),pa);Save(root,"pose-and-reference",pose);
  // 实际001.000_Player.anm2的Hit帧1含FX事件；Client只定位姿态，提交检查要求事件位清零。
  auto event=r::Copy(baseline);Revision(event,9);auto &es=Sprite(event);r::Put(es["fields"],"Sprite.GetAnimation","Hit",event.GetAllocator());r::Put(es["fields"],"Sprite.GetFrame",V(1),event.GetAllocator());
  es["pose_runtime"]["main"]["phase"].SetDouble(1);auto &selectors=es["pose_runtime"]["main"]["layers"];
  if(!selectors.IsArray()||selectors.Size()!=15)throw std::runtime_error("player_animation_shape");for(unsigned i=0;i<15;++i)selectors[i].SetInt(i==12?1:0);Save(root,"event-pose",event);
  auto cleared=r::Copy(baseline);Revision(cleared,10);Save(root,"clear-pose-reference",cleared);
  auto missing_reference=r::Copy(pose);Revision(missing_reference,11);
  r::Put(Effect(missing_reference)["common"]["fields"]["Entity.Parent"]["target"],"id",r::Decimal(999999,missing_reference.GetAllocator()),missing_reference.GetAllocator());Save(root,"missing-reference",missing_reference);
  auto budget=r::Copy(baseline);Revision(budget,11);auto &entries=Sprite(budget)["fields"]["Sprite.GetAllLayers"]["entries"];while(entries.Size()<65){V entry;entry.CopyFrom(entries[0],budget.GetAllocator());r::Put(entry,"key",V(entries.Size()+1),budget.GetAllocator());entries.PushBack(std::move(entry),budget.GetAllocator());}Save(root,"resource-budget",budget);
  auto newer=r::Copy(baseline);Revision(newer,11);auto sc=r::ReadScope(newer["scope"]);++sc.scene;r::Put(newer,"scope",r::ScopeValue(sc,newer.GetAllocator()),newer.GetAllocator());Save(root,"new-scene",newer);
  std::cout<<"{\"source_windows\":"<<complete<<",\"profile\":\"tx-04-17-representative-v1\"}\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
