set(tests tiles half sample writers noise scene settings palette galaxy bake)
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
add_test(NAME noise COMMAND test_noise)
add_test(NAME scene COMMAND test_scene)
add_test(NAME settings COMMAND test_settings)
add_test(NAME palette COMMAND test_palette)
add_test(NAME galaxy COMMAND test_galaxy)

# The headless path must work wherever the tests run; the hidden window skips
# when there is no display to open one on.
add_test(NAME bake_auto COMMAND test_bake auto)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_test(NAME bake_egl COMMAND test_bake egl)
endif()
add_test(NAME bake_window COMMAND test_bake window)
set_tests_properties(bake_window PROPERTIES SKIP_RETURN_CODE 77)

add_test(NAME cli_bake
  COMMAND starcanopy bake --size 32 --set seed=3 --context auto --out ${scratch}
          --formats exr-faces,exr-equirect,ktx2,png-faces,png-cross,png-equirect)
