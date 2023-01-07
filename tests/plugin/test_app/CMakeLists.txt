project(test-plugin)

add_executable(${PROJECT_NAME}
    ./source/test_common.cpp
    ./source/test_main.cpp
    ./source/test_plugin.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        violet::core
        Catch2::Catch2)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)