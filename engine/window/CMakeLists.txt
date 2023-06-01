project(violet-window)

add_library(${PROJECT_NAME} STATIC
    ./private/input.cpp
    ./private/window_impl_win32.cpp
    ./private/window_module.cpp)
add_library(violet::window ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::math
    violet::core)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)