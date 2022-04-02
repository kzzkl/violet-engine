project(ash-physics)

add_library(${PROJECT_NAME} SHARED
    ./source/physics_plugin.cpp
    ./source/physics.cpp
    ./source/rigidbody.cpp)
add_library(ash::physics ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include
        ./interface)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::core
        ash::math
        ash::scene
        debug ash::graphics)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_MACRO_NAME PHYSICS_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/physics_exports.hpp)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)