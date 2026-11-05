#pragma once
#ifdef ISAACCS_E2E
#include <rapidjson/document.h>
#include <type_traits>
#include <cmath>
#include <stdexcept>
#include <cstring>
namespace cs::game {
template<class T,class P> T ClientInspectRaw(const P &p,unsigned offset){T value{};std::memcpy(&value,reinterpret_cast<const unsigned char *>(&p)+offset,sizeof(T));return value;}
// 只序列化已核具名存储的当前原值；不复制DTO、容器头或本机地址。
template<class T> rapidjson::Value ClientInspectValue(const T &v,rapidjson::Document::AllocatorType &a){
 rapidjson::Value out;
 if constexpr(std::is_same_v<T,bool>)out.SetBool(v);
 else if constexpr(std::is_floating_point_v<T>){if(!std::isfinite(v))throw std::runtime_error("inspect_nonfinite");out.SetDouble(v);}
 else if constexpr(std::is_integral_v<T>||std::is_enum_v<T>){if constexpr(std::is_signed_v<T>)out.SetInt64(v);else out.SetUint64(std::uint64_t(v));}
 else if constexpr(std::is_same_v<T,Vector>){out.SetObject();out.AddMember("x",v.x,a);out.AddMember("y",v.y,a);}
 else if constexpr(std::is_same_v<T,BitSet128>){out.SetArray();for(auto n:{v.ll,v.lh,v.hl,v.hh})out.PushBack(n,a);}
 else if constexpr(std::is_same_v<T,ColorMod>){out.SetArray();for(auto n:v._tint)out.PushBack(n,a);for(auto n:v._colorize)out.PushBack(n,a);for(auto n:v._offset)out.PushBack(n,a);}
 else static_assert(!sizeof(T),"未知原生值不能伪装为实测");return out;
}
template<class T> void ClientInspectField(rapidjson::Value &row,const char *name,const T &value,rapidjson::Document::AllocatorType &a){rapidjson::Value key(name,a);row.AddMember(key,ClientInspectValue(value,a),a);}
}
#endif
