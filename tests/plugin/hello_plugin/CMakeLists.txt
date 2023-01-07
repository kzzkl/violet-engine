project(test-hello-plugin)

add_library(${PROJECT_NAME} SHARED
    ./source/hello.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${VIOLET_CORE_INTERFACE_DIR})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)