add_library(violet-d3d12 SHARED
    source/d3d12_cache.cpp
    source/d3d12_command.cpp
    source/d3d12_common.cpp
    source/d3d12_context.cpp
    source/d3d12_descriptor_heap.cpp
    source/d3d12_frame_resource.cpp
    source/d3d12_image_loader.cpp
    source/d3d12_pipeline.cpp
    source/d3d12_renderer.cpp
    source/d3d12_resource.cpp
    source/d3d12_rhi.cpp
    source/d3d12_utility.cpp
    source/DDSTextureLoader12.cpp)

set_target_properties(violet-d3d12 PROPERTIES DEBUG_POSTFIX "")

target_include_directories(violet-d3d12
    PRIVATE
        include
        ${VIOLET_ROOT_DIR}/engine/core/public
        ${VIOLET_ROOT_DIR}/engine/graphics/public
        ${VIOLET_THIRDPARTY_DIR}/stb)

target_link_libraries(violet-d3d12
    PRIVATE
        violet::math
        d3d12
        dxgi
        dxguid
        d3dcompiler)

install(TARGETS violet-d3d12
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)