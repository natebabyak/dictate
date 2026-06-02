#include "settings.hpp"

#include "paths.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace dictate {

Settings g_settings;

std::filesystem::path config_path() { return exe_dir() / "dictate.conf"; }

std::filesystem::path replacements_path() {
  return exe_dir() / "replacements.txt";
}

static std::string trim(std::string s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
    s.erase(s.begin());
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    s.pop_back();
  return s;
}

static std::string lower(std::string s) {
  for (char &c : s)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

Settings load_settings() {
  Settings s;
  std::ifstream in(config_path());
  if (!in)
    return s;

  std::string line;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#')
      continue;
    const auto eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    const std::string key = lower(trim(line.substr(0, eq)));
    const std::string val = trim(line.substr(eq + 1));
    if (key == "mode") {
      if (lower(val) == "toggle")
        s.mode = Settings::Mode::Toggle;
      else
        s.mode = Settings::Mode::Hold;
    } else if (key == "hotkey") {
      s.hotkey = val;
    } else if (key == "model") {
      s.model = val;
    }
  }
  return s;
}

bool save_settings(const Settings &s) {
  std::ofstream out(config_path());
  if (!out) {
    std::cerr << "Cannot write " << config_path() << '\n';
    return false;
  }
  out << "mode=" << (s.mode == Settings::Mode::Toggle ? "toggle" : "hold")
      << '\n';
  out << "hotkey=" << s.hotkey << '\n';
  out << "model=" << s.model << '\n';
  return true;
}

} // namespace dictate
