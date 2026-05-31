#include "platform/ptt.hpp"

#include "common/state.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace dictate::platform {

const char *ptt_key_name() { return "(not implemented)"; }

void run_ptt_loop() {
  std::cerr << "Push-to-talk is not implemented on Linux yet.\n";
  while (g_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

} // namespace dictate::platform
