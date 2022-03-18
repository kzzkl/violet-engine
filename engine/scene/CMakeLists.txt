project(ash-scene)

add_library(${PROJECT_NAME} SHARED
    ./source/scene_node.cpp
    ./source/scene.cpp)
add_library(ash::scene ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::core
        ash::math)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_MACRO_NAME SCENE_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/scene_exports.hpp)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)