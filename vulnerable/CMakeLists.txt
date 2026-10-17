cmake_minimum_required(VERSION 3.20)
project(textzip C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ZLIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib-1.2.11)

add_library(zlib_static STATIC
    ${ZLIB_SRC_DIR}/adler32.c
    ${ZLIB_SRC_DIR}/compress.c
    ${ZLIB_SRC_DIR}/crc32.c
    ${ZLIB_SRC_DIR}/deflate.c
    ${ZLIB_SRC_DIR}/gzclose.c
    ${ZLIB_SRC_DIR}/gzlib.c
    ${ZLIB_SRC_DIR}/gzread.c
    ${ZLIB_SRC_DIR}/gzwrite.c
    ${ZLIB_SRC_DIR}/infback.c
    ${ZLIB_SRC_DIR}/inffast.c
    ${ZLIB_SRC_DIR}/inflate.c
    ${ZLIB_SRC_DIR}/inftrees.c
    ${ZLIB_SRC_DIR}/trees.c
    ${ZLIB_SRC_DIR}/uncompr.c
    ${ZLIB_SRC_DIR}/zutil.c
)

target_include_directories(zlib_static PUBLIC ${ZLIB_SRC_DIR})

if(MSVC)
    target_compile_definitions(zlib_static PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_executable(textzip src/main.cpp)
target_link_libraries(textzip PRIVATE zlib_static)
