add_library(
  main
  backend.cpp
  main.cpp
  mainWindow.cpp
  filtering.cpp
  dataGathering.cpp
  searching.cpp
  nexusInteraction.cpp
  mainWindow.h
  mainWindow.cpp
  mainWindow.ui
  httpRequestWorker.cpp
  httpRequestWorker.h
  jsonTableModel.cpp
  jsonTableModel.h
  jsonTableFilterProxy.cpp
  jsonTableFilterProxy.h
  chartView.cpp
  chartView.h
  graphWidget.cpp
  graphWidget.h
  graphWidget.ui
  version.h
  icons.qrc)

# Link in static plugins - only Linux supported
if(NOT BUILD_SHARED_LIBS)
  list(APPEND PROJECT_SOURCES platformInit.cpp)
endif(NOT BUILD_SHARED_LIBS)

target_include_directories(
  main
  PRIVATE ${PROJECT_SOURCE_DIR}/src ${Qt6Widgets_INCLUDE_DIRS}
          ${Qt6Xml_INCLUDE_DIRS} ${Qt6Network_INCLUDE_DIRS}
          ${Qt6Charts_INCLUDE_DIRS})
