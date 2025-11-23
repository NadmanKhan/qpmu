set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find Qt
find_package(
  QT REQUIRED
  NAMES Qt6 Qt5
)
find_package(
  Qt${QT_VERSION_MAJOR} REQUIRED
  COMPONENTS Core Gui Widgets Charts Network
)
