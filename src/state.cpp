#include "state.hpp"

#include <iostream>

namespace dictate {

void start_recording() {
  {
    std::lock_guard lock(g_session.mutex);
    g_session.pcm.clear();
  }
  g_session.ready.store(false);
  g_session.recording.store(true);
  std::cerr << "[dictate] recording\n";
}

void stop_recording() {
  if (!g_session.recording.exchange(false))
    return;
  g_session.ready.store(true);
  std::cerr << "[dictate] processing\n";
}

bool should_abort(void *) { return !g_running.load(); }

} // namespace dictate
