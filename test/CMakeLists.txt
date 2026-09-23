foreach(name tiles pfm noise empty_bake)
  add_executable(test_${name} test_${name}.cpp)
  target_link_libraries(test_${name} PRIVATE starcanopy_core starcanopy_warnings)
endforeach()

set(scratch ${CMAKE_CURRENT_BINARY_DIR}/scratch)
file(MAKE_DIRECTORY ${scratch})

add_test(NAME tiles COMMAND test_tiles)
add_test(NAME pfm COMMAND test_pfm ${scratch})
add_test(NAME noise COMMAND test_noise)

# The headless path must work wherever the tests run; the hidden window skips
# when there is no display to open one on.
add_test(NAME empty_bake_auto COMMAND test_empty_bake auto)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_test(NAME empty_bake_egl COMMAND test_empty_bake egl)
endif()
add_test(NAME empty_bake_window COMMAND test_empty_bake window)
set_tests_properties(empty_bake_window PROPERTIES SKIP_RETURN_CODE 77)

add_test(NAME cli_bake_empty
  COMMAND starcanopy bake-empty --size 64 --context auto --out ${scratch})
