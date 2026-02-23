cmake_minimum_required(VERSION 3.16)

project(qpmu-gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick Graphs)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(appqpmu
    src/main.cpp
    src/signaldatamodel.h
    src/signaldatamodel.cpp
    src/applicationdatamodel.h
    src/applicationdatamodel.cpp
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
        src/views/components/StatusBar.qml
        src/views/components/PhasorPlotModel.qml
        src/views/components/WaveformPlotModel.qml
        src/views/components/SignalTable.qml
        src/views/components/GraphViewSwitcher.qml
        src/AppTheme.qml
    SOURCES
        src/signaldatamodel.h
        src/signaldatamodel.cpp
        src/applicationdatamodel.h
        src/applicationdatamodel.cpp
)

set_target_properties(appqpmu PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appqpmu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
