#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <whisper.h>
#include <iostream>

#include <vector>
#include <thread>
#include <chrono>

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    auto* audioBuffer = static_cast<std::vector<float>*>(pDevice->pUserData);
    if (pInput == nullptr || frameCount == 0) return;

    const float* pInputF32 = static_cast<const float*>(pInput);

    audioBuffer->insert(audioBuffer->end(), pInputF32, pInputF32 + frameCount);
}

int main() {
    const char* model_path = "ggml-base.en.bin";

    whisper_context* ctx = whisper_init_from_file_with_params(model_path, {});
    if (ctx == nullptr) {
        std::cerr << "Failed to load model" << std::endl;
        return 1;
    }

    std::vector<float> capturedAudio;

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format   = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate       = 16000;
    config.dataCallback     = data_callback;
    config.pUserData        = &capturedAudio;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        whisper_free(ctx);
        return -1;
    }

    std::cout << "Recording for 5 seconds... Speak into your mic!" << std::endl;
        if (ma_device_start(&device) != MA_SUCCESS) {
            std::cerr << "Error: Failed to start capture device." << std::endl;
            ma_device_uninit(&device);
            whisper_free(ctx);
            return 1;
        }

        // Record for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Stop recording
        std::cout << "Stopping recording. Processing audio..." << std::endl;
        ma_device_stop(&device);
        ma_device_uninit(&device);

        // -------------------------------------------------------------------------
        // 3. Run Whisper Inference
        // -------------------------------------------------------------------------
        whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        params.print_progress = false;
        params.print_special  = false;
        params.language       = "en"; // Force English for faster/cleaner testing

        if (whisper_full(ctx, params, capturedAudio.data(), capturedAudio.size()) != 0) {
            std::cerr << "Error: Failed to process audio in Whisper." << std::endl;
            whisper_free(ctx);
            return 1;
        }

        // Print the results
        std::cout << "\n--- Transcription Result ---" << std::endl;
        int n_segments = whisper_full_n_segments(ctx);
        for (int i = 0; i < n_segments; ++i) {
            const char* text = whisper_full_get_segment_text(ctx, i);
            std::cout << text << std::endl;
        }
        std::cout << "----------------------------\n" << std::endl;

        // Cleanup Whisper
        whisper_free(ctx);
        return 0;
}
