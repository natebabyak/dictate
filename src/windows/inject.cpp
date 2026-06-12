#include "platform/inject.hpp"

#include <iostream>

namespace dictate {

void inject_text(const std::string &text) {
  if (!text.empty())
    std::cerr << text << '\n';
}

} // namespace dictate
