project(violet-physics-bullet3)

set(HPP_FILES
    )
source_group("Header Files" FILES ${HPP_FILES})

set(CPP_FILES
    ./source/bt3_joint.cpp
    ./source/bt3_plugin.cpp
    ./source/bt3_rigidbody.cpp
    ./source/bt3_shape.cpp
    ./source/bt3_world.cpp)
source_group("Source Files" FILES ${CPP_FILES})

add_library(${PROJECT_NAME} SHARED ${HPP_FILES} ${CPP_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./include
    ${VIOLET_ROOT_DIR}/engine/core/public
    ${VIOLET_ROOT_DIR}/engine/physics/public
    ${VIOLET_THIRDPARTY_DIR}/bullet3/src)

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