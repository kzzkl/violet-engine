project(violet-graphics-d3d12)

add_library(${PROJECT_NAME} SHARED
    ./source/d3d12_cache.cpp
    ./source/d3d12_command.cpp
    ./source/d3d12_common.cpp
    ./source/d3d12_context.cpp
    ./source/d3d12_descriptor_heap.cpp
    ./source/d3d12_frame_resource.cpp
    ./source/d3d12_image_loader.cpp
    ./source/d3d12_pipeline.cpp
    ./source/d3d12_renderer.cpp
    ./source/d3d12_resource.cpp
    ./source/d3d12_rhi.cpp
    ./source/d3d12_utility.cpp
    ./source/DDSTextureLoader12.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./include
    ${VIOLET_ROOT_DIR}/engine/core/public
    ${VIOLET_ROOT_DIR}/engine/graphics/public
    ${VIOLET_THIRD_PARTY_DIR}/stb)

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