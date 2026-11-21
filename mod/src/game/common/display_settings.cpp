#include "game/common/display_settings.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <rapidjson/document.h>
#include <string>

namespace cs::game {
namespace {
constexpr const char *kPath = "isaac-cs/display.json";
constexpr const char *kTemporaryPath = "isaac-cs/display.json.tmp";
// 描边颜色表：下标与 UI 下拉框一一对应，0 号（关闭）没有颜色。
// 描边按纯色剪影绘制（原图 RGB 乘 0 再加这里的偏移），所以这些就是屏幕上的最终颜色。
constexpr OutlineRgb kOutlineColors[kOutlineColorCount] = {
    {0.00f, 0.00f, 0.00f}, // 0 关闭（不使用）
    {1.00f, 1.00f, 1.00f}, // 1 白
    {1.00f, 0.25f, 0.25f}, // 2 红
    {1.00f, 0.60f, 0.15f}, // 3 橙
    {1.00f, 0.92f, 0.20f}, // 4 黄
    {0.35f, 1.00f, 0.35f}, // 5 绿
    {0.25f, 0.95f, 1.00f}, // 6 青
    {0.30f, 0.50f, 1.00f}, // 7 蓝
    {0.75f, 0.40f, 1.00f}, // 8 紫
    {1.00f, 0.45f, 0.80f}, // 9 粉
};
DisplayPreferences current;
bool loaded = false;

// 读取 display.json；只接受 schema 1 与范围内的整数，其余字段或越界值回落为默认。
void Load() noexcept {
  loaded = true;
  try {
    std::ifstream file(kPath, std::ios::binary);
    if (!file) return;
    const std::string text((std::istreambuf_iterator<char>(file)), {});
    if (text.size() > 4096) return;
    rapidjson::Document d;
    d.Parse(text.data(), text.size());
    if (d.HasParseError() || !d.IsObject() || !d.HasMember("schema") || !d["schema"].IsUint() ||
        d["schema"].GetUint() != 1)
      return;
    if (d.HasMember("outline_color") && d["outline_color"].IsUint() &&
        d["outline_color"].GetUint() < kOutlineColorCount)
      current.outline_color = d["outline_color"].GetUint();
    if (d.HasMember("remote_opacity") && d["remote_opacity"].IsUint() &&
        d["remote_opacity"].GetUint() <= kRemoteOpacityMax)
      current.remote_opacity = d["remote_opacity"].GetUint();
  } catch (...) {
  }
}
// 先写临时文件再替换，写失败时旧文件保持完整。
bool Save() noexcept {
  try {
    {
      std::ofstream file(kTemporaryPath, std::ios::binary | std::ios::trunc);
      if (!file) return false;
      file << "{\n  \"schema\": 1,\n  \"outline_color\": " << current.outline_color
           << ",\n  \"remote_opacity\": " << current.remote_opacity << "\n}\n";
      if (!file.good()) return false;
    }
    std::remove(kPath);
    return std::rename(kTemporaryPath, kPath) == 0;
  } catch (...) {
    return false;
  }
}
} // namespace

DisplayPreferences CurrentDisplayPreferences() noexcept {
  if (!loaded) Load();
  return current;
}
bool SetOutlineColor(unsigned index, bool &saved) noexcept {
  saved = false;
  if (!loaded) Load();
  if (index >= kOutlineColorCount) return false;
  current.outline_color = index;
  saved = Save();
  return true;
}
bool SetRemoteOpacity(unsigned level, bool &saved) noexcept {
  saved = false;
  if (!loaded) Load();
  if (level > kRemoteOpacityMax) return false;
  current.remote_opacity = level;
  saved = Save();
  return true;
}
bool OutlineColorRgb(unsigned index, OutlineRgb &rgb) noexcept {
  if (!index || index >= kOutlineColorCount) return false;
  rgb = kOutlineColors[index];
  return true;
}
float RemoteOpacityFactor(unsigned level) noexcept {
  if (level > kRemoteOpacityMax) level = kRemoteOpacityMax;
  return 0.1f + 0.09f * float(level);
}
} // namespace cs::game
