cmake_minimum_required(VERSION 4.3)

project(Dictate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(whisper CONFIG REQUIRED)

set(DICTATE_COMMON_SOURCES
    src/common/state.cpp
    src/common/audio.cpp
    src/common/transcription.cpp
    src/common/app.cpp
)

if(APPLE)
  set(DICTATE_PLATFORM_SOURCES
      src/macos/ptt.mm
      src/macos/signals.cpp
  )
elseif(WIN32)
  set(DICTATE_PLATFORM_SOURCES
      src/windows/ptt.cpp
      src/windows/signals.cpp
  )
else()
  set(DICTATE_PLATFORM_SOURCES
      src/linux/ptt.cpp
      src/linux/signals.cpp
  )
endif()

add_executable(dictate src/main.cpp ${DICTATE_COMMON_SOURCES}
                           ${DICTATE_PLATFORM_SOURCES})

target_include_directories(dictate PRIVATE src)

target_link_libraries(dictate PRIVATE whisper)

if(APPLE)
  target_link_libraries(dictate PRIVATE
      "-framework ApplicationServices"
      "-framework Cocoa"
      "-framework CoreFoundation")
endif()
