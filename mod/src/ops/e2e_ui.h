#pragma once
#include <istream>
#include <string>
namespace cs {
// 仅E2E链接：主线程只交换自有字符串，磁盘和后台启动全部交给独立worker。
std::string E2eUiCommand(std::istream &arguments);
}
