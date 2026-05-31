#include "platform/ptt.hpp"

#include "common/state.hpp"

#include <csignal>

namespace dictate::platform {

namespace {

void on_signal(int) {
  g_running = false;
  g_ptt.active.store(false);
  g_ptt.finalize.store(false);
  g_audio.cv.notify_all();
}

} // namespace

void init_signals() {
  struct sigaction sa {};
  sa.sa_handler = on_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
}

} // namespace dictate::platform
