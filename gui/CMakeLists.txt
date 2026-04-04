cmake_minimum_required(VERSION 3.10)

project(qpmu-gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Support both Qt 5 and Qt 6
set(QT_DEFAULT_MAJOR_VERSION 5 CACHE STRING "Qt major version (5 or 6)")

if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
    find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)
    set(QT_VERSION_MAJOR 6)
    qt_standard_project_setup()
else()
    find_package(Qt5 5.12 COMPONENTS Quick REQUIRED)
    set(QT_VERSION_MAJOR 5)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
endif()

set(SOURCES
    src/main.cpp
    src/application.h
    src/application.cpp
    src/models/signaldatamodel.h
    src/models/signaldatamodel.cpp
    src/ipc/guiclient.h
    src/ipc/guiclient.cpp
)

set(QML_FILES
    src/views/Main.qml
    src/views/LiveMonitor.qml
    src/views/components/AppStatusBar.qml
    src/views/components/PhasorPlotModel.qml
    src/views/components/WaveformPlotModel.qml
    src/views/components/SignalTable.qml
    src/views/components/DataTable.qml
    src/views/components/GraphViewSwitcher.qml
    src/views/components/Screen.qml
    src/views/components/AppToolBar.qml
    src/views/components/LiveMonitorContextMenu.qml
    src/views/components/controls/Button.qml
    src/views/components/controls/ToggleButton.qml
    src/views/components/controls/Dropdown.qml
    src/views/components/controls/SliderWithValue.qml
    src/views/components/controls/SectionHeader.qml
    src/AppTheme.qml
)

if(QT_VERSION_MAJOR EQUAL 6)
    # Qt 6: Use simple resource system like Qt 5 for consistency
    qt_add_resources(QML_RESOURCES qml.qrc)
    qt_add_executable(appqpmu ${SOURCES} ${QML_RESOURCES})
else()
    # Qt 5: Use add_executable and qrc file
    qt5_add_resources(QML_RESOURCES qml.qrc)
    add_executable(appqpmu ${SOURCES} ${QML_RESOURCES})
endif()

set_target_properties(appqpmu PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appqpmu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ipc
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(appqpmu PRIVATE Qt6::Quick)
else()
    target_link_libraries(appqpmu PRIVATE Qt5::Quick)
endif()

include(GNUInstallDirs)
install(TARGETS appqpmu
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
