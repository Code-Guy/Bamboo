set(ENGINE_ROOT_DIR "${BAMBOO_ROOT_DIR}/engine")

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options("/MP")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BambooEditor)
endif()

add_subdirectory(external)
add_subdirectory(source/runtime)
add_subdirectory(source/editor)