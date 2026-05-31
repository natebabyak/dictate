#define MINIAUDIO_IMPLEMENTATION
#include "audio.hpp"

#include "config.hpp"
#include "state.hpp"

#include <miniaudio.h>

namespace dictate {

namespace {

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  (void)pOutput;
  if (!g_ptt.active.load(std::memory_order_relaxed))
    return;

  auto *capture = static_cast<AudioCapture *>(pDevice->pUserData);
  if (pInput == nullptr || frameCount == 0)
    return;

  const float *pInputF32 = static_cast<const float *>(pInput);

  {
    std::lock_guard<std::mutex> lock(capture->mutex);
    capture->samples.insert(capture->samples.end(), pInputF32,
                            pInputF32 + frameCount);
    capture->session.insert(capture->session.end(), pInputF32,
                            pInputF32 + frameCount);
  }
  capture->cv.notify_one();
}

} // namespace

bool audio_init(ma_device &device) {
  ma_device_config config = ma_device_config_init(ma_device_type_capture);
  config.capture.format = ma_format_f32;
  config.capture.channels = 1;
  config.sampleRate = config::kSampleRate;
  config.dataCallback = data_callback;
  config.pUserData = &g_audio;

  if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS)
    return false;

  return ma_device_start(&device) == MA_SUCCESS;
}

void audio_shutdown(ma_device &device) {
  ma_device_stop(&device);
  ma_device_uninit(&device);
}

} // namespace dictate
