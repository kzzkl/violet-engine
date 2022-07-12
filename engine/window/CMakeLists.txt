project(ash-window)

option(ASH_WINDOW_SHOW_FPS "Whether to display the frame rate on the title bar." ON)

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

if (${ASH_WINDOW_SHOW_FPS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ASH_WINDOW_SHOW_FPS)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)