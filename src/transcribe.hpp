#pragma once

#include "replace.hpp"

struct whisper_context;

namespace dictate {

void process_loop(whisper_context *ctx,
                  const std::vector<Replacement> &replacements);

} // namespace dictate
