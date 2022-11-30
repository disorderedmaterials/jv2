cmake_minimum_required(VERSION 3.15)

project(
  jv2
  VERSION 0.1
  LANGUAGES CXX)

# Metadata
set(DESCRIPTION "jv2")
set(AUTHOR "Team jv2")
set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
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

# Setup Conan for dependency management
option(BUILD_SHARED_LIBS "If ON, build shared libraries otherwise build static" ON)
include(conan-jv2)

# Dependencies
find_package(
  Qt6
  COMPONENTS Core
  Gui
  Widgets
  Network
  Charts
  Xml
  OpenGL
  OpenGLWidgets
  REQUIRED)

# Common sources
set(PROJECT_SOURCES
    src/backend.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/filtering.cpp
    src/datagathering.cpp
    src/searching.cpp
    src/nexusInteraction.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/httprequestworker.cpp
    src/httprequestworker.h
    src/jsontablemodel.cpp
    src/jsontablemodel.h
    src/chartview.cpp
    src/chartview.h
    src/graphwidget.cpp
    src/graphwidget.h
    src/graphwidget.ui
    src/mysortfilterproxymodel.cpp
    src/mysortfilterproxymodel.h)

if(NOT BUILD_SHARED_LIBS)
  # Link in static plugins - only Linux supported
  list(APPEND PROJECT_SOURCES src/platforminit.cpp)
  set(STATIC_PLATFORM_PLUGINS Qt6::QXcbIntegrationPlugin Qt6::XcbQpaPrivate)
endif()

# Resources
set(jv2_RES_QRC "src/icons.qrc")

qt6_add_executable(jv2 MACOSX_BUNDLE ${PROJECT_SOURCES} ${jv2_RES_QRC})
target_include_directories(jv2 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(
  jv2
  PRIVATE Qt6::Core
          Qt6::Widgets
          Qt6::Network
          Qt6::Charts
          Qt6::Xml
          Qt6::OpenGLWidgets
          ${STATIC_PLATFORM_PLUGINS}
)
