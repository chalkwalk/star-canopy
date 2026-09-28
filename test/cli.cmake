# The command line, end to end: `new` writes a project, `render` renders it
# small in every format, and the files and the sidecar are where they should be.

set(project ${SCRATCH}/cli/sky.toml)
file(REMOVE_RECURSE ${SCRATCH}/cli)
file(MAKE_DIRECTORY ${SCRATCH}/cli)

execute_process(COMMAND ${STARCANOPY} new ${project} --seed 3 RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "new failed")
endif()
execute_process(COMMAND ${STARCANOPY} new ${project} RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
if(result EQUAL 0)
  message(FATAL_ERROR "new overwrote an existing project")
endif()

# Every format, and a turned sky.
file(READ ${project} text)
string(REPLACE "formats = [\"exr-faces\", \"ktx2\", \"png-faces\"]"
       "formats = [\"exr-faces\", \"exr-equirect\", \"ktx2\", \"png-faces\", \"png-cross\", \"png-equirect\"]"
       text "${text}")
string(REPLACE "yaw = 0.0" "yaw = 30.0" text "${text}")
file(WRITE ${project} "${text}")

execute_process(COMMAND ${STARCANOPY} render ${project} --size 32 --set exposure=0.2
                --macro open=0.5 RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "render failed")
endif()
foreach(f sky_px.exr sky_nz.exr sky_equirect.exr sky.ktx2 sky_px.png sky_cross.png
          sky_equirect.png sky.json)
  if(NOT EXISTS ${SCRATCH}/cli/sky/${f})
    message(FATAL_ERROR "render did not write ${f}")
  endif()
endforeach()
file(READ ${SCRATCH}/cli/sky/sky.json sidecar)
foreach(key "\"look\": 3" "\"seed\": 3" "\"macros\": {\"open\": 0.5}" "\"yaw\": 30" "\"key_light\"" "\"light_travels\"")
  string(FIND "${sidecar}" "${key}" at)
  if(at EQUAL -1)
    message(FATAL_ERROR "sidecar lacks ${key}")
  endif()
endforeach()

# A project from a newer look is refused, not rendered differently.
file(WRITE ${SCRATCH}/cli/future.toml "look = 99\n")
execute_process(COMMAND ${STARCANOPY} render ${SCRATCH}/cli/future.toml
                RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE message)
if(result EQUAL 0 OR NOT message MATCHES "newer")
  message(FATAL_ERROR "a project from a newer look was not refused: ${message}")
endif()
