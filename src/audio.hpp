#pragma once

struct ma_device;

namespace dictate {

bool audio_start(ma_device &device);
void audio_stop(ma_device &device);

} // namespace dictate
