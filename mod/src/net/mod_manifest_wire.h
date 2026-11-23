#pragma once
#include "net/mod_manifest.h"
#include "replica/stream_codec.h"

namespace cs::net {
// 同一可靠握手轮次生成清单正文；确认前丢失或乱序由原握手重发处理。
void WriteModManifest(replica::stream::Message &, const ModManifestRef &);
std::string CompareModManifest(const ModManifestRef &, const replica::Value &chunks);
}
