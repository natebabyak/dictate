cmake_minimum_required(VERSION 4.3)

project(Dictate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(whisper CONFIG REQUIRED)

add_executable(dictate src/main.cpp)

target_link_libraries(dictate PRIVATE whisper)
