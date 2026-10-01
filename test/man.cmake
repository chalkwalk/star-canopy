# The command reference keeps up with the command line: every command and
# option `starcanopy --help` lists is in the manual's page, and, where pandoc
# built the man page from it, in the man page too.

execute_process(COMMAND ${STARCANOPY} --help OUTPUT_VARIABLE help RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "--help failed")
endif()
string(REGEX MATCHALL "starcanopy [a-z]+" commands "${help}")
string(REGEX MATCHALL "--[a-z][a-z-]*" options "${help}")
list(REMOVE_DUPLICATES commands)
list(REMOVE_DUPLICATES options)
list(LENGTH commands n)
if(n LESS 4)
  message(FATAL_ERROR "found only ${n} commands in --help: ${commands}")
endif()

set(sources ${PAGE})
if(MAN)
  list(APPEND sources ${MAN})
endif()
foreach(source ${sources})
  if(NOT EXISTS ${source})
    message(FATAL_ERROR "${source} is missing")
  endif()
  file(READ ${source} text)
  # roff escapes a hyphen as \- ; read it as one.
  string(REPLACE "\\-" "-" text "${text}")
  # Each option described, as a bold term, and not only in the synopsis's code:
  # typography that turns -- into a dash there makes an option no one can type.
  foreach(word ${options})
    string(FIND "${text}" "**${word}**" md)
    string(FIND "${text}" "\\f[B]${word}" roff)
    if(md EQUAL -1 AND roff EQUAL -1)
      message(SEND_ERROR "${source} does not describe ${word} as a bold term")
    endif()
  endforeach()
  foreach(word ${commands} ${options})
    string(FIND "${text}" "${word}" at)
    if(at EQUAL -1)
      message(SEND_ERROR "${source} does not mention ${word}")
    endif()
  endforeach()
endforeach()
