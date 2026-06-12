#include "audio.hpp"
#include "platform/ptt.hpp"
#include "settings.hpp"
#include "state.hpp"
#include "transcribe.hpp"

#include <miniaudio.h>
#include <whisper.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

using namespace dictate;

int main() {
  if (const auto s = load_settings())
    g_settings = *s;
  else
    return 1;

  init_signals();

  whisper_context_params cparams = whisper_context_default_params();
  cparams.use_gpu = true;

  struct CtxDeleter {
    void operator()(whisper_context *c) const { whisper_free(c); }
  };
  const char *model_file = model_filename(g_settings.model);
  std::unique_ptr<whisper_context, CtxDeleter> ctx(
      whisper_init_from_file_with_params(model_file, cparams));
  if (!ctx) {
    std::cerr << "Failed to load model: " << model_file << '\n';
    return 1;
  }

  ma_device device{};
  if (!audio_start(device))
    return 1;

  std::thread worker(process_loop, ctx.get());
  std::thread ptt(run_ptt);

  while (g_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  g_session.cv.notify_all();
  ptt.join();
  worker.join();
  audio_stop(device);
  return 0;
}
