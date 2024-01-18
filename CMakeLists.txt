cmake_minimum_required(VERSION 3.16)
project(qpmu)

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

add_executable(qpmu
        resources/icons.qrc
        sources/main.cpp
        sources/MainWindow.cpp
        sources/MainWindow.h
        sources/WithDock.cpp
        sources/WithDock.h
        sources/WaveformView.cpp
        sources/WaveformView.h
        sources/AdcDataModel.cpp
        sources/AdcDataModel.h
        sources/App.cpp
        sources/App.h
        sources/SignalInfo.cpp
        sources/SignalInfo.h
)

target_link_libraries(qpmu
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Charts
)
