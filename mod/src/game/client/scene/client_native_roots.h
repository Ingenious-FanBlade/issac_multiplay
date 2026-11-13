#pragma once
namespace cs::game {
// 只在当前主线程用途窗口借用，根的原生所有权由准备与安装事务管理。
struct ClientNativeRoots final {void *game=nullptr,*room=nullptr,*descriptor=nullptr,*camera=nullptr;};
}
