project(ash-graphics-vulkan)

find_package(Vulkan REQUIRED)

add_library(${PROJECT_NAME} SHARED
    ./source/vk_command.cpp
    ./source/vk_context.cpp
    ./source/vk_frame_buffer.cpp
    ./source/vk_plugin.cpp
    ./source/vk_render_pass.cpp
    ./source/vk_renderer.cpp
    ./source/vk_resource.cpp)
add_library(ash::plugin::vulkan ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include
        ${ASH_CORE_INTERFACE_DIR}
        ${ASH_GRAPHICS_INTERFACE_DIR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::math
        Vulkan::Vulkan)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)