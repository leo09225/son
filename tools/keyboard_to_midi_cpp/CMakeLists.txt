cmake_minimum_required(VERSION 3.16)
project(keyboard_to_midi_teensy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(RTMIDI REQUIRED rtmidi)

add_executable(keyboard_to_midi main.cpp)

target_include_directories(keyboard_to_midi PRIVATE
  ${SDL2_INCLUDE_DIRS}
  ${RTMIDI_INCLUDE_DIRS}
)

target_link_libraries(keyboard_to_midi PRIVATE
  ${SDL2_LIBRARIES}
  ${RTMIDI_LIBRARIES}
)

target_compile_options(keyboard_to_midi PRIVATE
  ${SDL2_CFLAGS_OTHER}
  ${RTMIDI_CFLAGS_OTHER}
)
