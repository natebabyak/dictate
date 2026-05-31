#include "state.hpp"

#include <iostream>

namespace dictate {

std::atomic<bool> g_running{true};
PushToTalk g_ptt;
AudioCapture g_audio;

void begin_session() {
  {
    std::lock_guard<std::mutex> lock(g_audio.mutex);
    g_audio.samples.clear();
    g_audio.session.clear();
    g_audio.read_offset = 0;
  }
  g_ptt.finalize.store(false);
  g_ptt.session_id.fetch_add(1);
  g_ptt.active.store(true);
  g_audio.cv.notify_all();
  // Live transcript goes to stdout; PTT status is logged from macos/ptt.mm.
  std::cout << std::flush;
}

void end_session() {
  if (!g_ptt.active.exchange(false))
    return;
  g_ptt.finalize.store(true);
  g_audio.cv.notify_all();
}

bool should_abort(void *) { return !g_running.load(); }

} // namespace dictate
