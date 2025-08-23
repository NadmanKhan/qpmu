cmake_minimum_required(VERSION 3.15)
project(open-c37118 VERSION 0.2.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define library sources
set(SOURCES 
        src/c37118.cpp
        src/c37118command.cpp
        src/c37118configuration.cpp
        src/c37118data.cpp
        src/c37118header.cpp
        src/c37118pmustation.cpp
)

# Define library headers
set(HEADERS 
        src/c37118.h
        src/c37118command.h
        src/c37118configuration.h
        src/c37118data.h
        src/c37118header.h
        src/c37118pmustation.h
)

# Create library
add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC  
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# Define installation rules
include(GNUInstallDirs)
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
