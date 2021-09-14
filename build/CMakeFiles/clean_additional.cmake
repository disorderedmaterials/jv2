# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/backEndTest_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/backEndTest_autogen.dir/ParseCache.txt"
  "backEndTest_autogen"
  )
endif()
