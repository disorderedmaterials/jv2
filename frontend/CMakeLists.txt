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
include(conan-jv2)
find_package(
  Qt6
  COMPONENTS Core Gui Widgets Network Charts Xml
  REQUIRED)

# Build main binary
qt6_add_executable(
  jv2
  MACOSX_BUNDLE
  # Base Classes
  journalSource.cpp
  journalSource.h
  instrument.cpp
  instrument.h
  journal.cpp
  journal.h
  lock.cpp
  lock.h
  optionalRef.h
  # Backend
  backend.cpp
  # Main Window
  data.cpp
  filtering.cpp
  finding.cpp
  generation.cpp
  httpRequestWorker.cpp
  httpRequestWorker.h
  instruments.cpp
  journalSources.cpp
  mainWindow.cpp
  mainWindow.cpp
  mainWindow.h
  mainWindow.ui
  networkHandling.cpp
  nexusInteraction.cpp
  searching.cpp
  settings.cpp
  visualisation.cpp
  version.h
  # Models
  genericTreeModel.cpp
  genericTreeModel.h
  instrumentModel.cpp
  instrumentModel.h
  journalModel.cpp
  journalModel.h
  journalSourceModel.cpp
  journalSourceModel.h
  runDataModel.cpp
  runDataModel.h
  runDataFilterProxy.cpp
  runDataFilterProxy.h
  # Widgets
  chartView.cpp
  chartView.h
  graphWidget.cpp
  graphWidget.h
  graphWidget.ui
  journalSourcesDialog.cpp
  journalSourcesDialog.h
  journalSourcesDialog.ui
  searchDialog.cpp
  searchDialog.h
  searchDialog.ui
  seLogChooserDialog.cpp
  seLogChooserDialog.h
  seLogChooserDialog.ui
  # Resources
  resources.qrc
  # Main
  main.cpp)

target_link_libraries(jv2 PRIVATE Qt6::Core Qt6::Widgets Qt6::Network
                                  Qt6::Charts Qt6::Xml)

target_include_directories(
  jv2
  PRIVATE ${PROJECT_SOURCE_DIR} ${Qt6Widgets_INCLUDE_DIRS}
          ${Qt6Xml_INCLUDE_DIRS} ${Qt6Network_INCLUDE_DIRS}
          ${Qt6Charts_INCLUDE_DIRS})
