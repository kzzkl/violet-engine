project(violet-graphics-vulkan)

find_package(Vulkan REQUIRED)

set(STB_IMAGE_INCLUDE ${VIOLET_THIRD_PARTY_DIR}/stb)

add_library(${PROJECT_NAME} SHARED
    ./source/vk_command.cpp
    ./source/vk_common.cpp
    ./source/vk_context.cpp
    ./source/vk_descriptor_pool.cpp
    ./source/vk_image_loader.cpp
    ./source/vk_pipeline.cpp
    ./source/vk_plugin.cpp
    ./source/vk_renderer.cpp
    ./source/vk_resource.cpp
    ./source/vk_sampler.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./include
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