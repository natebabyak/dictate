#pragma once

#include <atomic>
#include <mutex>
#include <vector>

namespace dictate {

inline std::atomic<bool> g_running{true};

struct Session {
  std::mutex mutex;
  std::atomic<bool> recording{false};
  std::atomic<bool> ready{false};
  std::vector<float> pcm;
};

inline Session g_session;

void start_recording();
void stop_recording();
bool should_abort(void *);

} // namespace dictate
