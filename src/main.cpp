#include "audio.hpp"
#include "platform/ptt.hpp"
#include "replace.hpp"
#include "settings.hpp"
#include "state.hpp"
#include "transcribe.hpp"

#include <miniaudio.h>
#include <whisper.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

using namespace dictate;

namespace {

void usage() {
  std::cerr
      << "Usage:\n"
      << "  dictate run              Start listening (uses dictate.conf)\n"
      << "  dictate config show      Print config\n"
      << "  dictate config mode hold|toggle\n"
      << "  dictate config hotkey <modifiers+key>   e.g. control+shift+d\n"
      << "\n"
      << "Files (next to the dictate binary):\n"
      << "  dictate.conf       mode, hotkey, model\n"
      << "  replacements.txt   lines: from => to\n"
      << "  ggml-*.bin         whisper model\n";
}

int cmd_config_show() {
  g_settings = load_settings();
  std::cout << "config: " << config_path() << '\n';
  std::cout << "mode=" << (g_settings.mode == Settings::Mode::Toggle ? "toggle"
                                                                     : "hold")
            << '\n';
  std::cout << "hotkey=" << g_settings.hotkey << '\n';
  std::cout << "model=" << g_settings.model << '\n';
  std::cout << "replacements: " << replacements_path() << '\n';
  return 0;
}

int cmd_config_mode(const char *mode) {
  g_settings = load_settings();
  if (std::string(mode) == "toggle")
    g_settings.mode = Settings::Mode::Toggle;
  else if (std::string(mode) == "hold")
    g_settings.mode = Settings::Mode::Hold;
  else {
    std::cerr << "mode must be hold or toggle\n";
    return 1;
  }
  return save_settings(g_settings) ? 0 : 1;
}

int cmd_config_hotkey(int argc, char **argv, int i) {
  if (i + 1 >= argc) {
    std::cerr << "missing hotkey, e.g. control+shift+d\n";
    return 1;
  }
  g_settings = load_settings();
  g_settings.hotkey = argv[i + 1];
  return save_settings(g_settings) ? 0 : 1;
}

int cmd_run() {
  g_settings = load_settings();
  const auto replacements = load_replacements();

  init_signals();

  whisper_context_params cparams = whisper_context_default_params();
  cparams.use_gpu = true;

  struct CtxDeleter {
    void operator()(whisper_context *c) const { whisper_free(c); }
  };
  std::unique_ptr<whisper_context, CtxDeleter> ctx(
      whisper_init_from_file_with_params(g_settings.model.c_str(), cparams));
  if (!ctx) {
    std::cerr << "Failed to load model: " << g_settings.model << '\n';
    std::cerr << "Put the .bin file next to dictate or set model= in "
                 "dictate.conf\n";
    return 1;
  }

  ma_device device{};
  if (!audio_start(device)) {
    std::cerr << "Failed to open microphone\n";
    return 1;
  }

  std::thread worker(process_loop, ctx.get(), replacements);
  std::thread ptt(run_ptt);

  while (g_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ptt.join();
  worker.join();
  audio_stop(device);
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    return cmd_run();
  }

  const std::string cmd = argv[1];
  if (cmd == "run") {
    return cmd_run();
  }
  if (cmd == "config") {
    if (argc < 3) {
      usage();
      return 1;
    }
    const std::string sub = argv[2];
    if (sub == "show")
      return cmd_config_show();
    if (sub == "mode" && argc >= 4)
      return cmd_config_mode(argv[3]);
    if (sub == "hotkey")
      return cmd_config_hotkey(argc, argv, 2);
    usage();
    return 1;
  }
  if (cmd == "help" || cmd == "--help" || cmd == "-h") {
    usage();
    return 0;
  }

  usage();
  return 1;
}
