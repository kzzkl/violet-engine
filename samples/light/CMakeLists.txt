add_executable(light
    source/light_viewer.cpp
    source/main.cpp)

target_include_directories(light
    PRIVATE
        include)

target_link_libraries(light
    PRIVATE
        violet::graphics
        violet::editor)

install(TARGETS light
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)