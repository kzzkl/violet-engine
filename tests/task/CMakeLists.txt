project(test-task)

add_executable(${PROJECT_NAME}
    ./source/test_main.cpp
    ./source/test_task.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ./include)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    violet::core
    Catch2::Catch2)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin/test
    LIBRARY DESTINATION lib/test
    ARCHIVE DESTINATION lib/test)

if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/install/bin/test)
endif()