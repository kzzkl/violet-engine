project(violet-graphics-vulkan)

find_package(Vulkan REQUIRED)

set(STB_IMAGE_INCLUDE ${VIOLET_THIRD_PARTY_DIR}/stb)

add_library(${PROJECT_NAME} SHARED
    ./private/vk_command.cpp
    ./private/vk_common.cpp
    ./private/vk_context.cpp
    ./private/vk_descriptor_pool.cpp
    ./private/vk_image_loader.cpp
    ./private/vk_pipeline.cpp
    ./private/vk_plugin.cpp
    ./private/vk_renderer.cpp
    ./private/vk_resource.cpp
    ./private/vk_sampler.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./private
    ${VIOLET_CORE_INTERFACE_DIR}
    ${VIOLET_GRAPHICS_INTERFACE_DIR}
    ${STB_IMAGE_INCLUDE})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::math
    Vulkan::Vulkan)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)