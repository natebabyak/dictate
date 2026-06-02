#define MINIAUDIO_IMPLEMENTATION
#include "audio.hpp"

#include <miniaudio.h>

#include "state.hpp"

namespace dictate {

namespace {

void callback(ma_device *, void *, const void *input, ma_uint32 frames) {
  if (!g_session.recording.load() || input == nullptr || frames == 0)
    return;
  const float *samples = static_cast<const float *>(input);
  std::lock_guard lock(g_session.mutex);
  g_session.pcm.insert(g_session.pcm.end(), samples, samples + frames);
}

} // namespace

bool audio_start(ma_device &device) {
  ma_device_config cfg = ma_device_config_init(ma_device_type_capture);
  cfg.capture.format = ma_format_f32;
  cfg.capture.channels = 1;
  cfg.sampleRate = 16000;
  cfg.dataCallback = callback;
  if (ma_device_init(nullptr, &cfg, &device) != MA_SUCCESS)
    return false;
  return ma_device_start(&device) == MA_SUCCESS;
}

void audio_stop(ma_device &device) {
  ma_device_stop(&device);
  ma_device_uninit(&device);
}

} // namespace dictate
