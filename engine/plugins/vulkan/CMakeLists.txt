find_package(Vulkan REQUIRED)

add_library(violet-vulkan SHARED
    source/vk_bindless.cpp
    source/vk_command.cpp
    source/vk_context.cpp
    source/vk_deletion_queue.cpp
    source/vk_framebuffer.cpp
    source/vk_layout.cpp
    source/vk_parameter.cpp
    source/vk_pipeline.cpp
    source/vk_render_pass.cpp
    source/vk_resource.cpp
    source/vk_rhi.cpp
    source/vk_swapchain.cpp
    source/vk_sync.cpp
    source/vk_utils.cpp)

set_target_properties(violet-vulkan PROPERTIES DEBUG_POSTFIX "")

target_include_directories(violet-vulkan
    PRIVATE
        ./include
        ${VIOLET_ROOT_DIR}/engine/core/public
        ${VIOLET_ROOT_DIR}/engine/graphics/public)

target_link_libraries(violet-vulkan
    PRIVATE
        violet::algorithm
        violet::common
        volk
        VulkanMemoryAllocator)

install(TARGETS violet-vulkan
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)