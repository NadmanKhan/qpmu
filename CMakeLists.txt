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
        VirtualKeyboard
        REQUIRED)

add_executable(qpmu
        resources/icons.qrc
        sources/main.cpp
        sources/MainWindow.cpp
        sources/MainWindow.h
        sources/WaveformView.cpp
        sources/WaveformView.h
        sources/AdcDataModel.cpp
        sources/AdcDataModel.h
        sources/App.cpp
        sources/App.h
        sources/SignalInfoModel.cpp
        sources/SignalInfoModel.h
        sources/VisibilityControlView.cpp
        sources/VisibilityControlView.h
)

target_link_libraries(qpmu
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Charts
        Qt5::VirtualKeyboard
)

configure_file(scripts/adc_simulator.py ${CMAKE_CURRENT_BINARY_DIR}/adc_simulator.py COPYONLY)
configure_file(qpmu.ini ${CMAKE_CURRENT_BINARY_DIR}/qpmu.ini COPYONLY)
