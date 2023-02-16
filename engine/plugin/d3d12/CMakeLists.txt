project(violet-graphics-d3d12)

add_library(${PROJECT_NAME} SHARED
    ./private/d3d12_cache.cpp
    ./private/d3d12_command.cpp
    ./private/d3d12_common.cpp
    ./private/d3d12_context.cpp
    ./private/d3d12_descriptor_heap.cpp
    ./private/d3d12_frame_resource.cpp
    ./private/d3d12_image_loader.cpp
    ./private/d3d12_pipeline.cpp
    ./private/d3d12_plugin.cpp
    ./private/d3d12_renderer.cpp
    ./private/d3d12_resource.cpp
    ./private/d3d12_utility.cpp
    ./private/DDSTextureLoader12.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

set(STB_IMAGE_INCLUDE ${VIOLET_THIRD_PARTY_DIR}/stb)
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./private
    ${VIOLET_CORE_INTERFACE_DIR}
    ${VIOLET_GRAPHICS_INTERFACE_DIR}
    ${STB_IMAGE_INCLUDE})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    d3d12
    dxgi
    dxguid
    d3dcompiler
    violet::math)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)