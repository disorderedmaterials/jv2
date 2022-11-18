# Fetch and include Conan-cmake integration if it doesn't exist
if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake")
endif ()
include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_configure(
  REQUIRES
    qt/6.3.1
    fontconfig/2.13.93
    glib/2.73.1
    openssl/1.1.1q
  GENERATORS cmake_find_package
  OPTIONS
    qt:shared=False qt:qtcharts=True
    glib:shared=False
    fontconfig:shared=False
)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})
