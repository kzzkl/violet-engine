add_executable(bvh
    source/bvh_viewer.cpp
    source/main.cpp)

target_include_directories(bvh
    PRIVATE
        include)

target_link_libraries(bvh
    PRIVATE
        violet::graphics
        violet::ui)

install(TARGETS bvh
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)