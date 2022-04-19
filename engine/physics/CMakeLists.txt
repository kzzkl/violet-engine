project(ash-physics)

option(ASH_PHYSICS_DEBUG_DRAW "Whether to show rigid body wireframe" OFF)

add_library(${PROJECT_NAME} STATIC
    ./source/joint.cpp
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
        ash::scene)

if (${ASH_PHYSICS_DEBUG_DRAW})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ASH_PHYSICS_DEBUG_DRAW)
    target_link_libraries(${PROJECT_NAME} PUBLIC ash::graphics)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)