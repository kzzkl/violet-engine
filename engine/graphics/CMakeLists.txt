project(violet-graphics)

add_library(${PROJECT_NAME} STATIC
    #./private/material/basic_material.cpp
    ./private/rhi/node_parameter.cpp
    ./private/rhi/pipeline_parameter.cpp
    ./private/rhi/render_pipeline.cpp
    ./private/rhi/rhi_plugin.cpp
    ./private/graphics.cpp)
add_library(violet::graphics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::core
    violet::window
    violet::math
    violet::scene)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)