#pragma once
#include <string>
namespace cs {
// 专用E2E配置内的正式consumer导出；没有客户端原生世界写入能力。
std::string ExportClientFixture(const std::string &token,const std::string &label,bool entity19=false) noexcept;
std::string ImportClientIndependent(const std::string &token,const std::string &label) noexcept;
std::string ClientFixtureSampling(const std::string &token,const std::string &mode) noexcept;
}
