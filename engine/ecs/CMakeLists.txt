project(ash-ecs)

add_library(${PROJECT_NAME} SHARED
    ./source/archetype_manager.cpp
    ./source/archetype.cpp
    ./source/component.cpp
    ./source/storage.cpp
    ./source/world.cpp)
add_library(ash::ecs ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::common)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_MACRO_NAME ECS_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/ecs_exports.hpp)

install(DIRECTORY ./include/ DESTINATION include/ash/ecs)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)