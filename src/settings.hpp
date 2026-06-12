#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace dictate {

struct Settings {
  enum class Mode { Hold, Toggle };
  enum class Model { TinyEn, BaseEn, SmallEn, MediumEn, LargeV3 };

  Mode mode = Mode::Hold;
  std::string hotkey;
  Model model = Model::TinyEn;
};

extern Settings g_settings;

std::optional<Settings> load_settings();
std::filesystem::path config_path();
const char *model_filename(Settings::Model model);

} // namespace dictate
