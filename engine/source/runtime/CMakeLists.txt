set(TARGET_NAME BambooRuntime)

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_include_directories(
  ${TARGET_NAME} 
  PUBLIC ${ENGINE_ROOT_DIR}/source
)

target_link_libraries(${TARGET_NAME} PUBLIC spdlog)