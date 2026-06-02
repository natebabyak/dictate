#pragma once

#include <string>
#include <vector>

namespace dictate {

struct Replacement {
  std::string from;
  std::string to;
};

std::vector<Replacement> load_replacements();
std::string apply_replacements(const std::string &text,
                               const std::vector<Replacement> &rules);

} // namespace dictate
