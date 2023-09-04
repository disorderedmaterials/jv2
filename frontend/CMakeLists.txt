cmake_minimum_required(VERSION 3.15)

project(jv2 LANGUAGES CXX)

# Metadata
set(DESCRIPTION
    "Journal data browsing, searching, and plotting for the ISIS Facility")
set(AUTHOR "Team JV2")
set(VERSION_MAJOR "1")
set(VERSION_MINOR "2")
set(VERSION_PATCH "0")

# Global CMake settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Custom CMake modules & prefix
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

# Build output locations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

# Dependencies
option(BUILD_SHARED_LIBS "If ON, build shared libraries otherwise build static"
       ON)
include(conan-jv2)

find_package(
  Qt6
  COMPONENTS Core Gui Widgets Network Charts Xml
  REQUIRED)

# Set source and target binary
set(SRCS src/)
add_subdirectory(${SRCS})
qt6_add_executable(jv2 MACOSX_BUNDLE ${SRCS}/main.cpp)

# Link in static plugins if required - only Linux supported
if(NOT BUILD_SHARED_LIBS)
  set(STATIC_PLATFORM_PLUGINS Qt6::QXcbIntegrationPlugin Qt6::XcbQpaPrivate)
endif(NOT BUILD_SHARED_LIBS)

target_link_libraries(
  jv2
  PUBLIC main
  PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Charts Qt6::Xml
          ${STATIC_PLATFORM_PLUGINS})
