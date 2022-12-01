# Fetch and include Conan-cmake integration if it doesn't exist
if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake")
endif ()
include(${CMAKE_BINARY_DIR}/conan.cmake)

set(_conan_requires_os)
set(_conan_options qt:qtcharts=True qt:qtmultimedia=False qt:with_gstreamer=False)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(_conan_requires_os ${_conan_requires_os} glib/2.73.1)  # Override to avoid clash with Qt
  set(_conan_options ${_conan_options} qt:with_glib=True)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_conan_options_os ${_conan_options} qt:with_glib=False)
endif()

if(BUILD_SHARED_LIBS)
    set(_conan_options ${_conan_options} qt:shared=True)
else()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_conan_options ${_conan_options} qt:shared=False glib:shared=False)
  else()
    message(FATAL_ERROR "Static library builds are currently only supported on Linux")
  endif()
endif()

conan_cmake_configure(
  REQUIRES
    qt/6.3.1
    fontconfig/2.13.93
    openssl/1.1.1q
    ${_conan_requires_os}
  GENERATORS cmake_find_package
  OPTIONS
    ${_conan_options}
)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})
