#pragma once
#include <cstdint>

namespace cs::state {
// Q12只描述Host Update后的显示分支，不能把paused当作相同选择器。
enum class DisplayBranch : std::uint8_t {
  Normal=0,ConsoleHold=1,PauseCountdown=2,GameOver=3,Leaderboard=4,StageTransition=5,
  RoomTransition=6,ActiveOverlay=7,GenericPrompt=8,UnknownUiByte=9,SecondaryFade=10,PauseScreen=11,UnknownUiWord=12
};
// Queue自然接续由本机音乐tick维护；不能当成外部Play(0音量)再次重播。
enum class MusicRequestMode : std::uint8_t {
  Idle=0,Play=1,Crossfade=2,Fadein=3,Fadeout=4,QueuedContinuation=5
};
}
