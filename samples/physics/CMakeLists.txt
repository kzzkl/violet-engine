add_executable(physics
    source/main.cpp)

target_include_directories(physics
    PRIVATE
        include)

target_link_libraries(physics
    PRIVATE
        violet::graphics
        violet::physics
        violet::control
        violet::sample-base)

install(TARGETS physics
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION bin/assets)