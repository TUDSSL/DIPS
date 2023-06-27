# CMake toolchain for the STM32F373 microcontroller
#
# Author: Jasper de Winkel

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CPU "cortex-m4" CACHE STRING "")
set(DEVICE "STM32F373xC" CACHE STRING "")
set(LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/config/STM32F373CCTx_FLASH.ld")

set(OUTPUT_SUFFIX ".elf" CACHE STRING "")

set(CMAKE_C_COMPILER    "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER  "arm-none-eabi-g++")
set(CMAKE_AR            "arm-none-eabi-ar")
set(CMAKE_LINKER        "arm-none-eabi-ld")
set(CMAKE_NM            "arm-none-eabi-nm")
set(CMAKE_OBJDUMP       "arm-none-eabi-objdump")
set(CMAKE_STRIP         "arm-none-eabi-strip")
set(CMAKE_RANLIB        "arm-none-eabi-ranlib")
set(CMAKE_SIZE          "arm-none-eabi-size")

set(CMAKE_C_COMPILER_WORKS 1)

# General compiler flags
add_compile_options(
    -mthumb -mlittle-endian # -mabi=aapcs
    -mcpu=${CPU}
    -Dgcc
    -D${DEVICE}
    )

# Device linker flags
add_link_options(
    -mthumb # -mabi=aapcs
    -mcpu=${CPU}
    #-specs=nano.specs
    )
