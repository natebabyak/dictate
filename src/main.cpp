#define MINIAUDIO_IMPLEMENTATION
#include <iostream>
#include <miniaudio.h>
#include <whisper.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kSampleRate = WHISPER_SAMPLE_RATE;

// Stream-style window (see whisper.cpp examples/stream)
constexpr int kStepMs = 500;
constexpr int kLengthMs = 3000;
constexpr int kKeepMs = 200;

constexpr float kVadThold = 0.6f;
constexpr float kFreqThold = 100.0f;
constexpr int kVadLastMs = 1000;

std::atomic<bool> g_running{true};

struct AudioCapture {
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<float> samples;
  size_t read_offset = 0;
};

AudioCapture g_audio;

void on_sigint(int) { g_running = false; }

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  (void)pOutput;
  auto *capture = static_cast<AudioCapture *>(pDevice->pUserData);
  if (pInput == nullptr || frameCount == 0)
    return;

  const float *pInputF32 = static_cast<const float *>(pInput);

  {
    std::lock_guard<std::mutex> lock(capture->mutex);
    capture->samples.insert(capture->samples.end(), pInputF32,
                            pInputF32 + frameCount);
  }
  capture->cv.notify_one();
}

int ms_to_samples(int ms) {
  return static_cast<int>((1e-3 * ms) * kSampleRate);
}

void high_pass_filter(std::vector<float> &data, float cutoff,
                      float sample_rate) {
  const float rc = 1.0f / (2.0f * static_cast<float>(M_PI) * cutoff);
  const float dt = 1.0f / sample_rate;
  const float alpha = dt / (rc + dt);

  float y = data[0];
  for (size_t i = 1; i < data.size(); ++i) {
    y = alpha * (y + data[i] - data[i - 1]);
    data[i] = y;
  }
}

// Energy-based gate (inspired by whisper.cpp examples/common.cpp vad_simple).
// Returns true when the most recent audio is louder than the window baseline.
bool has_speech(const std::vector<float> &pcm, int sample_rate, int last_ms,
                float vad_thold, float freq_thold) {
  std::vector<float> filtered = pcm;
  const int n_samples = static_cast<int>(filtered.size());
  const int n_samples_last = (sample_rate * last_ms) / 1000;

  if (n_samples_last >= n_samples)
    return false;

  if (freq_thold > 0.0f)
    high_pass_filter(filtered, freq_thold, static_cast<float>(sample_rate));

  float energy_all = 0.0f;
  float energy_last = 0.0f;

  for (int i = 0; i < n_samples; ++i) {
    energy_all += std::fabs(filtered[i]);
    if (i >= n_samples - n_samples_last)
      energy_last += std::fabs(filtered[i]);
  }

  energy_all /= n_samples;
  energy_last /= n_samples_last;

  return energy_last > vad_thold * std::max(energy_all, 1e-8f);
}

bool take_step_samples(AudioCapture &capture, int n_samples_step,
                       std::vector<float> &out) {
  std::unique_lock<std::mutex> lock(capture.mutex);

  const size_t available = capture.samples.size() - capture.read_offset;
  if (available < static_cast<size_t>(n_samples_step))
    return false;

  out.assign(capture.samples.begin() + capture.read_offset,
             capture.samples.begin() + capture.read_offset + n_samples_step);
  capture.read_offset += n_samples_step;

  if (capture.read_offset > static_cast<size_t>(n_samples_step) * 32) {
    capture.samples.erase(capture.samples.begin(),
                          capture.samples.begin() + capture.read_offset);
    capture.read_offset = 0;
  }

  return true;
}

void build_stream_window(const std::vector<float> &pcmf32_new,
                         std::vector<float> &pcmf32_old, int n_samples_len,
                         int n_samples_keep, std::vector<float> &pcmf32) {
  const int n_samples_new = static_cast<int>(pcmf32_new.size());
  const int n_samples_take =
      std::min(static_cast<int>(pcmf32_old.size()),
               std::max(0, n_samples_keep + n_samples_len - n_samples_new));

  pcmf32.resize(static_cast<size_t>(n_samples_new + n_samples_take));

  for (int i = 0; i < n_samples_take; ++i)
    pcmf32[static_cast<size_t>(i)] =
        pcmf32_old[pcmf32_old.size() - n_samples_take + i];

  std::memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(),
              static_cast<size_t>(n_samples_new) * sizeof(float));

  pcmf32_old = pcmf32;
}

void print_new_text(whisper_context *ctx, std::string &last_printed) {
  std::string current;
  const int n_segments = whisper_full_n_segments(ctx);
  for (int i = 0; i < n_segments; ++i) {
    const char *text = whisper_full_get_segment_text(ctx, i);
    if (text != nullptr)
      current += text;
  }

  if (current.empty())
    return;

  if (!last_printed.empty() && current.size() >= last_printed.size() &&
      current.compare(0, last_printed.size(), last_printed) == 0) {
    const std::string delta = current.substr(last_printed.size());
    if (!delta.empty()) {
      std::cout << delta << std::flush;
      last_printed = current;
    }
    return;
  }

  if (current != last_printed) {
    std::cout << current << std::endl;
    last_printed = current;
  }
}

void inference_loop(whisper_context *ctx) {
  const int n_samples_step = ms_to_samples(kStepMs);
  const int n_samples_len = ms_to_samples(kLengthMs);
  const int n_samples_keep = ms_to_samples(kKeepMs);

  whisper_full_params wparams =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  wparams.print_progress = false;
  wparams.print_special = false;
  wparams.print_realtime = false;
  wparams.print_timestamps = false;
  wparams.no_timestamps = true;
  wparams.single_segment = false;
  wparams.no_context = true;
  wparams.language = "en";
  wparams.n_threads =
      std::min(4, static_cast<int>(std::thread::hardware_concurrency()));

  std::vector<float> pcmf32_old;
  std::vector<float> pcmf32_new;
  std::vector<float> pcmf32;
  std::string last_printed;

  auto next_step = std::chrono::steady_clock::now();

  while (g_running) {
    {
      std::unique_lock<std::mutex> lock(g_audio.mutex);
      g_audio.cv.wait_until(lock, next_step, [] {
        if (!g_running)
          return true;
        const size_t available = g_audio.samples.size() - g_audio.read_offset;
        return available >= static_cast<size_t>(ms_to_samples(kStepMs));
      });
    }

    if (!g_running)
      break;

    const auto now = std::chrono::steady_clock::now();
    if (now < next_step)
      continue;
    next_step = now + std::chrono::milliseconds(kStepMs);

    if (!take_step_samples(g_audio, n_samples_step, pcmf32_new))
      continue;

    build_stream_window(pcmf32_new, pcmf32_old, n_samples_len, n_samples_keep,
                        pcmf32);

    if (!has_speech(pcmf32, kSampleRate, kVadLastMs, kVadThold, kFreqThold))
      continue;

    if (whisper_full(ctx, wparams, pcmf32.data(),
                     static_cast<int>(pcmf32.size())) != 0) {
      std::cerr << "Error: Failed to process audio in Whisper." << std::endl;
      continue;
    }

    print_new_text(ctx, last_printed);
  }
}

} // namespace

int main() {
  const char *model_path = "ggml-medium.en.bin";

  whisper_context_params cparams = whisper_context_default_params();
  cparams.use_gpu = true;
  cparams.flash_attn = true;

  whisper_context *ctx =
      whisper_init_from_file_with_params(model_path, cparams);
  if (ctx == nullptr) {
    std::cerr << "Failed to load model" << std::endl;
    return 1;
  }

  ma_device_config config = ma_device_config_init(ma_device_type_capture);
  config.capture.format = ma_format_f32;
  config.capture.channels = 1;
  config.sampleRate = kSampleRate;
  config.dataCallback = data_callback;
  config.pUserData = &g_audio;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
    whisper_free(ctx);
    return -1;
  }

  std::signal(SIGINT, on_sigint);

  std::cout << "Listening (stream: " << kStepMs << "ms step, " << kLengthMs
            << "ms window, VAD on). Press Ctrl+C to stop." << std::endl;

  if (ma_device_start(&device) != MA_SUCCESS) {
    std::cerr << "Error: Failed to start capture device." << std::endl;
    ma_device_uninit(&device);
    whisper_free(ctx);
    return 1;
  }

  std::thread inference(inference_loop, ctx);

  while (g_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  g_audio.cv.notify_all();
  inference.join();

  std::cout << "\nStopping..." << std::endl;
  ma_device_stop(&device);
  ma_device_uninit(&device);
  whisper_free(ctx);
  return 0;
}
