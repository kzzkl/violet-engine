project(violet-common)

add_library(${PROJECT_NAME} STATIC
    ./source/log.cpp)
add_library(violet::common ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json)

install(DIRECTORY ./include/ DESTINATION include/violet)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)