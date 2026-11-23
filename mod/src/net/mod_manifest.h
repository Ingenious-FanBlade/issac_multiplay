#pragma once
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace cs::net {
// 握手只交换已封存的纯值。清单键在两端使用同一排序，描述正文按字节比较。
using ModManifest = std::map<std::string, std::string>;
using ModManifestRef = std::shared_ptr<const ModManifest>;
inline constexpr unsigned kModManifestBytes = 60 * 1024;
inline constexpr unsigned kModManifestChunk = 4096;
inline constexpr char kModManifestCapability[] = "mod-manifest-v1";

// 错误详情会进入大厅的逐行状态文本，清理换行及控制字符。
std::string ModManifestDetail(std::string_view text);
}
