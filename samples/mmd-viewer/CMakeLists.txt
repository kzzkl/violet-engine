add_executable(mmd-viewer
    source/gf2/gf2_material.cpp
    source/bezier.cpp
    source/encode.cpp
    source/main.cpp
    source/mmd_animation.cpp
    source/mmd_loader.cpp
    source/mmd_material.cpp
    source/mmd_renderer.cpp
    source/mmd_viewer.cpp
    source/pmx.cpp
    source/vmd.cpp)

target_include_directories(mmd-viewer
    PRIVATE
        include)

target_link_libraries(mmd-viewer
    PRIVATE
        violet::graphics
        violet::physics
        violet::animation
        violet::control
        violet::sample-base)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)

install(TARGETS mmd-viewer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)