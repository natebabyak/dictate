#pragma once

struct whisper_context;

namespace dictate {

void inference_loop(whisper_context *ctx);

} // namespace dictate
