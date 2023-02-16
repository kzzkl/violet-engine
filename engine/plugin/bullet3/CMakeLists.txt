project(violet-physics-bullet3)

add_library(${PROJECT_NAME} SHARED
    ./private/bt3_joint.cpp
    ./private/bt3_plugin.cpp
    ./private/bt3_rigidbody.cpp
    ./private/bt3_shape.cpp
    ./private/bt3_world.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./private
    ${VIOLET_CORE_INTERFACE_DIR}
    ${VIOLET_PHYSICS_INTERFACE_DIR}
    ${VIOLET_THIRD_PARTY_DIR}/bullet3/src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    LinearMath
    Bullet3Common
    BulletCollision
    BulletDynamics
    violet::math)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)