#ifdef ISAACCS_E2E
#include "IsaacRepentance.h"
#include "game/world_sources/random_sources.h"
#include "runtime.h"
#include <sstream>
#include <cstdlib>
#include <cstring>
namespace cs::game {
namespace {
void Array(std::ostringstream &o,const void *data,unsigned words){
  const auto *p=static_cast<const std::uint32_t *>(data);o<<'[';for(unsigned i=0;i<words;++i){if(i)o<<',';o<<p[i];}o<<']';
}
}
// 仅为真实进程E2E开放的动作入口；空token配置下不能到达，不被正式观测调用。
std::string TestNativeRandomSources(const char *token,const char *mode,unsigned count,unsigned seed){
  if(!IsMain()||Settings.world_e2e_token.empty()||Settings.world_e2e_token!=token||count>2048)throw std::runtime_error("native_random_test_disabled_or_bounds");
  std::ostringstream o;
  if(!std::strcmp(mode,"providers"))return RuntimeRandomDiagnostics();
  if(!std::strcmp(mode,"floatstate"))return EncodeRandomWords(CaptureRuntimeRandomWords(2),RandomAlgorithm::MsvcMT19937);
  if(!std::strcmp(mode,"mathstate"))return EncodeRandomWords(CaptureRuntimeRandomWords(3),RandomAlgorithm::Xoshiro256);
  if(!std::strcmp(mode,"crtstate"))return EncodeRandomWords(CaptureRuntimeRandomWords(0),RandomAlgorithm::CrtRand);
  if(!std::strcmp(mode,"primitives")){
    RNG rng{};rng.game_constructor(1234567,35);rng.SetSeed(seed?seed:1,35);
    o<<"{\"before\":";Array(o,&rng,4);o<<",\"results\":[";
    for(unsigned i=0;i<count;++i){
      if(i)o<<',';const auto kind=i%3;std::uint32_t bits=0;const unsigned max=i%7?1009u:0u;
      if(kind==0)bits=rng.Next();else if(kind==1)bits=rng.RandomInt(max);else{const auto f=rng.RandomFloat();std::memcpy(&bits,&f,4);}
      o<<"{\"kind\":"<<kind<<",\"argument\":"<<max<<",\"bits\":"<<bits<<'}';
    }o<<"],\"after\":";Array(o,&rng,4);o<<'}';return o.str();
  }
  if(!std::strcmp(mode,"mt")){
    MTRNG rng{};rng.Init(seed);o<<"{\"before\":";Array(o,&rng,625);o<<",\"results\":[";
    for(unsigned i=0;i<count;++i){if(i)o<<',';o<<rng.Next();}o<<"],\"after\":";Array(o,&rng,625);o<<'}';return o.str();
  }
  if(!std::strcmp(mode,"crt")){
    std::srand(seed);const auto before=CaptureRuntimeRandomWords(0);
    o<<"{\"before\":"<<EncodeRandomWords(before,RandomAlgorithm::CrtRand)<<",\"results\":[";
    for(unsigned i=0;i<count;++i){if(i)o<<',';o<<std::rand();}
    o<<"],\"after\":"<<EncodeRandomWords(CaptureRuntimeRandomWords(0),RandomAlgorithm::CrtRand)<<'}';return o.str();
  }
  if(!std::strcmp(mode,"inline")){
    RNG rng{};rng.game_constructor(seed?seed:1,35);
    const RandomBinding a{&rng,RandomAlgorithm::XorShift32,"{\"namespace\":\"E2E.call-scope\"}","E2E.inline","native-test-call","test-inline"};
    BindRandomSource(a);const auto first=RandomSourceReference(&rng);o<<"{\"before\":";Array(o,&rng,4);
    for(unsigned i=0;i<count;++i){auto v=rng._seed;v^=v>>rng._shift1;v^=v<<rng._shift2;v^=v>>rng._shift3;rng._seed=v;}
    BindRandomSource(a);o<<",\"after\":";Array(o,&rng,4);
    RNG copy=rng;BindRandomSource({&copy,RandomAlgorithm::XorShift32,"{\"namespace\":\"E2E.call-scope\"}","E2E.copy","native-test-call","test-copy"});
    const auto copied=RandomSourceReference(&copy);
    rng.game_constructor(7654321,35);BindRandomSource(a);const auto reused=RandomSourceReference(&rng);
    o<<",\"first\":"<<first<<",\"copy\":"<<copied<<",\"reused\":"<<reused<<'}';
    RetireRandomSource(&rng,"test-call-ended");RetireRandomSource(&copy,"test-call-ended");return o.str();
  }
  if(!std::strcmp(mode,"owners")){
    if(!g_Game)throw std::runtime_error("no_game");
    o<<"{\"players\":[";bool first=true;
    const unsigned offsets[]={0x1760,0x176c,0x1778,0x1784};
    for(auto *player:g_Game->_playerManager._playerList){
      if(!player)continue;if(!first)o<<',';first=false;o<<"{\"counts\":[";
      for(unsigned k=0;k<4;++k){
        if(k)o<<',';std::uint32_t v[3]{};SIZE_T bytes=0;
        if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<unsigned char *>(player)+offsets[k],v,sizeof(v),&bytes)||bytes!=sizeof(v)||v[1]<v[0]||v[2]<v[1]||(v[1]-v[0])%16)throw std::runtime_error("player_rng_vector");
        o<<(v[1]-v[0])/16;
      }o<<"]}";
    }o<<"]}";return o.str();
  }
  throw std::runtime_error("unknown_native_random_test");
}
}

#endif
