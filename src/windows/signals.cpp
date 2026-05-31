#include "platform/ptt.hpp"

#include "common/state.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace dictate::platform {

namespace {

#ifdef _WIN32
BOOL WINAPI console_handler(DWORD type) {
  if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT ||
      type == CTRL_CLOSE_EVENT) {
    g_running = false;
    g_ptt.active.store(false);
    g_ptt.finalize.store(false);
    g_audio.cv.notify_all();
    return TRUE;
  }
  return FALSE;
}
#endif

} // namespace

void init_signals() {
#ifdef _WIN32
  SetConsoleCtrlHandler(console_handler, TRUE);
#endif
}

} // namespace dictate::platform
