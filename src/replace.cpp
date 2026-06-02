#include "replace.hpp"

#include "settings.hpp"

#include <fstream>
#include <iostream>

namespace dictate {

static std::string trim(std::string s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
    s.erase(s.begin());
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
    s.pop_back();
  return s;
}

std::vector<Replacement> load_replacements() {
  std::vector<Replacement> rules;
  std::ifstream in(replacements_path());
  if (!in)
    return rules;

  std::string line;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#')
      continue;

    std::string from, to;
    if (const auto arrow = line.find("=>"); arrow != std::string::npos) {
      from = trim(line.substr(0, arrow));
      to = trim(line.substr(arrow + 2));
    } else if (const auto tab = line.find('\t'); tab != std::string::npos) {
      from = trim(line.substr(0, tab));
      to = trim(line.substr(tab + 1));
    } else
      continue;

    if (!from.empty())
      rules.push_back({from, to});
  }
  return rules;
}

std::string apply_replacements(const std::string &text,
                               const std::vector<Replacement> &rules) {
  std::string out = text;
  for (const auto &r : rules) {
    if (r.from.empty())
      continue;
    size_t pos = 0;
    while ((pos = out.find(r.from, pos)) != std::string::npos) {
      out.replace(pos, r.from.size(), r.to);
      pos += r.to.size();
    }
  }
  return out;
}

} // namespace dictate
