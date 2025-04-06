project(violet-algorithm)

add_library(${PROJECT_NAME} STATIC
    ./private/city_hash.cpp
    ./private/hash.cpp)
add_library(violet::algorithm ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)