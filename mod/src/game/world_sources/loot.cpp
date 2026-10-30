#include "game/world_sources/loot.h"
#include "game/world_sources/lua_entity.h"
#include "runtime.h"
#include <atomic>
#include <exception>
#include <sstream>
namespace cs::game {
namespace {
std::atomic<bool> armed{false};
std::atomic<std::uint64_t> foreign_count{0},exception_count{0},budget_count{0};
}
void ArmObservationValueSources(bool value) noexcept { armed.store(value,std::memory_order_release); }
ValueSourceFailures ObservationValueSourceFailures() noexcept { return {foreign_count.load(),exception_count.load(),budget_count.load()}; }
ActualValueScope::ActualValueScope() noexcept : main(IsMain()),armed_(armed.load(std::memory_order_acquire)),exceptions_(std::uncaught_exceptions()) {
  if(armed_&&!main) ++foreign_count;
}
ActualValueScope::~ActualValueScope() { if(armed_&&std::uncaught_exceptions()>exceptions_) ++exception_count; }
void ObserveActualLoot(void *owner,const char *member,bool advance,const LootList &values) noexcept {
  if(!IsMain()||!armed.load(std::memory_order_acquire)||!ObservationLuaNeeded("ActualCall")) return;
  try {
    const auto subject=ObservationSubject(owner,true);
    std::ostringstream json;
    json<<"{\"member\":\""<<member<<"\",\"subject\":{\"local_id\":\""<<subject.local
        <<"\",\"use\":\""<<subject.use<<"\"},\"should_advance\":"<<(advance?"true":"false")<<",\"entries\":[";
    std::uint32_t count=0;
    for(const auto &entry:values) {
      if(count==4096) break;
      if(count) json<<',';
      json<<"{\"type\":"<<entry._type<<",\"variant\":"<<entry._variant<<",\"subtype\":"<<entry._subType
          <<",\"seed\":"<<entry._seed<<",\"rng_reference\":{\"resolution\":\""<<(entry._rng?"UnsupportedTarget":"Null")
          <<"\",\"owner_field\":\"loot-entry\",\"responsible_tx\":\"tx-04-14\"}}";
      ++count;
    }
    const bool complete=values.size()<=4096&&subject.local;
    json<<"],\"required_entries\":"<<values.size()<<",\"truncated\":"<<(values.size()>4096?"true":"false")<<'}';
    const auto payload=json.str();
    if(!ObservationPublishLua(payload.data(),payload.size(),complete,1,count*5,complete?"":"loot_entry_budget_or_subject",3)) ++budget_count;
  } catch(...) { ++budget_count; }
}
}
