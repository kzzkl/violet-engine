project(violet-resource)

add_library(${PROJECT_NAME} STATIC
    ./private/resource_manager.cpp)
add_library(violet::resource ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::core)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)