set(tests tiles half sample writers project macros noise scene settings palette galaxy stars accents bake look)
foreach(name ${tests})
  add_executable(test_${name} test_${name}.cpp)
  target_link_libraries(test_${name} PRIVATE starcanopy_core starcanopy_warnings)
endforeach()

set(scratch ${CMAKE_CURRENT_BINARY_DIR}/scratch)
file(MAKE_DIRECTORY ${scratch})

add_test(NAME tiles COMMAND test_tiles)
add_test(NAME half COMMAND test_half)
add_test(NAME sample COMMAND test_sample)
add_test(NAME writers COMMAND test_writers ${scratch})
add_test(NAME project COMMAND test_project ${scratch})
add_test(NAME macros COMMAND test_macros)
add_test(NAME noise COMMAND test_noise)
add_test(NAME scene COMMAND test_scene)
add_test(NAME settings COMMAND test_settings)
add_test(NAME palette COMMAND test_palette)
add_test(NAME galaxy COMMAND test_galaxy)
add_test(NAME stars COMMAND test_stars)
add_test(NAME accents COMMAND test_accents)
add_test(NAME look COMMAND test_look)

# The headless path must work wherever the tests run; the hidden window skips
# when there is no display to open one on.
add_test(NAME bake_auto COMMAND test_bake auto)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_test(NAME bake_egl COMMAND test_bake egl)
endif()
add_test(NAME bake_window COMMAND test_bake window)
# A display it cannot open can leave SDL waiting for ever; fail rather than hang.
# Ten minutes: on a software renderer (Windows CI) the bake alone takes three.
set_tests_properties(bake_window PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)

# The command line end to end: a new project, rendered small in every format.
add_test(NAME cli
  COMMAND ${CMAKE_COMMAND} -DSTARCANOPY=$<TARGET_FILE:starcanopy> -DSCRATCH=${scratch}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cli.cmake)

# The command reference keeps up with --help; the man page too, where pandoc
# made one (the root CMakeLists.txt).
if(STARCANOPY_MAN_PAGE)
  set(man_page_arg -DMAN=${STARCANOPY_MAN_PAGE})
endif()
add_test(NAME man
  COMMAND ${CMAKE_COMMAND} -DSTARCANOPY=$<TARGET_FILE:starcanopy>
          -DPAGE=${PROJECT_SOURCE_DIR}/website/docs/man.md ${man_page_arg}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/man.cmake)
