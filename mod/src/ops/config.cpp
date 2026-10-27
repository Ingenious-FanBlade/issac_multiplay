#include "runtime.h"
#include <fstream>
#include <iterator>
#include <rapidjson/document.h>
#include <stdexcept>
#include <ws2tcpip.h>
namespace cs {
// 配置缺失、格式错误或未知 schema 均默认关闭，不注册任何 Hook。
Config ReadConfig() {
  Config c;
  std::ifstream file("isaac-cs/config.json", std::ios::binary);
  if (!file) {
    c.reason = "config_missing";
    return c;
  }
  const std::string text((std::istreambuf_iterator<char>(file)), {});
  if (text.size() > 65536) {
    c.reason = "config_too_large";
    return c;
  }
  rapidjson::Document d;
  d.Parse(text.data(), text.size());
  if (d.HasParseError() || !d.IsObject() || !d.HasMember("schema") || !d["schema"].IsUint() ||
      d["schema"].GetUint() != 1) {
    c.reason = "config_schema";
    return c;
  }
  try {
    if (!d.HasMember("enabled") || !d["enabled"].IsBool())
      throw std::runtime_error("enabled");
    c.enabled = d["enabled"].GetBool();
    if (d.HasMember("trace_dir")) {
      if (!d["trace_dir"].IsString())
        throw std::runtime_error("trace_dir");
      c.trace_dir = d["trace_dir"].GetString();
    }
    if (d.HasMember("log_level")) {
      if (!d["log_level"].IsString())
        throw std::runtime_error("log_level");
      c.log_level = d["log_level"].GetString();
    }
    // 端口严格限定到有效 TCP 范围。
    auto port = [](const rapidjson::Value &v, const char *key) -> unsigned {
      if (!v.IsObject() || !v.HasMember(key) || !v[key].IsUint() || v[key].GetUint() < 1 ||
          v[key].GetUint() > 65535)
        throw std::runtime_error("port");
      return v[key].GetUint();
    };
    if (d.HasMember("host")) {
      c.host_port = port(d["host"], "default_port");
      auto &host = d["host"];
      if (host.HasMember("bind_address")) {
        const auto &v = host["bind_address"];
        IN_ADDR address{};
        if (!v.IsString() || v.GetStringLength() == 0 ||
            std::strlen(v.GetString()) != v.GetStringLength() ||
            InetPtonA(AF_INET, v.GetString(), &address) != 1)
          throw std::runtime_error("host.bind_address");
        c.host_bind_address = v.GetString();
      }
      if (host.HasMember("peer_timeout_ms")) {
        const auto &v = host["peer_timeout_ms"];
        if (!v.IsUint() || v.GetUint() < 500 || v.GetUint() > 60000)
          throw std::runtime_error("host.peer_timeout_ms");
        c.peer_timeout_ms = v.GetUint();
      }
    }
    if (d.HasMember("control_port")) {
      auto &v = d["control_port"];
      if (!v.IsObject() || !v.HasMember("enabled") || !v["enabled"].IsBool())
        throw std::runtime_error("control_port");
      c.control = v["enabled"].GetBool();
      c.control_port = port(v, "port");
    }
    if (d.HasMember("world_record")) {
      const auto &v = d["world_record"];
      if (!v.IsObject() || !v.HasMember("enabled") || !v["enabled"].IsBool())
        throw std::runtime_error("world_record.enabled");
      c.world_record_enabled = v["enabled"].GetBool();
      if (v.HasMember("output_dir")) {
        if (!v["output_dir"].IsString() || !v["output_dir"].GetStringLength() ||
            v["output_dir"].GetStringLength() > 240 ||
            std::strlen(v["output_dir"].GetString()) != v["output_dir"].GetStringLength())
          throw std::runtime_error("world_record.output_dir");
        c.world_output_dir = v["output_dir"].GetString();
      }
      if (v.HasMember("close_timeout_ms")) {
        if (!v["close_timeout_ms"].IsUint() || !v["close_timeout_ms"].GetUint() ||
            v["close_timeout_ms"].GetUint() > 60000)
          throw std::runtime_error("world_record.close_timeout_ms");
        c.world_close_timeout_ms = v["close_timeout_ms"].GetUint();
      }
    }
    if (!c.enabled)
      c.reason = "config_disabled";
  } catch (const std::exception &e) {
    c.enabled = false;
    c.reason = std::string("config_invalid:") + e.what();
  }
  return c;
}
} // namespace cs
