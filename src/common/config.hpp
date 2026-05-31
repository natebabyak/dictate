#pragma once

#include <whisper.h>

namespace dictate::config {

inline constexpr int kSampleRate = WHISPER_SAMPLE_RATE;
inline constexpr int kStepMs = 500;
inline constexpr int kLengthMs = 3000;
inline constexpr int kKeepMs = 200;

inline constexpr float kVadThold = 0.6f;
inline constexpr float kFreqThold = 100.0f;
inline constexpr int kVadLastMs = 1000;

inline constexpr int kMinSessionSamples = kSampleRate / 5;

inline const char *kDefaultModelPath = "ggml-tiny.en.bin";

} // namespace dictate::config
