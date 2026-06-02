#pragma once

#include <filesystem>
#include <string>

namespace dictate {

struct Settings {
  enum class Mode { Hold, Toggle };
  Mode mode = Mode::Hold;
  std::string hotkey = "control+shift+d";
  std::string model = "ggml-tiny.en.bin";
};

extern Settings g_settings;

Settings load_settings();
bool save_settings(const Settings &s);

std::filesystem::path config_path();
std::filesystem::path replacements_path();

} // namespace dictate
