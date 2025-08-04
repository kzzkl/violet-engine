add_library(violet-task STATIC
    private/task_executor.cpp
    private/task_graph.cpp
    private/task_group.cpp
    private/task.cpp)
add_library(violet::task ALIAS violet-task)

target_include_directories(violet-task
    PUBLIC
        public
    PRIVATE
        private)

target_link_libraries(violet-task
    PUBLIC
        violet::common)

install(TARGETS violet-task
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)