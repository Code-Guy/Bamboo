set(TARGET_NAME BambooRuntime)

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_link_libraries(${TARGET_NAME} PUBLIC ${Vulkan_LIBRARY})
target_link_libraries(${TARGET_NAME} PUBLIC glfw)
target_link_libraries(${TARGET_NAME} PUBLIC imgui)
target_link_libraries(${TARGET_NAME} PUBLIC spdlog)
target_link_libraries(${TARGET_NAME} PUBLIC yaml-cpp)
target_link_libraries(${TARGET_NAME} PUBLIC rttr_core_lib)

target_include_directories(${TARGET_NAME} PUBLIC ${ENGINE_ROOT_DIR}/source)
target_include_directories(${TARGET_NAME} PUBLIC ${ENGINE_ROOT_DIR}/external)
target_include_directories(${TARGET_NAME} PUBLIC ${ENGINE_ROOT_DIR}/external/rttr/src)