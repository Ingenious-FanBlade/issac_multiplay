#pragma once
#include "game/observation/binary_production.h"
#include <array>

namespace cs::game {
// 正常API已接受但room生命周期尚未收束时，仅保存这些自有意图，稍后绑定合法world。
// frame是原调用的Host帧；stream只作本进程主线程逻辑别名，不进入packet。
struct BinaryMusicIntent {
  BinaryProductionKind kind=BinaryProductionKind::MusicPlay;
  std::array<std::uint32_t,5> args{};
  int previous=0,current=0,queued=0;
};
struct BinarySfxIntent {
  BinaryProductionKind kind=BinaryProductionKind::SfxPlay;
  int id=0,delay=0,ambient=-1;
  float volume=0,pitch=1,pan=0;
  bool loop=false;
  std::uint32_t stream=0,frame=0,path_size=0;
  std::array<char,4096> path{};
};
bool BinaryIntentActive() noexcept;
std::uint32_t BinaryNativeMusicCurrent(void *) noexcept;
void BinaryNativeMusicFallback(BinaryProductionKind,void *,const std::array<std::uint32_t,5> &,std::uint32_t) noexcept;
void BinaryNativeSfxFallback(BinaryProductionKind,void *,int,float,int,bool,float,float,void *,int) noexcept;
void BinaryMusicIntentObserved(const BinaryMusicIntent &) noexcept;
void BinarySfxIntentObserved(const BinarySfxIntent &) noexcept;
// Map／Overlay只保留正常调用已选出的主线程关联，稍后普通窗口读取其当前状态。
void BinaryNativeMapFocusFallback(void *,std::uint32_t) noexcept;
void BinaryMapFocusObserved(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) noexcept;
void BinaryNativeOverlayFallback(void *,const std::array<std::uint32_t,5> &) noexcept;
void BinaryOverlayObserved(std::uint32_t,std::int32_t) noexcept;
void BinaryNativeBossFallback(void *,const std::array<std::uint32_t,5> &) noexcept;
void BinaryBossObserved(std::uint32_t,std::uint32_t,std::uint32_t,std::int32_t,std::int32_t) noexcept;
}
