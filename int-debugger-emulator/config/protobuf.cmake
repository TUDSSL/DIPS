
set(PROTOBUF_SOURCE_FILES
${CMAKE_SOURCE_DIR}/interface/int-debugger-interface/debugger-interface.proto
${CMAKE_SOURCE_DIR}/interface/int-debugger-interface/calibration.proto
${CMAKE_SOURCE_DIR}/interface/int-debugger-interface/supply.proto
)

set(NANOPB_SRC_ROOT_FOLDER "${CMAKE_SOURCE_DIR}/third-party/nanopb")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${NANOPB_SRC_ROOT_FOLDER}/extra)

find_package(Nanopb REQUIRED)
include_directories(${NANOPB_INCLUDE_DIRS})

nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS RELPATH ${CMAKE_SOURCE_DIR}/interface/int-debugger-interface/ ${PROTOBUF_SOURCE_FILES})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
    PROPERTIES GENERATED TRUE)