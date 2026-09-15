add_executable(sdf
    source/main.cpp
    source/sdf_debug_pass.cpp
    source/sdf_renderer.cpp)

target_include_directories(sdf
    PRIVATE
        include)

target_link_libraries(sdf
    PRIVATE
        violet::graphics
        violet::control
        violet::sample-base)

install(TARGETS sdf
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)