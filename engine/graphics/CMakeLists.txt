project(ash-graphics)

add_library(${PROJECT_NAME} SHARED
    ./source/graphics.cpp)
add_library(ash::graphics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include
        ./interface)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::core)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_MACRO_NAME GRAPHICS_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/graphics_exports.hpp)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)