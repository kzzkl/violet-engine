project(violet-task)

add_library(${PROJECT_NAME} STATIC
    ./private/task_manager.cpp
    ./private/task_queue.cpp
    ./private/task.cpp
    ./private/thread_pool.cpp
    ./private/work_thread.cpp)
add_library(violet::task ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::common
    violet::core)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)