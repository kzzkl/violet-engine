add_library(violet-bullet3 SHARED
    source/bt3_joint.cpp
    source/bt3_plugin.cpp
    source/bt3_rigidbody.cpp
    source/bt3_shape.cpp
    source/bt3_world.cpp)

set_target_properties(violet-bullet3 PROPERTIES DEBUG_POSTFIX "")

target_include_directories(violet-bullet3
    PRIVATE
        include
        ${VIOLET_ROOT_DIR}/engine/core/public
        ${VIOLET_ROOT_DIR}/engine/physics/public
        ${VIOLET_THIRDPARTY_DIR}/bullet3/src)

target_link_libraries(violet-bullet3
    PRIVATE
        violet::math
        LinearMath
        Bullet3Common
        BulletCollision
        BulletDynamics)

install(TARGETS violet-bullet3
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)