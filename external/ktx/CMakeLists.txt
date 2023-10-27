cmake_minimum_required(VERSION 3.20)
project(ktx VERSION 0.1.0)

set(TARGET_NAME ktx)

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "*.c" "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

target_compile_definitions(ktx PUBLIC LIBKTX)
if (WIN32)
    target_compile_definitions(ktx PUBLIC "KTX_API=__declspec(dllexport)")
endif()
target_compile_definitions(ktx PUBLIC KTX_FEATURE_WRITE=1)
target_compile_definitions(ktx PUBLIC BASISD_SUPPORT_KTX2_ZSTD=1)
target_compile_definitions(ktx PUBLIC BASISU_SUPPORT_OPENCL=0)

set(KTX_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils/KHR
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/zstd
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/transcoder
)
target_include_directories(ktx PUBLIC ${KTX_INCLUDE_DIRS})