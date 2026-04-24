cmake_minimum_required(VERSION 3.16)

project(qpmu-gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(qpmu-gui
    main.cpp
    mainwindow.h mainwindow.cpp
    phasorplot.h phasorplot.cpp
    waveformplot.h waveformplot.cpp
    signaldatamodel.h signaldatamodel.cpp
    guiclient.h guiclient.cpp
    contextmenupanel.h contextmenupanel.cpp
    theme.h
)

target_include_directories(qpmu-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)

target_link_libraries(qpmu-gui PRIVATE Qt6::Widgets)

set_target_properties(qpmu-gui PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS qpmu-gui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
