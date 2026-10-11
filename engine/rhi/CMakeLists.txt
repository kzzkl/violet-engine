add_library(violet-rhi STATIC
    private/rhi.cpp)
add_library(violet::rhi ALIAS violet-rhi)

target_include_directories(violet-rhi
    PUBLIC
        public
    PRIVATE
        private)
target_link_libraries(violet-rhi
    PUBLIC
        violet::algorithm
        violet::common)

if (VIOLET_ENABLE_VULKAN)
    target_sources(violet-rhi
        PRIVATE
            private/vulkan/vk_bindless.cpp
            private/vulkan/vk_command.cpp
            private/vulkan/vk_context.cpp
            private/vulkan/vk_deletion_queue.cpp
            private/vulkan/vk_framebuffer.cpp
            private/vulkan/vk_layout.cpp
            private/vulkan/vk_parameter.cpp
            private/vulkan/vk_pipeline.cpp
            private/vulkan/vk_query.cpp
            private/vulkan/vk_render_pass.cpp
            private/vulkan/vk_resource.cpp
            private/vulkan/vk_rhi.cpp
            private/vulkan/vk_swapchain.cpp
            private/vulkan/vk_sync.cpp
            private/vulkan/vk_utils.cpp)
    target_include_directories(violet-rhi
        PRIVATE
            private/vulkan)
    target_link_libraries(violet-rhi
        PUBLIC
            volk
        PRIVATE
            VulkanMemoryAllocator)
endif()