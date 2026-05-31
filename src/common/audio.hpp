#pragma once

struct ma_device;

namespace dictate {

bool audio_init(ma_device &device);
void audio_shutdown(ma_device &device);

} // namespace dictate
