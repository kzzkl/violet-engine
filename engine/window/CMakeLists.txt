project(ash-window)

add_library(${PROJECT_NAME} SHARED
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

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_MACRO_NAME WINDOW_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/window_exports.hpp)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)