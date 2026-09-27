add_executable(starcanopy main.cpp)
target_link_libraries(starcanopy PRIVATE starcanopy_core starcanopy_warnings)
# At the top of the build tree, where the documented commands run it from.
set_target_properties(starcanopy PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
