cmake_minimum_required(VERSION 4.3)

project(Dictate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(whisper CONFIG REQUIRED)

set(SOURCES
    src/main.cpp
    src/paths.cpp
    src/settings.cpp
    src/replace.cpp
    src/state.cpp
    src/audio.cpp
    src/transcribe.cpp
)

if(APPLE)
  list(APPEND SOURCES src/macos/ptt.mm)
elseif(WIN32)
  list(APPEND SOURCES src/windows/ptt.cpp)
else()
  list(APPEND SOURCES src/linux/ptt.cpp)
endif()

add_executable(dictate ${SOURCES})

target_include_directories(dictate PRIVATE src)

target_link_libraries(dictate PRIVATE whisper)

if(APPLE)
  target_link_libraries(dictate PRIVATE
      "-framework ApplicationServices"
      "-framework Cocoa"
      "-framework Carbon")
endif()
