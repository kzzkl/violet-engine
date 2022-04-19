project(ash-physics-bullet3)

find_package(Bullet REQUIRED PATHS ${THIRD_PARTY_DIR}/bullet3)

add_library(${PROJECT_NAME} SHARED
    ./source/bt3_joint.cpp
    ./source/bt3_plugin.cpp
    ./source/bt3_rigidbody.cpp
    ./source/bt3_shape.cpp
    ./source/bt3_world.cpp)

message(${BULLET_INCLUDE_DIR})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include
        ${ASH_CORE_INTERFACE_DIR}
        ${ASH_PHYSICS_INTERFACE_DIR}
        ${BULLET_ROOT_DIR}/${BULLET_INCLUDE_DIR})

target_link_directories(${PROJECT_NAME}
    PRIVATE
        ${BULLET_ROOT_DIR}/${BULLET_LIBRARY_DIRS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        optimized LinearMath.lib
        optimized Bullet3Common.lib
        optimized BulletCollision.lib
        optimized BulletDynamics.lib
        debug LinearMath_Debug.lib
        debug Bullet3Common_Debug.lib
        debug BulletCollision_Debug.lib
        debug BulletDynamics_Debug.lib
        ash::math)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)