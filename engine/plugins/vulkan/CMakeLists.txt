project(violet-graphics-vulkan)

find_package(Vulkan REQUIRED)

set(HPP_FILES
    ./include/vk_command.hpp
    ./include/vk_common.hpp
    ./include/vk_framebuffer.hpp
    ./include/vk_image_loader.hpp
    ./include/vk_pipeline.hpp
    ./include/vk_render_pass.hpp
    ./include/vk_resource.hpp
    ./include/vk_rhi.hpp
    ./include/vk_swapchain.hpp
    ./include/vk_sync.hpp
    ./include/vk_util.hpp)
source_group("Header Files" FILES ${HPP_FILES})

set(CPP_FILES
    ./source/vk_command.cpp
    ./source/vk_context.cpp
    ./source/vk_framebuffer.cpp
    ./source/vk_image_loader.cpp
    ./source/vk_pipeline.cpp
    ./source/vk_render_pass.cpp
    ./source/vk_rhi.cpp
    ./source/vk_resource.cpp
    ./source/vk_swapchain.cpp
    ./source/vk_sync.cpp
    ./source/vk_util.cpp)
source_group("Source Files" FILES ${CPP_FILES})

add_library(${PROJECT_NAME} SHARED ${HPP_FILES} ${CPP_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./include
    ${VIOLET_ROOT_DIR}/engine/core/public
    ${VIOLET_ROOT_DIR}/engine/graphics/public)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::common
    stb
    volk
    VulkanMemoryAllocator)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)