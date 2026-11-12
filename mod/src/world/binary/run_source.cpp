#include "world/binary/entity_source_internal.h"

namespace cs::world::binary {
EntityRead RunRecipe(std::string_view name) noexcept {
  return name=="run_seed_display_flags_read" || name=="run_daily_display_read" || name=="run_ascent_display_read" ||
    name=="run_time_counter_anchor_read" || name=="run_hud_progress_result_observed"?EntityRead::Run:EntityRead::None;
}
// 只读当前已声明Run窄投影；分数与最终进度资格必须有正常生产证言，不调用其生产器。
bool EntitySource::Impl::RunLeaf(const CaptureFieldAccess &a,CaptureValueWriter &writer,CaptureDiagnostic &d) noexcept {
  std::uint64_t result=0;const auto address=std::uint64_t(a.root.address)+a.field.native_offset;
  if(a.field.layout==230){
    if(!facts.scene)return Reject(a,d,CaptureFailure::MissingQualification);
    constexpr unsigned effects[]={8,9,10,29,30,78,79};for(unsigned i=0;i<std::size(effects);++i)if(facts.scene->seed_effects[effects[i]])result|=std::uint64_t{1}<<i;
  }else if(a.field.layout==233){std::uint32_t id=0;if(!Value(address,id,d))return false;result=id!=0;}
  else if(a.field.layout==236){std::uint32_t upper=0;if(!Value(address,upper,d))return false;result=(upper>>16)&1u;}
  else if(a.field.layout==237 && std::string_view(a.field.name)=="time_counter_base"){
    std::uint32_t counter=0;if(!Value(address,counter,d))return false;result=std::uint32_t(a.window.witness.game_frame-counter);
  }else if(a.field.layout==237){if(!facts.progress_ready)return Reject(a,d,CaptureFailure::MissingQualification);result=facts.progress_blocked;}
  else return Reject(a,d,CaptureFailure::MissingHandler);
  std::array<std::byte,8> bytes{};const auto width=CaptureScalarWidth(a.format.scalar);
  if(!width || width>8 || (width<8 && result>>(width*8)))return Reject(a,d,CaptureFailure::InvalidValue);
  for(unsigned i=0;i<width;++i)bytes[i]=std::byte(result>>(8*i));return writer.PutScalar(a.location,a.format.scalar,1,std::span(bytes).first(width));
}
}
