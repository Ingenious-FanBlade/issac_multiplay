#include "net/mod_manifest_wire.h"
#include <algorithm>

namespace cs::net {
namespace s = replica::stream;
namespace r = replica;
namespace {
// 网络使用带长度的净字符串列表，保留规范声明中的全部原始字节。
void AppendField(std::string &out, const std::string &value) {
  if (value.size() > kModManifestBytes ||
      out.size() + value.size() + 10 > kModManifestBytes)
    throw r::ContractError("mod_manifest_too_large");
  out += std::to_string(value.size());
  out += ':';
  out += value;
}

std::string ReadField(std::string_view &rest) {
  const auto separator = rest.find(':');
  if (!separator || separator > 5)
    throw r::ContractError("mod_manifest_invalid");
  unsigned length = 0;
  for (const auto ch : rest.substr(0, separator)) {
    if (ch < '0' || ch > '9') throw r::ContractError("mod_manifest_invalid");
    length = length * 10 + unsigned(ch - '0');
  }
  rest.remove_prefix(separator + 1);
  if (length > rest.size()) throw r::ContractError("mod_manifest_invalid");
  std::string field(rest.substr(0, length));
  rest.remove_prefix(length);
  return field;
}
}

std::string ModManifestDetail(std::string_view text) {
  std::string safe;
  for (std::size_t i=0;i<text.size() && safe.size()<976;) {
    const auto lead=static_cast<unsigned char>(text[i]);
    const auto width=lead<0x80?1u:lead>=0xc2&&lead<=0xdf?2u:lead>=0xe0&&lead<=0xef?3u:lead>=0xf0&&lead<=0xf4?4u:0u;
    bool valid=width && width<=text.size()-i;
    if (valid && width>1) {
      const auto second=static_cast<unsigned char>(text[i+1]);
      valid=!((lead==0xe0&&second<0xa0)||(lead==0xed&&second>=0xa0)||
              (lead==0xf0&&second<0x90)||(lead==0xf4&&second>=0x90));
      for (unsigned j=1;j<width;++j)
        valid=valid && (static_cast<unsigned char>(text[i+j])&0xc0)==0x80;
    }
    if (!valid || lead<32) {
      safe+=valid?' ':'?';
      ++i;
    } else {
      if (safe.size()+width>976) break;
      safe.append(text.substr(i,width));
      i+=width;
    }
  }
  return safe;
}

void WriteModManifest(s::Message &message, const ModManifestRef &manifest) {
  std::string bytes;
  if (manifest) {
    for (const auto &[key, value] : *manifest) {
      AppendField(bytes, key);
      AppendField(bytes, value);
    }
  }
  auto &a = message.body.GetAllocator();
  r::Value chunks(rapidjson::kArrayType);
  for (std::size_t offset = 0; offset < bytes.size(); offset += kModManifestChunk) {
    const auto count = (std::min)(std::size_t(kModManifestChunk), bytes.size() - offset);
    chunks.PushBack(r::Value(bytes.data() + offset, rapidjson::SizeType(count), a), a);
  }
  r::Put(message.body, "chunks", std::move(chunks), a);
}

// 有序比较显示首个异项；远端只提供内容，从不参与本机声明编译。
std::string CompareModManifest(const ModManifestRef &local, const r::Value &chunks) {
  std::string bytes;
  for (const auto &part : chunks.GetArray())
    bytes.append(part.GetString(), part.GetStringLength());
  const ModManifest empty;
  const auto &expected = local ? *local : empty;
  auto item = expected.begin();
  std::string_view remaining(bytes);
  std::string previous, difference;
  while (!remaining.empty()) {
    auto key = ReadField(remaining);
    auto value = ReadField(remaining);
    if (key.size()<=2 || key[1]!=' ' || key <= previous)
      throw r::ContractError("mod_manifest_invalid");
    previous = key;
    if (!difference.empty()) continue;
    if (item == expected.end() || key < item->first)
      difference = "本端缺少 " + key.substr(2);
    else if (item->first < key)
      difference = "对端缺少 " + item->first.substr(2);
    else if (item->second != value) {
      difference = "两端内容不同 " + key.substr(2);
      if(key[0]!='6')difference+="（本端 "+item->second+"，对端 "+value+"）";
    } else
      ++item;
  }
  if (difference.empty() && item != expected.end())
    difference = "对端缺少 " + item->first.substr(2);
  return difference.empty() ? difference : "mod_manifest: " + ModManifestDetail(difference);
}
}
