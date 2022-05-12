project(test-task)

find_package(Catch2 REQUIRED PATHS ${ASH_THIRD_PARTY_INSTALL_DIR}/Catch2)

add_executable(${PROJECT_NAME}
    ./source/test_lock_free_queue.cpp
    ./source/test_main.cpp
    ./source/test_tagged_pointer.cpp
    ./source/test_task_manager.cpp
    ./source/test_task.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ash::task
        Catch2::Catch2)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin)
endif()