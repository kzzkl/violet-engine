project(violet-common)

add_library(${PROJECT_NAME} STATIC
    ./private/allocator.cpp
    ./private/log.cpp
    ./private/utility.cpp)
add_library(violet::common ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    OffsetAllocator)

install(DIRECTORY ./public/ DESTINATION include/violet)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)