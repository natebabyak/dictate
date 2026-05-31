#include "app.hpp"

#include "audio.hpp"
#include "config.hpp"
#include "platform/ptt.hpp"
#include "state.hpp"
#include "transcription.hpp"

#include <miniaudio.h>
#include <whisper.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

namespace {

struct WhisperContextDeleter {
  void operator()(whisper_context *ctx) const { whisper_free(ctx); }
};

} // namespace

int run_app() {
  dictate::platform::init_signals();

  whisper_context_params cparams = whisper_context_default_params();
  cparams.use_gpu = true;
  cparams.flash_attn = true;

  std::unique_ptr<whisper_context, WhisperContextDeleter> ctx(
      whisper_init_from_file_with_params(dictate::config::kDefaultModelPath,
                                         cparams));
  if (ctx == nullptr) {
    std::cerr << "Failed to load model: " << dictate::config::kDefaultModelPath
              << std::endl;
    return 1;
  }

  ma_device device{};
  if (!dictate::audio_init(device)) {
    std::cerr << "Error: Failed to start capture device." << std::endl;
    return 1;
  }

  std::cout << "Hold " << dictate::platform::ptt_key_name()
            << " anywhere to dictate (live while held, polished on release). "
               "Ctrl+C to quit.\n";
#if defined(__APPLE__)
  std::cout << "Watch stderr for [ptt] lines when the hotkey fires.\n";
#else
  std::cout << "Note: push-to-talk is only active on macOS in this build.\n";
#endif
  std::cout << std::endl;

  std::thread inference(dictate::inference_loop, ctx.get());
  std::thread ptt(dictate::platform::run_ptt_loop);

  while (dictate::g_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  dictate::g_audio.cv.notify_all();
  ptt.join();
  inference.join();

  std::cout << "\nStopping..." << std::endl;
  dictate::audio_shutdown(device);
  return 0;
}
