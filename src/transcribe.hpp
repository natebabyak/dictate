#pragma once

struct whisper_context;

namespace dictate {

void process_loop(whisper_context *ctx);

} // namespace dictate
