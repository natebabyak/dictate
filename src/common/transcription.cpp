#include "transcription.hpp"

#include "config.hpp"
#include "state.hpp"

#include <whisper.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace dictate {

namespace {

int ms_to_samples(int ms) {
  return static_cast<int>((1e-3 * ms) * config::kSampleRate);
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
  std::lock_guard<std::mutex> lock(capture.mutex);

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

void apply_abort(whisper_full_params &params) {
  params.abort_callback = should_abort;
  params.abort_callback_user_data = nullptr;
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
    std::cout << current << std::flush;
    last_printed = current;
  }
}

std::string transcribe_text(whisper_context *ctx,
                            const whisper_full_params &params,
                            const std::vector<float> &pcm) {
  if (pcm.size() < static_cast<size_t>(config::kMinSessionSamples))
    return {};

  if (whisper_full(ctx, params, pcm.data(), static_cast<int>(pcm.size())) != 0)
    return {};

  std::string text;
  const int n_segments = whisper_full_n_segments(ctx);
  for (int i = 0; i < n_segments; ++i) {
    const char *seg = whisper_full_get_segment_text(ctx, i);
    if (seg != nullptr)
      text += seg;
  }
  return text;
}

void run_polished_pass(whisper_context *ctx,
                       const std::vector<float> &session_pcm) {
  whisper_full_params params =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  params.print_progress = false;
  params.print_special = false;
  params.print_realtime = false;
  params.print_timestamps = false;
  params.no_timestamps = true;
  params.no_context = false;
  params.single_segment = false;
  params.language = "en";
  params.n_threads =
      std::min(8, static_cast<int>(std::thread::hardware_concurrency()));
  params.temperature = 0.0f;
  params.suppress_blank = true;
  apply_abort(params);

  const std::string polished = transcribe_text(ctx, params, session_pcm);
  std::cout << "\n\n";
  if (polished.empty()) {
    std::cout << "(no speech detected)" << std::endl;
  } else {
    std::cout << polished << std::endl;
  }
}

} // namespace

void inference_loop(whisper_context *ctx) {
  const int n_samples_step = ms_to_samples(config::kStepMs);
  const int n_samples_len = ms_to_samples(config::kLengthMs);
  const int n_samples_keep = ms_to_samples(config::kKeepMs);

  whisper_full_params live_params =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  live_params.print_progress = false;
  live_params.print_special = false;
  live_params.print_realtime = false;
  live_params.print_timestamps = false;
  live_params.no_timestamps = true;
  live_params.single_segment = false;
  live_params.no_context = true;
  live_params.language = "en";
  live_params.n_threads =
      std::min(4, static_cast<int>(std::thread::hardware_concurrency()));
  apply_abort(live_params);

  std::vector<float> pcmf32_old;
  std::vector<float> pcmf32_new;
  std::vector<float> pcmf32;
  std::string last_printed;
  uint64_t stream_session = 0;

  auto next_step = std::chrono::steady_clock::now();

  while (g_running) {
    if (g_ptt.finalize.exchange(false)) {
      std::vector<float> session_pcm;
      {
        std::lock_guard<std::mutex> lock(g_audio.mutex);
        session_pcm = g_audio.session;
      }

      std::cout << std::endl;
      if (g_running)
        run_polished_pass(ctx, session_pcm);

      pcmf32_old.clear();
      last_printed.clear();
      next_step = std::chrono::steady_clock::now();
      continue;
    }

    if (!g_ptt.active.load()) {
      std::unique_lock<std::mutex> lock(g_audio.mutex);
      g_audio.cv.wait_for(lock, std::chrono::milliseconds(100), [] {
        return !g_running || g_ptt.active.load() || g_ptt.finalize.load();
      });
      continue;
    }

    const uint64_t sid = g_ptt.session_id.load();
    if (sid != stream_session) {
      stream_session = sid;
      pcmf32_old.clear();
      last_printed.clear();
      next_step = std::chrono::steady_clock::now();
    }

    {
      std::unique_lock<std::mutex> lock(g_audio.mutex);
      g_audio.cv.wait_for(lock, std::chrono::milliseconds(config::kStepMs),
                          [n_samples_step] {
                            if (!g_running || !g_ptt.active.load())
                              return true;
                            const size_t available =
                                g_audio.samples.size() - g_audio.read_offset;
                            return available >=
                                   static_cast<size_t>(n_samples_step);
                          });
    }

    if (!g_running || !g_ptt.active.load())
      continue;

    const auto now = std::chrono::steady_clock::now();
    if (now < next_step)
      continue;
    next_step = now + std::chrono::milliseconds(config::kStepMs);

    if (!take_step_samples(g_audio, n_samples_step, pcmf32_new))
      continue;

    build_stream_window(pcmf32_new, pcmf32_old, n_samples_len, n_samples_keep,
                        pcmf32);

    if (!has_speech(pcmf32, config::kSampleRate, config::kVadLastMs,
                    config::kVadThold, config::kFreqThold))
      continue;

    if (whisper_full(ctx, live_params, pcmf32.data(),
                     static_cast<int>(pcmf32.size())) != 0) {
      if (g_running)
        std::cerr << "Error: live transcription failed." << std::endl;
      continue;
    }

    print_new_text(ctx, last_printed);
  }
}

} // namespace dictate
