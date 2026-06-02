#include "paths.hpp"

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <vector>

namespace dictate {

std::filesystem::path exe_dir() {
#if defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::vector<char> buf(size);
  _NSGetExecutablePath(buf.data(), &size);
  return std::filesystem::path(buf.data()).parent_path();
#elif defined(_WIN32)
  std::vector<wchar_t> buf(MAX_PATH);
  GetModuleFileNameW(nullptr, buf.data(), MAX_PATH);
  return std::filesystem::path(buf.data()).parent_path();
#else
  return std::filesystem::read_symlink("/proc/self/exe").parent_path();
#endif
}

} // namespace dictate
