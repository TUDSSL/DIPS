cmake_minimum_required(VERSION 3.5)

project(int-debugger-emulator-host VERSION 0.1 LANGUAGES CXX)

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
message(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS interface/int-debugger-interface/debugger-interface.proto
    interface/int-debugger-interface/supply.proto
    interface/int-debugger-interface/calibration.proto
    interface/nanopb.proto)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Charts SerialPort REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Charts SerialPort REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        emulator.cpp
        emulator.h
        emulatorcomms.cpp
        emulatorcomms.h
        emulator.ui
        graph/chartview.cpp
        graph/chartview.h
        graph/chart.cpp
        graph/chart.h
        profiler/profiler.cpp
        profiler/profiler.h
        buckboostemulator.cpp
        buckboostemulator.h
        serialconnection.cpp
        serialconnection.h
    )

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(int-debugger-emulator-host
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET int-debugger-emulator-host APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(int-debugger-emulator-host SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(int-debugger-emulator-host
            ${PROJECT_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS}
        )
    endif()
endif()


set_target_properties(int-debugger-emulator-host PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(int-debugger-emulator-host)
endif()

# Use HighFive HDF5 file library
add_subdirectory(lib/HighFive)
target_link_libraries(int-debugger-emulator-host PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Charts
        Qt${QT_VERSION_MAJOR}::SerialPort
        ${Protobuf_LIBRARIES}
        HighFive
    )
