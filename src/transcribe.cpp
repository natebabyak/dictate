#include <whisper.h>

#include "platform/inject.hpp"
#include "state.hpp"

#include <string>
#include <thread>
#include <vector>

namespace dictate {

static std::string whisper_text(whisper_context *ctx, const float *pcm,
                                int n) {
  whisper_full_params p =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  p.print_progress = false;
  p.print_special = false;
  p.language = "en";
  p.no_timestamps = true;
  p.n_threads = static_cast<int>(std::thread::hardware_concurrency());
  p.abort_callback = should_abort;
  p.abort_callback_user_data = nullptr;

  if (whisper_full(ctx, p, pcm, n) != 0)
    return {};

  std::string text;
  for (int i = 0, nseg = whisper_full_n_segments(ctx); i < nseg; ++i) {
    const char *seg = whisper_full_get_segment_text(ctx, i);
    if (seg)
      text += seg;
  }
  return text;
}

void process_loop(whisper_context *ctx) {
  while (g_running) {
    std::vector<float> pcm;
    {
      std::unique_lock lock(g_session.mutex);
      g_session.cv.wait(lock, [] {
        return g_session.ready || !g_running.load();
      });
      if (!g_running)
        break;
      g_session.ready = false;
      pcm.swap(g_session.pcm);
    }

    if (pcm.size() < 1600)
      continue;

    const std::string text =
        whisper_text(ctx, pcm.data(), static_cast<int>(pcm.size()));
    if (!text.empty())
      inject_text(text);
  }
}

} // namespace dictate
