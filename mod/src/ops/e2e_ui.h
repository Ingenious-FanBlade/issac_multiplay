#pragma once
#include <istream>
#include <string>
namespace cs {
// 仅E2E链接：主线程只交换自有字符串，磁盘和后台启动全部交给独立worker。
std::string E2eUiCommand(std::istream &arguments);
// 原生Manager正常返回后再交接；Render回调不直接发布会恢复测试操作的回答。
void E2eUiManagerReturned() noexcept;
// 仅E2E的纯值计数，供自动按键和采样排除UI屏蔽/渲染窗口。
std::string E2eUiFields();
}
