set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(external_folder "External")

if (NOT TARGET Vulkan)
    find_package(Vulkan)
    if (NOT Vulkan_FOUND)
        message(FATAL_ERROR "vulkan not found")
    elseif(NOT Vulkan_glslangValidator_FOUND)
        message(FATAL_ERROR "vulkan shader compiler not found")
    elseif(NOT Vulkan_INCLUDE_DIR)
        message(FATAL_ERROR "vulkan include not found")
    endif()
endif()

if(NOT TARGET glfw)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(glfw)
    set_target_properties(glfw PROPERTIES FOLDER ${external_folder}/glfw)
    set_target_properties(update_mappings PROPERTIES FOLDER ${external_folder}/glfw)
endif()

if(NOT TARGET imgui)
    set(IMGUI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
    file(GLOB imgui_sources CONFIGURE_DEPENDS  "${IMGUI_SOURCE_DIR}/*.cpp")
    file(GLOB imgui_impl CONFIGURE_DEPENDS
    "${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp" 
    "${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.h"
    "${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp" 
    "${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.h")
    
    add_library(imgui ${imgui_sources} ${imgui_impl})
    target_include_directories(imgui PUBLIC ${IMGUI_SOURCE_DIR})
    target_include_directories(imgui PUBLIC ${Vulkan_INCLUDE_DIR})
    target_link_libraries(imgui PUBLIC glfw ${Vulkan_LIBRARY})
    set_target_properties(imgui PROPERTIES FOLDER ${external_folder}/imgui)
endif()

if (NOT TARGET spdlog)
    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_INSTALL "" OFF)
    add_subdirectory(spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER ${external_folder}/spdlog)
endif()

if (NOT TARGET yaml-cpp)
    option(YAML_CPP_BUILD_CONTRIB "" OFF)
    option(YAML_CPP_BUILD_TOOLS "" OFF)
    option(YAML_CPP_INSTALL "" OFF)
    option(YAML_CPP_FORMAT_SOURCE OFF)
    add_subdirectory(yaml-cpp)
    set_target_properties(yaml-cpp PROPERTIES FOLDER ${external_folder}/yaml-cpp)
    set_target_properties(uninstall PROPERTIES FOLDER ${external_folder}/yaml-cpp)
endif()

if (NOT TARGET rttr_core_lib)
    option(BUILD_RTTR_DYNAMIC "" OFF)
    option(BUILD_UNIT_TESTS "" OFF)
    option(BUILD_STATIC "" ON)
    option(BUILD_EXAMPLES "" OFF)
    option(BUILD_DOCUMENTATION "" OFF)
    option(BUILD_INSTALLER OFF)
    option(BUILD_PACKAGE OFF)
    add_subdirectory(rttr)
    set_target_properties(rttr_core_lib PROPERTIES FOLDER ${external_folder}/rttr)
endif()

if (NOT TARGET ktx)
    option(KTX_FEATURE_TESTS "" OFF)
    option(KTX_FEATURE_GL_UPLOAD "" OFF)
    
    add_subdirectory(ktx)
    target_link_libraries(ktx PUBLIC ${Vulkan_LIBRARY})
    set_target_properties(ktx PROPERTIES FOLDER ${external_folder}/ktx)
endif()

if(NOT TARGET Jolt)
    option(TARGET_HELLO_WORLD "" OFF)
    option(TARGET_PERFORMANCE_TEST "" OFF)
    option(TARGET_SAMPLES "" OFF)
    option(TARGET_UNIT_TESTS "" OFF)
    option(TARGET_VIEWER "" OFF)

    add_subdirectory(jolt)
    set_target_properties(Jolt PROPERTIES FOLDER ${external_folder}/jolt)
endif()

if (NOT TARGET effolkronium_random)
    option(Random_BuildTests "" OFF)
    add_subdirectory(random)
    set_target_properties(effolkronium_random PROPERTIES FOLDER ${external_folder}/random)
endif()