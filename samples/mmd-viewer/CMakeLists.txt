project(mmd-viewer)

add_executable(${PROJECT_NAME}
    ./source/gf2/gf2_material.cpp
    ./source/bezier.cpp
    ./source/encode.cpp
    ./source/main.cpp
    ./source/mmd_animation.cpp
    ./source/mmd_loader.cpp
    ./source/mmd_material.cpp
    ./source/mmd_renderer.cpp
    ./source/mmd_viewer.cpp
    ./source/pmx.cpp
    ./source/vmd.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::graphics
    violet::physics
    violet::animation
    violet::control
    violet::sample-base)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()