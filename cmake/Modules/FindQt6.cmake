set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_VERSION_MAJOR 6)

find_package(qt6 REQUIRED)
add_library(Qt6 ALIAS qt::qt)
add_executable(Qt6::moc IMPORTED GLOBAL)
set_target_properties(Qt6::moc PROPERTIES IMPORTED_LOCATION ${AUTOMOC_EXECUTABLE})
add_executable(Qt6::rcc IMPORTED GLOBAL)
set_target_properties(Qt6::rcc PROPERTIES IMPORTED_LOCATION ${AUTORCC_EXECUTABLE})
add_executable(Qt6::uic IMPORTED GLOBAL)
set_target_properties(Qt6::uic PROPERTIES IMPORTED_LOCATION ${AUTOUIC_EXECUTABLE})