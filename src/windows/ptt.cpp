#include "platform/ptt.hpp"

#include "state.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace dictate {

void init_signals() {
#ifdef _WIN32
  SetConsoleCtrlHandler(
      [](DWORD t) -> BOOL {
        if (t == CTRL_C_EVENT || t == CTRL_BREAK_EVENT) {
          g_running = false;
          return TRUE;
        }
        return FALSE;
      },
      TRUE);
#endif
}

void run_ptt() {
  std::cerr << "Push-to-talk is not implemented on Windows yet.\n";
  while (g_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

} // namespace dictate
