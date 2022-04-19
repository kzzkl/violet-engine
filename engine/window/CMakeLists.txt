project(ash-window)

add_library(${PROJECT_NAME} STATIC
    ./source/input.cpp
    ./source/window_impl_win32.cpp
    ./source/window.cpp)
add_library(ash::window ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::core)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)