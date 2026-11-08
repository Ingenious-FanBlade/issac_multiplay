#include "replica/stream_source_value.h"
#include "replica/model.h"
#include <cmath>
namespace cs::replica::stream {
Json ExpandSource(const capture::Source &source){
  if(!source)throw ContractError("capture_source_missing");Json out;std::uint64_t work=0;
  // 分块直接输出到同一个SAX构建栈；数组只在End时分配一次，不保留每块临时数组。
  auto generate=[&](auto &handler){
  const auto expand=[&](const auto &self,const capture::OwnedSource &owner,const Value &v,unsigned depth)->bool{
    if(depth>64||++work>world::observation::kNodeBudget)throw ContractError("capture_source_expand_budget");
    if(v.IsObject()&&v.HasMember("__capture_owned_block")){
      if(v.MemberCount()!=1||!v["__capture_owned_block"].IsUint()||v["__capture_owned_block"].GetUint()>=owner.Blocks().size())throw ContractError("capture_source_block_reference");
      const auto &block=*owner.Blocks()[v["__capture_owned_block"].GetUint()];
      if(block.Concatenate()){
        handler.StartArray();std::uint64_t count=0;
        for(const auto &chunk:block.Blocks()){
          if(!chunk||chunk->Concatenate()||!chunk->Read().IsArray())throw ContractError("capture_source_chunk_type");
          if(++work>world::observation::kNodeBudget)throw ContractError("capture_source_expand_budget");
          count+=chunk->Read().Size();if(count>world::observation::kNodeBudget)throw ContractError("capture_source_expand_budget");
          for(const auto &item:chunk->Read().GetArray())if(!self(self,*chunk,item,depth+1))return false;
        }return handler.EndArray(rapidjson::SizeType(count));
      }
      return self(self,block,block.Read(),depth);
    }
    if(v.IsArray()){handler.StartArray();for(const auto &item:v.GetArray())if(!self(self,owner,item,depth+1))return false;return handler.EndArray(v.Size());}
    if(v.IsObject()){handler.StartObject();for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){handler.Key(i->name.GetString(),i->name.GetStringLength(),true);if(!self(self,owner,i->value,depth+1))return false;}return handler.EndObject(v.MemberCount());}
    // 来源字符串属于另一个arena，必须复制；数值仍使用原始种类及双精度位值。
    if(v.IsString())return handler.String(v.GetString(),v.GetStringLength(),true);return v.Accept(handler);
  };
  if(source->Concatenate()){
    handler.StartArray();std::uint64_t count=0;
    for(const auto &block:source->Blocks()){
      if(!block||block->Concatenate()||!block->Read().IsArray())throw ContractError("capture_source_chunk_type");
      if(++work>world::observation::kNodeBudget)throw ContractError("capture_source_expand_budget");
      count+=block->Read().Size();if(count>world::observation::kNodeBudget)throw ContractError("capture_source_expand_budget");
      for(const auto &item:block->Read().GetArray())if(!expand(expand,*block,item,1))return false;
    }return handler.EndArray(rapidjson::SizeType(count));
  }return expand(expand,*source,source->Read(),0);
  };out.Populate(generate);return out;
}
}
namespace cs::capture {
std::string SourceText(const Source &source){
  if(!source)throw std::runtime_error("capture_source_missing");
  auto memory=memory::Reserve(memory::Pool::Values,65536+16*source->Bytes(),"capture_diagnostic_block");
  return replica::Text(replica::stream::ExpandSource(source));
}
bool EqualSources(const Source &left,const Source &right){
  if(!left||!right)return left==right;
  if(left->Concatenate()!=right->Concatenate()||left->Blocks().size()!=right->Blocks().size())return false;
  const auto equal=[&](const auto &self,const replica::Value &a,const replica::Value &b)->bool{
    if(a.GetType()!=b.GetType())return false;
    if(a.IsNumber())return a==b&&(!(a.IsDouble()&&b.IsDouble()&&a.GetDouble()==0)||std::signbit(a.GetDouble())==std::signbit(b.GetDouble()));
    if(a.IsArray()){if(a.Size()!=b.Size())return false;for(unsigned i=0;i<a.Size();++i)if(!self(self,a[i],b[i]))return false;return true;}
    if(a.IsObject()){if(a.MemberCount()!=b.MemberCount())return false;for(auto i=a.MemberBegin();i!=a.MemberEnd();++i){const auto j=b.FindMember(i->name);if(j==b.MemberEnd()||!self(self,i->value,j->value))return false;}return true;}return a==b;
  };
  if(!equal(equal,left->Read(),right->Read()))return false;
  for(unsigned i=0;i<left->Blocks().size();++i)if(!EqualSources(left->Blocks()[i],right->Blocks()[i]))return false;return true;
}
}
