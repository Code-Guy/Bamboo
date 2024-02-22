cmake_minimum_required(VERSION 3.20)
set(TARGET_NAME bamboo_test)
set(BAMBOO_DIR "/home/robotberry/Code/Bamboo")
project(${TARGET_NAME} VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

add_compile_definitions(BAMBOO_DIR=\"${BAMBOO_DIR}\")

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "source/*.h")
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "source/*.cpp")
file(GLOB RESOURCE CONFIGURE_DEPENDS "source/resource/bamboo.rc")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCE})

add_executable(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCE})

if(NOT TARGET Bamboo)
    add_subdirectory(${BAMBOO_DIR} ${BAMBOO_DIR}/build)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE Editor)
target_link_libraries(${TARGET_NAME} PRIVATE Engine)
target_include_directories(${TARGET_NAME} PRIVATE ${BAMBOO_DIR}/source)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})
    set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_OPTIONS "/GL")
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_OPTIONS "/INCREMENTAL:NO;/LTCG")
endif()

if (WIN32)
    set(SUBSYSTEM_LINKER_OPTIONS "$<IF:$<CONFIG:Debug>,/SUBSYSTEM:CONSOLE,/SUBSYSTEM:WINDOWS>" "/ENTRY:mainCRTStartup")
else()
    set(SUBSYSTEM_LINKER_OPTIONS "$<IF:$<NOT:$<CONFIG:Debug>>,-mwindows,>")
endif()
