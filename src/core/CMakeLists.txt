set(STARCANOPY_SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/empty_bake.frag)

# Shaders are compiled into the binary, so a render depends on nothing but the
# project (PRINCIPLES §7) -- not on files beside the executable.
set(embedded ${CMAKE_CURRENT_BINARY_DIR}/shaders_embedded.cpp)
# "|" rather than ";" between paths, which CMake would split into separate
# arguments; VERBATIM keeps the shell from reading "|" as a pipe.
string(REPLACE ";" "|" shaderList "${STARCANOPY_SHADERS}")
add_custom_command(
  OUTPUT ${embedded}
  COMMAND ${CMAKE_COMMAND} -DOUTPUT=${embedded} "-DSHADERS=${shaderList}"
          -P ${PROJECT_SOURCE_DIR}/cmake/EmbedShaders.cmake
  DEPENDS ${STARCANOPY_SHADERS} ${PROJECT_SOURCE_DIR}/cmake/EmbedShaders.cmake
  COMMENT "Embedding shaders"
  VERBATIM)

add_library(starcanopy_core STATIC
  cubemap.cpp
  cubemap_target.cpp
  empty_bake.cpp
  gl_context.cpp
  noise.cpp
  pfm.cpp
  shader.cpp
  tiles.cpp
  ${embedded})
target_include_directories(starcanopy_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(starcanopy_core
  PUBLIC glad mtwist
  PRIVATE SDL3::SDL3-static starcanopy_warnings)

# Headless contexts come from EGL where there is one (Linux); elsewhere, and
# as the fallback, from a hidden SDL window (DESIGN.md §2).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(OpenGL COMPONENTS EGL)
  if(OpenGL_EGL_FOUND)
    target_link_libraries(starcanopy_core PRIVATE OpenGL::EGL)
    target_compile_definitions(starcanopy_core PRIVATE STARCANOPY_HAVE_EGL=1)
  else()
    message(WARNING "EGL not found: headless bakes will need a display (hidden SDL window).")
  endif()
endif()
