#include "platform/ptt.hpp"

#include "state.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace dictate {

void init_signals() {
  signal(SIGINT, [](int) { g_running = false; });
  signal(SIGTERM, [](int) { g_running = false; });
}

void run_ptt() {
  std::cerr << "Push-to-talk is not implemented on Linux yet.\n";
  while (g_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

} // namespace dictate
