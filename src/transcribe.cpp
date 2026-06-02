#include <whisper.h>

#include "replace.hpp"
#include "settings.hpp"
#include "state.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
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
  p.n_threads =
      std::min(8, static_cast<int>(std::thread::hardware_concurrency()));
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

void process_loop(whisper_context *ctx,
                  const std::vector<Replacement> &replacements) {
  while (g_running) {
    if (!g_session.ready.exchange(false)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      continue;
    }

    std::vector<float> pcm;
    {
      std::lock_guard lock(g_session.mutex);
      pcm.swap(g_session.pcm);
    }

    if (pcm.size() < 1600) {
      std::cout << "(too short)\n";
      continue;
    }

    std::string text = whisper_text(ctx, pcm.data(), static_cast<int>(pcm.size()));
    text = apply_replacements(text, replacements);

    if (text.empty())
      std::cout << "(no speech)\n";
    else
      std::cout << text << '\n';
    std::cout.flush();
  }
}

} // namespace dictate
