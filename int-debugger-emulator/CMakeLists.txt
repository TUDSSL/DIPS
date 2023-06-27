cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_TOOLCHAIN_FILE "config/stm32f373-toolchain.cmake")

project(int-debugger-emulator LANGUAGES C ASM)

# Prefix path to install executables
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})

add_subdirectory(apps)

