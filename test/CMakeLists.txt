set(tests tiles pfm noise scene settings bake)
list(APPEND tests galaxy)
list(APPEND tests palette)
foreach(name ${tests})
  add_executable(test_${name} test_${name}.cpp)
  target_link_libraries(test_${name} PRIVATE starcanopy_core starcanopy_warnings)
endforeach()

set(scratch ${CMAKE_CURRENT_BINARY_DIR}/scratch)
file(MAKE_DIRECTORY ${scratch})

add_test(NAME tiles COMMAND test_tiles)
add_test(NAME pfm COMMAND test_pfm ${scratch})
add_test(NAME noise COMMAND test_noise)
add_test(NAME scene COMMAND test_scene)
add_test(NAME settings COMMAND test_settings)
add_test(NAME palette COMMAND test_palette)

# The headless path must work wherever the tests run; the hidden window skips
# when there is no display to open one on.
add_test(NAME bake_auto COMMAND test_bake auto)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_test(NAME bake_egl COMMAND test_bake egl)
endif()
add_test(NAME bake_window COMMAND test_bake window)
set_tests_properties(bake_window PROPERTIES SKIP_RETURN_CODE 77)
add_test(NAME galaxy COMMAND test_galaxy)

add_test(NAME cli_bake
  COMMAND starcanopy bake --size 32 --set seed=3 --context auto --out ${scratch})
