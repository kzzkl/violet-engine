project(ash-graphics-d3d12)

add_library(${PROJECT_NAME} SHARED
    ./source/d3d12_plugin.cpp
    ./source/d3d12_renderer.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include
        ${ASH_CORE_INTERFACE_DIR}
        ${ASH_GRAPHICS_INTERFACE_DIR})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)