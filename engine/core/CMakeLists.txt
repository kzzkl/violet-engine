project(violet-core)

set(CONTEXT_SOURCE
    ./private/context/engine_module.cpp
    ./private/context/engine.cpp
    ./private/context/plugin.cpp)

set(NODE_SOURCE
    ./private/node/archetype.cpp
    ./private/node/node.cpp
    ./private/node/view.cpp
    ./private/node/world.cpp)

set(TASK_SOURCE
    ./private/task/task_manager.cpp
    ./private/task/task_queue.cpp
    ./private/task/task.cpp
    ./private/task/thread_pool.cpp
    ./private/task/work_thread.cpp)

add_library(${PROJECT_NAME} STATIC
    ${CONTEXT_SOURCE}
    ${NODE_SOURCE}
    ${TASK_SOURCE})
add_library(violet::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./interface
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::math
    violet::common)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)