#include "world/binary/entity_source_internal.h"

namespace cs::world::binary {
// 声音资源只用正常已选sample路径；文件字节、PCM与私有选样本RNG不上线。
bool EntitySource::AudioResource(const CaptureWitness &w,EntityFacts f,std::uint32_t sample,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;if(s.active || std::this_thread::get_id()!=s.thread)return FailCapture(d,CaptureFailure::Reentrant,{f.subject});
  s.facts=f;s.active=true;s.scratch_used=s.witness_used=0;
  struct End{bool &active;~End(){active=false;}}end{s.active};
  s.window={s.thread,w,&s,[](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept {return static_cast<Impl *>(p)->Current(v,r);},nullptr};
  std::array<std::byte,4096> path{};std::uint32_t size=0;
  if(!s.StringAt(sample,path,size,d,true) || !size)return FailCapture(d,CaptureFailure::ResourcePending,{f.subject});
  for(auto &b:std::span(path).first(size))if(b==std::byte{'\\'})b=std::byte{'/'};
  const std::string_view text(reinterpret_cast<const char *>(path.data()),size);
  if(!(text.starts_with("sfx/")||text.starts_with("music/")) || text.find("..")!=text.npos || text.find_first_of(":<>\"|?*")!=text.npos ||
     !(text.ends_with(".wav")||text.ends_with(".ogg")||text.ends_with(".mp3")))return FailCapture(d,CaptureFailure::ResourcePending,{f.subject});
  if(!s.Verify({f.subject,f.address,0,f.layout},d))return false;
  NativeResource resource;resource.table=7;resource.kind=9;resource.values[0]=0;resource.values[1]=0;
  return s.Intern(resource,std::span(path).first(size),{},out,d);
}
}
