cmake_minimum_required(VERSION 3.20)
project(StarCanopy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(STARCANOPY_BUILD_TESTS "Build the unit and bake tests" ON)
option(STARCANOPY_BUILD_TOOLS "Build the measurement tools in tools/" ON)

foreach(sub SDL imgui)
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/${sub}/CMakeLists.txt"
     AND NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/${sub}/imgui.h")
    message(FATAL_ERROR
      "extern/${sub} is empty. Run: git submodule update --init --recursive")
  endif()
endforeach()

# SDL3, static, from the submodule: it is not installed system-wide on every
# machine this builds on, and a pinned copy keeps builds alike.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
# XTest only synthesises input, for SDL's own tests; not worth a build dependency.
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/SDL EXCLUDE_FROM_ALL SYSTEM)

# The labs' Mersenne Twister, unchanged, so a seed is the same sky here as there.
add_library(mtwist STATIC extern/mtwist/mtwist.c)
target_include_directories(mtwist SYSTEM PUBLIC extern/mtwist)

# Output formats: OpenEXR through tinyexr, with the miniz it bundles for ZIP
# compression; PNG through stb_image_write. Each is compiled once, here, in
# its own translation unit (extern/*/impl), so their implementation macros
# and warnings stay out of our code. KTX2 is written by our own code.
add_library(tinyexr STATIC extern/tinyexr/impl.cpp extern/tinyexr/miniz.c)
target_include_directories(tinyexr SYSTEM PUBLIC extern/tinyexr)
add_library(stb_image_write STATIC extern/stb/impl.c)
target_include_directories(stb_image_write SYSTEM PUBLIC extern/stb)
# Project files: toml++, header only.
add_library(tomlplusplus INTERFACE)
target_include_directories(tomlplusplus SYSTEM INTERFACE extern/tomlplusplus)

add_library(glad STATIC extern/glad/src/gl.c)
target_include_directories(glad SYSTEM PUBLIC extern/glad/include)

# Dear ImGui with its SDL3 and OpenGL 3 backends, for the interface.
add_library(imgui STATIC
  extern/imgui/imgui.cpp
  extern/imgui/imgui_demo.cpp
  extern/imgui/imgui_draw.cpp
  extern/imgui/imgui_tables.cpp
  extern/imgui/imgui_widgets.cpp
  extern/imgui/backends/imgui_impl_sdl3.cpp
  extern/imgui/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui SYSTEM PUBLIC extern/imgui extern/imgui/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# Warnings for our own code only; the submodules are not ours to fix.
add_library(starcanopy_warnings INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(starcanopy_warnings INTERFACE -Wall -Wextra -Wpedantic)
elseif(MSVC)
  target_compile_options(starcanopy_warnings INTERFACE /W4)
endif()

add_subdirectory(src/core)
add_subdirectory(src/cli)
if(STARCANOPY_BUILD_TOOLS)
  add_subdirectory(tools/study)
endif()

if(STARCANOPY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
