#pragma once

namespace dictate::platform {

// Install signal handlers (Ctrl+C, etc.).
void init_signals();

// Block until g_running is false. Polls global hold-to-talk input.
void run_ptt_loop();

// Human-readable name of the push-to-talk key.
const char *ptt_key_name();

} // namespace dictate::platform
