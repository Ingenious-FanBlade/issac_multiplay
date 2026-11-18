#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace cs::game {
// 背景墙面构建的真实独立随机流：不包含Entity指针、共享玩法RNG或图形句柄。
struct ClientBackdropEffectStep final {
  std::array<std::uint32_t,4> arguments{};
  std::array<float,8> quad{};
  std::uint32_t before=0,after=0;
  bool flip_x=false,flip_y=false;
  bool operator==(const ClientBackdropEffectStep &b) const {return arguments==b.arguments&&quad==b.quad&&before==b.before&&after==b.after&&flip_x==b.flip_x&&flip_y==b.flip_y;}
};
struct ClientBackdropWallTrace final {
  bool present=false,applicable=false;
  std::uint32_t initial=0,before=0,after=0;
  std::vector<ClientBackdropEffectStep> steps;
  bool operator==(const ClientBackdropWallTrace &b) const {return present==b.present&&applicable==b.applicable&&initial==b.initial&&before==b.before&&after==b.after&&steps==b.steps;}
};
// Source在原调用前后只复制值。Client在声明的NativeApply内使用本机背景构图。
class ClientBackdropBuildScope final {
public:
  explicit ClientBackdropBuildScope(void *) noexcept;
  ~ClientBackdropBuildScope();
  bool Ready() const noexcept {return allowed_;}
  void Returned() noexcept;
private:
  void *backdrop_=nullptr;
  bool source_=false,returned_=false;
  bool allowed_=true;
  unsigned initial_=0;
};
class ClientBackdropWallScope final {
public:
  explicit ClientBackdropWallScope(void *) noexcept;
  ~ClientBackdropWallScope();
  bool Ready() const noexcept {return allowed_;}
  void Returned() noexcept;
private:
  void *backdrop_=nullptr;
  bool source_=false,client_=false,returned_=false;
  bool allowed_=true;
  unsigned null_count_=0;
  bool nulls_borrowed_=false;
};
class ClientBackdropEffectScope final {
public:
  ClientBackdropEffectScope(void *,int,int,unsigned,int,const void *,unsigned char,bool) noexcept;
  ~ClientBackdropEffectScope();
  bool Handled() const noexcept {return client_;}
  void Returned() noexcept;
private:
  void *backdrop_=nullptr;
  ClientBackdropEffectStep step_{};
  bool source_=false,client_=false,returned_=false;
};
// 允许当前Client Room用自身外观随机流构图，仍跳过原生装饰生成。
class ClientBackdropLocalBuild final {
public:
  explicit ClientBackdropLocalBuild(void *) noexcept;
  ~ClientBackdropLocalBuild();
  bool Complete() const noexcept;
private:
  bool active_=false;
};
int ClientBackdropWallTraceLua(void *);
bool ClientBackdropSkipDirectEffect(void *) noexcept;
}
