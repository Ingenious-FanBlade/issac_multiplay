#pragma once
#include <string>
namespace cs::game {
// 所有常驻wrapper共用这一角色判断；Host/Off不进入Client策略。
bool ClientSuppress(const char *entry) noexcept;
std::string ClientIsolationFields();
// Shutdown与析构中的直接写入另经清单补丁处理，不能只靠PGD方法hook。
bool ClientProgressProtected() noexcept;
void ClientProtectionBegin() noexcept;
void ClientProtectionEnd() noexcept;
void ClientOriginalRender(void *manager);
void ClientOriginalManagerDestroy(void *manager);
} // namespace cs::game
