# Author: TU Delft Sustainable Systems Laboratory
# License: MIT License

project(hw-test VERSION 1.0.0 LANGUAGES C ASM)

#set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# options
set(PROTOBUF 1)

set(DEPENDENCIES
    config/
    config/ST
    platforms/
    libs/voltage-sense/
    libs/current-sense/
    libs/supply/
    libs/spi/
    libs/calibration/
    libs/pc-comms/
    misc/
    apps/hw-test/
    )
    

# Source files of libraries and externals
foreach(dep ${DEPENDENCIES})
    list(APPEND DEP_SOURCES "${CMAKE_SOURCE_DIR}/${dep}/*.[cs]")
endforeach()

# List source files to be compiled
file(GLOB SOURCES
    "${PROJECT_SOURCE_DIR}/*.[cs]"
    ${DEP_SOURCES}
    )

if(PROTOBUF)
include(${CMAKE_SOURCE_DIR}/config/protobuf.cmake)
endif()

# Add executable target
add_executable(${PROJECT_NAME} ${SOURCES} ${PROTO_SRCS})# ${PROTO_HDRS}

include(${CMAKE_SOURCE_DIR}/config/sdk-sources.cmake)

# Change target suffix to have <name>.out
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})

# Defines (-D)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE HWREV=1
    PRIVATE USE_HAL_DRIVER
)

# Compiler options for this project
target_compile_options(${PROJECT_NAME}
    PUBLIC -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    PRIVATE -O0
    PRIVATE -g -gdwarf-3
    PRIVATE -Wall
    PRIVATE -std=c99
    PRIVATE -MMD -MP
    PRIVATE -ffunction-sections -fdata-sections -fomit-frame-pointer
    )

# Include directories of libraries and externals
foreach(dep ${DEPENDENCIES})
    target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_SOURCE_DIR}/${dep}/)
endforeach()

# Adds the generated protobuf headers to the include workaround
target_include_directories(${PROJECT_NAME}
     PRIVATE  ${CMAKE_SOURCE_DIR}/build/apps/${PROJECT_NAME}/
)

# Linker options for this project
target_link_options(${PROJECT_NAME}
    PUBLIC -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    PRIVATE -nostartfiles -static
    PRIVATE -Wl,--gc-sections,--entry,Reset_Handler -u _printf_float
    PRIVATE -Wl,-Map=${PROJECT_NAME}.map
    PRIVATE -T ${LINKER_SCRIPT}
    )

target_link_libraries(${PROJECT_NAME}
    gcc
    c
    m
    nosys
    )

# Print size of binary after linking
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}${OUTPUT_SUFFIX}
    )

# Install executable in [CMAKE_SOURCE_DIR]/bin
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
