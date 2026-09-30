# Shared code first where a program concatenates several (field.glsl before
# the passes that evaluate the gas).
set(STARCANOPY_SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/field.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/galaxy.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light.shader
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/bake.shader
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/denoise.shader
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/galaxy.shader
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/galaxy_probe.shader
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/stars.shader
  )

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
  accents.cpp
  bake.cpp
  cubemap.cpp
  cubemap_target.cpp
  galaxy.cpp
  gl_context.cpp
  half.cpp
  macros.cpp
  noise.cpp
  outputs.cpp
  palette.cpp
  project.cpp
  random.cpp
  render.cpp
  sample.cpp
  scene.cpp
  settings.cpp
  shader.cpp
  sky.cpp
  stars.cpp
  tiles.cpp
  writers.cpp
  ${embedded})
target_include_directories(starcanopy_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(starcanopy_core
  PUBLIC glad mtwist
  PRIVATE SDL3::SDL3-static tinyexr stb_image_write tomlplusplus starcanopy_warnings)

# Headless contexts come from EGL where there is one (Linux); elsewhere, and
# as the fallback, from a hidden SDL window (DESIGN.md §2).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(OpenGL COMPONENTS EGL)
  if(OpenGL_EGL_FOUND)
    # FindOpenGL makes OpenGL::EGL only beside GLVND's libOpenGL, which a
    # system can lack (libopengl-dev) while EGL itself is there; link it
    # directly then.
    if(TARGET OpenGL::EGL)
      target_link_libraries(starcanopy_core PRIVATE OpenGL::EGL)
    else()
      target_link_libraries(starcanopy_core PRIVATE ${OPENGL_egl_LIBRARY})
      target_include_directories(starcanopy_core PRIVATE ${OPENGL_EGL_INCLUDE_DIRS})
    endif()
    target_compile_definitions(starcanopy_core PRIVATE STARCANOPY_HAVE_EGL=1)
  else()
    message(WARNING "EGL not found: headless bakes will need a display (hidden SDL window).")
  endif()
endif()
