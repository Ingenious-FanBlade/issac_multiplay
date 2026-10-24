#pragma once
#include <Windows.h>
#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/authority_tick_pipeline.hpp"
#include <array>
#include <cstdint>

namespace isaac_cs_t0::world {
inline constexpr std::uint64_t kMagic=0x31524F5743534949ull;
inline constexpr std::uint32_t kObjectBudget=16384,kMembershipBudget=65536,
    kSourceCount=15,kFieldBudget=52,kEventBudget=65536,kIdentityBudget=32768,kBatchCount=3;
enum class Class : std::uint32_t { Player,NPC,Tear,Projectile,Pickup,Familiar,Effect,Grid,Unknown };
enum class Reason : std::uint32_t { None,PhaseIncomplete,OddOnly,Lifecycle,ReadFailed,
    Budget,RelationBudget,Backpressure,Continuity,WrongThread,Nonfinite,SequenceExhausted,
    RootChanged,EventOverflow,Stopped,UnknownClass,NotReady,SourceChanged,UnpairedScope };
enum class FieldStatus : std::uint8_t { Value,Unconfirmed,NotApplicable,NotReady,ReadFailed };
enum class Encoding : std::uint8_t { Raw,F32,Reference,Derived };
enum class Relation : std::uint32_t { Null,KnownCurrent,Unresolved,Retired,AmbiguousReuse };
enum class Stage : std::uint32_t { Outer,Owner,Room,Tail };
enum class Hook : std::uint32_t { Factory,Init,Publish,Remove,ClearRefs,Destroy,PlayerCtor,
    PlayerCreate,ManagerRestore,Replace,PlayerRestore,NpcMorph,PickupMorph,
    GridFactory,GridInit,GridSpawn,Door,GridFree,Spawn,Count };
struct Field {
    std::uint16_t id;FieldStatus status;Encoding encoding;
    std::uint32_t length,relation,reserved;
    std::uint64_t target_id,target_use;
    std::array<std::uint8_t,16> raw;
};
static_assert(sizeof(Field)==48);
struct Object {
    std::uint64_t id,use,birth,revision;
    std::uint32_t domain,kind,address,vptr,origin,lifecycle,init,
        birth_kind,birth_type,birth_variant,birth_subtype,birth_seed,
        birth_known,ordinal,field_count,reserved;
    std::array<Field,kFieldBudget> fields;
};
struct Membership {std::uint32_t object,source,index,reserved;};
struct Source {std::uint32_t status,count,descriptor[4];};
struct alignas(8) Batch {
    volatile LONG state;std::uint32_t reason;
    std::uint64_t run,room,mutation,attempt,capture,outer,tick,scope;
    std::uint32_t authority,commit_kind,thread,complete,
        object_count,membership_count,required_objects,required_memberships;
    std::array<Source,kSourceCount> sources;
    std::array<Object,kObjectBudget> objects;
    std::array<Membership,kMembershipBudget> memberships;
};
struct alignas(8) Event {
    volatile LONG64 published;
    std::uint64_t run,room,outer,id,use,birth,revision;
    std::uint32_t hook,entering,address,result,kind,reason;
};
static_assert(sizeof(Event)==88);
struct alignas(8) Shared {
    std::uint64_t magic;
    std::uint32_t version,size,pid,controller_pid;
    std::array<std::uint8_t,32> nonce;
    volatile LONG enabled,error,callbacks,closed;
    volatile LONG64 produced,consumed,backpressure,attempts,captures;
    std::array<std::uint64_t,19> enters,returns;
    std::array<Event,kEventBudget> events;
    std::array<Batch,kBatchCount> batches;
};
static_assert(sizeof(Object)==2592 && offsetof(Batch,objects)==464);
static_assert(sizeof(Batch)==43516368 && offsetof(Shared,batches)==5767584);
static_assert(sizeof(Shared)==136316688);
bool Prepare(const wchar_t* name,std::uint32_t pid,std::uint32_t controller,
             const std::array<std::uint8_t,32>& nonce) noexcept;
mode::EntryPatch* Entries() noexcept;
const std::uint32_t* Ordinals() noexcept;
std::size_t EntryCount() noexcept;
void Arm() noexcept;
void Close() noexcept;
bool Drained() noexcept;
void StageEvent(Stage stage,bool entering,void* owner=nullptr) noexcept;
void Lifecycle(std::uint32_t signal,bool entering) noexcept;
void Commit(std::uint64_t tick,std::uint32_t kind) noexcept;
coop_active::TickPostState PendingPostState() noexcept;
bool ObservationIdentity(std::uint64_t expected_tick,std::uint64_t& run,std::uint64_t& room,std::uint64_t& token) noexcept;
bool PublishPostState(bool committed) noexcept;
void PlayerAnchor(std::uint32_t ordinal,void* player,bool valid) noexcept;
bool Observing() noexcept;
void PlayerReadReady(std::uint32_t ordinal,void* player) noexcept;
bool PlayerPairMatches(void* player,void* device,void* control,std::int32_t key) noexcept;
} // namespace isaac_cs_t0::world
