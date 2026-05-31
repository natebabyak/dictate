#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

namespace dictate {

struct PushToTalk {
  std::atomic<bool> active{false};
  std::atomic<bool> finalize{false};
  std::atomic<uint64_t> session_id{0};
};

struct AudioCapture {
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<float> samples;
  std::vector<float> session;
  size_t read_offset = 0;
};

extern std::atomic<bool> g_running;
extern PushToTalk g_ptt;
extern AudioCapture g_audio;

void begin_session();
void end_session();
bool should_abort(void *user_data);

} // namespace dictate
