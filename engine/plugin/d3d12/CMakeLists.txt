project(ash-graphics-d3d12)

add_library(${PROJECT_NAME} SHARED
    ./source/d3d12_command.cpp
    ./source/d3d12_context.cpp
    ./source/d3d12_frame_resource.cpp
    ./source/d3d12_pipeline.cpp
    ./source/d3d12_plugin.cpp
    ./source/d3d12_renderer.cpp
    ./source/d3d12_resource.cpp
    ./source/d3d12_utility.cpp
    ./source/DDSTextureLoader12.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include
        ${ASH_CORE_INTERFACE_DIR}
        ${ASH_GRAPHICS_INTERFACE_DIR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        d3d12
        dxgi
        dxguid
        d3dcompiler
        ash::math)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)