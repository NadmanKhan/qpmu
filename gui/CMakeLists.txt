cmake_minimum_required(VERSION 3.16)

project(qpmu-gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick Graphs)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(appqpmu
    src/main.cpp
    src/models/signaldatamodel.h
    src/models/signaldatamodel.cpp
    src/models/applicationdatamodel.h
    src/models/applicationdatamodel.cpp
    src/models/viewstatemodel.h
    src/models/viewstatemodel.cpp
)

# Add qtquickcontrols2.conf as a resource at the root level
qt_add_resources(appqpmu "config"
    PREFIX "/"
    FILES qtquickcontrols2.conf
)

# Mark AppTheme.qml as a singleton so qt_add_qml_module generates correct qmldir
set_source_files_properties(src/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(appqpmu
    URI qpmu
    VERSION 1.0
    QML_FILES
        src/views/Main.qml
        src/views/LiveMonitor.qml
        src/views/components/AppStatusBar.qml
        src/views/components/PhasorPlotModel.qml
        src/views/components/WaveformPlotModel.qml
        src/views/components/SignalTable.qml
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
    SOURCES
        src/models/signaldatamodel.h
        src/models/signaldatamodel.cpp
        src/models/applicationdatamodel.h
        src/models/applicationdatamodel.cpp
        src/models/viewstatemodel.h
        src/models/viewstatemodel.cpp
)

set_target_properties(appqpmu PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appqpmu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)

target_link_libraries(appqpmu
    PRIVATE
        Qt6::Quick
        Qt6::Graphs
)

include(GNUInstallDirs)
install(TARGETS appqpmu
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
