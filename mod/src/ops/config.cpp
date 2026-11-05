#include "runtime.h"
#include "core/replication_memory.h"
#include <fstream>
#include <iterator>
#include <rapidjson/document.h>
#include <stdexcept>
#include <set>
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
  if (d.HasParseError() || !d.IsObject() || !d.HasMember("schema") ||
      !d["schema"].IsUint() || d["schema"].GetUint() != 1) {
    c.reason = "config_schema";
    return c;
  }
  try {
    if (!d.HasMember("enabled") || !d["enabled"].IsBool())
      throw std::runtime_error("enabled");
    c.enabled = d["enabled"].GetBool();
    if (d.HasMember("diagnostics")) {
      const auto &v = d["diagnostics"];
      if (!v.IsObject() || !v.HasMember("enabled") || !v["enabled"].IsBool())
        throw std::runtime_error("diagnostics.enabled");
      c.diagnostics = v["enabled"].GetBool();
      if (v.HasMember("repeat_topology_reads")) {
        if (!v["repeat_topology_reads"].IsBool())
          throw std::runtime_error("diagnostics.repeat_topology_reads");
        c.repeat_topology_reads =
            c.diagnostics && v["repeat_topology_reads"].GetBool();
      }
    }
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
      if (!v.IsObject() || !v.HasMember(key) || !v[key].IsUint() ||
          v[key].GetUint() < 1 || v[key].GetUint() > 65535)
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
      if (v.HasMember("e2e_token")) {
#ifndef ISAACCS_E2E
        throw std::runtime_error("e2e_token_requires_e2e_build");
#endif
        const auto &token = v["e2e_token"];
        if (!token.IsString() || !token.GetStringLength() ||
            token.GetStringLength() > 96 ||
            std::strlen(token.GetString()) != token.GetStringLength())
          throw std::runtime_error("world_record.e2e_token");
        c.world_e2e_token = token.GetString();
        if (c.world_e2e_token.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                "_.-") != std::string::npos)
          throw std::runtime_error("world_record.e2e_token_chars");
      }
      if (v.HasMember("output_dir")) {
        if (!v["output_dir"].IsString() || !v["output_dir"].GetStringLength() ||
            v["output_dir"].GetStringLength() > 240 ||
            std::strlen(v["output_dir"].GetString()) !=
                v["output_dir"].GetStringLength())
          throw std::runtime_error("world_record.output_dir");
        c.world_output_dir = v["output_dir"].GetString();
        for (const unsigned char byte : c.world_output_dir)
          if (byte < 0x20 || byte == 0x7f)
            throw std::runtime_error("world_record.output_dir_control");
      }
      if (v.HasMember("close_timeout_ms")) {
        if (!v["close_timeout_ms"].IsUint() ||
            !v["close_timeout_ms"].GetUint() ||
            v["close_timeout_ms"].GetUint() > 60000)
          throw std::runtime_error("world_record.close_timeout_ms");
        c.world_close_timeout_ms = v["close_timeout_ms"].GetUint();
      }
    }
    // 严格接纳具名配置；未知字段、负数及溢出均拒绝，不悄悄采用另一档。
    c.stream.peer_timeout_ms=c.peer_timeout_ms;
    if(d.HasMember("stream")) {
      const auto &v=d["stream"];
      if(!v.IsObject())throw std::runtime_error("stream.object");
      std::set<std::string> seen;
      for(auto i=v.MemberBegin();i!=v.MemberEnd();++i) {
        if(std::strlen(i->name.GetString())!=i->name.GetStringLength()||!seen.insert(i->name.GetString()).second)throw std::runtime_error("stream.duplicate_or_invalid_name");
        bool known=false;
#define CS_STREAM_PARSE(name,initial,lower,upper) if(std::strcmp(i->name.GetString(),#name)==0){if(!i->value.IsUint64())throw std::runtime_error("stream." #name);c.stream.name=i->value.GetUint64();known=true;}
        CS_STREAM_SETTINGS(CS_STREAM_PARSE)
#undef CS_STREAM_PARSE
        for(const char *flag:{"acceptance_profile","borrow_initial_lanes"})if(std::strcmp(i->name.GetString(),flag)==0){if(!i->value.IsBool())throw std::runtime_error(flag);if(!std::strcmp(flag,"acceptance_profile"))c.stream.acceptance_profile=i->value.GetBool();else c.stream.borrow_initial_lanes=i->value.GetBool();known=true;}
        if(!known)throw std::runtime_error("stream.unknown_field");
      }
      if(d.HasMember("host")&&d["host"].HasMember("peer_timeout_ms")&&c.stream.peer_timeout_ms!=c.peer_timeout_ms)throw std::runtime_error("stream.host_timeout_conflict");
    }
    if(const auto *why=c.stream.Validate();*why)throw std::runtime_error(why);
    c.peer_timeout_ms=unsigned(c.stream.peer_timeout_ms);
    if(c.enabled&&!memory::ReplicationBudget()->Configure({c.stream.value_pool_bytes,c.stream.wire_pool_bytes,c.stream.workspace_pool_bytes}))throw std::runtime_error("stream.pool_in_use");
    if (!c.enabled)
      c.reason = "config_disabled";
  } catch (const std::exception &e) {
    c.enabled = false;
    c.reason = std::string("config_invalid:") + e.what();
  }
  return c;
}
} // namespace cs
