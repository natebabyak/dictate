cmake_minimum_required(VERSION 4.3)

project(Dictate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(READ "${CMAKE_SOURCE_DIR}/schema.json" CONFIG_SCHEMA)
configure_file(src/schema.hpp.in "${CMAKE_BINARY_DIR}/generated/schema.hpp" @ONLY)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(valijson CONFIG REQUIRED)
find_package(whisper CONFIG REQUIRED)

set(SOURCES
    src/main.cpp
    src/paths.cpp
    src/settings.cpp
    src/state.cpp
    src/audio.cpp
    src/transcribe.cpp
)

if(APPLE)
  list(APPEND SOURCES src/macos/ptt.mm src/macos/inject.mm)
elseif(WIN32)
  list(APPEND SOURCES src/windows/ptt.cpp src/windows/inject.cpp)
else()
  list(APPEND SOURCES src/linux/ptt.cpp src/linux/inject.cpp)
endif()

add_executable(dictate ${SOURCES})

target_include_directories(dictate PRIVATE src "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(dictate PRIVATE whisper nlohmann_json::nlohmann_json valijson)

if(APPLE)
  target_link_libraries(dictate PRIVATE
      "-framework ApplicationServices"
      "-framework Cocoa"
      "-framework Carbon")
endif()
