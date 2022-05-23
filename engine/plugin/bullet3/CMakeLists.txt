project(ash-physics-bullet3)

add_library(${PROJECT_NAME} SHARED
    ./source/bt3_joint.cpp
    ./source/bt3_plugin.cpp
    ./source/bt3_rigidbody.cpp
    ./source/bt3_shape.cpp
    ./source/bt3_world.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include
        ${ASH_CORE_INTERFACE_DIR}
        ${ASH_PHYSICS_INTERFACE_DIR}
        ${ASH_THIRD_PARTY_DIR}/bullet3/src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        LinearMath
        Bullet3Common
        BulletCollision
        BulletDynamics
        ash::math)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)