project(ash-task)

add_library(${PROJECT_NAME} SHARED
    ./source/task_manager.cpp
    ./source/task_queue.cpp
    ./source/task.cpp
    ./source/thread_pool.cpp
    ./source/work_thread.cpp)
add_library(ash::task ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::common)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_MACRO_NAME TASK_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/task_exports.hpp)

install(DIRECTORY ./include/ DESTINATION include/ash/task)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)