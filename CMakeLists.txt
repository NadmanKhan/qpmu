cmake_minimum_required(VERSION 3.26)
project(QPMU)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        Network
        Charts
        REQUIRED)

add_executable(QPMU sources/main.cpp
        icons.qrc
        sources/MainWindow.cpp
        sources/MainWindow.h
        sources/WithDock.cpp
        sources/WithDock.h
        sources/WaveformView.cpp
        sources/WaveformView.h
        sources/AdcSampleModel.cpp
        sources/AdcSampleModel.h
        sources/App.cpp
        sources/App.h
)

target_link_libraries(QPMU
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Charts
)

add_subdirectory(includes)

