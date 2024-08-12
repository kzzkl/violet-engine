project(violet-task)

add_library(${PROJECT_NAME} STATIC
    ./private/task_executor.cpp
    ./private/task.cpp)
add_library(violet::task ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./public
    PRIVATE
        ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        violet::common)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)