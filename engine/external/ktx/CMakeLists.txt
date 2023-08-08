cmake_minimum_required(VERSION 3.20)
project(ktx VERSION 0.1.0)

set(TARGET_NAME ktx)

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "include/*.h")
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "lib/*.c")

add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(${TARGET_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/other_include)