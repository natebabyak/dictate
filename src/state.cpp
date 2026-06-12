#include "state.hpp"

namespace dictate {

void start_recording() {
  {
    std::lock_guard lock(g_session.mutex);
    g_session.pcm.clear();
    g_session.pcm.reserve(16000 * 30);
    g_session.ready = false;
  }
  g_session.recording.store(true);
}

void stop_recording() {
  if (!g_session.recording.exchange(false))
    return;
  {
    std::lock_guard lock(g_session.mutex);
    g_session.ready = true;
  }
  g_session.cv.notify_one();
}

bool should_abort(void *) { return !g_running.load(); }

} // namespace dictate
