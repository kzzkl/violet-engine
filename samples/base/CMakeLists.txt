project(violet-sample-base)

add_library(${PROJECT_NAME} STATIC
    ./private/gltf_loader.cpp)
add_library(violet::sample-base ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./public
    PRIVATE
    ./private)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    violet::graphics
    assimp::assimp
    MikkTSpace)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)